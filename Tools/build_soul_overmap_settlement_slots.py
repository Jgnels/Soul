"""Build candidate settlement density layer for Soul's structural overmap."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT/"Data"/"soul_overmap_settlement_slots_v1_20260922.json"

MAJOR = {
    "human_capital": ("humans","city.human_capital"),
    "viking_harbour": ("vikings","city.viking_harbour"),
    "dwarf_hold": ("dwarves","city.dwarf_hold"),
    "orc_camp": ("orcs","city.orc_ruinhold"),
    "nature_treehold": ("nature","city.nature_treehold"),
    "dark_fortress": ("dark","city.dark_fortress"),
}

MINOR = {
    "crossroads": ("humans","market_town","Medieval Megapack market/civic cluster",["market","rest","caravan"]),
    "northwest_march": (None,"trade_post","human/Viking roadside trade composition",["market","caravan","intel"]),
    "viking_forest_track": ("vikings","village","Modular Viking Village",["recruit_light","rest","supplies"]),
    "dwarf_forge_approach": ("dwarves","forge_outpost","Legendary Forge outer works",["forge","supplies","repair"]),
    "orc_war_camp": ("orcs","war_camp","Ruins + BanditCamp composition",["recruit_light","supplies","raid"]),
    "nature_forest_clearing": ("nature","grove_village","Fantasy Forest Village clearing",["healing","supplies","recruit_light"]),
    "dark_castle_approach": ("dark","ward_bastion","Fantasy Alien Castle outer works",["ward","supplies","intel"]),
    "coastal_ruins": (None,"ruined_settlement","Coastal Ruins",["exploration","salvage","quest"]),
}
nodes = {n["id"]:n for n in WORLD["nodes"]}
slots = []
for rid,(faction,settlement_id) in MAJOR.items():
    n = nodes[rid]
    slots.append({
        "region_id": rid,
        "display_name": n["name"],
        "tier": "major",
        "faction_affinity": faction,
        "settlement_id": settlement_id,
        "status": "LOCKED_MAJOR_SEAT",
        "visit_scope": "full_visit_scene",
        "proxy_priority": "high",
        "services": ["recruitment","hero_services","market","siege"],
        "donor": settlement_id,
    })

for rid,(faction,kind,donor,services) in MINOR.items():
    n = nodes[rid]
    slots.append({
        "region_id": rid,
        "display_name": n["name"],
        "tier": "minor",
        "minor_kind": kind,
        "faction_affinity": faction,
        "settlement_id": None,
        "status": "CANDIDATE_NONCANONICAL",
        "visit_scope": "bounded_scene_or_proxy",
        "proxy_priority": "medium",
        "services": services,
        "donor": donor,
    })
payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "authority": {
        "major_seats": "current city/siege blueprint IDs",
        "minor_slots": "candidate density layer only; not campaign-start ownership or final service balance",
    },
    "rules": [
        "Minor settlements reuse existing strategic regions; no extra graph nodes are added.",
        "Minor sites default to bounded visit scenes or simplified proxies, not capital-scale bespoke cities.",
        "Interactive space / static clutter rule applies to any visitable minor settlement.",
        "A minor slot may be removed or changed without invalidating the overmap topology.",
    ],
    "counts": {
        "major": len(MAJOR),
        "minor_candidates": len(MINOR),
        "total_slots": len(slots),
    },
    "slots": sorted(slots,key=lambda x:(x["tier"],x["region_id"])),
}
OUT.write_text(json.dumps(payload,indent=2)+"\n",encoding="utf-8")
print(f"WROTE {OUT} major={len(MAJOR)} minor={len(MINOR)}")
