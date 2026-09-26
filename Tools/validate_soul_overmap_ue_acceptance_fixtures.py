"""Validate Soul future-UE overmap acceptance fixtures against current non-UE contracts."""
from __future__ import annotations

import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
UE = DATA / "UEImport"
FIXTURES = DATA / "soul_overmap_ue_acceptance_fixtures_v1_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "ue_acceptance_fixture_validation.json"

doc = json.loads(FIXTURES.read_text(encoding="utf-8"))
world = json.loads((DATA / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
approach_doc = json.loads((DATA / "soul_overmap_approach_profiles_v1_20260922.json").read_text(encoding="utf-8"))
handoff_doc = json.loads((DATA / "soul_overmap_battle_handoff_v1_20260922.json").read_text(encoding="utf-8"))
starts = json.loads((DATA / "soul_campaign_start_states_v1_20260922.json").read_text(encoding="utf-8"))
import_doc = json.loads((UE / "soul_founder_import_bundle_v2_20260922.json").read_text(encoding="utf-8"))

regions = {x["id"]: x for x in world["nodes"]}
routes = {"route." + "_".join(sorted((x["a"], x["b"]))): x for x in world["edges"]}
approaches = {x["id"]: x for x in approach_doc["approaches"]}
handoffs = {x["id"]: x for x in handoff_doc["handoffs"]}
import_regions = {x["RegionId"]: x for x in import_doc["regions"]}
candidate_sites = {}
with (UE / "soul_overmap_candidate_sites_v1_20260922.csv").open("r", encoding="utf-8-sig", newline="") as handle:
    for row in csv.DictReader(handle):
        candidate_sites[row["site_id"]] = row

source_maps = {
    "region": regions,
    "route": routes,
    "approach": approaches,
    "handoff": handoffs,
    "import_region": import_regions,
    "candidate_site": candidate_sites,
}

def get_path(value, dotted: str):
    current = value
    for part in dotted.split("."):
        if not isinstance(current, dict) or part not in current:
            raise KeyError(dotted)
        current = current[part]
    return current

errors: list[str] = []
warnings: list[str] = []
fixtures = doc.get("fixtures", [])
ids = [x.get("id") for x in fixtures]
if doc.get("schema") != 1:
    errors.append("fixture schema must be 1")
if doc.get("status") != "UE_ACCEPTANCE_FIXTURES_PREPARED_NON_UE":
    errors.append("fixture document status mismatch")
if len(fixtures) != 6:
    errors.append(f"expected 6 acceptance fixtures, got {len(fixtures)}")
if len(ids) != len(set(ids)):
    errors.append("duplicate fixture IDs")

required_fixture_fields = [
    "id","scope","purpose","setup","trigger","runtime_expectations",
    "required_runtime_observations","pass_conditions","source_checks",
]
for fixture in fixtures:
    fid = fixture.get("id", "<missing>")
    for field in required_fixture_fields:
        if field not in fixture or fixture[field] in (None, "", [], {}):
            errors.append(f"{fid}: missing fixture field {field}")
    if len(fixture.get("required_runtime_observations", [])) < 4:
        errors.append(f"{fid}: insufficient runtime observations")
    if len(fixture.get("pass_conditions", [])) < 3:
        errors.append(f"{fid}: insufficient exercised pass conditions")

    for check in fixture.get("source_checks", []):
        kind = check["kind"]
        item_id = check["id"]
        if kind not in source_maps:
            errors.append(f"{fid}: unsupported source check kind {kind}")
            continue
        if item_id not in source_maps[kind]:
            errors.append(f"{fid}: source {kind} missing {item_id}")
            continue
        source = source_maps[kind][item_id]
        for dotted, expected in check["equals"].items():
            try:
                actual = get_path(source, dotted)
            except KeyError:
                errors.append(f"{fid}: {kind} {item_id} missing path {dotted}")
                continue
            if actual != expected:
                errors.append(f"{fid}: {kind} {item_id} {dotted} expected {expected!r}, got {actual!r}")

    trigger = fixture.get("trigger", {})
    if trigger.get("approach_id") and trigger["approach_id"] not in approaches:
        errors.append(f"{fid}: trigger approach missing")
    if trigger.get("battle_handoff_id") and trigger["battle_handoff_id"] not in handoffs:
        errors.append(f"{fid}: trigger handoff missing")
    if trigger.get("special_site_id") and trigger["special_site_id"] not in candidate_sites:
        errors.append(f"{fid}: trigger special site missing")

    setup = fixture.get("setup", {})
    if "path_regions" in setup:
        path_regions = setup["path_regions"]
        path_routes = setup.get("path_route_ids", [])
        if len(path_routes) != len(path_regions) - 1:
            errors.append(f"{fid}: path route/region count mismatch")
        for index, route_id in enumerate(path_routes):
            if route_id not in routes:
                errors.append(f"{fid}: unknown path route {route_id}")
                continue
            edge = routes[route_id]
            expected_pair = {path_regions[index], path_regions[index + 1]}
            if {edge["a"], edge["b"]} != expected_pair:
                errors.append(f"{fid}: route {route_id} does not connect expected path pair")
    if setup.get("route_id"):
        route_id = setup["route_id"]
        if route_id not in routes:
            errors.append(f"{fid}: setup route missing {route_id}")
        elif setup.get("source_region") and setup.get("destination_region"):
            edge = routes[route_id]
            if {edge["a"], edge["b"]} != {setup["source_region"], setup["destination_region"]}:
                errors.append(f"{fid}: setup route endpoints mismatch")
    if setup.get("approach_id") and setup["approach_id"] not in approaches:
        errors.append(f"{fid}: setup approach missing")
    for nested_key in ("primary_attack","reserve_reinforcement","retreat"):
        nested = setup.get(nested_key)
        if not nested:
            continue
        aid = nested.get("approach_id")
        if aid not in approaches:
            errors.append(f"{fid}: {nested_key} approach missing {aid}")
            continue
        source = approaches[aid]
        if source["source_region"] != nested["source_region"] or source["destination_region"] != nested["destination_region"]:
            errors.append(f"{fid}: {nested_key} approach endpoints mismatch")

fixture_by_id = {x["id"]: x for x in fixtures}
forest = fixture_by_id.get("ue.fixture.founder_forest_contact")
if forest:
    if forest["runtime_expectations"]["route_visual_sequence"] != ["road","trail","trail"]:
        errors.append("forest-contact route signature changed")
    forest_expect = forest["runtime_expectations"]
    if forest_expect.get("pre_battle_player_region") != "forest_edge":
        errors.append("forest-contact must retain Forest Edge as strategic origin until hostile battle resolves")
    if forest_expect.get("hostile_destination_occupation_before_battle") is not False:
        errors.append("forest-contact must forbid pre-battle occupation of Orc Watch")
    if forest_expect.get("battle_result_region") != "orc_watch":
        errors.append("forest-contact victory must resolve Orc Watch, not another strategic region")
    required_observations = set(forest.get("required_runtime_observations", []))
    for observation in ("pre_battle_player_region_id","battle_result_region_id","destination_owner_after_victory"):
        if observation not in required_observations:
            errors.append(f"forest-contact missing runtime observation {observation}")

capital = fixture_by_id.get("ue.fixture.human_capital_east_assault")
founder_owners = starts["scenarios"]["founder_human_orc_micro"]["owners"]
if capital:
    if founder_owners.get("human_capital") != "humans":
        errors.append("canonical Human Capital founder ownership changed")
    if not capital["setup"].get("canonical_start_ownership_unchanged"):
        errors.append("capital assault fixture must declare canonical ownership unchanged")

dragon = fixture_by_id.get("ue.fixture.dragon_graveyard_special_site")
if dragon:
    site = candidate_sites.get("dragon_graveyard")
    if not site:
        errors.append("Dragon Graveyard candidate site missing")
    else:
        if site["region_id"] != "orc_badlands" or site["topology_change"].lower() != "false":
            errors.append("Dragon Graveyard must remain an Orc Badlands non-topology-changing site")
    if not dragon["runtime_expectations"].get("special_site_override_required"):
        errors.append("Dragon Graveyard fixture must require explicit override")
    if dragon["runtime_expectations"].get("topology_change") is not False:
        errors.append("Dragon Graveyard fixture must forbid topology change")

reinforce = fixture_by_id.get("ue.fixture.stronghold_reinforcement_retreat")
if reinforce:
    expected_dirs = ("north_west","west","south_east")
    actual_dirs = (
        reinforce["runtime_expectations"]["attacker_entry_direction"],
        reinforce["runtime_expectations"]["reinforcement_entry_direction"],
        reinforce["runtime_expectations"]["retreat_destination_entry_direction"],
    )
    if actual_dirs != expected_dirs:
        errors.append(f"stronghold reinforcement/retreat directions changed: {actual_dirs}")
    h = handoffs["battle_handoff.north_pass.orc_camp"]
    slots = {x["source_region"]: x["entry_direction"] for x in h["reinforcement_context"]["alternate_adjacent_entry_slots"]}
    if slots.get("orc_watch") != "west":
        errors.append("Orc Watch is not available as west-entry reinforcement slot for North Pass stronghold handoff")

river = fixture_by_id.get("ue.fixture.river_ford_context")
if river and river["runtime_expectations"].get("known_asset_gate") != "UE_CROP_REQUIRED":
    errors.append("River Ford fixture must retain explicit UE_CROP_REQUIRED gate until source changes")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "fixture_count": len(fixtures),
    "fixture_ids": sorted(ids),
    "source_check_count": sum(len(x.get("source_checks", [])) for x in fixtures),
    "known_asset_gates": {
        "river_ford": "UE_CROP_REQUIRED",
        "orc_stronghold": "UE_COMPOSITE",
        "dragon_graveyard": "separate proof-lane payload binding required",
    },
    "warnings": warnings,
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
