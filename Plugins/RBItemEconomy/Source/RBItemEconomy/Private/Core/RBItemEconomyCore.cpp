#include "Core/RBItemEconomyCore.h"
#include <algorithm>
#include <limits>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace RB::Items {
namespace {
struct Failure : std::runtime_error {
    Error error;
    Failure(Error e, const char* message) : std::runtime_error(message), error(e) {}
};
void need(bool ok, Error error, const char* message) { if(!ok) throw Failure(error,message); }
bool validName(const std::string& s) {
    return !s.empty() && s.size()<=128 && std::all_of(s.begin(),s.end(),[](unsigned char c){
        return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c==':'||c=='/'||c=='-';
    });
}
Count add(Count a,Count b) {
    need(a>=0 && b>=0 && a<=std::numeric_limits<Count>::max()-b,Error::Invalid,"integer addition overflow");
    return a+b;
}
Count mul(Count a,Count b) {
    need(a>=0 && b>=0 && (b==0 || a<=std::numeric_limits<Count>::max()/b),Error::Invalid,"integer multiplication overflow");
    return a*b;
}
Count mulBps(Count value,std::int32_t bps) {
    need(value>=0 && bps>=0,Error::Invalid,"negative basis-point input");
    return mul(value,static_cast<Count>(bps))/BasisPoints;
}
void checkNames(const std::set<std::string>& values) {
    need(values.size()<=128,Error::Budget,"tag budget exceeded");
    for(const auto& v:values) need(validName(v),Error::Invalid,"invalid tag or principal name");
}
void checkMeta(const Meta& m) {
    need(m.quality>=0 && m.quality<=10000 && m.durability>=0 && m.durability<=10000 && m.expiresAt>=-1 &&
         m.remainingShelfLife>=-1 && m.decayUpdatedAt>=0 && m.decayRemainderBasisPoints>=0 &&
         m.decayRemainderBasisPoints<BasisPoints,Error::Invalid,"invalid metadata range");
    need(m.provenance.size()<=256 && m.properties.size()<=16,Error::Budget,"metadata budget exceeded");
    for(const auto&[k,v]:m.properties) need(validName(k)&&v.size()<=1024,Error::Invalid,"invalid metadata property");
}
const Container& container(const State& s,Id id) {
    auto p=s.containers.find(id);need(p!=s.containers.end(),Error::Missing,"container not found");return p->second;
}
const Item& item(const State& s,Id id) {
    auto p=s.items.find(id);need(p!=s.items.end(),Error::Missing,"item not found");return p->second;
}
const Definition& definition(const Catalog& c,const std::string& id) {
    auto p=c.definitions.find(id);need(p!=c.definitions.end(),Error::Missing,"definition not found");return p->second;
}
std::vector<Cell> cells(const Definition& d,bool rotated) {
    std::vector<Cell> out;
    if(d.shape.empty()) {
        out.reserve(static_cast<std::size_t>(d.width*d.height));
        for(int y=0;y<d.height;++y) for(int x=0;x<d.width;++x) out.push_back({x,y});
    } else out=d.shape;
    if(rotated) for(auto& c:out) c={d.height-1-c.y,c.x};
    return out;
}
int shapeWidth(const Definition& d,bool rotated){return rotated?d.height:d.width;}
int shapeHeight(const Definition& d,bool rotated){return rotated?d.width:d.height;}
const Container& root(const State& s,Id id) {
    std::set<Id> visited;const Container* p=&container(s,id);
    while(p->parentItem) {
        need(visited.insert(p->id).second&&visited.size()<=32,Error::Invalid,"nested container cycle or depth limit");
        p=&container(s,item(s,p->parentItem).container);
    }
    return *p;
}
bool access(const State& s,const std::string& actor,Id id) {
    const auto& c=root(s,id);return c.owner==actor||c.publicAccess||c.delegates.contains(actor);
}
bool owns(const State& s,const std::string& actor,Id id){return root(s,id).owner==actor;}
void permit(const State& s,const Context& ctx,Id id){need(ctx.privileged||access(s,ctx.actor,id),Error::Forbidden,"container access denied");}
Count reservedIn(const State& s,Id id) {
    Count total=0;
    for(const auto&[jobId,job]:s.jobs) {
        (void)jobId;
        for(const auto&r:job.inputs) if(r.item==id) total=add(total,r.quantity);
        for(const auto&r:job.fuels) if(r.item==id) total=add(total,r.quantity);
        if(std::find(job.tools.begin(),job.tools.end(),id)!=job.tools.end()) total=add(total,1);
    }
    return total;
}
bool equipped(const State& s,Id id) {
    for(const auto&[actor,slots]:s.equipment){(void)actor;for(const auto&[slot,value]:slots){(void)slot;if(value==id)return true;}}
    return false;
}
bool fresh(const Item& i,Time now) {
    if(i.meta.remainingShelfLife>=0) return i.meta.remainingShelfLife>0;
    return i.meta.expiresAt<0||i.meta.expiresAt>now;
}
Id allocate(State& s){need(s.nextId>0&&s.nextId<MaxId,Error::Budget,"ID allocation exhausted");return s.nextId++;}
Count massRecursive(const Catalog& c,const State& s,Id id,std::set<Id>& chain) {
    need(chain.insert(id).second&&chain.size()<=32,Error::Invalid,"nested mass cycle");Count total=0;
    for(const auto&[iid,i]:s.items) if(i.container==id){(void)iid;total=add(total,mul(i.quantity,definition(c,i.definition).unitMassGrams));if(i.childContainer)total=add(total,massRecursive(c,s,i.childContainer,chain));if(i.attachmentContainer)total=add(total,massRecursive(c,s,i.attachmentContainer,chain));}
    chain.erase(id);return total;
}
bool installedAttachment(const State& s,Id id);
void pruneHotbars(State& s) {
    for(auto&[actor,slots]:s.hotbars) for(auto it=slots.begin();it!=slots.end();) {
        auto p=s.items.find(it->second);if(p==s.items.end()||!owns(s,actor,p->second.container)||installedAttachment(s,p->second.id))it=slots.erase(it);else ++it;
    }
}
void pruneEquipment(State& s) {
    for(auto&[actor,slots]:s.equipment) {
        (void)actor;
        for(auto it=slots.begin();it!=slots.end();) {
            auto p=s.items.find(it->second);
            if(p==s.items.end()||!fresh(p->second,s.now)||p->second.meta.durability<=0) it=slots.erase(it);else ++it;
        }
    }
}
void eraseItem(State& s,Id id) {
    const auto& i=item(s,id);
    auto clearChild=[&](Id cid,const char* nonempty,const char* money){
        if(!cid)return;need(std::none_of(s.items.begin(),s.items.end(),[&](const auto&p){return p.second.container==cid;}),Error::Invalid,nonempty);
        const auto& wallet=container(s,cid).money;need(std::all_of(wallet.begin(),wallet.end(),[](const auto&p){return p.second==0;}),Error::Invalid,money);s.containers.erase(cid);
    };
    clearChild(i.childContainer,"cannot remove a nonempty bag","bag contains currency");
    clearChild(i.attachmentContainer,"detach installed attachments before removing host","attachment container contains currency");
    s.items.erase(id);
}
bool sharesAny(const std::set<std::string>& a,const std::set<std::string>& b);
bool installedAttachment(const State& s,Id id) {
    const auto& i=item(s,id);const auto& ct=container(s,i.container);if(!ct.parentItem)return false;const auto& parent=item(s,ct.parentItem);return parent.attachmentContainer==ct.id;
}
const std::pair<const std::string,std::set<std::string>>& attachmentSlotAt(const Catalog& c,const State& s,const Container& ct,int slot) {
    need(ct.parentItem!=0,Error::Invalid,"attachment container has no host");const auto& host=item(s,ct.parentItem);need(host.attachmentContainer==ct.id,Error::Invalid,"container is not an attachment container");const auto& hd=definition(c,host.definition);need(slot>=0&&static_cast<std::size_t>(slot)<hd.attachmentSlots.size(),Error::Invalid,"attachment slot outside bounds");auto it=hd.attachmentSlots.begin();std::advance(it,slot);return *it;
}
bool acceptsAttachmentSlot(const Catalog& c,const State& s,const Container& ct,const Definition& d,int slot) {
    if(!ct.parentItem)return true;const auto& host=item(s,ct.parentItem);if(host.attachmentContainer!=ct.id)return true;
    if(slot<0||static_cast<std::size_t>(slot)>=definition(c,host.definition).attachmentSlots.size())return false;
    const auto& spec=attachmentSlotAt(c,s,ct,slot);return spec.second.empty()||sharesAny(d.tags,spec.second);
}
bool fits(const Catalog& c,const State& s,const Item& v) {
    const auto& ct=container(s,v.container);const auto& d=definition(c,v.definition);
    if(ct.gridWidth==0) {
        if(v.slot<0||v.slot>=ct.slots)return false;
        for(const auto&[id,other]:s.items)if(id!=v.id&&other.container==v.container&&other.slot==v.slot)return false;
        return true;
    }
    const int w=shapeWidth(d,v.rotated),h=shapeHeight(d,v.rotated);
    if(v.x<0||v.y<0||v.x>ct.gridWidth-w||v.y>ct.gridHeight-h)return false;
    std::set<int> occupied;
    for(const auto&[id,other]:s.items) if(id!=v.id&&other.container==v.container) {
        const auto& od=definition(c,other.definition);
        for(const auto& cell:cells(od,other.rotated)) occupied.insert((other.y+cell.y)*ct.gridWidth+(other.x+cell.x));
    }
    for(const auto& cell:cells(d,v.rotated)) if(occupied.contains((v.y+cell.y)*ct.gridWidth+(v.x+cell.x))) return false;
    return true;
}
void place(const Catalog& c,State& s,Item& i,int slot=-1,int x=-1,int y=-1,bool rotated=false) {
    need(slot>=-1&&x>=-1&&y>=-1,Error::Invalid,"invalid negative placement");const auto& ct=container(s,i.container);i.rotated=rotated;
    if(!ct.gridWidth) {
        if(slot>=0){i.slot=slot;need(acceptsAttachmentSlot(c,s,ct,definition(c,i.definition),slot),Error::Forbidden,"attachment incompatible with slot");need(fits(c,s,i),Error::Capacity,"slot occupied or outside bounds");return;}
        for(int n=0;n<ct.slots;++n){i.slot=n;if(acceptsAttachmentSlot(c,s,ct,definition(c,i.definition),n)&&fits(c,s,i))return;}
    } else {
        need((x<0)==(y<0),Error::Invalid,"grid placement requires both coordinates");
        if(x>=0){i.x=x;i.y=y;need(fits(c,s,i),Error::Capacity,"grid collision or outside bounds");return;}
        for(int py=0;py<ct.gridHeight;++py)for(int px=0;px<ct.gridWidth;++px){i.x=px;i.y=py;if(fits(c,s,i))return;}
    }
    throw Failure(Error::Capacity,"no free placement");
}
void projectExpiry(const Container& ct,Meta& m,Time now) {
    if(m.remainingShelfLife<0)return;
    if(m.remainingShelfLife==0){m.expiresAt=now;return;}
    if(ct.decayRateBasisPoints==0){m.expiresAt=-1;return;}
    Count numerator=add(mul(m.remainingShelfLife,BasisPoints),ct.decayRateBasisPoints-1);
    m.expiresAt=add(now,numerator/ct.decayRateBasisPoints);
}
void initializeDecay(const Catalog& c,const State& s,Item& i) {
    const auto& d=definition(c,i.definition);
    if(i.meta.remainingShelfLife<0) {
        if(i.meta.expiresAt>=0) i.meta.remainingShelfLife=std::max<Time>(0,i.meta.expiresAt-s.now);
        else if(d.defaultShelfLifeSeconds>=0) i.meta.remainingShelfLife=d.defaultShelfLifeSeconds;
    }
    i.meta.decayUpdatedAt=s.now;i.meta.decayRemainderBasisPoints=0;projectExpiry(container(s,i.container),i.meta,s.now);
}
void ageItem(const Catalog& c,const State& s,Item& i,Time newNow) {
    (void)c;need(newNow>=i.meta.decayUpdatedAt,Error::Invalid,"item decay clock rewind");
    if(i.meta.remainingShelfLife<0){i.meta.decayUpdatedAt=newNow;return;}
    const auto& ct=container(s,i.container);const Count elapsed=newNow-i.meta.decayUpdatedAt;
    Count numerator=add(mul(elapsed,ct.decayRateBasisPoints),i.meta.decayRemainderBasisPoints);
    Count consumed=numerator/BasisPoints;i.meta.decayRemainderBasisPoints=static_cast<std::int32_t>(numerator%BasisPoints);
    i.meta.remainingShelfLife=std::max<Time>(0,i.meta.remainingShelfLife-consumed);i.meta.decayUpdatedAt=newNow;
    projectExpiry(ct,i.meta,newNow);
}
void grant(const Catalog& c,State& s,const Grant& g,Result& result) {
    need(g.quantity>0,Error::Invalid,"positive grant quantity required");checkMeta(g.meta);const auto& d=definition(c,g.definition);container(s,g.destination);
    Count left=g.quantity;Meta normalized=g.meta;Item probe;probe.definition=g.definition;probe.container=g.destination;probe.meta=normalized;initializeDecay(c,s,probe);normalized=probe.meta;
    if(!d.bagSlots&&d.attachmentSlots.empty()) for(auto&[id,i]:s.items) {
        if(i.container==g.destination&&i.definition==g.definition&&i.meta==normalized&&!i.childContainer&&!equipped(s,id)&&!i.locked) {
            Count n=std::min(left,d.maxStack-i.quantity);i.quantity+=n;left-=n;if(!left)break;
        }
    }
    while(left>0) {
        need(s.items.size()<MaxRecords,Error::Budget,"item budget exhausted");Item i;i.id=allocate(s);i.container=g.destination;i.definition=g.definition;
        i.quantity=std::min(left,d.maxStack);i.meta=normalized;place(c,s,i);
        if(d.bagSlots){Container inner;inner.id=allocate(s);inner.parentItem=i.id;inner.slots=d.bagSlots;inner.maxMassGrams=d.bagMaxMassGrams;i.childContainer=inner.id;s.containers.emplace(inner.id,std::move(inner));}if(!d.attachmentSlots.empty()){Container sockets;sockets.id=allocate(s);sockets.parentItem=i.id;sockets.slots=static_cast<std::int32_t>(d.attachmentSlots.size());sockets.maxMassGrams=std::numeric_limits<Count>::max()/4;i.attachmentContainer=sockets.id;s.containers.emplace(sockets.id,std::move(sockets));}
        left-=i.quantity;result.created.push_back(i.id);s.items.emplace(i.id,std::move(i));
    }
}
void transfer(const Catalog& c,State& s,const Context& ctx,const Transfer& t,Result& result,bool checkAccess=true) {
    const Item original=item(s,t.item);need(t.quantity>0&&t.quantity<=original.quantity,Error::Invalid,"invalid transfer quantity");container(s,t.destination);
    if(checkAccess){permit(s,ctx,original.container);permit(s,ctx,t.destination);}need(!original.locked,Error::Locked,"item is locked");
    need(!equipped(s,t.item),Error::Equipped,"unequip before moving");need(t.quantity<=original.quantity-reservedIn(s,t.item),Error::Reserved,"quantity reserved for crafting");
    Item moved=original;moved.container=t.destination;projectExpiry(container(s,t.destination),moved.meta,s.now);
    if(t.quantity<original.quantity){need(!original.childContainer&&!original.attachmentContainer,Error::Invalid,"cannot split nested inventory");moved.id=allocate(s);moved.quantity=t.quantity;s.items.at(t.item).quantity-=t.quantity;result.created.push_back(moved.id);}
    place(c,s,moved,t.slot,t.x,t.y,t.rotated);s.items.insert_or_assign(moved.id,std::move(moved));pruneHotbars(s);
}
void consume(State& s,Id id,Count quantity,bool allowExpired=false) {
    const Item original=item(s,id);need(quantity>0&&quantity<=original.quantity,Error::Invalid,"invalid consumption quantity");
    need(!installedAttachment(s,id),Error::Equipped,"detach attachment before consuming");need(!original.locked,Error::Locked,"item is locked");need(!equipped(s,id),Error::Equipped,"cannot consume equipped item");
    need(allowExpired||fresh(original,s.now),Error::Expired,"item expired");need(quantity<=original.quantity-reservedIn(s,id),Error::Reserved,"reserved quantity cannot be consumed");
    if(quantity==original.quantity)eraseItem(s,id);else s.items.at(id).quantity-=quantity;pruneHotbars(s);
}
bool sharesAny(const std::set<std::string>& a,const std::set<std::string>& b){return std::any_of(a.begin(),a.end(),[&](const auto&x){return b.contains(x);});}
Count price(const Catalog& c,const State& s,Id vendorId,Id iid,Count quantity,bool buying) {
    auto it=s.vendors.find(vendorId);need(it!=s.vendors.end(),Error::Missing,"vendor not found");const auto& i=item(s,iid);const auto& d=definition(c,i.definition);
    need(!sharesAny(d.tags,it->second.refusedTags),Error::Forbidden,"vendor refuses this item category");const auto& prices=buying?it->second.sells:it->second.buys;
    auto p=prices.find(i.definition);need(p!=prices.end(),Error::Forbidden,"vendor does not trade this definition");need(quantity>0&&quantity<=i.quantity,Error::Invalid,"invalid trade quantity");
    Count base=mul(quantity,p->second);return mulBps(base,buying?it->second.sellMultiplierBasisPoints:it->second.buyMultiplierBasisPoints);
}
std::uint64_t randomWord(std::uint64_t& state){state+=0x9e3779b97f4a7c15ULL;auto z=state;z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27))*0x94d049bb133111ebULL;return z^(z>>31);}
std::uint64_t boundedRandom(std::uint64_t& state,std::uint64_t bound){need(bound>0,Error::Invalid,"empty random range");const auto threshold=(std::uint64_t{0}-bound)%bound;for(int n=0;n<128;++n){auto x=randomWord(state);if(x>=threshold)return x%bound;}throw Failure(Error::Budget,"random rejection budget exhausted");}

