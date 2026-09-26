"""Build machine-readable acceptance fixtures for Soul's future UE overmap implementation."""
from __future__ import annotations

import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
UE = DATA / "UEImport"
OUT = DATA / "soul_overmap_ue_acceptance_fixtures_v1_20260922.json"
DOC = ROOT / "Docs" / "SOUL_OVERMAP_UE_ACCEPTANCE_FIXTURES_20260922.md"

world = json.loads((DATA / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
approaches_doc = json.loads((DATA / "soul_overmap_approach_profiles_v1_20260922.json").read_text(encoding="utf-8"))
handoff_doc = json.loads((DATA / "soul_overmap_battle_handoff_v1_20260922.json").read_text(encoding="utf-8"))
import_bundle = json.loads((UE / "soul_founder_import_bundle_v2_20260922.json").read_text(encoding="utf-8"))

nodes = {x["id"]: x for x in world["nodes"]}
edges = {"route." + "_".join(sorted((x["a"], x["b"]))): x for x in world["edges"]}
approaches = {x["id"]: x for x in approaches_doc["approaches"]}
handoffs = {x["id"]: x for x in handoff_doc["handoffs"]}
import_regions = {x["RegionId"]: x for x in import_bundle["regions"]}

candidate_sites = {}
with (UE / "soul_overmap_candidate_sites_v1_20260922.csv").open("r", encoding="utf-8-sig", newline="") as handle:
    for row in csv.DictReader(handle):
        candidate_sites[row["site_id"]] = row

def source_check(kind: str, item_id: str, equals: dict) -> dict:
    return {"kind": kind, "id": item_id, "equals": equals}

fixtures = [
    {
        "id": "ue.fixture.founder_forest_contact",
        "scope": "founder_slice",
        "purpose": "Exercise Human Capital -> forest corridor -> hostile Orc Watch contact without a generic-map fallback.",
        "setup": {
            "active_faction": "humans",
            "start_region": "human_capital",
            "destination_enemy_region": "orc_watch",
            "path_regions": ["human_capital", "crossroads", "forest_edge", "orc_watch"],
            "path_route_ids": [
                "route.crossroads_human_capital",
                "route.crossroads_forest_edge",
                "route.forest_edge_orc_watch",
            ],
        },
        "trigger": {
            "kind": "hostile_region_entry",
            "approach_id": "approach.forest_edge.orc_watch",
            "battle_handoff_id": "battle_handoff.forest_edge.orc_watch",
        },
        "runtime_expectations": {
            "route_visual_sequence": ["road", "trail", "trail"],
            "destination_region": "orc_watch",
            "destination_owner_at_founder_start": "orcs",
            "entry_direction": "north_west",
            "battle_recipe_id": "orc.badlands",
            "encounter_kind": "hostile_field_encounter",
            "pre_battle_player_region": "forest_edge",
            "hostile_destination_occupation_before_battle": False,
            "battle_result_region": "orc_watch",
        },
        "required_runtime_observations": [
            "active_region_id",
            "selected_route_id",
            "pre_battle_player_region_id",
            "hostile_destination_owner_before_battle",
            "battle_approach_id",
            "battle_handoff_id",
            "battle_recipe_id",
            "battle_entry_direction",
            "battle_result_region_id",
            "destination_owner_after_victory",
        ],
        "pass_conditions": [
            "The campaign pawn reaches Orc Watch through the authored road/trail graph with no teleport or disconnected spline.",
            "Entering hostile Orc Watch launches the orc.badlands context from the north-west entry.",
            "Before battle resolution, the campaign player remains at Forest Edge and Orc Watch remains Orc-controlled; hostile entry cannot silently become occupation.",
            "Victory resolves and captures Orc Watch itself; it must not award or capture Orc Stronghold/orc_camp.",
            "The tactical launch consumes the strategic handoff rather than choosing an unrelated generic arena.",
        ],
        "source_checks": [
            source_check("approach", "approach.forest_edge.orc_watch", {
                "entry_direction": "north_west",
                "battlefield.recipe_id": "orc.badlands",
                "route.type": "trail",
            }),
            source_check("handoff", "battle_handoff.forest_edge.orc_watch", {
                "encounter_kind": "hostile_field_encounter",
                "scenario_start_control.destination_owner": "orcs",
                "battlefield_selection.recipe_id": "orc.badlands",
            }),
        ],
    },
    {
        "id": "ue.fixture.human_capital_east_assault",
        "scope": "founder_slice_test_override",
        "purpose": "Verify a walled major settlement approached from the east preserves its settlement/siege context.",
        "setup": {
            "test_only_attacker_faction": "orcs",
            "test_only_defender_faction": "humans",
            "source_region": "crossroads",
            "destination_region": "human_capital",
            "canonical_start_ownership_unchanged": True,
        },
        "trigger": {
            "kind": "test_hostile_settlement_entry",
            "approach_id": "approach.crossroads.human_capital",
            "battle_handoff_id": "battle_handoff.crossroads.human_capital",
        },
        "runtime_expectations": {
            "entry_direction": "east",
            "settlement_id": "city.human_capital",
            "anchor_type": "major_settlement_silhouette",
            "battle_recipe_id": "human.fortress_outskirts",
            "settlement_services_include": ["siege"],
        },
        "required_runtime_observations": [
            "destination_region_id",
            "settlement_id",
            "settlement_condition",
            "battle_entry_direction",
            "battle_recipe_id",
            "siege_context_present",
        ],
        "pass_conditions": [
            "The east approach resolves to the Human Capital settlement rather than a field-only encounter.",
            "The settlement silhouette/gate context is visible from the east approach.",
            "The battle launch carries settlement/siege state fields and human.fortress_outskirts context.",
        ],
        "source_checks": [
            source_check("approach", "approach.crossroads.human_capital", {
                "entry_direction": "east",
                "destination_context.settlement_id": "city.human_capital",
                "battlefield.recipe_id": "human.fortress_outskirts",
            }),
            source_check("handoff", "battle_handoff.crossroads.human_capital", {
                "settlement_and_siege_context.is_settlement": True,
                "settlement_and_siege_context.settlement_id": "city.human_capital",
            }),
            source_check("import_region", "human_capital", {
                "AnchorType": "major_settlement_silhouette",
                "SettlementId": "city.human_capital",
            }),
        ],
    },
    {
        "id": "ue.fixture.broken_bridge_chokepoint",
        "scope": "world_overmap",
        "purpose": "Verify the Orc-to-south broken bridge reads and launches as a ravine chokepoint, not generic badlands.",
        "setup": {
            "source_region": "orc_ruined_field",
            "destination_region": "orc_broken_bridge",
            "route_id": "route.orc_broken_bridge_orc_ruined_field",
        },
        "trigger": {
            "kind": "contested_region_entry",
            "approach_id": "approach.orc_ruined_field.orc_broken_bridge",
        },
        "runtime_expectations": {
            "road": True,
            "chokepoint": True,
            "entry_direction": "north",
            "destination_landform": "ravine",
            "destination_feature": "broken_bridge",
            "battle_recipe_id": "orc.broken_bridge",
        },
        "required_runtime_observations": [
            "selected_route_id",
            "route_chokepoint",
            "destination_region_id",
            "battle_recipe_id",
            "battle_entry_direction",
        ],
        "pass_conditions": [
            "The strategic route visibly converges on the broken-bridge chokepoint.",
            "The destination retains ravine/broken-bridge metadata.",
            "A contested entry selects orc.broken_bridge and the north entry direction.",
        ],
        "source_checks": [
            source_check("region", "orc_broken_bridge", {
                "landform": "ravine",
                "feature": "broken_bridge",
                "battle_recipe_hint": "orc.broken_bridge",
            }),
            source_check("approach", "approach.orc_ruined_field.orc_broken_bridge", {
                "route.road": True,
                "route.chokepoint": True,
                "entry_direction": "north",
                "battlefield.recipe_id": "orc.broken_bridge",
            }),
        ],
    },
    {
        "id": "ue.fixture.dragon_graveyard_special_site",
        "scope": "world_overmap_special_site",
        "purpose": "Exercise explicit Dragon Graveyard activation inside Orc Badlands without adding or rewiring a strategic graph node.",
        "setup": {
            "source_region": "orc_camp",
            "destination_region": "orc_badlands",
            "approach_id": "approach.orc_camp.orc_badlands",
            "activate_special_site_id": "dragon_graveyard",
        },
        "trigger": {
            "kind": "explicit_special_site_activation",
            "special_site_id": "dragon_graveyard",
        },
        "runtime_expectations": {
            "strategic_region_remains": "orc_badlands",
            "base_region_recipe_id": "orc.badlands",
            "special_site_override_required": True,
            "special_site_integration_class": "direct_battlefield",
            "topology_change": False,
            "entry_direction_preserved": "west",
        },
        "required_runtime_observations": [
            "destination_region_id",
            "active_special_site_id",
            "battlefield_source_kind",
            "battle_entry_direction",
            "overmap_node_count_before",
            "overmap_node_count_after",
        ],
        "pass_conditions": [
            "Selecting Dragon Graveyard does not create, delete, or reconnect an overmap node.",
            "The tactical launch reports dragon_graveyard as the explicit special-site override.",
            "The strategic approach direction from Orc Camp remains west-facing context in the battle handoff.",
            "If the Dragon Graveyard battle payload is unavailable, fail explicitly instead of silently falling back to orc.badlands.",
        ],
        "source_checks": [
            source_check("region", "orc_badlands", {
                "battle_recipe_hint": "orc.badlands",
                "biome": "badlands",
            }),
            source_check("approach", "approach.orc_camp.orc_badlands", {
                "entry_direction": "west",
                "battlefield.recipe_id": "orc.badlands",
            }),
            source_check("candidate_site", "dragon_graveyard", {
                "region_id": "orc_badlands",
                "integration_class": "direct_battlefield",
                "local_status": "PROJECT_INSTALLED",
                "topology_change": "False",
            }),
        ],
    },
    {
        "id": "ue.fixture.stronghold_reinforcement_retreat",
        "scope": "founder_slice",
        "purpose": "Verify bypass/reinforcement/retreat timing uses actual adjacent regions and distinct strategic entry directions.",
        "setup": {
            "primary_attack": {
                "source_region": "north_pass",
                "destination_region": "orc_camp",
                "approach_id": "approach.north_pass.orc_camp",
            },
            "reserve_reinforcement": {
                "source_region": "orc_watch",
                "destination_region": "orc_camp",
                "approach_id": "approach.orc_watch.orc_camp",
            },
            "retreat": {
                "source_region": "orc_camp",
                "destination_region": "north_pass",
                "approach_id": "approach.orc_camp.north_pass",
            },
        },
        "trigger": {
            "kind": "battle_with_adjacent_reserve_and_retreat",
            "battle_handoff_id": "battle_handoff.north_pass.orc_camp",
        },
        "runtime_expectations": {
            "attacker_entry_direction": "north_west",
            "reinforcement_entry_direction": "west",
            "retreat_destination_entry_direction": "south_east",
            "reinforcement_requires_strategic_timing": True,
            "retreat_requires_valid_adjacent_route": True,
        },
        "required_runtime_observations": [
            "primary_battle_entry_direction",
            "eligible_reinforcement_source_regions",
            "reinforcement_arrival_turn_or_time",
            "retreat_destination_region",
            "retreat_approach_id",
        ],
        "pass_conditions": [
            "An intact Orc Watch reserve is considered only through the real Orc Watch -> Stronghold road and timing rule.",
            "Primary North Pass attackers and Orc Watch reinforcements use distinct entry directions.",
            "Retreat to North Pass uses the reverse valid pass edge; no arbitrary destination teleport is allowed.",
        ],
        "source_checks": [
            source_check("handoff", "battle_handoff.north_pass.orc_camp", {
                "strategic_route.entry_direction": "north_west",
                "battlefield_selection.recipe_id": "orc.war_camp",
            }),
            source_check("approach", "approach.orc_watch.orc_camp", {
                "entry_direction": "west",
                "route.action_cost": 1,
            }),
            source_check("approach", "approach.orc_camp.north_pass", {
                "entry_direction": "south_east",
                "route.type": "pass",
            }),
        ],
    },
    {
        "id": "ue.fixture.river_ford_context",
        "scope": "founder_slice",
        "purpose": "Ensure the campaign river crossing selects a river-road battlefield context instead of a generic arena.",
        "setup": {
            "source_region": "crossroads",
            "destination_region": "river_ford",
            "approach_id": "approach.crossroads.river_ford",
        },
        "trigger": {
            "kind": "contested_crossing_entry",
            "battle_handoff_id": "battle_handoff.crossroads.river_ford",
        },
        "runtime_expectations": {
            "feature": "river_crossing",
            "landform": "valley",
            "entry_direction": "north_west",
            "battle_recipe_id": "human.river_road",
            "known_asset_gate": "UE_CROP_REQUIRED",
        },
        "required_runtime_observations": [
            "destination_feature",
            "battle_recipe_id",
            "battle_recipe_status",
            "battle_entry_direction",
        ],
        "pass_conditions": [
            "River Ford is visually and logically a crossing before battle commitment.",
            "The launch selects human.river_road with the north-west approach.",
            "Until the crop is qualified, the fixture reports UE_CROP_REQUIRED rather than claiming presentation acceptance.",
        ],
        "source_checks": [
            source_check("approach", "approach.crossroads.river_ford", {
                "destination_context.feature": "river_crossing",
                "destination_context.landform": "valley",
                "entry_direction": "north_west",
                "battlefield.recipe_id": "human.river_road",
                "battlefield.recipe_status": "UE_CROP_REQUIRED",
            }),
        ],
    },
]

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "UE_ACCEPTANCE_FIXTURES_PREPARED_NON_UE",
    "authority": {
        "role": "acceptance fixtures only; they do not change campaign rules or ownership",
        "geography": "Data/soul_world_overmap_v1_20260922.json",
        "approaches": "Data/soul_overmap_approach_profiles_v1_20260922.json",
        "founder_handoffs": "Data/soul_overmap_battle_handoff_v1_20260922.json",
        "founder_import": "Data/UEImport/soul_founder_import_bundle_v2_20260922.json",
        "special_sites": "Data/UEImport/soul_overmap_candidate_sites_v1_20260922.csv",
    },
    "fixture_policy": [
        "Test-only faction/hostility overrides must never mutate canonical start-state data.",
        "A fixture passes only when runtime observations exercise its stated pass conditions.",
        "Known asset-production gates are explicit expected blockers, not hidden fallbacks.",
        "Special-site activation cannot alter strategic topology unless separately admitted.",
    ],
    "fixtures": fixtures,
}
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul UE Overmap Acceptance Fixtures — 2026-09-22", "",
    "These are machine-readable future UE acceptance scenarios built from the current strategic contracts. They are not a second campaign ruleset.", "",
]
for fixture in fixtures:
    lines += [
        f"## {fixture['id']}", "",
        fixture["purpose"], "",
        "Pass conditions:",
    ]
    lines += [f"- {x}" for x in fixture["pass_conditions"]]
    lines.append("")
lines += [
    "## Known gates", "",
    "- River Ford is intentionally expected to report UE_CROP_REQUIRED until its battlefield crop is qualified.",
    "- Dragon Graveyard requires the separate proof lane to provide the actual special-site battlefield payload/binding; this fixture defines how that payload must connect to campaign geography.",
    "- Orc Stronghold remains UE_COMPOSITE; fixture acceptance must distinguish metadata correctness from final art qualification.",
]
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps({"fixtures": len(fixtures), "output": str(OUT)}, indent=2))
