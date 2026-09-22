"""Build the compact non-UE founder-slice presentation import for Soul."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INTEGRATED_PATH = ROOT / "Data" / "soul_overmap_integrated_import_v1_20260922.json"
SIM_PATH = ROOT / "Evidence" / "WorldOvermap" / "founder_slice_simulation.json"
FIDELITY_PATH = ROOT / "Evidence" / "WorldOvermap" / "battlefield_coverage_analysis.json"
OUT_PATH = ROOT / "Data" / "soul_founder_slice_presentation_import_v1_20260922.json"

integrated = json.loads(INTEGRATED_PATH.read_text(encoding="utf-8"))
sim = json.loads(SIM_PATH.read_text(encoding="utf-8"))
fidelity = json.loads(FIDELITY_PATH.read_text(encoding="utf-8"))
projection = integrated["founder_slice_projection"]
scenario = projection["scenario"]
region_ids = scenario["region_ids"]
fidelity_by_region = {row["region_id"]: row for row in fidelity["regions"]}
visible = set(scenario["initial_knowledge"]["humans"]["visible_regions"])
explored = set(scenario["initial_knowledge"]["humans"]["explored_regions"])
def fog_state(region_id: str) -> str:
    if region_id in visible:
        return "visible"
    if region_id in explored:
        return "explored_not_visible"
    return "unexplored"

regions = []
for region_id in region_ids:
    entry = projection["regions"][region_id]
    runtime = entry["runtime"]
    fidelity_row = fidelity_by_region[region_id]
    regions.append({
        "region_id": region_id,
        "display_name": runtime["display_name"],
        "ue_position_cm": runtime["ue_position_cm"],
        "selection_radius_cm": runtime["selection_radius_cm"],
        "initial_owner": entry["start_owner"],
        "initial_fog_state": fog_state(region_id),
        "biome": runtime["biome"],
        "landform": runtime["landform"],
        "feature": runtime["feature"],
        "resource": runtime["resource"],
        "visual_anchor": entry["visual_anchor"],
        "battlefield_recipe": entry["battlefield_recipe"],
        "battlefield_fidelity": fidelity_row,
    })
route_lookup = {}
for route_id, route in projection["routes"].items():
    route_lookup[frozenset((route["a"], route["b"]))] = route_id

routes = []
for route_id, route in sorted(projection["routes"].items()):
    routes.append({
        "route_id": route_id,
        "a": route["a"],
        "b": route["b"],
        "route_class": route["route_class"],
        "road": route["road"],
        "chokepoint": route["chokepoint"],
        "action_cost": route["action_cost"],
        "logistics_movement_cost": route["logistics_movement_cost"],
        "ue_spline_points_cm": route["ue_spline_points_cm"],
        "visual_cue": projection["route_visual_cues"][route_id],
    })

corridors = []
goal = scenario["enemy_primary_region"]
for index, approach in enumerate(sim["shortest_approaches"], start=1):
    path = list(approach["path"]) + [goal]
    route_ids = []
    route_classes = []
    for a, b in zip(path, path[1:]):
        route_id = route_lookup[frozenset((a, b))]
        route_ids.append(route_id)
        route_classes.append(projection["routes"][route_id]["route_class"])
    corridors.append({
        "id": f"founder_corridor_{index}",
        "path": path,
        "final_approach": approach["final_approach"],
        "route_ids": route_ids,
        "route_classes": route_classes,
        "visual_signature": " -> ".join(route_classes),
        "total_actions_to_battle": approach["total_actions_to_battle"],
        "logistics_cost_to_approach": approach["logistics_cost"],
        "road_edges_to_approach": approach["road_edges"],
        "chokepoints_to_approach": approach["chokepoints"],
    })

priority_review = [
    rid for rid in region_ids
    if fidelity_by_region[rid]["fidelity_class"] == "context_transform_review"
]
source_sha = hashlib.sha256(INTEGRATED_PATH.read_bytes()).hexdigest()
payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "FOUNDER_PRESENTATION_IMPORT_NON_UE",
    "source_integrated_import": INTEGRATED_PATH.name,
    "source_integrated_import_sha256": source_sha,
    "authority": {
        "campaign_state": "SoulCore / integrated overmap contracts",
        "weather": "RB Weather",
        "optimization": "RB Optimization",
        "save": "RB Save",
        "presentation": "Unreal consumes this artifact; it does not invent campaign state",
    },
    "scenario": scenario,
    "camera": integrated["presentation_contract"]["camera"],
    "fog_contract": integrated["presentation_contract"]["fog"],
    "regions": regions,
    "routes": routes,
    "directed_approaches": projection["directed_approaches"],
    "shortest_attack_corridors": corridors,
    "representative_reveal_sequence": sim["representative_reveal_sequence"],
    "battlefield_fidelity_review_regions": priority_review,
    "acceptance_targets": {
        "regions": 9,
        "routes": 10,
        "directed_approaches": 20,
        "distinct_shortest_attack_corridors": 3,
        "initial_visible_regions": len(visible),
        "initial_explored_regions": len(explored),
    },
}
OUT_PATH.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
print(f"WROTE {OUT_PATH}")
print(json.dumps(payload["acceptance_targets"], indent=2))
