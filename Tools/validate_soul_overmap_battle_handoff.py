"""Validate Soul founder overmap -> battle handoff completeness and determinism."""
from __future__ import annotations

import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HANDOFF = ROOT / "Data" / "soul_overmap_battle_handoff_v1_20260922.json"
WORLD = ROOT / "Data" / "soul_world_overmap_v1_20260922.json"
APPROACHES = ROOT / "Data" / "soul_overmap_approach_profiles_v1_20260922.json"
STARTS = ROOT / "Data" / "soul_campaign_start_states_v1_20260922.json"
RECIPES = ROOT / "Data" / "battlefield_recipes.json"
CANDIDATES = ROOT / "Data" / "UEImport" / "soul_overmap_candidate_sites_v1_20260922.csv"
OUT = ROOT / "Evidence" / "WorldOvermap" / "battle_handoff_validation.json"

handoff = json.loads(HANDOFF.read_text(encoding="utf-8"))
world = json.loads(WORLD.read_text(encoding="utf-8"))
approaches_doc = json.loads(APPROACHES.read_text(encoding="utf-8"))
starts = json.loads(STARTS.read_text(encoding="utf-8"))
recipes_doc = json.loads(RECIPES.read_text(encoding="utf-8"))

scenario = starts["scenarios"]["founder_human_orc_micro"]
founder_ids = set(scenario["region_ids"])
owners = scenario["owners"]
nodes = {n["id"]: n for n in world["nodes"]}
recipes = {r["id"]: r for r in recipes_doc["recipes"]}
directed = [a for a in approaches_doc["approaches"] if a["founder_internal"]]
expected_pairs = {(a["source_region"], a["destination_region"]): a for a in directed}
actual = {(h["source_region"], h["destination_region"]): h for h in handoff["handoffs"]}

candidate_by_region: dict[str, set[str]] = defaultdict(set)
with CANDIDATES.open("r", encoding="utf-8-sig", newline="") as handle:
    for row in csv.DictReader(handle):
        candidate_by_region[row["region_id"]].add(row["site_id"])

errors: list[str] = []
warnings: list[str] = []

if handoff["schema"] != 1:
    errors.append("handoff schema must be 1")
if len(actual) != len(handoff["handoffs"]):
    errors.append("duplicate source/destination handoff pair")
if set(actual) != set(expected_pairs):
    missing = sorted(set(expected_pairs) - set(actual))
    extra = sorted(set(actual) - set(expected_pairs))
    if missing:
        errors.append(f"missing directed handoffs: {missing}")
    if extra:
        errors.append(f"unexpected directed handoffs: {extra}")

def stable_seed(parts: list[str]) -> int:
    payload = "|".join(parts).encode("utf-8")
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], "big", signed=False)

for pair, h in sorted(actual.items()):
    src, dst = pair
    if src not in founder_ids or dst not in founder_ids:
        errors.append(f"{h['id']}: endpoint outside founder slice")
        continue
    expected = expected_pairs[pair]
    node = nodes[dst]
    route = h["strategic_route"]
    for key in ("type", "road", "chokepoint", "action_cost", "logistics_movement_cost"):
        if route[key] != expected["route"][key]:
            errors.append(f"{h['id']}: route {key} diverges from directed approach")
    if route["entry_direction"] != expected["entry_direction"]:
        errors.append(f"{h['id']}: entry direction mismatch")

    terrain = h["terrain_context"]
    for key in ("biome", "landform", "feature", "elevation_band"):
        if terrain[key] != node[key]:
            errors.append(f"{h['id']}: terrain {key} mismatch")

    selection = h["battlefield_selection"]
    recipe_id = expected["battlefield"]["recipe_id"]
    if selection["recipe_id"] != recipe_id:
        errors.append(f"{h['id']}: recipe id mismatch")
    if recipe_id not in recipes:
        errors.append(f"{h['id']}: unknown recipe {recipe_id}")
    elif selection["recipe_status"] != recipes[recipe_id]["status"]:
        errors.append(f"{h['id']}: recipe status mismatch")
    if selection["recipe_status"] not in ("READY_FOR_UE", "LOCAL_VERIFIED"):
        warnings.append(f"{h['id']}: recipe readiness {selection['recipe_status']}")

    control = h["scenario_start_control"]
    if control["destination_owner"] != owners.get(dst):
        errors.append(f"{h['id']}: scenario owner mismatch")
    expected_hostile = bool(owners.get(dst) and owners.get(dst) != scenario["player_faction"])
    if control["hostile_at_start"] != expected_hostile:
        errors.append(f"{h['id']}: hostile-at-start mismatch")

    siege = h["settlement_and_siege_context"]
    expected_settlement = bool(node.get("settlement_id"))
    if siege["is_settlement"] != expected_settlement:
        errors.append(f"{h['id']}: settlement flag mismatch")
    if siege["settlement_id"] != node.get("settlement_id"):
        errors.append(f"{h['id']}: settlement id mismatch")
    if expected_settlement and len(siege["runtime_fields"]) < 5:
        errors.append(f"{h['id']}: settlement handoff missing siege runtime fields")

    weather = h["weather_time_context"]
    if weather["weather_authority"] != "RBWeather":
        errors.append(f"{h['id']}: weather must stay under RBWeather authority")
    for field in ("weather_profile_id", "time_of_day_minutes"):
        if field not in weather["snapshot_fields"]:
            errors.append(f"{h['id']}: missing dynamic field {field}")

    seed = h["seed_contract"]
    if seed["sample_seed_u64"] != stable_seed(seed["sample_inputs"]):
        errors.append(f"{h['id']}: unstable sample seed")

    actual_sites = {x["site_id"] for x in h["special_site_overrides"]}
    if actual_sites != candidate_by_region.get(dst, set()):
        errors.append(f"{h['id']}: special-site candidates mismatch")
    if any(x["active_by_default"] for x in h["special_site_overrides"]):
        errors.append(f"{h['id']}: candidate special site activated by default")

    for slot in h["reinforcement_context"]["alternate_adjacent_entry_slots"]:
        slot_pair = (slot["source_region"], dst)
        if slot_pair not in expected_pairs:
            errors.append(f"{h['id']}: reinforcement slot not backed by strategic route {slot_pair}")
        elif slot["entry_direction"] != expected_pairs[slot_pair]["entry_direction"]:
            errors.append(f"{h['id']}: reinforcement entry direction mismatch {slot_pair}")

stronghold_entries = sorted(
    (h["source_region"], h["strategic_route"]["entry_direction"])
    for h in handoff["handoffs"]
    if h["destination_region"] == scenario["enemy_primary_region"]
)
if len(stronghold_entries) != 2:
    errors.append(f"expected 2 directed Orc Stronghold entries, got {len(stronghold_entries)}")
if not any(
    x["site_id"] == "lost_shrine"
    for h in handoff["handoffs"] if h["destination_region"] == "ancient_shrine"
    for x in h["special_site_overrides"]
):
    errors.append("Ancient Shrine is missing Lost Shrine candidate override")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "expected_directed_handoffs": len(expected_pairs),
    "actual_directed_handoffs": len(actual),
    "stronghold_entries": stronghold_entries,
    "warnings": sorted(set(warnings)),
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
