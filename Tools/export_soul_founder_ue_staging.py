"""Flatten Soul founder overmap data into Unreal/DataTable-friendly staging CSVs."""
import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESENTATION = ROOT / "Data" / "soul_founder_slice_presentation_import_v1_20260922.json"
STATES = ROOT / "Data" / "soul_founder_presentation_state_vectors_v1_20260922.json"
OUT_DIR = ROOT / "Data" / "UEImport"
MANIFEST = OUT_DIR / "soul_founder_ue_import_manifest_v1_20260922.json"

p = json.loads(PRESENTATION.read_text(encoding="utf-8"))
s = json.loads(STATES.read_text(encoding="utf-8"))
OUT_DIR.mkdir(parents=True, exist_ok=True)

def row_name(prefix, value):
    return prefix + "_" + value.replace(".", "_").replace("-", "_")

def join(values):
    return "|".join(str(v) for v in values)

def write_csv(path, fieldnames, rows):
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="raise")
        writer.writeheader()
        writer.writerows(rows)
region_rows = []
for region in p["regions"]:
    x, y, z = region["ue_position_cm"]
    anchor = region["visual_anchor"]
    recipe = region["battlefield_recipe"]
    fidelity = region["battlefield_fidelity"]
    region_rows.append({
        "Name": row_name("region", region["region_id"]),
        "RegionId": region["region_id"],
        "DisplayName": region["display_name"],
        "X": x, "Y": y, "Z": z,
        "SelectionRadius": region["selection_radius_cm"],
        "InitialOwner": region["initial_owner"] or "",
        "InitialFogState": region["initial_fog_state"],
        "Biome": region["biome"],
        "Landform": region["landform"],
        "Feature": region["feature"],
        "Resource": region["resource"] or "",
        "AnchorType": anchor["anchor_type"],
        "AnchorLabel": anchor["anchor_label"],
        "AnchorSource": anchor["source"],
        "AnchorScaleClass": anchor["scale_class"],
        "BattleRecipe": recipe["id"],
        "BattleRecipeStatus": recipe["status"],
        "FidelityClass": fidelity["fidelity_class"],
    })
region_fields = [
    "Name","RegionId","DisplayName","X","Y","Z","SelectionRadius","InitialOwner",
    "InitialFogState","Biome","Landform","Feature","Resource","AnchorType",
    "AnchorLabel","AnchorSource","AnchorScaleClass","BattleRecipe",
    "BattleRecipeStatus","FidelityClass",
]
region_path = OUT_DIR / "soul_founder_regions_v1_20260922.csv"
write_csv(region_path, region_fields, region_rows)

route_rows = []
for route in p["routes"]:
    points = route["ue_spline_points_cm"]
    cue = route["visual_cue"]
    flat = {}
    for i in range(3):
        point = points[i] if i < len(points) else ["", "", ""]
        flat[f"Spline{i}X"], flat[f"Spline{i}Y"], flat[f"Spline{i}Z"] = point
    route_rows.append({
        "Name": row_name("route", route["route_id"]),
        "RouteId": route["route_id"],
        "A": route["a"], "B": route["b"],
        "RouteClass": route["route_class"],
        "Road": str(route["road"]).lower(),
        "Chokepoint": str(route["chokepoint"]).lower(),
        "ActionCost": route["action_cost"],
        "LogisticsCost": route["logistics_movement_cost"],
        "SplineWidth": cue["spline_width_cm"],
        "RenderFamily": cue["render_family"],
        "Surface": cue["surface"],
        "NavigationCue": cue["navigation_cue"],
        **flat,
    })
route_fields = [
    "Name","RouteId","A","B","RouteClass","Road","Chokepoint","ActionCost",
    "LogisticsCost","SplineWidth","RenderFamily","Surface","NavigationCue",
    "Spline0X","Spline0Y","Spline0Z","Spline1X","Spline1Y","Spline1Z",
    "Spline2X","Spline2Y","Spline2Z",
]
route_path = OUT_DIR / "soul_founder_routes_v1_20260922.csv"
write_csv(route_path, route_fields, route_rows)

