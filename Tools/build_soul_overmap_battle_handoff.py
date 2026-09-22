"""Build deterministic strategic-overmap -> battle handoff records for Soul."""
from __future__ import annotations

import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = ROOT / "Data" / "soul_world_overmap_v1_20260922.json"
APPROACHES = ROOT / "Data" / "soul_overmap_approach_profiles_v1_20260922.json"
STARTS = ROOT / "Data" / "soul_campaign_start_states_v1_20260922.json"
FOUNDER = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
RECIPES = ROOT / "Data" / "battlefield_recipes.json"
CANDIDATES = ROOT / "Data" / "UEImport" / "soul_overmap_candidate_sites_v1_20260922.csv"
OUT = ROOT / "Data" / "soul_overmap_battle_handoff_v1_20260922.json"
DOC = ROOT / "Docs" / "WORLD_OVERMAP_BATTLE_HANDOFF_20260922.md"

world = json.loads(WORLD.read_text(encoding="utf-8"))
approaches_doc = json.loads(APPROACHES.read_text(encoding="utf-8"))
starts = json.loads(STARTS.read_text(encoding="utf-8"))
founder = json.loads(FOUNDER.read_text(encoding="utf-8"))
recipes_doc = json.loads(RECIPES.read_text(encoding="utf-8"))

scenario_id = "founder_human_orc_micro"
scenario = starts["scenarios"][scenario_id]
player_faction = scenario["player_faction"]
enemy_primary = scenario["enemy_primary_region"]
owners = scenario["owners"]
nodes = {node["id"]: node for node in world["nodes"]}
recipes = {recipe["id"]: recipe for recipe in recipes_doc["recipes"]}
founder_regions = set(scenario["region_ids"])
roles = {rid: set(data.get("site_roles", [])) for rid, data in founder["regions"].items()}
directed = [a for a in approaches_doc["approaches"] if a["founder_internal"]]
incoming: dict[str, list[dict]] = defaultdict(list)
for a in directed:
    incoming[a["destination_region"]].append(a)

candidate_sites: dict[str, list[dict]] = defaultdict(list)
with CANDIDATES.open("r", encoding="utf-8-sig", newline="") as handle:
    for row in csv.DictReader(handle):
        candidate_sites[row["region_id"]].append({
            "site_id": row["site_id"],
            "title": row["title"],
            "integration_class": row["integration_class"],
            "local_status": row["local_status"],
            "candidate_status": row["candidate_status"],
            "topology_change": row["topology_change"].lower() == "true",
        })

def stable_seed(parts: list[str]) -> int:
    payload = "|".join(parts).encode("utf-8")
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], "big", signed=False)

def encounter_kind(region_id: str) -> str:
    node = nodes[region_id]
    site_roles = roles.get(region_id, set())
    owner = owners.get(region_id)
    if node.get("settlement_id"):
        return "siege_or_settlement_assault" if owner and owner != player_faction else "settlement_defense_or_entry"
    if owner and owner != player_faction:
        return "hostile_field_encounter"
    if "encounter" in site_roles or "chokepoint" in site_roles:
        return "conditional_field_encounter"
    if "resource_site" in site_roles:
        return "resource_site_contest_if_occupied"
    if "magic_landmark" in site_roles:
        return "special_site_encounter_if_activated"
    return "field_battle_if_contested"

