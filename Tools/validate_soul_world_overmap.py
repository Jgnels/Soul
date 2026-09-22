"""Validate Soul world-overmap v1 without Unreal."""
import json
from collections import defaultdict, deque
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/"Data"/"soul_world_overmap_v1_20260922.json"
FULL=ROOT/"Evidence"/"WorldOvermap"/"soul_world_overmap_v1.svg"
SLICE=ROOT/"Evidence"/"WorldOvermap"/"soul_founder_slice_overmap_v1.svg"
FOUNDER_SEED=ROOT/"Data"/"soul_founder_slice_runtime_seed_20260922.json"
PRESENTATION=ROOT/"Data"/"soul_overmap_presentation_contract_v1_20260922.json"
SURFACES=ROOT/"Data"/"soul_overmap_surface_bindings_v1_20260922.json"
OUT=ROOT/"Evidence"/"WorldOvermap"/"validation.json"

d=json.loads(DATA.read_text(encoding="utf-8"))
nodes={n["id"]:n for n in d["nodes"]}
edges=d["edges"]
errors=[]

battlefield_data=json.loads((ROOT/"Data"/"battlefield_recipes.json").read_text(encoding="utf-8"))
battlefield_ids={x["id"] for x in battlefield_data["recipes"]}
city_data=json.loads((ROOT/"Data"/"city_siege_blueprints.json").read_text(encoding="utf-8"))
city_ids={x["id"] for x in city_data["cities"]}
macro_ids={x["id"] for x in d["macro_regions"]}
presentation=json.loads(PRESENTATION.read_text(encoding="utf-8"))
surfaces=json.loads(SURFACES.read_text(encoding="utf-8"))
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
founder_seed=json.loads(FOUNDER_SEED.read_text(encoding="utf-8"))
seed_regions=founder_seed.get("regions",{})
if set(seed_regions) != set(founder):
    errors.append("founder runtime seed region set does not match founder slice")
for rid,region in seed_regions.items():
    if any(x not in seed_regions for x in region.get("neighbors",[])):
        errors.append(f"founder runtime seed leaks outside region: {rid}")
    if any(x not in seed_regions for x in region.get("road_neighbors",[])):
        errors.append(f"founder runtime road seed leaks outside region: {rid}")
    if set(region.get("approach_from_neighbor",{})) - set(seed_regions):
        errors.append(f"founder runtime approach seed leaks outside region: {rid}")
    if not region.get("site_roles"):
        errors.append(f"founder runtime site roles missing: {rid}")
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
    elif n["battle_recipe_hint"] not in battlefield_ids:
        errors.append(f"node battlefield recipe not found: {n['id']} -> {n['battle_recipe_hint']}")
    if n["macro_region"] not in macro_ids:
        errors.append(f"node macro region not found: {n['id']} -> {n['macro_region']}")
    if n.get("settlement_id") and n["settlement_id"] not in city_ids:
        errors.append(f"node settlement binding not found: {n['id']} -> {n['settlement_id']}")
    if not (0 <= n["x"] <= 1000 and 0 <= n["y"] <= 950):
        errors.append(f"node coordinate out of range: {n['id']}")
for e in edges:
    if e.get("action_cost") != 1:
        errors.append(f"edge action cost must remain one founder-playtest action: {e}")
    logistics_cost=e.get("logistics_movement_cost", e["movement_cost"])
    if not 6 <= logistics_cost <= 10:
        errors.append(f"edge logistics cost outside initial balance band: {e}")

if set(surfaces.get("macro_regions",{})) != macro_ids:
    errors.append("surface-plan macro regions do not match overmap macro regions")
if presentation.get("terrain",{}).get("final_region_nodes_visible") is not False:
    errors.append("presentation contract must hide logical region nodes by default")
if presentation.get("terrain",{}).get("final_province_borders_visible") is not False:
    errors.append("presentation contract must hide permanent province borders by default")
authority=presentation.get("authority",{})
for domain,name in (("weather","RB Weather"),("optimization","RB Optimization"),("save","RB Save")):
    if authority.get(domain) != name:
        errors.append(f"presentation authority drift: {domain} != {name}")

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
