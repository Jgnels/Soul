"""Build scenario-specific campaign start-state overlays for Soul's overmap."""
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
SLOTS = json.loads((ROOT/"Data"/"soul_overmap_settlement_slots_v1_20260922.json").read_text(encoding="utf-8"))
FOUNDER = json.loads((ROOT/"Data"/"soul_founder_slice_runtime_seed_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT/"Data"/"soul_campaign_start_states_v1_20260922.json"

nodes = {n["id"]:n for n in WORLD["nodes"]}
adj = defaultdict(set)
for e in WORLD["edges"]:
    adj[e["a"]].add(e["b"])
    adj[e["b"]].add(e["a"])

FACTION_STARTS = {
    "humans": ("human_capital","crossroads"),
    "vikings": ("viking_harbour","viking_forest_track"),
    "dwarves": ("dwarf_hold","dwarf_forge_approach"),
    "orcs": ("orc_camp","orc_war_camp"),
    "nature": ("nature_treehold","nature_forest_clearing"),
    "dark": ("dark_fortress","dark_castle_approach"),
}
SAFE_EXPANSION = {
    "humans": ["old_quarry"],
    "vikings": ["viking_fjord_ridge"],
    "dwarves": ["dwarf_high_quarry"],
    "orcs": ["orc_badlands"],
    "nature": ["nature_river_woodland"],
    "dark": ["dark_ash_plain"],
}

VALUE_SITE_CANDIDATES = {
    "old_quarry": {"theme":"ore quarry","economy_resource_id":None,"status":"BALANCE_PENDING"},
    "forest_edge": {"theme":"timber / woodland","economy_resource_id":None,"status":"BALANCE_PENDING"},
    "viking_fjord_ridge": {"theme":"fishing / coastal trade","economy_resource_id":None,"status":"BALANCE_PENDING"},
    "dwarf_high_quarry": {"theme":"ore quarry","economy_resource_id":None,"status":"BALANCE_PENDING"},
    "orc_badlands": {"theme":"beast / salvage grounds","economy_resource_id":None,"status":"BALANCE_PENDING"},
    "nature_river_woodland": {"theme":"herbs / food / river resources","economy_resource_id":None,"status":"BALANCE_PENDING"},
    "dark_ash_plain": {"theme":"occult / arcane resource site","economy_resource_id":None,"status":"BALANCE_PENDING"},
}

FOUNDER_OWNERS = {
    rid: region["owner"] for rid,region in FOUNDER["regions"].items()
    if region.get("owner")
}

def initial_visibility(capital):
    return sorted({capital, *adj[capital]})

def frontier(possessions):
    owned = set(possessions)
    return sorted({nxt for rid in owned for nxt in adj[rid] if nxt not in owned})
sandbox_factions = {}
sandbox_owner = {}
for faction,(capital,minor) in FACTION_STARTS.items():
    for rid in (capital,minor):
        sandbox_owner[rid] = faction
    sandbox_factions[faction] = {
        "capital_region": capital,
        "secondary_settlement_region": minor,
        "starting_possessions": [capital,minor],
        "safe_expansion_targets": SAFE_EXPANSION[faction],
        "army_spawn_anchors": [
            {"region_id":capital,"role":"primary_field_army","default_active":True},
            {"region_id":minor,"role":"frontier_reserve_or_second_hero","default_active":False},
        ],
        "starting_army": {
            "region": capital,
            "profile": "STARTING_FIELD_ARMY_BALANCE_PENDING",
            "commander_slot": "primary_commander",
        },
        "secondary_garrison": {
            "region": minor,
            "profile": "STARTING_MINOR_GARRISON_BALANCE_PENDING",
        },
        "initial_knowledge": {
            "explored_regions": sorted({capital,minor}),
            "visible_regions": initial_visibility(capital),
        },
        "immediate_neutral_frontier_candidates": frontier([capital,minor]),
    }
all_regions = set(nodes)
sandbox_neutral = sorted(all_regions - set(sandbox_owner))
region_control = {}
for rid,node in nodes.items():
    start_owner = sandbox_owner.get(rid)
    affinity = node.get("owner")
    if start_owner:
        control_class = "starting_possession"
    elif affinity:
        control_class = "homeland_neutral"
    else:
        control_class = "frontier_neutral"
    region_control[rid] = {
        "start_owner": start_owner,
        "control_class": control_class,
        "homeland_affinity": affinity,
    }

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "rules": {
        "world_graph_authority": "Data/soul_world_overmap_v1_20260922.json",
        "start_state_is_overlay": True,
        "full_world_homeland_affinity_is_not_start_ownership": True,
        "hostile_entry_policy_required": "Entering hostile-controlled territory must resolve an encounter/battle before occupation or traversal.",
        "numeric_army_garrison_balance": "BALANCE_LAB_OWNED",
        "diplomacy": "OUT_OF_SCOPE_UNSET",
    },
    "scenarios": {
        "founder_human_orc_micro": {
            "status": "LOCKED_TO_CURRENT_FOUNDER_PROTOTYPE",
            "region_ids": WORLD["founder_slice"]["region_ids"],
            "owners": FOUNDER_OWNERS,
            "player_faction": "humans",
            "player_start_region": FOUNDER["start_region"],
            "enemy_primary_region": FOUNDER["enemy_region"],
            "initial_knowledge": FOUNDER["initial_knowledge"],
            "diplomacy": {"humans_vs_orcs":"war"},
        },
        "six_faction_sandbox_candidate": {
            "status": "CANDIDATE_NONCANONICAL",
            "design": "one major seat + one aligned minor settlement per faction; remaining strategic regions neutral",
            "factions": sandbox_factions,
            "region_owners": dict(sorted(sandbox_owner.items())),
            "region_control": region_control,
            "neutral_regions": sandbox_neutral,
            "neutral_minor_sites": ["northwest_march","coastal_ruins"],
            "value_site_candidates": VALUE_SITE_CANDIDATES,
            "diplomacy": "OUT_OF_SCOPE_UNSET",
        },
    },
}
OUT.write_text(json.dumps(payload,indent=2)+"\n",encoding="utf-8")
print(f"WROTE {OUT}")
print("sandbox_owned",len(sandbox_owner),"neutral",len(sandbox_neutral))