void selectReservations(const State& s,const std::set<Id>& sources,const std::string& def,Count required,std::vector<Reservation>& out,std::map<Id,Count>& local,bool allowEquipped=false,std::int32_t minQuality=0) {
    Count remaining=required;
    for(const auto&[id,i]:s.items) if(sources.contains(i.container)&&i.definition==def&&!i.childContainer&&!i.locked&&fresh(i,s.now)&&i.meta.quality>=minQuality&&(allowEquipped||!equipped(s,id))) {
        Count blocked=add(reservedIn(s,id),local[id]);Count available=std::max<Count>(0,i.quantity-blocked);Count use=std::min(remaining,available);
        if(use>0){out.push_back({id,use});local[id]=add(local[id],use);remaining-=use;}if(!remaining)break;
    }
    need(!remaining,Error::Insufficient,"usable unreserved ingredients missing");
}

void operate(const Catalog& c,State& s,const Context& ctx,const Operation& operation,Result& result) {
    std::visit([&](const auto& cmd){using T=std::decay_t<decltype(cmd)>;
        if constexpr(std::is_same_v<T,CreateContainer>) {
            need(ctx.privileged,Error::Forbidden,"host creates root inventories");need(!cmd.value.id&&!cmd.value.parentItem,Error::Invalid,"new root must have unassigned identity");
            auto ct=cmd.value;ct.id=allocate(s);result.created.push_back(ct.id);s.containers.emplace(ct.id,std::move(ct));
        } else if constexpr(std::is_same_v<T,Grant>) {
            need(ctx.privileged,Error::Forbidden,"client minting forbidden");grant(c,s,cmd,result);
        } else if constexpr(std::is_same_v<T,Transfer>) {
            transfer(c,s,ctx,cmd,result);
        } else if constexpr(std::is_same_v<T,TransferAll>) {
            permit(s,ctx,cmd.source);permit(s,ctx,cmd.destination);need(cmd.source!=cmd.destination,Error::Invalid,"transfer-all destination must differ");
            std::vector<Id> ids;for(const auto&[id,i]:s.items)if(i.container==cmd.source&&!i.locked&&!equipped(s,id)&&reservedIn(s,id)==0)ids.push_back(id);
            for(Id id:ids){auto q=item(s,id).quantity;transfer(c,s,ctx,{id,cmd.destination,q},result,false);}
        } else if constexpr(std::is_same_v<T,QuickStack>) {
            permit(s,ctx,cmd.source);permit(s,ctx,cmd.destination);
            std::vector<Id> ids;for(const auto&[id,i]:s.items)if(i.container==cmd.source&&!i.childContainer&&!i.locked&&!equipped(s,id)&&reservedIn(s,id)==0)ids.push_back(id);
            for(Id sid:ids){auto sp=s.items.find(sid);if(sp==s.items.end())continue;auto& src=sp->second;const auto& d=definition(c,src.definition);Count left=src.quantity;
                for(auto&[did,dst]:s.items)if(did!=sid&&dst.container==cmd.destination&&dst.definition==src.definition&&dst.meta==src.meta&&!dst.childContainer&&!dst.locked&&!equipped(s,did)&&dst.quantity<d.maxStack){
                    Count n=std::min(left,d.maxStack-dst.quantity);dst.quantity+=n;left-=n;if(!left)break;
                }
                if(left==0)eraseItem(s,sid);else src.quantity=left;
            }
            pruneHotbars(s);
        } else if constexpr(std::is_same_v<T,Merge>) {
            need(cmd.source!=cmd.target,Error::Invalid,"cannot merge item into itself");const auto a=item(s,cmd.source),b=item(s,cmd.target);permit(s,ctx,a.container);permit(s,ctx,b.container);
            need(!a.locked&&!b.locked,Error::Locked,"locked stack cannot be merged");need(a.definition==b.definition&&a.meta==b.meta&&!a.childContainer&&!b.childContainer,Error::Invalid,"incompatible stack metadata");
            need(!equipped(s,a.id)&&!equipped(s,b.id),Error::Equipped,"cannot merge equipped items");need(cmd.quantity>0&&cmd.quantity<=a.quantity,Error::Invalid,"invalid merge quantity");
            need(cmd.quantity<=a.quantity-reservedIn(s,a.id),Error::Reserved,"source reserved");need(cmd.quantity<=definition(c,a.definition).maxStack-b.quantity,Error::Capacity,"stack limit");
            s.items.at(b.id).quantity+=cmd.quantity;if(cmd.quantity==a.quantity){for(auto&[actor,slots]:s.hotbars)if(owns(s,actor,b.container))for(auto&[slot,id]:slots){(void)slot;if(id==a.id)id=b.id;}eraseItem(s,a.id);}else s.items.at(a.id).quantity-=cmd.quantity;pruneHotbars(s);
        } else if constexpr(std::is_same_v<T,Consume>||std::is_same_v<T,Discard>) {
            permit(s,ctx,item(s,cmd.item).container);consume(s,cmd.item,cmd.quantity,std::is_same_v<T,Discard>);
        } else if constexpr(std::is_same_v<T,Hotbar>) {
            need(cmd.slot>=0&&cmd.slot<32,Error::Invalid,"invalid hotbar slot");if(cmd.item){need(owns(s,ctx.actor,item(s,cmd.item).container),Error::Forbidden,"hotbar requires possession");need(!installedAttachment(s,cmd.item),Error::Equipped,"detach attachment before hotbar binding");s.hotbars[ctx.actor][cmd.slot]=cmd.item;}else s.hotbars[ctx.actor].erase(cmd.slot);
        } else if constexpr(std::is_same_v<T,Equip>) {
            const auto& i=item(s,cmd.item);need(owns(s,ctx.actor,i.container),Error::Forbidden,"equipment requires possession");need(!installedAttachment(s,i.id),Error::Equipped,"installed attachment cannot be equipped separately");auto& slots=s.equipment[ctx.actor];
            if(cmd.remove){for(auto it=slots.begin();it!=slots.end();)if(it->second==i.id)it=slots.erase(it);else ++it;}
            else {const auto& d=definition(c,i.definition);need(i.quantity==1&&!d.equipmentSlots.empty()&&!i.childContainer,Error::Invalid,"not single-instance equipment");need(fresh(i,s.now)&&i.meta.durability>0,Error::Expired,"expired or broken equipment");need(!reservedIn(s,i.id),Error::Reserved,"equipment reserved");for(const auto& slot:d.equipmentSlots)need(!slots.contains(slot)||slots.at(slot)==i.id,Error::Conflict,"equipment slot occupied");for(const auto& slot:d.equipmentSlots)slots[slot]=i.id;}
        } else if constexpr(std::is_same_v<T,SetItemFlags>) {
            auto& i=s.items.at(item(s,cmd.item).id);need(owns(s,ctx.actor,i.container)||ctx.privileged,Error::Forbidden,"item flags require ownership");if(cmd.setLocked&&cmd.locked)need(!reservedIn(s,i.id),Error::Reserved,"reserved item cannot be locked");if(cmd.setFavorite)i.favorite=cmd.favorite;if(cmd.setLocked)i.locked=cmd.locked;
        } else if constexpr(std::is_same_v<T,Trade>) {
            need(ctx.privileged||ctx.vendorPermits.contains(cmd.vendor),Error::Forbidden,"trusted vendor interaction permit required");permit(s,ctx,cmd.customerInventory);auto vi=s.vendors.find(cmd.vendor);need(vi!=s.vendors.end(),Error::Missing,"vendor missing");const auto v=vi->second;
            need(cmd.customerInventory!=v.stock,Error::Invalid,"cannot trade with the same inventory");const auto original=item(s,cmd.item);need(!original.locked,Error::Locked,"locked item cannot be traded");need(!original.childContainer,Error::Invalid,"bag trade requires future contents policy");need(original.container==(cmd.buying?v.stock:cmd.customerInventory),Error::Forbidden,"item outside quoted inventory");need(fresh(original,s.now),Error::Expired,"expired merchandise");
            Count total=price(c,s,cmd.vendor,cmd.item,cmd.quantity,cmd.buying);need(total==cmd.quotedTotal,Error::Stale,"server price differs from quote");auto& payer=s.containers.at(cmd.buying?cmd.customerInventory:v.stock).money;auto& receiver=s.containers.at(cmd.buying?v.stock:cmd.customerInventory).money;Count available=payer.contains(v.currency)?payer.at(v.currency):0;need(available>=total,Error::Insufficient,"insufficient buyer or vendor funds");receiver[v.currency]=add(receiver[v.currency],total);payer[v.currency]=available-total;transfer(c,s,ctx,{cmd.item,cmd.buying?cmd.customerInventory:v.stock,cmd.quantity},result,false);
        } else if constexpr(std::is_same_v<T,RestockVendor>) {
            need(ctx.privileged,Error::Forbidden,"host restock authority required");auto it=s.vendors.find(cmd.vendor);need(it!=s.vendors.end(),Error::Missing,"vendor missing");auto& v=it->second;need(cmd.generation==v.restockGeneration,Error::Stale,"stale restock generation");need(v.restockEverySeconds>0&&s.now>=v.nextRestockAt,Error::NotReady,"vendor restock not ready");
            for(const auto&[def,target]:v.restockTargets){Count current=0;for(const auto&[id,i]:s.items)if(i.container==v.stock&&i.definition==def){(void)id;current=add(current,i.quantity);}if(current<target){Count batch=v.restockBatches.contains(def)?v.restockBatches.at(def):target;Count amount=std::min(batch,target-current);if(amount>0){Meta m;m.provenance="vendor-restock:"+std::to_string(v.id)+":"+std::to_string(v.restockGeneration);grant(c,s,{v.stock,def,amount,m},result);}}}
            need(v.restockGeneration<MaxId,Error::Budget,"vendor restock generation exhausted");++v.restockGeneration;v.nextRestockAt=add(s.now,v.restockEverySeconds);
        } else if constexpr(std::is_same_v<T,StartCraft>) {
            auto ri=c.recipes.find(cmd.recipe);need(ri!=c.recipes.end(),Error::Missing,"recipe missing");const auto& recipe=ri->second;permit(s,ctx,cmd.station);permit(s,ctx,cmd.destination);const auto& station=container(s,cmd.station);
            need(station.tags.contains(recipe.stationTag),Error::Forbidden,"wrong workstation");need(station.craftEnabled,Error::NotReady,"workstation disabled");std::size_t queued=0;for(const auto&[id,j]:s.jobs){(void)id;if(j.station==cmd.station)++queued;}need(queued<static_cast<std::size_t>(station.craftQueueLimit),Error::Capacity,"workstation queue full");
            auto unlock=s.unlocks.find(ctx.actor);need(!recipe.requiresUnlock||(unlock!=s.unlocks.end()&&unlock->second.contains(recipe.id)),Error::Forbidden,"recipe locked");need(cmd.batches>0&&!cmd.sources.empty()&&cmd.sources.size()<=32,Error::Invalid,"invalid crafting request");std::set<Id> sources(cmd.sources.begin(),cmd.sources.end());need(sources.size()==cmd.sources.size(),Error::Invalid,"duplicate input container");for(Id id:sources)permit(s,ctx,id);
            CraftJob job;job.id=allocate(s);job.owner=ctx.actor;job.recipe=recipe.id;job.station=cmd.station;job.destination=cmd.destination;job.batches=cmd.batches;job.startedAt=s.now;job.due=add(s.now,mul(recipe.durationSeconds,cmd.batches));std::map<Id,Count> local;
            for(const auto&[def,qty]:recipe.inputs)selectReservations(s,sources,def,mul(qty,cmd.batches),job.inputs,local,false,recipe.minQuality);
            for(const auto&[def,qty]:recipe.fuelInputs)selectReservations(s,sources,def,mul(qty,cmd.batches),job.fuels,local);
            for(const auto& toolDef:recipe.requiredTools){bool found=false;for(const auto&[id,i]:s.items)if(sources.contains(i.container)&&i.definition==toolDef&&!i.childContainer&&fresh(i,s.now)&&i.meta.durability>=recipe.toolDurabilityCost&&reservedIn(s,id)==0&&!local[id]){job.tools.push_back(id);local[id]=1;found=true;break;}need(found,Error::Insufficient,"required crafting tool missing or unusable");}
            result.created.push_back(job.id);s.jobs.emplace(job.id,std::move(job));
        } else if constexpr(std::is_same_v<T,FinishCraft>||std::is_same_v<T,CancelCraft>) {
            auto ji=s.jobs.find(cmd.job);need(ji!=s.jobs.end(),Error::Missing,"job missing");const auto job=ji->second;need(ctx.privileged||job.owner==ctx.actor,Error::Forbidden,"job belongs to another principal");
            if constexpr(std::is_same_v<T,CancelCraft>)s.jobs.erase(ji);else {need(s.now>=job.due,Error::NotReady,"craft is not ready");permit(s,ctx,job.destination);permit(s,ctx,job.station);const auto& recipe=c.recipes.at(job.recipe);std::int32_t quality=10000;
                for(const auto&r:job.inputs){const auto& i=item(s,r.item);permit(s,ctx,i.container);need(fresh(i,s.now),Error::Expired,"reserved ingredient expired; cancel job to release");quality=std::min(quality,i.meta.quality);}for(const auto&r:job.fuels){const auto& i=item(s,r.item);permit(s,ctx,i.container);need(fresh(i,s.now),Error::Expired,"reserved fuel expired; cancel job to release");}
                for(Id toolId:job.tools){const auto& i=item(s,toolId);permit(s,ctx,i.container);need(fresh(i,s.now)&&i.meta.durability>=recipe.toolDurabilityCost,Error::Expired,"crafting tool became unusable");}
                s.jobs.erase(ji);for(const auto&r:job.inputs)consume(s,r.item,r.quantity);for(const auto&r:job.fuels)consume(s,r.item,r.quantity);for(Id toolId:job.tools)if(recipe.toolDurabilityCost>0)s.items.at(toolId).meta.durability-=recipe.toolDurabilityCost;
                pruneEquipment(s);for(const auto&[def,qty]:recipe.outputs){Meta m;m.quality=quality;m.provenance="craft:"+std::to_string(job.id);grant(c,s,{job.destination,def,mul(qty,job.batches),m},result);}
            }
        } else if constexpr(std::is_same_v<T,Repair>) {
            auto& target=s.items.at(item(s,cmd.item).id);need(owns(s,ctx.actor,target.container)||ctx.privileged,Error::Forbidden,"repair requires ownership");need(cmd.targetDurability>target.meta.durability&&cmd.targetDurability<=10000,Error::Invalid,"invalid repair target");const auto& d=definition(c,target.definition);need(!d.repairInputs.empty(),Error::Forbidden,"item has no repair recipe");need(!cmd.sources.empty()&&cmd.sources.size()<=32,Error::Invalid,"repair source inventories required");std::set<Id> sources(cmd.sources.begin(),cmd.sources.end());for(Id id:sources)permit(s,ctx,id);std::vector<Reservation> costs;std::map<Id,Count> local;for(const auto&[def,qty]:d.repairInputs)selectReservations(s,sources,def,qty,costs,local);for(const auto&r:costs)consume(s,r.item,r.quantity);target.meta.durability=cmd.targetDurability;
        } else if constexpr(std::is_same_v<T,Attach>) {
            const auto& host=item(s,cmd.host);const auto& attachment=item(s,cmd.attachment);need(cmd.host!=cmd.attachment,Error::Invalid,"cannot attach item to itself");need(owns(s,ctx.actor,host.container)&&owns(s,ctx.actor,attachment.container),Error::Forbidden,"attachment requires possession");need(!installedAttachment(s,cmd.host)&&!installedAttachment(s,cmd.attachment),Error::Invalid,"nested installed attachment unsupported");need(host.attachmentContainer!=0,Error::Forbidden,"host has no attachment slots");const auto& hd=definition(c,host.definition);auto slotIt=hd.attachmentSlots.find(cmd.slot);need(slotIt!=hd.attachmentSlots.end(),Error::Missing,"attachment slot missing");const auto& ad=definition(c,attachment.definition);need(attachment.quantity==1&&!attachment.childContainer&&!attachment.attachmentContainer,Error::Invalid,"attachment must be a single simple item");need(slotIt->second.empty()||sharesAny(ad.tags,slotIt->second),Error::Forbidden,"attachment incompatible with slot");auto idx=static_cast<std::int32_t>(std::distance(hd.attachmentSlots.begin(),slotIt));transfer(c,s,ctx,{attachment.id,host.attachmentContainer,1,idx,-1,-1,false},result);
        } else if constexpr(std::is_same_v<T,Detach>) {
            const auto& attachment=item(s,cmd.attachment);need(installedAttachment(s,attachment.id),Error::Invalid,"item is not installed as an attachment");const auto& socketContainer=container(s,attachment.container);const auto& host=item(s,socketContainer.parentItem);need(owns(s,ctx.actor,host.container),Error::Forbidden,"detachment requires host ownership");transfer(c,s,ctx,{attachment.id,cmd.destination,1,cmd.slot,cmd.x,cmd.y,cmd.rotated},result);        } else if constexpr(std::is_same_v<T,AdvanceTime>) {
            need(ctx.privileged,Error::Forbidden,"host clock authority required");need(cmd.now>=s.now,Error::Invalid,"clock rewind forbidden");for(auto&[id,i]:s.items){(void)id;ageItem(c,s,i,cmd.now);}s.now=cmd.now;pruneEquipment(s);
            std::vector<Id> expired;for(const auto&[id,i]:s.items)if(!fresh(i,s.now)&&!definition(c,i.definition).decaysTo.empty()&&reservedIn(s,id)==0&&!equipped(s,id))expired.push_back(id);
            for(Id id:expired){auto& i=s.items.at(id);const auto old=i.definition;const auto& target=definition(c,definition(c,old).decaysTo);i.definition=target.id;i.meta.expiresAt=-1;i.meta.remainingShelfLife=target.defaultShelfLifeSeconds;i.meta.decayUpdatedAt=s.now;i.meta.decayRemainderBasisPoints=0;i.meta.provenance="decay:"+old;projectExpiry(container(s,i.container),i.meta,s.now);}
        } else if constexpr(std::is_same_v<T,Unlock>) {
            need(ctx.privileged&&validName(cmd.actor),Error::Forbidden,"host unlock authority required");need(c.recipes.contains(cmd.recipe),Error::Missing,"recipe missing");s.unlocks[cmd.actor].insert(cmd.recipe);
        } else if constexpr(std::is_same_v<T,RollLoot>) {
            need(ctx.privileged||ctx.sourcePermits.contains(cmd.source),Error::Forbidden,"trusted loot/harvest interaction permit required");permit(s,ctx,cmd.destination);auto si=s.sources.find(cmd.source);need(si!=s.sources.end(),Error::Missing,"source missing");auto& source=si->second;need(!source.exhausted&&source.generation==cmd.generation,Error::Stale,"exhausted or stale source");need(s.now>=source.availableAt,Error::NotReady,"source restock not ready");const auto& table=c.lootTables.at(source.table);std::uint64_t seed=source.seed^(source.generation*0xd6e8feb86659fd93ULL),weight=0;for(const auto&e:table.entries)weight+=e.weight;
            for(std::uint32_t n=0;n<table.rolls;++n){auto roll=boundedRandom(seed,weight);const LootEntry* selected=nullptr;for(const auto&e:table.entries){if(roll<e.weight){selected=&e;break;}roll-=e.weight;}need(selected!=nullptr,Error::Invalid,"no loot result");Count quantity=selected->minimum+static_cast<Count>(boundedRandom(seed,static_cast<std::uint64_t>(selected->maximum-selected->minimum)+1));if(quantity){Meta m;m.provenance="loot:"+source.id+":"+std::to_string(source.generation);grant(c,s,{cmd.destination,selected->definition,quantity,m},result);}}
            need(source.generation<MaxId,Error::Budget,"source generation exhausted");++source.generation;source.exhausted=!source.renewable;source.availableAt=add(s.now,source.cooldownSeconds);
        } else if constexpr(std::is_same_v<T,Award>) {
            need(ctx.privileged,Error::Forbidden,"host production authority required");need(validName(cmd.eventId),Error::Invalid,"invalid source event ID");need(!s.awards.contains(cmd.eventId),Error::Conflict,"production award already committed");need(!cmd.grants.empty()&&cmd.grants.size()<=128,Error::Invalid,"invalid production batch");for(const auto&g:cmd.grants)grant(c,s,g,result);s.awards.insert(cmd.eventId);
        }
    },operation);
}

