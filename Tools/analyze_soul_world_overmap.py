"""Analyze Soul overmap topology and strategic travel without Unreal."""
import heapq
import json
from collections import defaultdict, deque
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/"Data"/"soul_world_overmap_v1_20260922.json"
OUT_JSON=ROOT/"Evidence"/"WorldOvermap"/"route_analysis.json"
OUT_MD=ROOT/"Evidence"/"WorldOvermap"/"route_analysis.md"

d=json.loads(DATA.read_text(encoding="utf-8"))
nodes={n["id"]:n for n in d["nodes"]}
edges=d["edges"]
adj=defaultdict(list)
for e in edges:
    adj[e["a"]].append((e["b"],e))
    adj[e["b"]].append((e["a"],e))

def bfs_hops(start):
    dist={start:0}; q=deque([start])
    while q:
        cur=q.popleft()
        for nxt,_ in adj[cur]:
            if nxt not in dist:
                dist[nxt]=dist[cur]+1; q.append(nxt)
    return dist

def dijkstra(start):
    dist={start:0}; prev={}
    q=[(0,start)]
    while q:
        cost,cur=heapq.heappop(q)
        if cost!=dist[cur]:
            continue
        for nxt,e in adj[cur]:
            step=e.get("logistics_movement_cost",e["movement_cost"])
            nc=cost+step
            if nc < dist.get(nxt,10**9):
                dist[nxt]=nc; prev[nxt]=cur
                heapq.heappush(q,(nc,nxt))
    return dist,prev

def simple_paths(start,goal,max_edges=7):
    paths=[]
    def walk(cur,path):
        if len(path)-1>max_edges:
            return
        if cur==goal:
            paths.append(path[:]); return
        for nxt,_ in adj[cur]:
            if nxt not in path:
                walk(nxt,path+[nxt])
    walk(start,[start])
    return paths

def articulation_points():
    time=0; disc={}; low={}; parent={}; result=set()
    def dfs(u):
        nonlocal time
        time+=1; disc[u]=low[u]=time; children=0
        for v,_ in adj[u]:
            if v not in disc:
                parent[v]=u; children+=1; dfs(v); low[u]=min(low[u],low[v])
                if u not in parent and children>1:
                    result.add(u)
                if u in parent and low[v]>=disc[u]:
                    result.add(u)
            elif parent.get(u)!=v:
                low[u]=min(low[u],disc[v])
    for u in nodes:
        if u not in disc:
            dfs(u)
    return sorted(result)

def bridge_edges():
    time=0; disc={}; low={}; parent={}; bridges=[]
    def dfs(u):
        nonlocal time
        time+=1; disc[u]=low[u]=time
        for v,_ in adj[u]:
            if v not in disc:
                parent[v]=u; dfs(v); low[u]=min(low[u],low[v])
                if low[v]>disc[u]:
                    bridges.append(tuple(sorted((u,v))))
            elif parent.get(u)!=v:
                low[u]=min(low[u],disc[v])
    for u in nodes:
        if u not in disc:
            dfs(u)
    return sorted(set(bridges))

capitals={n["owner"]:n["id"] for n in nodes.values() if n["kind"]=="capital"}
capital_matrix={}
for faction,start in capitals.items():
    hops=bfs_hops(start)
    costs,_=dijkstra(start)
    capital_matrix[faction]={}
    for other,target in capitals.items():
        capital_matrix[faction][other]={
            "hops":hops[target],
            "logistics_cost":costs[target],
        }

founder_paths=simple_paths("human_capital","orc_camp",6)
founder_paths=sorted(founder_paths,key=lambda x:(len(x),x))
slice_ids=set(d["founder_slice"]["region_ids"])
slice_paths=[p for p in founder_paths if all(x in slice_ids for x in p)]
slice_paths=[p for p in slice_paths if len(p)<=5]

articulation=articulation_points()
bridges=bridge_edges()
designated={tuple(sorted((e["a"],e["b"]))) for e in edges if e["chokepoint"]}
undesignated_bridges=[b for b in bridges if b not in designated]
degree={k:len(v) for k,v in adj.items()}

result={
    "schema":1,
    "status":"pass",
    "capital_matrix":capital_matrix,
    "founder_human_to_orc_short_paths":slice_paths,
    "founder_short_route_count":len(slice_paths),
    "articulation_points":articulation,
    "bridge_edges":[list(x) for x in bridges],
    "undesignated_bridge_edges":[list(x) for x in undesignated_bridges],
    "degree":degree,
    "notes":[
        "Hops model one AP per region move in the current founder slice.",
        "Logistics cost is independent of AP and uses the initial 6-10 edge band.",
        "Bridges are graph bridges, not necessarily physical bridges.",
    ],
}
OUT_JSON.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")

lines=[
    "# Soul World Overmap — Route Analysis",
    "",
    "## Founder Human-to-Orc approaches",
]
for path in slice_paths:
    lines.append("- " + " -> ".join(nodes[x]["name"] for x in path))
lines += ["", "## Faction-seat travel matrix", ""]
factions=sorted(capitals)
lines.append("| From | To | Hops | Logistics cost |")
lines.append("|---|---|---:|---:|")

for a in factions:
    for b in factions:
        if a>=b:
            continue
        v=capital_matrix[a][b]
        lines.append(f"| {a} | {b} | {v['hops']} | {v['logistics_cost']} |")

lines += [
    "",
    "## Structural choke audit",
    "",
    f"- Articulation points: {', '.join(articulation) if articulation else 'none'}.",
    f"- Graph-bridge edges: {len(bridges)}.",
    f"- Undesignated graph bridges: {len(undesignated_bridges)}.",
]
if undesignated_bridges:
    for a,b in undesignated_bridges:
        lines.append(f"  - {nodes[a]['name']} <-> {nodes[b]['name']}")
lines += [
    "",
    "Interpretation: an undesignated graph bridge is a place where losing one connection cuts the strategic graph.",
    "Some are desirable geographic gates; accidental ones should receive an alternate route before the map is treated as a broad campaign.",
]
OUT_MD.write_text("\n".join(lines)+"\n",encoding="utf-8")
print(json.dumps(result,indent=2))
