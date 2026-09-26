"""Validate Soul campaign start-state overlays without Unreal."""
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
START = json.loads((ROOT/"Data"/"soul_campaign_start_states_v1_20260922.json").read_text(encoding="utf-8"))
PROJECTION = json.loads((ROOT/"Data"/"soul_campaign_start_state_v1_20260922.json").read_text(encoding="utf-8"))
SLOTS = json.loads((ROOT/"Data"/"soul_overmap_settlement_slots_v1_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT/"Evidence"/"WorldOvermap"/"start_state_validation.json"

nodes = {n["id"]:n for n in WORLD["nodes"]}
adj = defaultdict(set)
for e in WORLD["edges"]:
    adj[e["a"]].add(e["b"])
    adj[e["b"]].add(e["a"])
slot_by_region = {s["region_id"]:s for s in SLOTS["slots"]}

errors = []
sandbox = START["scenarios"]["six_faction_sandbox_candidate"]
factions = sandbox["factions"]
owners = sandbox["region_owners"]

if set(factions) != {"humans","vikings","dwarves","orcs","nature","dark"}:
    errors.append("six-faction sandbox must contain exactly the six current factions")
if len(owners) != 12:
    errors.append(f"expected 12 owned regions (2/faction), got {len(owners)}")
if len(sandbox["neutral_regions"]) != len(nodes)-len(owners):
    errors.append("neutral region count does not complement owned region count")
for faction,state in factions.items():
    possessions = state["starting_possessions"]
    if len(possessions) != 2 or len(set(possessions)) != 2:
        errors.append(f"{faction}: expected exactly two unique starting possessions")
        continue
    capital,minor = state["capital_region"],state["secondary_settlement_region"]
    if capital not in possessions or minor not in possessions:
        errors.append(f"{faction}: capital/minor not both in possession list")
    if owners.get(capital) != faction or owners.get(minor) != faction:
        errors.append(f"{faction}: ownership table disagrees with faction state")
    if minor not in adj[capital]:
        errors.append(f"{faction}: capital and minor settlement must be adjacent")
    if slot_by_region.get(capital,{}).get("tier") != "major":
        errors.append(f"{faction}: capital region is not a major settlement slot")
    if slot_by_region.get(minor,{}).get("tier") != "minor":
        errors.append(f"{faction}: secondary region is not a minor settlement slot")
    if state["starting_army"]["region"] != capital:
        errors.append(f"{faction}: starting army must anchor at capital")
    if state["secondary_garrison"]["region"] != minor:
        errors.append(f"{faction}: secondary garrison must anchor at minor settlement")
    if state["army_spawn_anchors"][0]["region_id"] != capital:
        errors.append(f"{faction}: primary spawn anchor must be capital")
    if state["army_spawn_anchors"][1]["region_id"] != minor:
        errors.append(f"{faction}: reserve spawn anchor must be minor settlement")
    frontier = set(state["immediate_neutral_frontier_candidates"])
    if len(frontier) < 2:
        errors.append(f"{faction}: fewer than two immediate frontier choices")
    if any(owners.get(r) not in (None,faction) for r in frontier):
        errors.append(f"{faction}: immediate frontier includes hostile-owned region")
    for target in state["safe_expansion_targets"]:
        if target not in frontier:
            errors.append(f"{faction}: safe expansion target is not immediate frontier: {target}")
# No faction starts directly adjacent to another faction-owned region.
hostile_edges=set()
for rid,faction in owners.items():
    for other in adj[rid]:
        other_owner=owners.get(other)
        if other_owner and other_owner != faction:
            hostile_edges.add(tuple(sorted((rid,other))))
if hostile_edges:
    errors.append(f"direct hostile starting borders: {sorted(hostile_edges)}")

# Founder micro remains the current prototype, not the sandbox diplomacy.
founder=START["scenarios"]["founder_human_orc_micro"]
expected_founder={
    "human_capital":"humans",
    "crossroads":"humans",
    "orc_watch":"orcs",
    "orc_camp":"orcs",
}
if founder["owners"] != expected_founder:
    errors.append(f"founder ownership drift: {founder['owners']}")
if founder.get("diplomacy",{}).get("humans_vs_orcs") != "war":
    errors.append("founder Human-Orc war lock missing")
if sandbox.get("diplomacy") != "OUT_OF_SCOPE_UNSET":
    errors.append("sandbox must not inherit founder diplomacy")

# Value sites remain thematic/balance-pending.
for rid,site in sandbox["value_site_candidates"].items():
    if rid not in nodes:
        errors.append(f"value site region missing: {rid}")
    if site.get("economy_resource_id") is not None:
        errors.append(f"value site prematurely locks economy resource id: {rid}")
    if site.get("status") != "BALANCE_PENDING":
        errors.append(f"value site must remain balance-pending: {rid}")
# Compatibility projection must derive exactly from authoritative scenario.
if PROJECTION.get("source") != "soul_campaign_start_states_v1_20260922.json":
    errors.append("singular start-state projection does not identify authoritative source")
if PROJECTION.get("status") != "DERIVED_COMPATIBILITY_PROJECTION":
    errors.append("singular start-state file is not marked derived")
for faction,state in factions.items():
    projected=PROJECTION.get("factions",{}).get(faction,{})
    if projected.get("starting_regions") != state["starting_possessions"]:
        errors.append(f"{faction}: compatibility projection possession drift")
    if projected.get("safe_expansion_targets") != state["safe_expansion_targets"]:
        errors.append(f"{faction}: compatibility projection expansion-target drift")
if PROJECTION.get("region_control") != sandbox.get("region_control"):
    errors.append("compatibility projection region-control drift")
if PROJECTION.get("authority",{}).get("diplomacy") != "OUT_OF_SCOPE_UNSET":
    errors.append("compatibility projection must not invent diplomacy")

result={
    "schema":1,
    "status":"pass" if not errors else "fail",
    "factions":len(factions),
    "owned_regions":len(owners),
    "neutral_regions":len(sandbox["neutral_regions"]),
    "starting_settlements_per_faction":2,
    "direct_hostile_start_borders":len(hostile_edges),
    "value_site_candidates":len(sandbox["value_site_candidates"]),
    "projection_consistent":not any("projection" in e for e in errors),
    "errors":errors,
}
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps(result,indent=2))
if errors:
    raise SystemExit(1)
