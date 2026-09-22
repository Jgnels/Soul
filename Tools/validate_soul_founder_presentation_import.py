"""Validate Soul's compact founder-slice presentation import without Unreal."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA_PATH = ROOT / "Data" / "soul_founder_slice_presentation_import_v1_20260922.json"
OUT_JSON = ROOT / "Evidence" / "WorldOvermap" / "founder_presentation_import_validation.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "founder_presentation_import_validation.md"

data = json.loads(DATA_PATH.read_text(encoding="utf-8"))
source_path = ROOT / "Data" / data["source_integrated_import"]
errors = []

source_sha = hashlib.sha256(source_path.read_bytes()).hexdigest()
if source_sha != data["source_integrated_import_sha256"]:
    errors.append("integrated import source hash drift")

regions = data["regions"]
routes = data["routes"]
approaches = data["directed_approaches"]
corridors = data["shortest_attack_corridors"]
region_by_id = {row["region_id"]: row for row in regions}
region_ids = set(region_by_id)
expected_ids = set(data["scenario"]["region_ids"])
targets = data["acceptance_targets"]
if len(regions) != targets["regions"] or region_ids != expected_ids:
    errors.append("founder region set/count mismatch")
if len(routes) != targets["routes"]:
    errors.append("founder route count mismatch")
if len(approaches) != targets["directed_approaches"]:
    errors.append("founder directed-approach count mismatch")
if len(corridors) != targets["distinct_shortest_attack_corridors"]:
    errors.append("shortest attack corridor count mismatch")

positions = []
for region in regions:
    if len(region["ue_position_cm"]) != 3:
        errors.append(f"{region['region_id']}: invalid UE position")
    positions.append(tuple(region["ue_position_cm"]))
    if region["selection_radius_cm"] <= 0:
        errors.append(f"{region['region_id']}: non-positive selection radius")
    anchor = region.get("visual_anchor") or {}
    if anchor.get("region_id") != region["region_id"] or not anchor.get("founder_slice"):
        errors.append(f"{region['region_id']}: missing founder visual anchor")
    recipe = region.get("battlefield_recipe") or {}
    if not recipe.get("id"):
        errors.append(f"{region['region_id']}: missing battlefield recipe")
if len(set(positions)) != len(positions):
    errors.append("duplicate founder UE positions")
route_ids = set()
for route in routes:
    rid = route["route_id"]
    if rid in route_ids:
        errors.append(f"duplicate founder route: {rid}")
    route_ids.add(rid)
    if route["a"] not in region_ids or route["b"] not in region_ids:
        errors.append(f"{rid}: endpoint outside founder slice")
    if len(route["ue_spline_points_cm"]) < 2:
        errors.append(f"{rid}: missing spline geometry")
    cue = route["visual_cue"]
    if cue["route_id"] != rid or cue["route_class"] != route["route_class"]:
        errors.append(f"{rid}: route cue drift")
    if cue["permanent_map_line"]:
        errors.append(f"{rid}: permanent graph line violates presentation contract")

for approach in approaches:
    src = approach["source_region"]
    dst = approach["destination_region"]
    if src not in region_ids or dst not in region_ids:
        errors.append(f"{approach['id']}: approach leaves founder slice")
        continue
    expected_recipe = region_by_id[dst]["battlefield_recipe"]["id"]
    if approach["battlefield"]["recipe_id"] != expected_recipe:
        errors.append(f"{approach['id']}: destination recipe drift")
    if not approach.get("entry_direction"):
        errors.append(f"{approach['id']}: missing entry direction")
goal = data["scenario"]["enemy_primary_region"]
signatures = set()
for corridor in corridors:
    if corridor["path"][0] != data["scenario"]["player_start_region"]:
        errors.append(f"{corridor['id']}: wrong start")
    if corridor["path"][-1] != goal:
        errors.append(f"{corridor['id']}: wrong battle objective")
    if len(corridor["route_ids"]) != len(corridor["path"]) - 1:
        errors.append(f"{corridor['id']}: route/path length mismatch")
    if any(route_id not in route_ids for route_id in corridor["route_ids"]):
        errors.append(f"{corridor['id']}: unknown route")
    signatures.add(corridor["visual_signature"])
if len(signatures) != len(corridors):
    errors.append("shortest attack corridors are not visually distinct")

fog_counts = {"visible": 0, "explored_not_visible": 0, "unexplored": 0}
for region in regions:
    fog_counts[region["initial_fog_state"]] += 1
if fog_counts["visible"] != targets["initial_visible_regions"]:
    errors.append("initial visible-region count drift")
if fog_counts["visible"] + fog_counts["explored_not_visible"] != targets["initial_explored_regions"]:
    errors.append("initial explored-region count drift")
authority = data["authority"]
for key, expected in {
    "weather": "RB Weather",
    "optimization": "RB Optimization",
    "save": "RB Save",
}.items():
    if authority.get(key) != expected:
        errors.append(f"RB authority drift: {key}")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "regions": len(regions),
    "routes": len(routes),
    "directed_approaches": len(approaches),
    "distinct_shortest_attack_corridors": len(signatures),
    "initial_fog_counts": fog_counts,
    "battlefield_fidelity_review_regions": data["battlefield_fidelity_review_regions"],
    "source_integrated_import_sha256": source_sha,
    "errors": errors,
}
OUT_JSON.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
lines = [
    "# Soul Founder Presentation Import Validation",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- Regions: {result['regions']}",
    f"- Routes: {result['routes']}",
    f"- Directed approaches: {result['directed_approaches']}",
    f"- Distinct shortest attack corridors: {result['distinct_shortest_attack_corridors']}",
    f"- Initial fog: {fog_counts}",
    "",
    "## Battlefield fidelity review required",
    "",
]
for region_id in data["battlefield_fidelity_review_regions"]:
    lines.append(f"- {region_by_id[region_id]['display_name']} (`{region_id}`)")
if errors:
    lines += ["", "## Errors", ""] + [f"- {error}" for error in errors]
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
