#pragma once
#include "Core/RBItemEconomyCore.h"
#include <stdexcept>
namespace RB::Items::Test {
inline Catalog catalog() {
    Catalog c;c.currencies={"coin"};
    auto def=[&](const char* id,Count stack,Count mass,const char* category="resource"){
        Definition d;d.id=id;d.category=category;d.maxStack=stack;d.unitMassGrams=mass;d.tags={category};c.definitions.emplace(id,d);
    };
    def("wheat",10,100);def("water",10,1000);def("bread",10,200,"food");def("spoiled_bread",10,200,"waste");def("wood",10,500);def("coal",10,300,"fuel");
    c.definitions.at("bread").defaultShelfLifeSeconds=100;c.definitions.at("bread").decaysTo="spoiled_bread";
    def("sword",1,1000,"weapon");c.definitions.at("sword").equipmentSlots={"mainhand"};c.definitions.at("sword").repairInputs={{"wood",2}};
    def("greatsword",1,2000,"weapon");c.definitions.at("greatsword").equipmentSlots={"mainhand","offhand"};
    def("shield",1,2000,"armor");c.definitions.at("shield").equipmentSlots={"offhand"};
    def("hammer",1,800,"tool");
    def("contraband",10,50,"contraband");
    def("rifle",1,3000,"weapon");c.definitions.at("rifle").equipmentSlots={"mainhand"};c.definitions.at("rifle").attachmentSlots={{"muzzle",{"muzzle"}},{"optic",{"optic"}}};
    def("scope",1,300,"attachment");c.definitions.at("scope").tags.insert("optic");
    def("suppressor",1,400,"attachment");c.definitions.at("suppressor").tags.insert("muzzle");
    def("grip",1,250,"attachment");c.definitions.at("grip").tags.insert("grip");    def("bag",1,500,"container");c.definitions.at("bag").bagSlots=4;c.definitions.at("bag").bagMaxMassGrams=10000;
    def("rectangle",1,100,"shape");c.definitions.at("rectangle").width=2;c.definitions.at("rectangle").height=1;
    def("lshape",1,100,"shape");c.definitions.at("lshape").width=2;c.definitions.at("lshape").height=2;c.definitions.at("lshape").shape={{0,0},{1,0},{0,1}};
    Recipe bread;bread.id="bread";bread.stationTag="oven";bread.inputs={{"water",1},{"wheat",2}};bread.outputs={{"bread",1}};bread.durationSeconds=10;bread.minQuality=1000;c.recipes.emplace(bread.id,bread);
    Recipe powered=bread;powered.id="powered_bread";powered.fuelInputs={{"coal",1}};powered.requiredTools={"hammer"};powered.toolDurabilityCost=250;c.recipes.emplace(powered.id,powered);
    Recipe secret;secret.id="secret";secret.stationTag="oven";secret.inputs={{"wheat",1}};secret.outputs={{"bread",1}};secret.durationSeconds=0;secret.requiresUnlock=true;secret.minQuality=9000;c.recipes.emplace(secret.id,secret);
    c.lootTables.emplace("herbs",LootTable{"herbs",{{"wheat",1,3,2},{"wood",1,2,1}},2});
    return c;
}
inline State initial() {
    State s;s.nextId=6;
    Container a;a.id=1;a.owner="alice";a.money["coin"]=100;
    Container b=a;b.id=2;b.owner="bob";b.money["coin"]=20;
    Container stock=a;stock.id=3;stock.owner="merchant";stock.money["coin"]=100;
    Container station=a;station.id=4;station.owner="world";station.publicAccess=true;station.tags={"oven"};station.money.clear();
    s.containers={{1,a},{2,b},{3,stock},{4,station}};
    Vendor vendor;vendor.id=5;vendor.stock=3;vendor.currency="coin";vendor.sells={{"bread",5},{"wheat",2},{"contraband",20}};vendor.buys={{"bread",3},{"wheat",1},{"contraband",10}};vendor.restockEverySeconds=20;vendor.nextRestockAt=20;vendor.restockTargets={{"bread",4},{"wheat",6}};vendor.restockBatches={{"bread",2},{"wheat",3}};s.vendors.emplace(vendor.id,vendor);
    s.sources.emplace("chest",Source{"chest","herbs",0,123,0,10,false,false});
    s.sources.emplace("bush",Source{"bush","herbs",0,456,0,10,false,true});
    return s;
}
inline Context host(){return {"host",true,{},{}};}
inline Context alice(){return {"alice",false,{5},{"chest","bush"}};}
inline Context bob(){return {"bob",false,{5},{"chest","bush"}};}
inline Command next(const Engine& e,const Context& ctx,Operation op) {
    auto p=e.state().receipts.find(ctx.actor);
    return {p==e.state().receipts.end()?1:p->second.sequence+1,e.state().revision,std::move(op)};
}
inline Result run(Engine& e,const Context& ctx,Operation op){return e.execute(ctx,next(e,ctx,std::move(op)));}
inline Result must(Engine& e,const Context& ctx,Operation op){auto r=run(e,ctx,std::move(op));if(!r)throw std::runtime_error(std::string(errorName(r.error))+": "+r.message);return r;}
inline Id give(Engine& e,Id destination,const std::string& def,Count qty=1,Meta meta={}) {
    auto r=must(e,host(),Grant{destination,def,qty,std::move(meta)});
    if(!r.created.empty())return r.created.front();
    for(const auto&[id,i]:e.state().items)if(i.container==destination && i.definition==def)return id;
    throw std::runtime_error("grant produced no item");
}
inline Count total(const Engine& e,const std::string& def) {
    Count n=0;for(const auto&[id,i]:e.state().items){(void)id;if(i.definition==def)n+=i.quantity;}return n;
}
}