handoffs = []
for a in sorted(directed, key=lambda x: x["id"]):
    src = a["source_region"]
    dst = a["destination_region"]
    dest = nodes[dst]
    recipe_id = a["battlefield"]["recipe_id"]
    recipe = recipes[recipe_id]
    owner = owners.get(dst)
    hostile_at_start = bool(owner and owner != player_faction)
    settlement_id = dest.get("settlement_id")
    special_sites = candidate_sites.get(dst, [])

    reinforcement_slots = []
    for option in sorted(incoming[dst], key=lambda x: (x["source_region"], x["id"])):
        if option["source_region"] == src:
            continue
        reinforcement_slots.append({
            "source_region": option["source_region"],
            "entry_direction": option["entry_direction"],
            "route_type": option["route"]["type"],
            "road": option["route"]["road"],
            "chokepoint": option["route"]["chokepoint"],
            "strategic_action_distance_to_battle": option["route"]["action_cost"],
            "eligibility": "runtime army presence + alliance/control + reinforcement timing",
        })

    sample_parts = [
        scenario_id,
        "20260922",
        "1",
        src,
        dst,
        "0",
    ]
    handoffs.append({
        "id": f"battle_handoff.{src}.{dst}",
        "scenario_id": scenario_id,
        "source_region": src,
        "destination_region": dst,
        "encounter_kind": encounter_kind(dst),
        "scenario_start_control": {
            "player_faction": player_faction,
            "destination_owner": owner,
            "hostile_at_start": hostile_at_start,
        },
        "strategic_route": {
            "type": a["route"]["type"],
            "road": a["route"]["road"],
            "chokepoint": a["route"]["chokepoint"],
            "action_cost": a["route"]["action_cost"],
            "logistics_movement_cost": a["route"]["logistics_movement_cost"],
            "entry_direction": a["entry_direction"],
        },
        "terrain_context": {
            "biome": dest["biome"],
            "landform": dest["landform"],
            "feature": dest["feature"],
            "elevation_band": dest["elevation_band"],
            "resource": dest.get("resource"),
            "site_roles": sorted(roles.get(dst, set())),
        },
        "battlefield_selection": {
            "recipe_id": recipe_id,
            "recipe_status": recipe["status"],
            "recipe_landmark": recipe.get("landmark"),
            "recipe_tactics": recipe.get("tactics", []),
            "selection_order": [
                "explicit_activated_special_site_override",
                "destination_region_recipe",
                "deterministic_fallback_with_matching biome/landform/feature",
            ],
        },
        "settlement_and_siege_context": {
            "is_settlement": bool(settlement_id),
            "settlement_id": settlement_id,
            "controller_at_scenario_start": owner,
            "pre_battle_siege_phase_required": bool(settlement_id and hostile_at_start),
            "runtime_fields": [
                "settlement_condition",
                "wall_integrity_permille",
                "gate_state",
                "breach_ids",
                "siege_preparation_state",
                "defender_supply_permille",
            ] if settlement_id else [],
            "authority": "Soul settlement/siege runtime; handoff transports state and does not invent it",
        },
        "weather_time_context": {
            "weather_authority": "RBWeather",
            "time_authority": "Soul campaign clock",
            "snapshot_fields": [
                "weather_profile_id",
                "wind_permille",
                "precipitation_permille",
                "visibility_permille",
                "time_of_day_minutes",
            ],
            "policy": "snapshot at encounter commitment; battle presentation consumes snapshot without changing strategic authority",
        },
        "reinforcement_context": {
            "primary_attacker_entry_direction": a["entry_direction"],
            "alternate_adjacent_entry_slots": reinforcement_slots,
            "timing_authority": "campaign action/time distance; battle runtime only admits arrivals that satisfy strategic timing",
        },
        "special_site_overrides": [{
            **site,
            "active_by_default": False,
            "activation_rule": "explicit scenario/event/site selection only",
            "topology_policy": "never changes overmap topology",
        } for site in special_sites],
        "seed_contract": {
            "algorithm": "sha256_first_64_bits_big_endian",
            "canonical_inputs": [
                "scenario_id",
                "campaign_seed",
                "campaign_turn",
                "source_region",
                "destination_region",
                "encounter_ordinal",
            ],
            "separator": "|",
            "sample_inputs": sample_parts,
            "sample_seed_u64": stable_seed(sample_parts),
        },
        "context_tags": sorted(set(a["approach_tags"] + [
            f"encounter.{encounter_kind(dst)}",
            f"owner.{owner or 'neutral'}",
        ])),
    })

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "FOUNDER_HANDOFF_CONTRACT_V1",
    "authority": {
        "geography": str(WORLD.relative_to(ROOT)).replace("\\", "/"),
        "directed_approaches": str(APPROACHES.relative_to(ROOT)).replace("\\", "/"),
        "scenario_ownership": str(STARTS.relative_to(ROOT)).replace("\\", "/"),
        "battlefield_recipes": str(RECIPES.relative_to(ROOT)).replace("\\", "/"),
        "weather": "RBWeather",
        "settlement_siege": "Soul runtime state",
        "seed": "handoff contract below",
    },
    "rules": [
        "Strategic geography selects battle context; battle presentation must not invent campaign geography.",
        "Every founder-directed edge has exactly one handoff record.",
        "Weather/time and settlement/siege condition are runtime snapshots, not static map authoring.",
        "Reinforcement entry slots come only from valid adjacent strategic routes.",
        "Candidate special sites require explicit activation and never rewrite topology.",
        "Encounter seed is stable for identical canonical inputs.",
    ],
    "counts": {
        "founder_regions": len(founder_regions),
        "founder_directed_edges": len(directed),
        "handoffs": len(handoffs),
        "handoffs_with_settlement": sum(1 for x in handoffs if x["settlement_and_siege_context"]["is_settlement"]),
        "handoffs_with_special_site_candidate": sum(1 for x in handoffs if x["special_site_overrides"]),
    },
    "handoffs": handoffs,
}
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

