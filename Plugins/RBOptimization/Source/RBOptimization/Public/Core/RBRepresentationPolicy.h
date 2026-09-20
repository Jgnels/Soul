#pragma once
// Original RefinedBadger implementation. Engine-independent production policy.
// Coordinates are Unreal units. IDs are persistent domain keys, never ISM indices.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace rbopt {
using Id = std::uint64_t;
struct Point { double x=0, y=0, z=0; };
inline bool valid(Point p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z)
        && std::abs(p.x)<=1e12 && std::abs(p.y)<=1e12 && std::abs(p.z)<=1e12;
}
inline double distance2(Point a, Point b) {
    const double x=a.x-b.x, y=a.y-b.y, z=a.z-b.z;
    return x*x+y*y+z*z;
}
enum class Representation { Instance, Actor, Consumed };
struct Config {
    double enterRadius=500, exitRadius=750, minimumResidence=2, cellSize=750;
    std::size_t maxChanges=8, maxItems=100000, maxViewers=32;
    bool valid() const {
        return std::isfinite(enterRadius) && std::isfinite(exitRadius)
            && std::isfinite(minimumResidence) && std::isfinite(cellSize)
            && enterRadius>0 && exitRadius>enterRadius && minimumResidence>=0
            && cellSize>0 && cellSize<=1e9 && exitRadius/cellSize<=8
            && maxChanges>0 && maxChanges<=4096 && maxItems>0 && maxItems<=1000000
            && maxViewers>0 && maxViewers<=128;
    }
};
struct Node {
    Id id=0; Point position; Representation representation=Representation::Instance;
    std::uint64_t revision=1; std::uint32_t pins=0; bool safeToRetire=false;
    double keepUntil=0, lastChange=0;
};
struct Change { Id id=0; std::uint64_t revision=0; Representation to=Representation::Actor; };
struct Plan { bool validInput=false; std::size_t inspected=0; std::vector<Change> changes; };
class Registry {
    using Cell=std::tuple<std::int64_t,std::int64_t,std::int64_t>;
    Config config_; bool usable_=false;
    std::map<Id,Node> nodes_;
    std::map<Cell,std::set<Id>> cells_;
    std::set<Id> active_, requested_;
    Cell cell(Point p) const {
        return {static_cast<std::int64_t>(std::floor(p.x/config_.cellSize)),
                static_cast<std::int64_t>(std::floor(p.y/config_.cellSize)),
                static_cast<std::int64_t>(std::floor(p.z/config_.cellSize))};
    }
    void removeCell(const Node& n) {
        auto it=cells_.find(cell(n.position));
        if(it!=cells_.end()) { it->second.erase(n.id); if(it->second.empty()) cells_.erase(it); }
    }
public:
    explicit Registry(Config c={}) : config_(c), usable_(c.valid() && c.cellSize>=1) {}
    bool usable() const { return usable_; }
    std::size_t size() const { return nodes_.size(); }
    std::size_t activeCount() const { return active_.size(); }
    std::vector<Id> activeIds() const { return {active_.begin(),active_.end()}; }
    bool updatePosition(Id id, Point p) {
        auto it=nodes_.find(id);
        if(it==nodes_.end() || !valid(p) || it->second.representation==Representation::Consumed) return false;
        auto& n=it->second;
        if(n.position.x==p.x && n.position.y==p.y && n.position.z==p.z) return true;
        removeCell(n); n.position=p; cells_[cell(p)].insert(id); ++n.revision; return true;
    }
    const Node* get(Id id) const { auto it=nodes_.find(id); return it==nodes_.end()?nullptr:&it->second; }
    bool add(Id id, Point p) {
        if(!usable_ || id==0 || !valid(p) || nodes_.count(id) || nodes_.size()>=config_.maxItems) return false;
        Node n; n.id=id; n.position=p; nodes_.emplace(id,n); cells_[cell(p)].insert(id); return true;
    }
    bool request(Id id, double now, double hold) {
        auto it=nodes_.find(id);
        if(it==nodes_.end() || !std::isfinite(now) || now<0 || !std::isfinite(hold)
            || hold<=0 || !std::isfinite(now+hold) || it->second.representation==Representation::Consumed) return false;
        auto& n=it->second; if(now<n.lastChange) return false; n.keepUntil=std::max(n.keepUntil,now+hold); ++n.revision;
        if(n.representation==Representation::Instance) requested_.insert(id);
        return true;
    }
    bool guard(Id id, std::uint32_t pins, bool safe) {
        auto it=nodes_.find(id); if(it==nodes_.end() || it->second.representation==Representation::Consumed) return false;
        auto& n=it->second;
        if(pins && n.representation==Representation::Instance) requested_.insert(id);
        if(n.pins!=pins || n.safeToRetire!=safe) { n.pins=pins; n.safeToRetire=safe; ++n.revision; }
        return true;
    }
    Plan plan(const std::vector<Point>& viewers, double now) const {
        Plan p;
        if(!usable_ || !std::isfinite(now) || now<0 || viewers.size()>config_.maxViewers) return p;
        for(auto v:viewers) if(!valid(v)) return p;
        p.validInput=true;
        std::set<Id> candidates=active_; candidates.insert(requested_.begin(),requested_.end());
        for(auto v:viewers) {
            auto [cx,cy,cz]=cell(v);
            const auto reach=static_cast<std::int64_t>(std::ceil(config_.exitRadius/config_.cellSize));
            for(auto x=cx-reach;x<=cx+reach;++x) for(auto y=cy-reach;y<=cy+reach;++y)
                for(auto z=cz-reach;z<=cz+reach;++z) {
                    auto it=cells_.find({x,y,z}); if(it!=cells_.end()) candidates.insert(it->second.begin(),it->second.end());
                }
        }
        struct Ranked { Change change; int priority; double distance; };
        std::vector<Ranked> ranked;
        for(Id id:candidates) {
            const auto& n=nodes_.at(id); ++p.inspected;
            if(n.representation==Representation::Consumed) continue;
            double d=std::numeric_limits<double>::infinity();
            for(auto v:viewers) d=std::min(d,distance2(v,n.position));
            const bool forced=n.pins!=0 || now<n.keepUntil;
            if(n.representation==Representation::Instance && (forced || d<=config_.enterRadius*config_.enterRadius))
                ranked.push_back({{id,n.revision,Representation::Actor},forced?0:1,d});
            // Missing views never grants permission to dematerialize an actor.
            if(n.representation==Representation::Actor && !viewers.empty() && !forced && n.safeToRetire
                && now>=n.lastChange+config_.minimumResidence && d>config_.exitRadius*config_.exitRadius)
                ranked.push_back({{id,n.revision,Representation::Instance},2,-d});
        }
        std::sort(ranked.begin(),ranked.end(),[](const Ranked&a,const Ranked&b) {
            return std::tie(a.priority,a.distance,a.change.id)<std::tie(b.priority,b.distance,b.change.id);
        });
        for(const auto& r:ranked) { if(p.changes.size()==config_.maxChanges) break; p.changes.push_back(r.change); }
        return p;
    }
    // Call only after the host has completed the matching representation change.
    // A stale token is rejected, so adapters must prevent reentrant mutations.
    bool commit(Change c, Point p, double now) {
        auto it=nodes_.find(c.id);
        if(it==nodes_.end() || !valid(p) || !std::isfinite(now) || now<0) return false;
        auto& n=it->second;
        if(n.revision!=c.revision || n.representation==Representation::Consumed || n.representation==c.to
            || now<n.lastChange) return false;
        if(c.to!=Representation::Actor && c.to!=Representation::Instance && c.to!=Representation::Consumed) return false;
        if(c.to==Representation::Instance && (!n.safeToRetire || n.pins || now<n.keepUntil
            || now<n.lastChange+config_.minimumResidence)) return false;
        removeCell(n); n.position=p; n.representation=c.to; n.lastChange=now; ++n.revision;
        active_.erase(c.id); requested_.erase(c.id);
        if(c.to==Representation::Actor) active_.insert(c.id);
        if(c.to!=Representation::Consumed) cells_[cell(p)].insert(c.id);
        return true;
    }
    // Failed activation requests must not make distant, inactive populations a permanent scan.
    std::size_t expireRequests(double now) {
        if(!std::isfinite(now) || now<0) return 0;
        std::size_t removed=0;
        for(auto it=requested_.begin();it!=requested_.end();) {
            const auto& n=nodes_.at(*it);
            if(!n.pins && now>=n.keepUntil) { it=requested_.erase(it); ++removed; }
            else ++it;
        }
        return removed;
    }
    // This is the only resurrection path: an explicit domain-authorized respawn.
    bool revive(Id id, Point p, double now) {
        auto it=nodes_.find(id);
        if(it==nodes_.end() || !valid(p) || !std::isfinite(now) || now<it->second.lastChange
            || it->second.representation!=Representation::Consumed) return false;
        auto& n=it->second; n.position=p; n.representation=Representation::Instance;
        n.pins=0; n.safeToRetire=false; n.keepUntil=0; n.lastChange=now; ++n.revision;
        cells_[cell(p)].insert(id); return true;
    }
    bool consume(Id id, double now) {
        const auto* n=get(id); if(!n) return false;
        return commit({id,n->revision,Representation::Consumed},n->position,now);
    }
};
} // namespace rbopt
