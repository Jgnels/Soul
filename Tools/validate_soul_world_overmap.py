"""Validate Soul world-overmap v1 without Unreal."""
import json
from collections import defaultdict, deque
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/"Data"/"soul_world_overmap_v1_20260922.json"
FULL=ROOT/"Evidence"/"WorldOvermap"/"soul_world_overmap_v1.svg"
SLICE=ROOT/"Evidence"/"WorldOvermap"/"soul_founder_slice_overmap_v1.svg"
OUT=ROOT/"Evidence"/"WorldOvermap"/"validation.json"

d=json.loads(DATA.read_text(encoding="utf-8"))
nodes={n["id"]:n for n in d["nodes"]}
edges=d["edges"]
errors=[]
adj=defaultdict(list)
for e in edges:
    if e["a"] not in nodes or e["b"] not in nodes:
        errors.append(f"edge endpoint missing: {e}")
        continue
    if e["a"]==e["b"]:
        errors.append(f"self edge: {e['a']}")
    adj[e["a"]].append(e["b"]); adj[e["b"]].append(e["a"])

seen=set()
if nodes:
    start=next(iter(nodes))
    q=deque([start]); seen.add(start)
    while q:
        cur=q.popleft()
        for nxt in adj[cur]:
            if nxt not in seen:
                seen.add(nxt); q.append(nxt)
if len(seen)!=len(nodes):
    errors.append(f"world graph disconnected: visited {len(seen)}/{len(nodes)}")

founder=d["founder_slice"]["region_ids"]
if len(founder)!=9 or len(set(founder))!=9:
    errors.append("founder slice must contain exactly 9 unique regions")
for n in founder:
    if n not in nodes:
        errors.append(f"founder node missing: {n}")

expected={
    frozenset(("human_capital","crossroads")),
    frozenset(("crossroads","old_quarry")),
    frozenset(("crossroads","river_ford")),
    frozenset(("crossroads","forest_edge")),
    frozenset(("old_quarry","ancient_shrine")),
    frozenset(("river_ford","orc_watch")),
    frozenset(("forest_edge","orc_watch")),
    frozenset(("forest_edge","north_pass")),
    frozenset(("north_pass","orc_camp")),
    frozenset(("orc_watch","orc_camp")),
}
actual={frozenset((e["a"],e["b"])) for e in edges if e["a"] in founder and e["b"] in founder}
if actual!=expected:
    errors.append(f"founder topology mismatch: expected {len(expected)} edges got {len(actual)}")

capitals=[n for n in nodes.values() if n["kind"]=="capital"]
owners={n["owner"] for n in capitals}
required={"humans","vikings","dwarves","orcs","dark","nature"}
if owners!=required:
    errors.append(f"capital owners mismatch: {sorted(str(x) for x in owners)}")
for n in capitals:
    if len(adj[n["id"]]) < 2:
        errors.append(f"capital lacks two strategic exits: {n['id']}")
    if not n.get("settlement_id"):
        errors.append(f"capital missing settlement id: {n['id']}")

for n in nodes.values():
    if not n.get("battle_recipe_hint"):
        errors.append(f"node missing battle recipe hint: {n['id']}")
    if not (0 <= n["x"] <= 1000 and 0 <= n["y"] <= 950):
        errors.append(f"node coordinate out of range: {n['id']}")
for e in edges:
    if e.get("action_cost") != 1:
        errors.append(f"edge action cost must remain one founder-playtest action: {e}")
    logistics_cost=e.get("logistics_movement_cost", e["movement_cost"])
    if not 6 <= logistics_cost <= 10:
        errors.append(f"edge logistics cost outside initial balance band: {e}")

for svg in (FULL,SLICE):
    try:
        ET.parse(svg)
    except Exception as exc:
        errors.append(f"invalid svg {svg.name}: {exc}")

def shortest(start,goal):
    q=deque([(start,0)]); used={start}
    while q:
        cur,distance=q.popleft()
        if cur==goal: return distance
        for nxt in adj[cur]:
            if nxt not in used:
                used.add(nxt); q.append((nxt,distance+1))
    return None

capital_hops={n["id"]:shortest("human_capital",n["id"]) for n in capitals}

result={
    "status":"pass" if not errors else "fail",
    "nodes":len(nodes),
    "edges":len(edges),
    "founder_slice_regions":len(founder),
    "founder_slice_edges":len(actual),
    "capitals":len(capitals),
    "capital_hops_from_human":capital_hops,
    "road_edges":sum(1 for e in edges if e["road"]),
    "chokepoints":sum(1 for e in edges if e["chokepoint"]),
    "errors":errors,
}
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps(result,indent=2))
if errors:
    raise SystemExit(1)