recipe_statuses = defaultdict(int)
for h in handoffs:
    recipe_statuses[h["battlefield_selection"]["recipe_status"]] += 1
stronghold = [h for h in handoffs if h["destination_region"] == enemy_primary]
special = [h for h in handoffs if h["special_site_overrides"]]
lines = [
    "# Soul Overmap -> Battle Handoff Contract — 2026-09-22",
    "",
    "This is a deterministic non-UE contract between Soul campaign geography and tactical battle launch.",
    "It does not resolve combat and does not replace RB Weather, settlement, siege, or battle runtime authority.",
    "",
    "## Coverage",
    "",
    f"- Founder regions: {len(founder_regions)}.",
    f"- Directed founder approaches: {len(directed)}.",
    f"- Generated battle handoffs: {len(handoffs)}.",
    f"- Stronghold directed entries: {len(stronghold)} ({', '.join(x['source_region'] for x in stronghold)}).",
    f"- Handoffs carrying candidate special-site overrides: {len(special)}.",
    "",
    "## Runtime contract",
    "",
    "- Destination biome/landform/feature/elevation and route/entry direction select the battlefield recipe context.",
    "- RB Weather owns the weather snapshot; the campaign clock owns time-of-day.",
    "- Settlement/siege state is passed through from Soul runtime and cannot be synthesized by presentation.",
    "- Reinforcement entry directions are constrained to actual adjacent strategic routes and runtime timing.",
    "- Candidate special sites are opt-in overrides only; they do not modify the campaign graph.",
    "- Stable battle seeds are SHA-256-derived from scenario, campaign seed, turn, source, destination, and encounter ordinal.",
    "",
    "## Current recipe readiness across founder directed handoffs",
    "",
]
for key in sorted(recipe_statuses):
    lines.append(f"- {key}: {recipe_statuses[key]}")
lines += [
    "",
    "## Important founder observations",
    "",
    "- Orc Stronghold can be entered from Orc Watch or North Pass; those produce distinct entry directions and reinforcement slots.",
    "- River Ford preserves the river-crossing context rather than falling back to a generic field.",
    "- Ancient Shrine carries the Lost Shrine candidate as an inactive special-site override; activation must be explicit.",
    "- Recipe readiness is an asset-production concern. A valid handoff may point to a recipe that still needs UE crop/composite work.",
]
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(payload["counts"], indent=2))
print(f"Wrote {OUT}")
print(f"Wrote {DOC}")
