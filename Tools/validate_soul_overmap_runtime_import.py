"""Validate Soul overmap UE-facing import data and founder traversal evidence."""
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
IMPORT = json.loads((ROOT/"Data"/"soul_overmap_runtime_import_v1_20260922.json").read_text(encoding="utf-8"))
SIM = json.loads((ROOT/"Evidence"/"WorldOvermap"/"founder_slice_simulation.json").read_text(encoding="utf-8"))
OUT = ROOT/"Evidence"/"WorldOvermap"/"runtime_import_validation.json"

errors = []
world_regions = {x["id"] for x in WORLD["nodes"]}
import_regions = set(IMPORT["regions"])
if import_regions != world_regions:
    errors.append("runtime import region set differs from structural world")

expected_routes = {"route."+"_".join(sorted((e["a"],e["b"]))) for e in WORLD["edges"]}
if set(IMPORT["routes"]) != expected_routes:
    errors.append("runtime import route set differs from structural world")

positions = {}
for rid, region in IMPORT["regions"].items():
    pos = region.get("ue_position_cm")
    if not isinstance(pos, list) or len(pos) != 3:
        errors.append(f"invalid UE position: {rid}")
        continue
    key = tuple(pos[:2])
    if key in positions:
        errors.append(f"overlapping UE XY positions: {rid} and {positions[key]}")
    positions[key] = rid
for route_id, route in IMPORT["routes"].items():
    pts = route.get("ue_spline_points_cm", [])
    if len(pts) != 3:
        errors.append(f"route spline must have 3 points: {route_id}")
        continue
    a = IMPORT["regions"][route["a"]]["ue_position_cm"]
    b = IMPORT["regions"][route["b"]]["ue_position_cm"]
    if pts[0] != a or pts[-1] != b:
        errors.append(f"route spline endpoints drift: {route_id}")
    if route["action_cost"] != 1:
        errors.append(f"route action cost drift: {route_id}")
    if route["road"] and route["spline_width_cm"] < 800:
        errors.append(f"road width too narrow: {route_id}")
    if not route["road"] and route["spline_width_cm"] > 600:
        errors.append(f"trail width too broad: {route_id}")

capitals = [r for r in IMPORT["regions"].values() if r["anchor_type"] == "settlement_proxy"]
if len(capitals) != 6:
    errors.append(f"expected 6 settlement proxies, got {len(capitals)}")

if SIM.get("status") != "pass":
    errors.append("founder traversal simulation failed")
if SIM.get("shortest_approach_count") != 3:
    errors.append("founder slice lost one of its three equal-action approaches")
if SIM.get("minimum_actions_including_battle") != 4:
    errors.append("founder minimum battle commitment drifted from 4 actions")
if SIM.get("resource_detours",{}).get("quarry_resource_loop",{}).get("days_at_3_ap") != 2:
    errors.append("quarry resource detour no longer fits a 2-day route")
xs = [p[0] for p in positions]
ys = [p[1] for p in positions]
extent_km = [
    round((max(xs)-min(xs))/100000.0, 2),
    round((max(ys)-min(ys))/100000.0, 2),
] if positions else [0,0]

result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "regions": len(import_regions),
    "routes": len(IMPORT["routes"]),
    "settlement_proxies": len(capitals),
    "actual_anchor_extent_km": extent_km,
    "declared_world_extent_km": IMPORT["world_space"]["approx_world_extent_km"],
    "founder_shortest_approaches": SIM.get("shortest_approach_count"),
    "founder_min_actions_to_battle": SIM.get("minimum_actions_including_battle"),
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
