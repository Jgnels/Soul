"""Build a single versioned, importer-ready Soul founder-slice bundle."""
from __future__ import annotations

import csv
import hashlib
import io
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
UE = DATA / "UEImport"

SOURCE_FILES = [
    DATA / "soul_world_overmap_v1_20260922.json",
    DATA / "soul_campaign_start_states_v1_20260922.json",
    DATA / "soul_founder_slice_presentation_import_v1_20260922.json",
    DATA / "soul_founder_presentation_state_vectors_v1_20260922.json",
    DATA / "soul_overmap_settlement_slots_v1_20260922.json",
    DATA / "soul_overmap_surface_bindings_v1_20260922.json",
    DATA / "soul_overmap_visual_anchors_v1_20260922.json",
    DATA / "soul_overmap_route_visual_cues_v1_20260922.json",
    DATA / "soul_overmap_approach_profiles_v1_20260922.json",
    DATA / "soul_overmap_battle_handoff_v1_20260922.json",
]
SOURCE_STATE_CSV = UE / "soul_founder_presentation_states_v1_20260922.csv"
SOURCE_CANDIDATES_CSV = UE / "soul_overmap_candidate_sites_v1_20260922.csv"

OUT_JSON = UE / "soul_founder_import_bundle_v2_20260922.json"
OUT_REGIONS = UE / "soul_founder_regions_import_v2_20260922.csv"
OUT_ROUTES = UE / "soul_founder_routes_import_v2_20260922.csv"
OUT_APPROACHES = UE / "soul_founder_approaches_import_v2_20260922.csv"
OUT_STATES = UE / "soul_founder_states_import_v2_20260922.csv"
DOC = ROOT / "Docs" / "SOUL_FOUNDER_IMPORT_BUNDLE_V2_20260922.md"