// Codec uses bounded, explicit little-endian fields. No native object-memory dumping.
struct Writer {
    std::vector<std::uint8_t> bytes;
    void room(std::size_t n){need(n<=MaxSnapshotBytes&&bytes.size()<=MaxSnapshotBytes-n,Error::Budget,"encoded data exceeds budget");}
    void u(std::uint64_t n){room(8);for(int b=0;b<8;++b)bytes.push_back(static_cast<std::uint8_t>(n>>(8*b)));}
    void i(std::int64_t n){u(static_cast<std::uint64_t>(n));}
    void b(bool n){room(1);bytes.push_back(n?1:0);}
    void text(const std::string&s){u(s.size());room(s.size());bytes.insert(bytes.end(),s.begin(),s.end());}
    void blob(const std::vector<std::uint8_t>&v){u(v.size());room(v.size());bytes.insert(bytes.end(),v.begin(),v.end());}
    void names(const std::set<std::string>&v){u(v.size());for(const auto&x:v)text(x);}
    void counts(const std::map<std::string,Count>&v){u(v.size());for(const auto&[k,n]:v){text(k);i(n);}}
    void meta(const Meta&v){i(v.quality);i(v.durability);i(v.expiresAt);i(v.remainingShelfLife);i(v.decayUpdatedAt);i(v.decayRemainderBasisPoints);text(v.provenance);u(v.properties.size());for(const auto&[k,n]:v.properties){text(k);text(n);}}
    void ct(const Container&v){u(v.id);text(v.owner);names(v.delegates);names(v.tags);names(v.acceptedTags);b(v.publicAccess);i(v.slots);i(v.maxMassGrams);i(v.gridWidth);i(v.gridHeight);u(v.parentItem);counts(v.money);i(v.decayRateBasisPoints);i(v.craftQueueLimit);b(v.craftEnabled);}
    void grantValue(const Grant&v){u(v.destination);text(v.definition);i(v.quantity);meta(v.meta);}
};
struct Reader {
    const std::vector<std::uint8_t>&bytes;std::size_t p=0;
    void room(std::size_t n){need(p<=bytes.size()&&n<=bytes.size()-p,Error::Corrupt,"truncated snapshot");}
    std::uint64_t u(){room(8);std::uint64_t v=0;for(int b=0;b<8;++b)v|=static_cast<std::uint64_t>(bytes[p++])<<(8*b);return v;}
    std::int64_t i(){auto n=u();return n<=MaxId?static_cast<std::int64_t>(n):-1-static_cast<std::int64_t>(~n);}
    std::int32_t i32(){auto n=i();need(n>=std::numeric_limits<std::int32_t>::min()&&n<=std::numeric_limits<std::int32_t>::max(),Error::Corrupt,"32-bit field overflow");return static_cast<std::int32_t>(n);}
    bool b(){room(1);auto v=bytes[p++];need(v<=1,Error::Corrupt,"invalid boolean");return v!=0;}
    std::size_t size(std::size_t max){auto n=u();need(n<=max,Error::Budget,"decoded size limit");return static_cast<std::size_t>(n);}
    std::string text(std::size_t max=1024){auto n=size(max);room(n);std::string v(bytes.begin()+static_cast<std::ptrdiff_t>(p),bytes.begin()+static_cast<std::ptrdiff_t>(p+n));p+=n;return v;}
    std::vector<std::uint8_t> blob(std::size_t max){auto n=size(max);room(n);std::vector<std::uint8_t>v(bytes.begin()+static_cast<std::ptrdiff_t>(p),bytes.begin()+static_cast<std::ptrdiff_t>(p+n));p+=n;return v;}
    std::set<std::string> names(){std::set<std::string>v;auto n=size(128);for(std::size_t k=0;k<n;++k)need(v.insert(text(128)).second,Error::Corrupt,"duplicate tag");return v;}
    std::map<std::string,Count> counts(){std::map<std::string,Count>v;auto n=size(128);for(std::size_t k=0;k<n;++k){auto key=text(128);auto value=i();need(v.emplace(key,value).second,Error::Corrupt,"duplicate amount");}return v;}
    Meta meta(){Meta m;m.quality=i32();m.durability=i32();m.expiresAt=i();m.remainingShelfLife=i();m.decayUpdatedAt=i();m.decayRemainderBasisPoints=i32();m.provenance=text(256);auto n=size(16);for(std::size_t k=0;k<n;++k){auto key=text(128);auto val=text();need(m.properties.emplace(key,val).second,Error::Corrupt,"duplicate property");}return m;}
    Container ct(){Container v;v.id=u();v.owner=text(128);v.delegates=names();v.tags=names();v.acceptedTags=names();v.publicAccess=b();v.slots=i32();v.maxMassGrams=i();v.gridWidth=i32();v.gridHeight=i32();v.parentItem=u();v.money=counts();v.decayRateBasisPoints=i32();v.craftQueueLimit=i32();v.craftEnabled=b();return v;}
};
std::uint32_t crc(const std::vector<std::uint8_t>&b,std::size_t size){std::uint32_t n=0xffffffffU;for(std::size_t k=0;k<size;++k){n^=b[k];for(int j=0;j<8;++j)n=(n>>1)^(0xedb88320U&(0U-(n&1U)));}return ~n;}
std::vector<std::uint8_t> catalogBytes(const Catalog& c) {
    Writer w;w.u(c.definitions.size());for(const auto&[id,d]:c.definitions){w.text(id);w.text(d.id);w.text(d.category);w.i(d.maxStack);w.i(d.unitMassGrams);w.i(d.width);w.i(d.height);w.u(d.shape.size());for(const auto&cell:d.shape){w.i(cell.x);w.i(cell.y);}w.names(d.tags);w.names(d.equipmentSlots);w.u(d.attachmentSlots.size());for(const auto&[slot,tags]:d.attachmentSlots){w.text(slot);w.names(tags);}w.i(d.bagSlots);w.i(d.bagMaxMassGrams);w.i(d.defaultShelfLifeSeconds);w.text(d.decaysTo);w.counts(d.repairInputs);w.i(d.rarity);}
    w.u(c.recipes.size());for(const auto&[id,r]:c.recipes){w.text(id);w.text(r.id);w.text(r.stationTag);w.counts(r.inputs);w.counts(r.outputs);w.counts(r.fuelInputs);w.names(r.requiredTools);w.i(r.durationSeconds);w.b(r.requiresUnlock);w.i(r.minQuality);w.i(r.toolDurabilityCost);}
    w.u(c.lootTables.size());for(const auto&[id,t]:c.lootTables){w.text(id);w.text(t.id);w.u(t.rolls);w.u(t.entries.size());for(const auto&e:t.entries){w.text(e.definition);w.i(e.minimum);w.i(e.maximum);w.u(e.weight);}}
    w.names(c.currencies);return w.bytes;
}
std::vector<std::uint8_t> requestBytes(const Command& request) {
    Writer w;w.u(request.expectedRevision);w.u(request.operation.index());
    std::visit([&](const auto&v){using T=std::decay_t<decltype(v)>;
        if constexpr(std::is_same_v<T,CreateContainer>)w.ct(v.value);
        else if constexpr(std::is_same_v<T,Grant>)w.grantValue(v);
        else if constexpr(std::is_same_v<T,Transfer>){w.u(v.item);w.u(v.destination);w.i(v.quantity);w.i(v.slot);w.i(v.x);w.i(v.y);w.b(v.rotated);}
        else if constexpr(std::is_same_v<T,TransferAll>||std::is_same_v<T,QuickStack>){w.u(v.source);w.u(v.destination);}
        else if constexpr(std::is_same_v<T,Merge>){w.u(v.source);w.u(v.target);w.i(v.quantity);}
        else if constexpr(std::is_same_v<T,Consume>||std::is_same_v<T,Discard>){w.u(v.item);w.i(v.quantity);}
        else if constexpr(std::is_same_v<T,Hotbar>){w.i(v.slot);w.u(v.item);}
        else if constexpr(std::is_same_v<T,Equip>){w.u(v.item);w.b(v.remove);}
        else if constexpr(std::is_same_v<T,SetItemFlags>){w.u(v.item);w.b(v.setFavorite);w.b(v.favorite);w.b(v.setLocked);w.b(v.locked);}
        else if constexpr(std::is_same_v<T,Trade>){w.u(v.vendor);w.u(v.item);w.u(v.customerInventory);w.i(v.quantity);w.i(v.quotedTotal);w.b(v.buying);}
        else if constexpr(std::is_same_v<T,RestockVendor>){w.u(v.vendor);w.u(v.generation);}
        else if constexpr(std::is_same_v<T,StartCraft>){w.text(v.recipe);w.u(v.station);w.u(v.destination);w.u(v.sources.size());for(auto id:v.sources)w.u(id);w.i(v.batches);}
        else if constexpr(std::is_same_v<T,FinishCraft>||std::is_same_v<T,CancelCraft>)w.u(v.job);
        else if constexpr(std::is_same_v<T,Repair>){w.u(v.item);w.u(v.sources.size());for(auto id:v.sources)w.u(id);w.i(v.targetDurability);}
        else if constexpr(std::is_same_v<T,Attach>){w.u(v.host);w.u(v.attachment);w.text(v.slot);}
        else if constexpr(std::is_same_v<T,Detach>){w.u(v.attachment);w.u(v.destination);w.i(v.slot);w.i(v.x);w.i(v.y);w.b(v.rotated);}
        else if constexpr(std::is_same_v<T,AdvanceTime>)w.i(v.now);
        else if constexpr(std::is_same_v<T,Unlock>){w.text(v.actor);w.text(v.recipe);}
        else if constexpr(std::is_same_v<T,RollLoot>){w.text(v.source);w.u(v.generation);w.u(v.destination);}
        else if constexpr(std::is_same_v<T,Award>){w.text(v.eventId);w.u(v.grants.size());for(const auto&g:v.grants)w.grantValue(g);}
    },request.operation);return w.bytes;
}
std::vector<std::uint8_t> encode(const Catalog& c,const State& s) {
    Writer w;w.text("RBITEMS3");w.u(3);w.blob(catalogBytes(c));w.u(s.revision);w.u(s.nextId);w.i(s.now);
    w.u(s.containers.size());for(const auto&[id,v]:s.containers){w.u(id);w.ct(v);}
    w.u(s.items.size());for(const auto&[id,v]:s.items){w.u(id);w.u(v.id);w.u(v.container);w.text(v.definition);w.i(v.quantity);w.meta(v.meta);w.i(v.slot);w.i(v.x);w.i(v.y);w.b(v.rotated);w.u(v.childContainer);w.u(v.attachmentContainer);w.b(v.favorite);w.b(v.locked);}
    w.u(s.jobs.size());for(const auto&[id,j]:s.jobs){w.u(id);w.u(j.id);w.u(j.station);w.u(j.destination);w.text(j.owner);w.text(j.recipe);w.i(j.startedAt);w.i(j.due);w.i(j.batches);w.u(j.inputs.size());for(const auto&r:j.inputs){w.u(r.item);w.i(r.quantity);}w.u(j.fuels.size());for(const auto&r:j.fuels){w.u(r.item);w.i(r.quantity);}w.u(j.tools.size());for(Id tool:j.tools)w.u(tool);}
    w.u(s.vendors.size());for(const auto&[id,v]:s.vendors){w.u(id);w.u(v.id);w.u(v.stock);w.text(v.currency);w.counts(v.sells);w.counts(v.buys);w.names(v.refusedTags);w.i(v.sellMultiplierBasisPoints);w.i(v.buyMultiplierBasisPoints);w.i(v.restockEverySeconds);w.i(v.nextRestockAt);w.u(v.restockGeneration);w.counts(v.restockTargets);w.counts(v.restockBatches);}
    w.u(s.sources.size());for(const auto&[id,v]:s.sources){w.text(id);w.text(v.id);w.text(v.table);w.u(v.generation);w.u(v.seed);w.i(v.availableAt);w.i(v.cooldownSeconds);w.b(v.exhausted);w.b(v.renewable);}
    w.u(s.hotbars.size());for(const auto&[actor,slots]:s.hotbars){w.text(actor);w.u(slots.size());for(const auto&[slot,id]:slots){w.i(slot);w.u(id);}}
    w.u(s.equipment.size());for(const auto&[actor,slots]:s.equipment){w.text(actor);w.u(slots.size());for(const auto&[slot,id]:slots){w.text(slot);w.u(id);}}
    w.u(s.unlocks.size());for(const auto&[actor,recipes]:s.unlocks){w.text(actor);w.names(recipes);}
    w.u(s.receipts.size());for(const auto&[actor,r]:s.receipts){w.text(actor);w.u(r.sequence);w.u(r.revision);w.blob(r.request);w.u(r.created.size());for(Id id:r.created)w.u(id);}
    w.u(s.awards.size());for(const auto&id:s.awards)w.text(id);w.room(4);auto sum=crc(w.bytes,w.bytes.size());for(int b=0;b<4;++b)w.bytes.push_back(static_cast<std::uint8_t>(sum>>(8*b)));return w.bytes;
}
State decode(const Catalog& c,const std::vector<std::uint8_t>&bytes) {
    need(bytes.size()>=4&&bytes.size()<=MaxSnapshotBytes,Error::Corrupt,"snapshot size invalid");std::uint32_t expected=0;for(int b=0;b<4;++b)expected|=static_cast<std::uint32_t>(bytes[bytes.size()-4+static_cast<std::size_t>(b)])<<(8*b);need(crc(bytes,bytes.size()-4)==expected,Error::Corrupt,"snapshot checksum mismatch");
    Reader r{bytes};need(r.text()=="RBITEMS3"&&r.u()==3,Error::Corrupt,"unsupported snapshot format; explicit migration required");need(r.blob(MaxSnapshotBytes)==catalogBytes(c),Error::CatalogMismatch,"catalog changed; explicit migration required");State s;s.revision=r.u();s.nextId=r.u();s.now=r.i();auto unique=[](auto&map,auto key,auto value){need(map.emplace(std::move(key),std::move(value)).second,Error::Corrupt,"duplicate snapshot record");};
    auto n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.u();auto v=r.ct();unique(s.containers,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.u();Item v;v.id=r.u();v.container=r.u();v.definition=r.text(128);v.quantity=r.i();v.meta=r.meta();v.slot=r.i32();v.x=r.i32();v.y=r.i32();v.rotated=r.b();v.childContainer=r.u();v.attachmentContainer=r.u();v.favorite=r.b();v.locked=r.b();unique(s.items,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.u();CraftJob v;v.id=r.u();v.station=r.u();v.destination=r.u();v.owner=r.text(128);v.recipe=r.text(128);v.startedAt=r.i();v.due=r.i();v.batches=r.i();auto m=r.size(MaxRecords);for(std::size_t j=0;j<m;++j)v.inputs.push_back({r.u(),r.i()});m=r.size(MaxRecords);for(std::size_t j=0;j<m;++j)v.fuels.push_back({r.u(),r.i()});m=r.size(128);for(std::size_t j=0;j<m;++j)v.tools.push_back(r.u());unique(s.jobs,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.u();Vendor v;v.id=r.u();v.stock=r.u();v.currency=r.text(128);v.sells=r.counts();v.buys=r.counts();v.refusedTags=r.names();v.sellMultiplierBasisPoints=r.i32();v.buyMultiplierBasisPoints=r.i32();v.restockEverySeconds=r.i();v.nextRestockAt=r.i();v.restockGeneration=r.u();v.restockTargets=r.counts();v.restockBatches=r.counts();unique(s.vendors,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.text(128);Source v;v.id=r.text(128);v.table=r.text(128);v.generation=r.u();v.seed=r.u();v.availableAt=r.i();v.cooldownSeconds=r.i();v.exhausted=r.b();v.renewable=r.b();unique(s.sources,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.text(128);std::map<std::int32_t,Id>v;auto m=r.size(32);for(std::size_t j=0;j<m;++j){auto slot=r.i32();auto id=r.u();unique(v,slot,id);}unique(s.hotbars,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.text(128);std::map<std::string,Id>v;auto m=r.size(128);for(std::size_t j=0;j<m;++j){auto slot=r.text(128);auto id=r.u();unique(v,slot,id);}unique(s.equipment,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.text(128);auto v=r.names();unique(s.unlocks,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k){auto key=r.text(128);Receipt v;v.sequence=r.u();v.revision=r.u();v.request=r.blob(1024*1024);auto m=r.size(MaxRecords);for(std::size_t j=0;j<m;++j)v.created.push_back(r.u());unique(s.receipts,key,std::move(v));}
    n=r.size(MaxRecords);for(std::size_t k=0;k<n;++k)need(s.awards.insert(r.text(128)).second,Error::Corrupt,"duplicate production award");need(r.p==bytes.size()-4,Error::Corrupt,"trailing snapshot data");return s;
}
void validateAll(const Catalog& c,const State& s) {
    need(c.definitions.size()<=MaxRecords&&c.recipes.size()<=10000&&c.lootTables.size()<=10000,Error::Budget,"catalog budget exceeded");checkNames(c.currencies);
    for(const auto&[key,d]:c.definitions){need(key==d.id&&validName(key)&&(d.category.empty()||validName(d.category)),Error::Invalid,"invalid definition identity");need(d.maxStack>0&&d.unitMassGrams>=0&&d.width>0&&d.width<=64&&d.height>0&&d.height<=64,Error::Invalid,"invalid definition limits");checkNames(d.tags);checkNames(d.equipmentSlots);need(d.attachmentSlots.size()<=32,Error::Budget,"attachment slot budget exceeded");for(const auto&[slot,tags]:d.attachmentSlots){need(validName(slot),Error::Invalid,"invalid attachment slot");checkNames(tags);}need(d.shape.size()<=static_cast<std::size_t>(d.width*d.height),Error::Budget,"shape cell budget exceeded");std::set<std::pair<int,int>> shape;for(const auto&cell:d.shape)need(cell.x>=0&&cell.y>=0&&cell.x<d.width&&cell.y<d.height&&shape.emplace(cell.x,cell.y).second,Error::Invalid,"invalid or duplicate shape cell");need(d.bagSlots>=0&&d.bagSlots<=4096&&d.bagMaxMassGrams>=0&&d.defaultShelfLifeSeconds>=-1&&d.rarity>=0&&d.rarity<=10000,Error::Invalid,"invalid definition limits");need((!d.bagSlots&&d.equipmentSlots.empty()&&d.attachmentSlots.empty())||d.maxStack==1,Error::Invalid,"bag/equipment/attachment host must be unstackable");need(!d.bagSlots||d.equipmentSlots.empty(),Error::Invalid,"equipped bag policy deferred");need(d.repairInputs.size()<=128,Error::Budget,"repair recipe too large");for(const auto&[def,qty]:d.repairInputs){definition(c,def);need(qty>0,Error::Invalid,"invalid repair input");}}
    for(const auto&[key,d]:c.definitions)if(!d.decaysTo.empty()){const auto& target=definition(c,d.decaysTo);need(!d.bagSlots&&d.equipmentSlots.empty()&&!target.bagSlots&&target.equipmentSlots.empty()&&target.maxStack>=d.maxStack&&target.width==d.width&&target.height==d.height&&target.shape==d.shape&&target.attachmentSlots==d.attachmentSlots,Error::Invalid,"decay transform must preserve safe storage shape and stack capacity");}
    for(const auto&[key,r]:c.recipes){need(key==r.id&&validName(key)&&validName(r.stationTag)&&r.durationSeconds>=0&&r.minQuality>=0&&r.minQuality<=10000&&r.toolDurabilityCost>=0&&r.toolDurabilityCost<=10000,Error::Invalid,"invalid recipe");need(!r.inputs.empty()&&!r.outputs.empty()&&r.inputs.size()<=128&&r.outputs.size()<=128&&r.fuelInputs.size()<=128&&r.requiredTools.size()<=32,Error::Invalid,"invalid recipe sizes");for(const auto&[id,q]:r.inputs)need(q>0&&!definition(c,id).bagSlots,Error::Invalid,"invalid recipe input");for(const auto&[id,q]:r.outputs){definition(c,id);need(q>0,Error::Invalid,"invalid output");}for(const auto&[id,q]:r.fuelInputs)need(q>0&&!definition(c,id).bagSlots,Error::Invalid,"invalid fuel input");for(const auto&id:r.requiredTools)need(definition(c,id).maxStack==1,Error::Invalid,"crafting tools must be unique items");}
    for(const auto&[key,t]:c.lootTables){need(key==t.id&&validName(key)&&t.rolls>0&&t.rolls<=64&&!t.entries.empty()&&t.entries.size()<=128,Error::Invalid,"invalid loot table");for(const auto&e:t.entries){definition(c,e.definition);need(e.minimum>=0&&e.maximum>=e.minimum&&e.maximum<=1000000&&e.weight>0,Error::Invalid,"invalid loot entry");}}
    need(s.revision<MaxId&&s.nextId>0&&s.nextId<=MaxId&&s.now>=0,Error::Invalid,"invalid world counters/time");need(s.items.size()<=MaxRecords&&s.containers.size()<=MaxRecords&&s.jobs.size()<=MaxRecords&&s.vendors.size()<=MaxRecords&&s.sources.size()<=MaxRecords&&s.receipts.size()<=MaxRecords&&s.awards.size()<=MaxRecords&&s.hotbars.size()<=MaxRecords&&s.equipment.size()<=MaxRecords&&s.unlocks.size()<=MaxRecords,Error::Budget,"record budget exceeded");
    std::set<Id> allIds;auto checkId=[&](Id key,Id value){need(key==value&&value>0&&value<s.nextId&&allIds.insert(value).second,Error::Invalid,"duplicate or invalid persistent ID");};std::map<Id,Count>masses,counts;std::map<Id,std::set<int>>occupied;
    for(const auto&[id,v]:s.containers){checkId(id,v.id);need(v.slots>0&&v.slots<=4096&&v.maxMassGrams>=0&&v.gridWidth>=0&&v.gridWidth<=64&&v.gridHeight>=0&&v.gridHeight<=64&&((v.gridWidth==0)==(v.gridHeight==0))&&v.decayRateBasisPoints>=0&&v.decayRateBasisPoints<=1000000&&v.craftQueueLimit>0&&v.craftQueueLimit<=128,Error::Invalid,"invalid container dimensions/capacity");checkNames(v.delegates);checkNames(v.tags);checkNames(v.acceptedTags);if(v.parentItem){const auto& parent=item(s,v.parentItem);need((parent.childContainer==id||parent.attachmentContainer==id)&&parent.quantity==1,Error::Invalid,"nested-container back-reference mismatch");need(v.owner.empty()&&!v.publicAccess&&v.delegates.empty(),Error::Invalid,"nested inventory cannot override access");}else need(validName(v.owner),Error::Invalid,"root owner identity required");need(v.money.size()<=128,Error::Budget,"currency count exceeded");for(const auto&[currency,amount]:v.money)need(c.currencies.contains(currency)&&amount>=0,Error::Invalid,"invalid currency");root(s,id);masses[id]=0;counts[id]=0;}
    for(const auto&[id,v]:s.items){checkId(id,v.id);checkMeta(v.meta);const auto&d=definition(c,v.definition);const auto&ct=container(s,v.container);need(v.quantity>0&&v.quantity<=d.maxStack,Error::Invalid,"invalid stack count");need(v.meta.decayUpdatedAt<=s.now,Error::Invalid,"item decay timestamp is in the future");if(!ct.acceptedTags.empty())need(std::any_of(d.tags.begin(),d.tags.end(),[&](const auto&t){return ct.acceptedTags.contains(t);}),Error::Forbidden,"container tag filter");if(d.bagSlots){const auto&child=container(s,v.childContainer);need(child.parentItem==id&&child.slots==d.bagSlots&&child.maxMassGrams==d.bagMaxMassGrams&&!child.gridWidth,Error::Invalid,"bag capacity/identity mismatch");}else need(!v.childContainer,Error::Invalid,"non-bag has child container");
        if(!d.attachmentSlots.empty()){const auto&sockets=container(s,v.attachmentContainer);need(sockets.parentItem==id&&sockets.slots==static_cast<std::int32_t>(d.attachmentSlots.size())&&!sockets.gridWidth,Error::Invalid,"attachment container mismatch");}else need(!v.attachmentContainer,Error::Invalid,"item without attachment slots has attachment container");
        if(ct.parentItem){const auto&host=item(s,ct.parentItem);if(host.attachmentContainer==ct.id){const auto&spec=attachmentSlotAt(c,s,ct,v.slot);need(v.quantity==1&&!v.childContainer&&!v.attachmentContainer&&(spec.second.empty()||sharesAny(d.tags,spec.second)),Error::Invalid,"invalid installed attachment");}}
        ++counts[v.container];auto& cellset=occupied[v.container];if(!ct.gridWidth)need(v.slot>=0&&v.slot<ct.slots&&cellset.insert(v.slot).second,Error::Capacity,"slot collision or out of bounds");else {const int w=shapeWidth(d,v.rotated),h=shapeHeight(d,v.rotated);need(v.x>=0&&v.y>=0&&v.x<=ct.gridWidth-w&&v.y<=ct.gridHeight-h,Error::Capacity,"grid bounds");for(const auto&cell:cells(d,v.rotated))need(cellset.insert((v.y+cell.y)*ct.gridWidth+(v.x+cell.x)).second,Error::Capacity,"grid collision");}Count ownMass=mul(v.quantity,d.unitMassGrams);Id cid=v.container;std::set<Id>chain;for(;;){need(chain.insert(cid).second&&chain.size()<=32,Error::Invalid,"bag cycle");masses[cid]=add(masses[cid],ownMass);const auto&p=container(s,cid);if(!p.parentItem)break;cid=item(s,p.parentItem).container;}}
    for (const auto& [id,ct] : s.containers) {
        need(masses[id] <= ct.maxMassGrams && counts[id] <= ct.slots, Error::Capacity, "recursive mass or slot limit");
    }
    std::map<Id,Count> reservations;
    std::map<Id,std::size_t> queueCounts;
    for(const auto&[id,j]:s.jobs){checkId(id,j.id);auto ri=c.recipes.find(j.recipe);need(ri!=c.recipes.end()&&validName(j.owner)&&j.batches>0&&j.startedAt>=0&&j.startedAt<=s.now,Error::Invalid,"invalid craft job");const auto&recipe=ri->second;need(j.due==add(j.startedAt,mul(recipe.durationSeconds,j.batches)),Error::Invalid,"invalid craft deadline");need(access(s,j.owner,j.station)&&access(s,j.owner,j.destination),Error::Forbidden,"craft lost access");need(container(s,j.station).tags.contains(recipe.stationTag),Error::Invalid,"craft station mismatch");++queueCounts[j.station];std::map<std::string,Count>amounts,fuels;std::set<Id>seen;need(j.inputs.size()+j.fuels.size()<=MaxRecords&&j.tools.size()<=32,Error::Budget,"reservation budget exceeded");for(const auto&r:j.inputs){const auto&i=item(s,r.item);need(r.quantity>0&&seen.insert(r.item).second&&!i.childContainer&&!equipped(s,i.id)&&!i.locked&&access(s,j.owner,i.container)&&i.meta.quality>=recipe.minQuality,Error::Invalid,"invalid reservation");reservations[i.id]=add(reservations[i.id],r.quantity);amounts[i.definition]=add(amounts[i.definition],r.quantity);}for(const auto&r:j.fuels){const auto&i=item(s,r.item);need(r.quantity>0&&seen.insert(r.item).second&&!i.childContainer&&!equipped(s,i.id)&&!i.locked&&access(s,j.owner,i.container),Error::Invalid,"invalid fuel reservation");reservations[i.id]=add(reservations[i.id],r.quantity);fuels[i.definition]=add(fuels[i.definition],r.quantity);}need(amounts.size()==recipe.inputs.size()&&fuels.size()==recipe.fuelInputs.size(),Error::Invalid,"reserved ingredient/fuel set mismatch");for(const auto&[def,qty]:recipe.inputs)need(amounts.contains(def)&&amounts.at(def)==mul(qty,j.batches),Error::Invalid,"reserved ingredient quantity mismatch");for(const auto&[def,qty]:recipe.fuelInputs)need(fuels.contains(def)&&fuels.at(def)==mul(qty,j.batches),Error::Invalid,"reserved fuel quantity mismatch");std::set<std::string>tools;for(Id tool:j.tools){const auto&i=item(s,tool);need(seen.insert(tool).second&&access(s,j.owner,i.container)&&i.quantity==1,Error::Invalid,"invalid craft tool");reservations[tool]=add(reservations[tool],1);tools.insert(i.definition);}need(tools==recipe.requiredTools,Error::Invalid,"craft tool set mismatch");}
    for (const auto& [station,n] : queueCounts) {
        need(n <= static_cast<std::size_t>(container(s,station).craftQueueLimit), Error::Capacity, "craft queue exceeds station capacity");
    }
    for (const auto& [id,n] : reservations) {
        need(n <= item(s,id).quantity, Error::Reserved, "stack over-reserved");
    }
    for(const auto&[id,v]:s.vendors){checkId(id,v.id);container(s,v.stock);need(c.currencies.contains(v.currency)&&v.sellMultiplierBasisPoints>=0&&v.sellMultiplierBasisPoints<=1000000&&v.buyMultiplierBasisPoints>=0&&v.buyMultiplierBasisPoints<=1000000&&v.restockEverySeconds>=0&&v.nextRestockAt>=0&&v.restockGeneration<=MaxId,Error::Invalid,"invalid vendor");checkNames(v.refusedTags);need(v.sells.size()<=128&&v.buys.size()<=128&&v.restockTargets.size()<=128&&v.restockBatches.size()<=128,Error::Budget,"vendor list limit");for(const auto&[def,p]:v.sells){definition(c,def);need(p>=0,Error::Invalid,"negative sell price");}for(const auto&[def,p]:v.buys){definition(c,def);need(p>=0,Error::Invalid,"negative buy price");}for(const auto&[def,n]:v.restockTargets){definition(c,def);need(n>=0&&v.sells.contains(def),Error::Invalid,"invalid restock target");}for(const auto&[def,n]:v.restockBatches){need(v.restockTargets.contains(def)&&n>0,Error::Invalid,"invalid restock batch");}}
    for(const auto&[key,v]:s.sources)need(key==v.id&&validName(key)&&c.lootTables.contains(v.table)&&v.generation<=MaxId&&v.availableAt>=0&&v.cooldownSeconds>=0,Error::Invalid,"invalid resource source");
    for(const auto&[actor,slots]:s.hotbars){need(validName(actor)&&slots.size()<=32,Error::Invalid,"invalid hotbar owner");for(const auto&[slot,id]:slots)need(slot>=0&&slot<32&&owns(s,actor,item(s,id).container)&&!installedAttachment(s,id),Error::Invalid,"dangling/unowned/installed hotbar reference");}
    for(const auto&[actor,slots]:s.equipment){need(validName(actor)&&slots.size()<=128,Error::Invalid,"invalid equipment owner");for(const auto&[slot,id]:slots){const auto&i=item(s,id);const auto&d=definition(c,i.definition);need(i.quantity==1&&owns(s,actor,i.container)&&!installedAttachment(s,i.id)&&d.equipmentSlots.contains(slot)&&fresh(i,s.now)&&i.meta.durability>0,Error::Invalid,"invalid equipment reference");for(const auto&required:d.equipmentSlots)need(slots.contains(required)&&slots.at(required)==id,Error::Invalid,"incomplete multi-slot equipment");}}
    for(const auto&[actor,recipes]:s.unlocks){need(validName(actor),Error::Invalid,"invalid unlock owner");checkNames(recipes);for(const auto&id:recipes)need(c.recipes.contains(id),Error::Invalid,"unknown unlocked recipe");}
    for(const auto&[actor,r]:s.receipts){need(validName(actor)&&r.sequence>0&&r.sequence<=MaxId&&r.revision<=s.revision&&!r.request.empty()&&r.request.size()<=1024*1024&&r.created.size()<=MaxRecords,Error::Invalid,"invalid receipt");for(Id id:r.created)need(id>0&&id<s.nextId,Error::Invalid,"invalid receipt identity");}
    for(const auto&id:s.awards)need(validName(id),Error::Invalid,"invalid award identity");
}

bool fastEligible(const Operation& op) {
    return std::visit([](const auto& cmd){using T=std::decay_t<decltype(cmd)>;
        return std::is_same_v<T,Transfer>||std::is_same_v<T,Merge>||std::is_same_v<T,Consume>||std::is_same_v<T,Discard>||std::is_same_v<T,Hotbar>||std::is_same_v<T,Equip>||std::is_same_v<T,SetItemFlags>;
    },op);
}
Count itemMassForTransfer(const Catalog& c,const State& s,const Item& i,Count quantity) {
    Count total=mul(quantity,definition(c,i.definition).unitMassGrams);
    if(i.childContainer||i.attachmentContainer){need(quantity==i.quantity,Error::Invalid,"cannot split nested inventory");if(i.childContainer){std::set<Id> chain;total=add(total,massRecursive(c,s,i.childContainer,chain));}if(i.attachmentContainer){std::set<Id> chain;total=add(total,massRecursive(c,s,i.attachmentContainer,chain));}}
    return total;
}
bool containerWithin(const State& s,Id child,Id ancestor) {
    Id cid=child;std::set<Id> chain;for(;;){if(cid==ancestor)return true;need(chain.insert(cid).second&&chain.size()<=32,Error::Invalid,"nested container cycle");const auto& ct=container(s,cid);if(!ct.parentItem)return false;cid=item(s,ct.parentItem).container;}
}
void checkDestinationMass(const Catalog& c,const State& s,Id source,Id destination,Count movingMass) {
    Id cid=destination;std::set<Id> chain;
    for(;;){need(chain.insert(cid).second&&chain.size()<=32,Error::Invalid,"nested destination cycle");std::set<Id> massChain;const auto& ct=container(s,cid);const Count current=massRecursive(c,s,cid,massChain);if(!containerWithin(s,source,cid))need(add(current,movingMass)<=ct.maxMassGrams,Error::Capacity,"destination mass limit");else need(current<=ct.maxMassGrams,Error::Capacity,"destination mass limit");if(!ct.parentItem)break;cid=item(s,ct.parentItem).container;}
}
void checkBagDestination(const State& s,const Item& moving,Id destination) {
    if(!moving.childContainer&&!moving.attachmentContainer)return;Id cid=destination;std::set<Id> chain;
    for(;;){need(chain.insert(cid).second&&chain.size()<=32,Error::Invalid,"nested destination cycle");const auto& ct=container(s,cid);if(!ct.parentItem)break;need(ct.parentItem!=moving.id,Error::Invalid,"cannot move bag into itself");cid=item(s,ct.parentItem).container;}
}
void fastTransfer(const Catalog& c,State& s,const Context& ctx,const Transfer& t,Result& result) {
    const Item original=item(s,t.item);need(t.quantity>0&&t.quantity<=original.quantity,Error::Invalid,"invalid transfer quantity");const auto& dest=container(s,t.destination);permit(s,ctx,original.container);permit(s,ctx,t.destination);need(!original.locked,Error::Locked,"item is locked");need(!equipped(s,t.item),Error::Equipped,"unequip before moving");need(t.quantity<=original.quantity-reservedIn(s,t.item),Error::Reserved,"quantity reserved for crafting");const auto& d=definition(c,original.definition);if(!dest.acceptedTags.empty())need(sharesAny(d.tags,dest.acceptedTags),Error::Forbidden,"container tag filter");checkBagDestination(s,original,t.destination);checkDestinationMass(c,s,original.container,t.destination,itemMassForTransfer(c,s,original,t.quantity));
    Item moved=original;moved.container=t.destination;projectExpiry(dest,moved.meta,s.now);if(t.quantity<original.quantity){need(!original.childContainer&&!original.attachmentContainer,Error::Invalid,"cannot split nested inventory");moved.id=s.nextId;need(moved.id>0&&moved.id<MaxId,Error::Budget,"ID allocation exhausted");moved.quantity=t.quantity;}place(c,s,moved,t.slot,t.x,t.y,t.rotated);
    if(t.quantity<original.quantity){++s.nextId;s.items.at(t.item).quantity-=t.quantity;result.created.push_back(moved.id);}s.items.insert_or_assign(moved.id,std::move(moved));pruneHotbars(s);
}
void operateFast(const Catalog& c,State& s,const Context& ctx,const Operation& operation,Result& result) {
    std::visit([&](const auto& cmd){using T=std::decay_t<decltype(cmd)>;
        if constexpr(std::is_same_v<T,Transfer>) fastTransfer(c,s,ctx,cmd,result);
        else if constexpr(std::is_same_v<T,Merge>){need(cmd.source!=cmd.target,Error::Invalid,"cannot merge item into itself");const auto a=item(s,cmd.source),b=item(s,cmd.target);permit(s,ctx,a.container);permit(s,ctx,b.container);need(!a.locked&&!b.locked,Error::Locked,"locked stack cannot be merged");need(a.definition==b.definition&&a.meta==b.meta&&!a.childContainer&&!b.childContainer,Error::Invalid,"incompatible stack metadata");need(!equipped(s,a.id)&&!equipped(s,b.id),Error::Equipped,"cannot merge equipped items");need(cmd.quantity>0&&cmd.quantity<=a.quantity-reservedIn(s,a.id),Error::Reserved,"invalid or reserved merge quantity");need(cmd.quantity<=definition(c,a.definition).maxStack-b.quantity,Error::Capacity,"stack limit");s.items.at(b.id).quantity+=cmd.quantity;if(cmd.quantity==a.quantity){for(auto&[actor,slots]:s.hotbars)if(owns(s,actor,b.container))for(auto&[slot,id]:slots){(void)slot;if(id==a.id)id=b.id;}eraseItem(s,a.id);}else s.items.at(a.id).quantity-=cmd.quantity;pruneHotbars(s);}
        else if constexpr(std::is_same_v<T,Consume>||std::is_same_v<T,Discard>){permit(s,ctx,item(s,cmd.item).container);consume(s,cmd.item,cmd.quantity,std::is_same_v<T,Discard>);}
        else if constexpr(std::is_same_v<T,Hotbar>){need(cmd.slot>=0&&cmd.slot<32,Error::Invalid,"invalid hotbar slot");if(cmd.item){need(owns(s,ctx.actor,item(s,cmd.item).container),Error::Forbidden,"hotbar requires possession");need(!installedAttachment(s,cmd.item),Error::Equipped,"detach attachment before hotbar binding");s.hotbars[ctx.actor][cmd.slot]=cmd.item;}else s.hotbars[ctx.actor].erase(cmd.slot);}
        else if constexpr(std::is_same_v<T,Equip>){
            const auto& i=item(s,cmd.item);need(owns(s,ctx.actor,i.container),Error::Forbidden,"equipment requires possession");need(!installedAttachment(s,i.id),Error::Equipped,"installed attachment cannot be equipped separately");
            if(cmd.remove){auto actorIt=s.equipment.find(ctx.actor);if(actorIt!=s.equipment.end()){auto& slots=actorIt->second;for(auto it=slots.begin();it!=slots.end();)if(it->second==i.id)it=slots.erase(it);else ++it;if(slots.empty())s.equipment.erase(actorIt);}}
            else{const auto& d=definition(c,i.definition);need(i.quantity==1&&!d.equipmentSlots.empty()&&!i.childContainer,Error::Invalid,"not single-instance equipment");need(fresh(i,s.now)&&i.meta.durability>0,Error::Expired,"expired or broken equipment");need(!reservedIn(s,i.id),Error::Reserved,"equipment reserved");auto existing=s.equipment.find(ctx.actor);if(existing!=s.equipment.end())for(const auto& slot:d.equipmentSlots)need(!existing->second.contains(slot)||existing->second.at(slot)==i.id,Error::Conflict,"equipment slot occupied");auto& slots=s.equipment[ctx.actor];for(const auto& slot:d.equipmentSlots)slots[slot]=i.id;}
        }
        else if constexpr(std::is_same_v<T,SetItemFlags>){auto& i=s.items.at(item(s,cmd.item).id);need(owns(s,ctx.actor,i.container)||ctx.privileged,Error::Forbidden,"item flags require ownership");if(cmd.setLocked&&cmd.locked)need(!reservedIn(s,i.id),Error::Reserved,"reserved item cannot be locked");if(cmd.setFavorite)i.favorite=cmd.favorite;if(cmd.setLocked)i.locked=cmd.locked;}
    },operation);
}
} // Internal implementation.

struct Engine::Impl { Catalog catalog; std::unique_ptr<State> state; std::shared_ptr<int> identity=std::make_shared<int>(0); std::uint64_t epoch=0; std::thread::id thread=std::this_thread::get_id(); };
struct Engine::Prepared::Data { std::shared_ptr<int> identity; std::uint64_t epoch=0; std::unique_ptr<State> state; Result result; };
Engine::Prepared::Prepared(std::unique_ptr<Data>d):data_(std::move(d)){}Engine::Prepared::~Prepared()=default;
Engine::Engine(Catalog c,State s):impl_(std::make_unique<Impl>()){validateAll(c,s);(void)encode(c,s);impl_->catalog=std::move(c);impl_->state=std::make_unique<State>(std::move(s));}
Engine::~Engine()=default;
const State& Engine::state() const {need(std::this_thread::get_id()==impl_->thread,Error::WrongThread,"state view thread violation");return *impl_->state;}
const Catalog& Engine::catalog() const {need(std::this_thread::get_id()==impl_->thread,Error::WrongThread,"catalog view thread violation");return impl_->catalog;}
Result Engine::validate(const Catalog&c,const State&s){try{validateAll(c,s);return {};}catch(const Failure&e){return {e.error,e.what(),s.revision,{},false};}catch(const std::exception&e){return {Error::Invalid,e.what(),s.revision,{},false};}}
Engine::Preparation Engine::prepare(const Context&ctx,const Command&command) const {
    if(std::this_thread::get_id()!=impl_->thread)return {{Error::WrongThread,"engine thread violation",0,{},false},nullptr};
    try {need(validName(ctx.actor)&&command.sequence>0&&command.sequence<=MaxId,Error::Invalid,"invalid actor/sequence");auto bytes=requestBytes(command);need(bytes.size()<=1024*1024,Error::Budget,"request budget exceeded");const auto&current=*impl_->state;auto old=current.receipts.find(ctx.actor);auto last=old==current.receipts.end()?0:old->second.sequence;if(command.sequence==last){need(old->second.request==bytes,Error::ReplayMismatch,"same sequence with different payload");return {{Error::None,"previous accepted receipt",old->second.revision,old->second.created,true},nullptr};}need(command.sequence==last+1,Error::Stale,"old or out-of-order request");need(command.expectedRevision==current.revision,Error::Conflict,"stale economy revision");need(current.revision<MaxId-1&&impl_->epoch<MaxId,Error::Budget,"revision exhausted");auto data=std::make_unique<Prepared::Data>();data->identity=impl_->identity;data->epoch=impl_->epoch;data->state=std::make_unique<State>(current);data->result.revision=current.revision+1;operate(impl_->catalog,*data->state,ctx,command.operation,data->result);data->state->revision=data->result.revision;data->state->receipts[ctx.actor]={command.sequence,data->result.revision,std::move(bytes),data->result.created};validateAll(impl_->catalog,*data->state);(void)encode(impl_->catalog,*data->state);auto result=data->result;return {std::move(result),std::unique_ptr<Prepared>(new Prepared(std::move(data)))};
    }catch(const Failure&e){return {{e.error,e.what(),impl_->state->revision,{},false},nullptr};}catch(const std::exception&e){return {{Error::Invalid,e.what(),impl_->state->revision,{},false},nullptr};}
}
Result Engine::commit(std::unique_ptr<Prepared> candidate){if(std::this_thread::get_id()!=impl_->thread)return {Error::WrongThread,"engine thread violation",0,{},false};if(!candidate||!candidate->data_)return {Error::Invalid,"empty prepared transaction",impl_->state->revision,{},false};auto&data=*candidate->data_;if(data.identity!=impl_->identity||data.epoch!=impl_->epoch)return {Error::Conflict,"foreign or stale prepared transaction",impl_->state->revision,{},false};impl_->state.swap(data.state);++impl_->epoch;return std::move(data.result);}
Result Engine::execute(const Context&ctx,const Command&command){
    if(!fastEligible(command.operation)){auto p=prepare(ctx,command);if(!p.result||p.result.replayed)return p.result;return commit(std::move(p.candidate));}
    if(std::this_thread::get_id()!=impl_->thread)return {Error::WrongThread,"engine thread violation",0,{},false};
    try {
        need(validName(ctx.actor)&&command.sequence>0&&command.sequence<=MaxId,Error::Invalid,"invalid actor/sequence");auto bytes=requestBytes(command);need(bytes.size()<=1024*1024,Error::Budget,"request budget exceeded");auto& current=*impl_->state;auto old=current.receipts.find(ctx.actor);auto last=old==current.receipts.end()?0:old->second.sequence;
        if(command.sequence==last){need(old->second.request==bytes,Error::ReplayMismatch,"same sequence with different payload");return {Error::None,"previous accepted receipt",old->second.revision,old->second.created,true};}
        need(command.sequence==last+1,Error::Stale,"old or out-of-order request");need(command.expectedRevision==current.revision,Error::Conflict,"stale economy revision");need(current.revision<MaxId-1&&impl_->epoch<MaxId,Error::Budget,"revision exhausted");Result result;result.revision=current.revision+1;operateFast(impl_->catalog,current,ctx,command.operation,result);current.revision=result.revision;current.receipts[ctx.actor]={command.sequence,result.revision,std::move(bytes),result.created};++impl_->epoch;return result;
    } catch(const Failure&e){return {e.error,e.what(),impl_->state->revision,{},false};} catch(const std::exception&e){return {Error::Invalid,e.what(),impl_->state->revision,{},false};}
}
Quote Engine::quote(Id vendor,Id iid,Count quantity,bool buying) const {if(std::this_thread::get_id()!=impl_->thread)return {Error::WrongThread,0,0};try{return {Error::None,price(impl_->catalog,*impl_->state,vendor,iid,quantity,buying),impl_->state->revision};}catch(const Failure&e){return {e.error,0,impl_->state->revision};}}
Count Engine::reserved(Id id) const{return reservedIn(state(),id);}Count Engine::mass(Id id) const{const auto&s=state();container(s,id);std::set<Id>chain;return massRecursive(impl_->catalog,s,id,chain);}bool Engine::canAccess(const std::string&actor,Id id) const{return access(state(),actor,id);}std::vector<std::uint8_t> Engine::snapshot() const{return encode(impl_->catalog,state());}
Result Engine::restore(const std::vector<std::uint8_t>&bytes){if(std::this_thread::get_id()!=impl_->thread)return {Error::WrongThread,"restore thread violation",0,{},false};try{need(impl_->epoch<MaxId,Error::Budget,"epoch exhausted");auto candidate=std::make_unique<State>(decode(impl_->catalog,bytes));validateAll(impl_->catalog,*candidate);impl_->state.swap(candidate);++impl_->epoch;return {Error::None,"restored",impl_->state->revision,{},false};}catch(const Failure&e){return {e.error,e.what(),impl_->state->revision,{},false};}catch(const std::exception&e){return {Error::Corrupt,e.what(),impl_->state->revision,{},false};}}
const char* errorName(Error e) noexcept {switch(e){
#define RB_ERROR_NAME(x) case Error::x:return #x;
    RB_ERROR_NAME(None) RB_ERROR_NAME(Invalid) RB_ERROR_NAME(Forbidden) RB_ERROR_NAME(Missing) RB_ERROR_NAME(Capacity) RB_ERROR_NAME(Conflict) RB_ERROR_NAME(Stale) RB_ERROR_NAME(ReplayMismatch) RB_ERROR_NAME(Insufficient) RB_ERROR_NAME(Reserved) RB_ERROR_NAME(Equipped) RB_ERROR_NAME(NotReady) RB_ERROR_NAME(Expired) RB_ERROR_NAME(Corrupt) RB_ERROR_NAME(CatalogMismatch) RB_ERROR_NAME(Budget) RB_ERROR_NAME(WrongThread) RB_ERROR_NAME(Locked)
#undef RB_ERROR_NAME
}return "Unknown";}
}