approach_rows = []
for approach in p["directed_approaches"]:
    route = approach["route"]
    context = approach["destination_context"]
    battlefield = approach["battlefield"]
    approach_rows.append({
        "Name": row_name("approach", approach["id"]),
        "ApproachId": approach["id"],
        "SourceRegion": approach["source_region"],
        "DestinationRegion": approach["destination_region"],
        "RouteType": route["type"],
        "Road": str(route["road"]).lower(),
        "Chokepoint": str(route["chokepoint"]).lower(),
        "ActionCost": route["action_cost"],
        "LogisticsCost": route["logistics_movement_cost"],
        "EntryDirection": approach["entry_direction"],
        "TransitionKind": context["transition_kind"],
        "SettlementId": context["settlement_id"] or "",
        "Resource": context["resource"] or "",
        "BattleRecipe": battlefield["recipe_id"],
        "BattleRecipeStatus": battlefield["recipe_status"],
        "DynamicContextRequired": join(battlefield["dynamic_context_required"]),
        "ApproachTags": join(approach["approach_tags"]),
    })
approach_fields = [
    "Name","ApproachId","SourceRegion","DestinationRegion","RouteType","Road",
    "Chokepoint","ActionCost","LogisticsCost","EntryDirection","TransitionKind",
    "SettlementId","Resource","BattleRecipe","BattleRecipeStatus",
    "DynamicContextRequired","ApproachTags",
]
approach_path = OUT_DIR / "soul_founder_approaches_v1_20260922.csv"
write_csv(approach_path, approach_fields, approach_rows)

state_rows = []
for source_kind, groups, id_key in (
    ("corridor", s["corridors"], "corridor_id"),
    ("detour", s["detours"], "detour_id"),
):
    for group in groups:
        scenario_id = group[id_key]
        for state in group["states"]:
            cx, cy, cz = state["camera_focus_cm"]
            battle = state.get("battle_commit", {})
            state_rows.append({
                "Name": f"{source_kind}_{scenario_id}_step_{state['step']}",
                "SourceKind": source_kind,
                "ScenarioId": scenario_id,
                "Step": state["step"],
                "ActiveRegion": state["active_region"],
                "CameraX": cx, "CameraY": cy, "CameraZ": cz,
                "TravelActions": state["cumulative_travel_actions"],
                "VisibleRegions": join(state["visible_regions"]),
                "ExploredRegions": join(state["explored_regions"]),
                "MemoryRegions": join(state["explored_not_visible_regions"]),
                "UnexploredRegions": join(state["unexplored_regions"]),
                "LiveDynamicRegions": join(state["live_dynamic_regions"]),
                "KnownRouteIds": join(state["known_route_ids"]),
                "SelectableRouteIds": join(state["selectable_route_ids"]),
                "SelectableDestinations": join(state["selectable_destinations"]),
                "PlannedTargetRegion": state["planned_target_region"],
                "PlannedRouteId": state["planned_route_id"],
                "PlannedAction": state["planned_action"],
                "BattleApproachId": battle.get("directed_approach_id", ""),
                "BattleEntryDirection": battle.get("entry_direction", ""),
                "BattleRecipeId": battle.get("battlefield_recipe_id", ""),
            })
state_fields = [
    "Name","SourceKind","ScenarioId","Step","ActiveRegion","CameraX","CameraY",
    "CameraZ","TravelActions","VisibleRegions","ExploredRegions","MemoryRegions",
    "UnexploredRegions","LiveDynamicRegions","KnownRouteIds","SelectableRouteIds",
    "SelectableDestinations","PlannedTargetRegion","PlannedRouteId","PlannedAction",
    "BattleApproachId","BattleEntryDirection","BattleRecipeId",
]
state_path = OUT_DIR / "soul_founder_presentation_states_v1_20260922.csv"
write_csv(state_path, state_fields, state_rows)

files = [region_path, route_path, approach_path, state_path]
manifest = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "UE_STAGING_NONAUTHORITATIVE",
    "authority": "SoulCore / source JSON contracts remain authoritative; CSVs are import staging only",
    "sources": {
        PRESENTATION.name: hashlib.sha256(PRESENTATION.read_bytes()).hexdigest(),
        STATES.name: hashlib.sha256(STATES.read_bytes()).hexdigest(),
    },
    "import_order": ["regions", "routes", "approaches", "presentation_states"],
    "counts": {
        "regions": len(region_rows),
        "routes": len(route_rows),
        "approaches": len(approach_rows),
        "presentation_states": len(state_rows),
    },
    "files": {
        path.name: hashlib.sha256(path.read_bytes()).hexdigest()
        for path in files
    },
    "array_encoding": "pipe-delimited stable IDs inside CSV fields",
}
MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print(json.dumps(manifest["counts"], indent=2))
print(f"WROTE {MANIFEST}")