def load(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def pipe(values) -> str:
    return "|".join(str(x) for x in values if x not in (None, ""))

def bool_text(value) -> str:
    return "true" if bool(value) else "false"

def write_csv(path: Path, fields: list[str], rows: list[dict]) -> None:
    buffer = io.StringIO(newline="")
    writer = csv.DictWriter(buffer, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    for row in rows:
        writer.writerow({key: row.get(key, "") for key in fields})
    path.write_text(buffer.getvalue(), encoding="utf-8")

world = load(DATA / "soul_world_overmap_v1_20260922.json")
starts = load(DATA / "soul_campaign_start_states_v1_20260922.json")
presentation = load(DATA / "soul_founder_slice_presentation_import_v1_20260922.json")
state_vectors = load(DATA / "soul_founder_presentation_state_vectors_v1_20260922.json")
settlements = load(DATA / "soul_overmap_settlement_slots_v1_20260922.json")
surfaces = load(DATA / "soul_overmap_surface_bindings_v1_20260922.json")
handoff_doc = load(DATA / "soul_overmap_battle_handoff_v1_20260922.json")

scenario_id = "founder_human_orc_micro"
scenario = starts["scenarios"][scenario_id]
founder_ids = set(scenario["region_ids"])
nodes = {n["id"]: n for n in world["nodes"]}
settlement_by_region = {x["region_id"]: x for x in settlements["slots"] if x["region_id"] in founder_ids}
approach_by_pair = {(x["source_region"], x["destination_region"]): x for x in presentation["directed_approaches"]}
handoff_by_pair = {(x["source_region"], x["destination_region"]): x for x in handoff_doc["handoffs"]}

candidate_by_region: dict[str, list[str]] = defaultdict(list)
with SOURCE_CANDIDATES_CSV.open("r", encoding="utf-8-sig", newline="") as handle:
    for row in csv.DictReader(handle):
        if row["region_id"] in founder_ids:
            candidate_by_region[row["region_id"]].append(row["site_id"])

incoming: dict[str, list[str]] = defaultdict(list)
outgoing: dict[str, list[str]] = defaultdict(list)
for approach in presentation["directed_approaches"]:
    incoming[approach["destination_region"]].append(approach["id"])
    outgoing[approach["source_region"]].append(approach["id"])

region_rows: list[dict] = []
regions_json: list[dict] = []
for region in sorted(presentation["regions"], key=lambda x: x["region_id"]):
    rid = region["region_id"]
    node = nodes[rid]
    macro = node["macro_region"]
    surface = surfaces["macro_regions"][macro]
    anchor = region["visual_anchor"]
    recipe = region["battlefield_recipe"]
    slot = settlement_by_region.get(rid)
    position = region["ue_position_cm"]
    row = {
        "RegionId": rid,
        "DisplayName": region["display_name"],
        "X": position[0], "Y": position[1], "Z": position[2],
        "SelectionRadius": region["selection_radius_cm"],
        "InitialOwner": region.get("initial_owner") or "",
        "InitialFogState": region["initial_fog_state"],
        "MacroRegion": macro,
        "Biome": region["biome"],
        "Landform": region["landform"],
        "Feature": region["feature"],
        "ElevationBand": node["elevation_band"],
        "Resource": region.get("resource") or "",
        "SurfaceIdentity": surface["identity"],
        "SurfaceStatus": surface["status"],
        "SurfaceTerrainSources": pipe(surface["terrain"]),
        "SurfaceSettlementSources": pipe(surface["settlement"]),
        "AnchorType": anchor["anchor_type"],
        "AnchorLabel": anchor["anchor_label"],
        "AnchorSource": anchor["source"],
        "AnchorScaleClass": anchor["scale_class"],
        "Interactive": bool_text(anchor["interactive"]),
        "InteractionScope": anchor["interaction_scope"],
        "PersistentStateProjection": bool_text(anchor["persistent_state_projection"]),
        "SettlementTier": slot["tier"] if slot else "",
        "SettlementStatus": slot["status"] if slot else "",
        "SettlementId": (slot.get("settlement_id") or "") if slot else "",
        "SettlementVisitScope": slot["visit_scope"] if slot else "",
        "SettlementServices": pipe(slot["services"]) if slot else "",
        "SettlementDonor": slot["donor"] if slot else "",
        "BattleRecipe": recipe["id"],
        "BattleRecipeStatus": recipe["status"],
        "BattleRecipeDonor": recipe["donor"],
        "BattleLandmark": recipe["landmark"],
        "CandidateSiteIds": pipe(sorted(candidate_by_region.get(rid, []))),
        "IncomingApproachIds": pipe(sorted(incoming[rid])),
        "OutgoingApproachIds": pipe(sorted(outgoing[rid])),
        "IncomingHandoffIds": pipe(sorted(handoff_by_pair[(presentation_a["source_region"], rid)]["id"] for presentation_a in presentation["directed_approaches"] if presentation_a["destination_region"] == rid)),
        "OutgoingHandoffIds": pipe(sorted(handoff_by_pair[(rid, presentation_a["destination_region"])]["id"] for presentation_a in presentation["directed_approaches"] if presentation_a["source_region"] == rid)),
    }
    region_rows.append(row)
    regions_json.append(row)

route_rows: list[dict] = []
routes_json: list[dict] = []
for route in sorted(presentation["routes"], key=lambda x: x["route_id"]):
    a, b = route["a"], route["b"]
    cue = route["visual_cue"]
    ab = approach_by_pair[(a, b)]
    ba = approach_by_pair[(b, a)]
    hab = handoff_by_pair[(a, b)]
    hba = handoff_by_pair[(b, a)]
    row = {
        "RouteId": route["route_id"], "A": a, "B": b,
        "RouteClass": route["route_class"],
        "Road": bool_text(route["road"]), "Chokepoint": bool_text(route["chokepoint"]),
        "ActionCost": route["action_cost"], "LogisticsCost": route["logistics_movement_cost"],
        "SplinePointsCmJson": json.dumps(route["ue_spline_points_cm"], separators=(",", ":")),
        "SplineWidth": cue["spline_width_cm"],
        "RenderFamily": cue["render_family"], "Surface": cue["surface"],
        "NavigationCue": cue["navigation_cue"],
        "ChokepointCue": cue.get("chokepoint_cue") or "",
        "SelectionOverlay": cue["selection_overlay"],
        "AtoBApproachId": ab["id"], "AtoBEntryDirection": ab["entry_direction"],
        "AtoBTransitionKind": ab["destination_context"]["transition_kind"],
        "AtoBBattleRecipe": ab["battlefield"]["recipe_id"],
        "AtoBBattleRecipeStatus": ab["battlefield"]["recipe_status"],
        "AtoBBattleHandoffId": hab["id"],
        "BtoAApproachId": ba["id"], "BtoAEntryDirection": ba["entry_direction"],
        "BtoATransitionKind": ba["destination_context"]["transition_kind"],
        "BtoABattleRecipe": ba["battlefield"]["recipe_id"],
        "BtoABattleRecipeStatus": ba["battlefield"]["recipe_status"],
        "BtoABattleHandoffId": hba["id"],
    }
    route_rows.append(row)
    routes_json.append(row)

approach_rows: list[dict] = []
approaches_json: list[dict] = []
for approach in sorted(presentation["directed_approaches"], key=lambda x: x["id"]):
    key = (approach["source_region"], approach["destination_region"])
    h = handoff_by_pair[key]
    reinforcements = h["reinforcement_context"]["alternate_adjacent_entry_slots"]
    sites = h["special_site_overrides"]
    row = {
        "ApproachId": approach["id"],
        "SourceRegion": approach["source_region"],
        "DestinationRegion": approach["destination_region"],
        "RouteType": approach["route"]["type"],
        "Road": bool_text(approach["route"]["road"]),
        "Chokepoint": bool_text(approach["route"]["chokepoint"]),
        "ActionCost": approach["route"]["action_cost"],
        "LogisticsCost": approach["route"]["logistics_movement_cost"],
        "EntryDirection": approach["entry_direction"],
        "TransitionKind": approach["destination_context"]["transition_kind"],
        "BattleRecipe": approach["battlefield"]["recipe_id"],
        "BattleRecipeStatus": approach["battlefield"]["recipe_status"],
        "BattleHandoffId": h["id"],
        "EncounterKind": h["encounter_kind"],
        "DestinationOwnerAtStart": h["scenario_start_control"]["destination_owner"] or "",
        "HostileAtStart": bool_text(h["scenario_start_control"]["hostile_at_start"]),
        "WeatherAuthority": h["weather_time_context"]["weather_authority"],
        "TimeAuthority": h["weather_time_context"]["time_authority"],
        "ReinforcementSourceRegions": pipe(sorted(x["source_region"] for x in reinforcements)),
        "ReinforcementEntryDirections": pipe(sorted(f'{x["source_region"]}:{x["entry_direction"]}' for x in reinforcements)),
        "SpecialSiteIds": pipe(sorted(x["site_id"] for x in sites)),
        "ApproachTags": pipe(approach["approach_tags"]),
    }
    approach_rows.append(row)
    approaches_json.append(row)

with SOURCE_STATE_CSV.open("r", encoding="utf-8-sig", newline="") as handle:
    state_source_rows = list(csv.DictReader(handle))
region_row_map = {x["RegionId"]: x for x in region_rows}
approach_id_to_handoff = {
    approach_by_pair[pair]["id"]: h["id"] for pair, h in handoff_by_pair.items()
}
state_rows: list[dict] = []
for source in state_source_rows:
    rid = source["ActiveRegion"]
    row = dict(source)
    row["ActiveRegionBiome"] = region_row_map[rid]["Biome"]
    row["ActiveRegionAnchorType"] = region_row_map[rid]["AnchorType"]
    row["BattleHandoffId"] = approach_id_to_handoff.get(source.get("BattleApproachId", ""), "")
    state_rows.append(row)

region_fields = list(region_rows[0])
route_fields = list(route_rows[0])
approach_fields = list(approach_rows[0])
state_fields = list(state_rows[0])
write_csv(OUT_REGIONS, region_fields, region_rows)
write_csv(OUT_ROUTES, route_fields, route_rows)
write_csv(OUT_APPROACHES, approach_fields, approach_rows)
write_csv(OUT_STATES, state_fields, state_rows)

outputs = [OUT_REGIONS, OUT_ROUTES, OUT_APPROACHES, OUT_STATES]
manifest = {
    "schema": 2,
    "semantic_version": "2.0.0",
    "generated": "2026-09-22",
    "status": "FOUNDER_IMPORT_READY_NON_UE",
    "scenario_id": scenario_id,
    "authority": {
        "campaign_rules": "SoulCore/current source contracts",
        "weather": "RBWeather",
        "optimization": "RBOptimization",
        "presentation": "this bundle is import staging; source JSON contracts remain authoritative",
        "battle_handoff": "Data/soul_overmap_battle_handoff_v1_20260922.json",
    },
    "import_order": ["regions", "routes", "approaches", "states"],
    "counts": {
        "regions": len(region_rows), "routes": len(route_rows),
        "directed_approaches": len(approach_rows), "presentation_states": len(state_rows),
        "settlement_slots": len(settlement_by_region),
    },
    "sources": {str(x.relative_to(ROOT)).replace("\\", "/"): sha(x) for x in SOURCE_FILES + [SOURCE_STATE_CSV, SOURCE_CANDIDATES_CSV]},
    "files": {str(x.relative_to(ROOT)).replace("\\", "/"): sha(x) for x in outputs},
    "required_region_metadata": region_fields,
    "required_route_metadata": route_fields,
    "required_approach_metadata": approach_fields,
    "required_state_metadata": state_fields,
    "regions": regions_json,
    "routes": routes_json,
    "approaches": approaches_json,
    "state_vector_summary": {
        "corridors": len(state_vectors["corridors"]),
        "detours": len(state_vectors["detours"]),
        "decision_points": state_vectors["decision_points"],
        "fixture_active_regions": state_vectors["fixture_active_regions"],
    },
}
OUT_JSON.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder Import Bundle v2 — 2026-09-22", "",
    "Status: **FOUNDER_IMPORT_READY_NON_UE**", "",
    "This consolidates the founder geography, settlement, surface, visual-anchor, route, directed-approach, battle-handoff, and presentation-state contracts into one versioned import package.",
    "",
    f"- Regions: {len(region_rows)} / 9.",
    f"- Routes: {len(route_rows)} / 10.",
    f"- Directed approaches: {len(approach_rows)} / 20.",
    f"- Presentation states: {len(state_rows)}.",
    f"- Settlement slots carried into founder import: {len(settlement_by_region)}.",
    "",
    "## Import order", "",
    "1. Regions.",
    "2. Routes/splines.",
    "3. Directed approach and battle-launch metadata.",
    "4. Presentation-state fixtures.",
    "",
    "The JSON manifest embeds the same enriched region/route/approach data and SHA-256 pins every source and CSV output. Unreal remains presentation/import authority only; strategic rules stay in SoulCore and weather stays in RBWeather.",
]
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(manifest["counts"], indent=2))
print(OUT_JSON)
