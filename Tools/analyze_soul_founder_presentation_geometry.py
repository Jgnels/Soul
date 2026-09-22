"""Audit founder-slice selection and route geometry before Unreal presentation work."""
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA_PATH = ROOT / "Data" / "soul_founder_slice_presentation_import_v1_20260922.json"
OUT_JSON = ROOT / "Evidence" / "WorldOvermap" / "founder_presentation_geometry.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "founder_presentation_geometry.md"

data = json.loads(DATA_PATH.read_text(encoding="utf-8"))
regions = data["regions"]
routes = data["routes"]
region_by_id = {r["region_id"]: r for r in regions}
errors = []

pair_rows = []
for i, a in enumerate(regions):
    ax, ay, _ = a["ue_position_cm"]
    for b in regions[i + 1:]:
        bx, by, _ = b["ue_position_cm"]
        distance = math.hypot(ax - bx, ay - by)
        combined_radius = a["selection_radius_cm"] + b["selection_radius_cm"]
        pair_rows.append({
            "a": a["region_id"], "b": b["region_id"],
            "distance_cm": round(distance, 3),
            "combined_selection_radius_cm": combined_radius,
            "selection_overlap_cm": round(max(0.0, combined_radius - distance), 3),
        })
selection_overlaps = [row for row in pair_rows if row["selection_overlap_cm"] > 0]
if selection_overlaps:
    errors.append(f"selection overlaps: {len(selection_overlaps)}")

route_rows = []
for route in routes:
    points = route["ue_spline_points_cm"]
    polyline = sum(math.dist(a[:2], b[:2]) for a, b in zip(points, points[1:]))
    direct = math.dist(points[0][:2], points[-1][:2])
    expected_a = region_by_id[route["a"]]["ue_position_cm"]
    expected_b = region_by_id[route["b"]]["ue_position_cm"]
    endpoint_match = points[0] == expected_a and points[-1] == expected_b
    if not endpoint_match:
        errors.append(f"{route['route_id']}: spline endpoints drift from region anchors")
    if direct <= 0 or polyline <= 0:
        errors.append(f"{route['route_id']}: degenerate route geometry")
    route_rows.append({
        "route_id": route["route_id"],
        "route_class": route["route_class"],
        "polyline_length_cm": round(polyline, 3),
        "direct_distance_cm": round(direct, 3),
        "sinuosity_ratio": round(polyline / direct, 4) if direct else None,
        "endpoint_match": endpoint_match,
    })
def orient(a, b, c):
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])

def proper_intersection(a, b, c, d):
    o1, o2 = orient(a, b, c), orient(a, b, d)
    o3, o4 = orient(c, d, a), orient(c, d, b)
    return ((o1 > 0 > o2) or (o2 > 0 > o1)) and ((o3 > 0 > o4) or (o4 > 0 > o3))

crossings = []
for i, first in enumerate(routes):
    for second in routes[i + 1:]:
        if {first["a"], first["b"]} & {second["a"], second["b"]}:
            continue
        hit = False
        for a, b in zip(first["ue_spline_points_cm"], first["ue_spline_points_cm"][1:]):
            for c, d in zip(second["ue_spline_points_cm"], second["ue_spline_points_cm"][1:]):
                if proper_intersection(a, b, c, d):
                    hit = True
                    break
            if hit:
                break
        if hit:
            crossings.append([first["route_id"], second["route_id"]])
if crossings:
    errors.append(f"non-node route crossings: {len(crossings)}")
xs = [r["ue_position_cm"][0] for r in regions]
ys = [r["ue_position_cm"][1] for r in regions]
closest = min(pair_rows, key=lambda row: row["distance_cm"])
max_sinuosity = max(route_rows, key=lambda row: row["sinuosity_ratio"])

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "regions": len(regions),
    "routes": len(routes),
    "bounding_box_cm": {
        "min_x": min(xs), "max_x": max(xs),
        "min_y": min(ys), "max_y": max(ys),
        "width": max(xs) - min(xs), "height": max(ys) - min(ys),
    },
    "closest_region_pair": closest,
    "selection_overlap_count": len(selection_overlaps),
    "non_node_route_crossing_count": len(crossings),
    "non_node_route_crossings": crossings,
    "max_sinuosity_route": max_sinuosity,
    "region_pairs": sorted(pair_rows, key=lambda row: row["distance_cm"]),
    "route_geometry": sorted(route_rows, key=lambda row: row["route_id"]),
    "errors": errors,
}
OUT_JSON.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
lines = [
    "# Soul Founder Presentation Geometry Audit",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- Region footprint: {result['bounding_box_cm']['width']} x {result['bounding_box_cm']['height']} cm.",
    f"- Closest regions: {closest['a']} / {closest['b']} at {closest['distance_cm']:.0f} cm.",
    f"- Selection overlaps: {len(selection_overlaps)}.",
    f"- Non-node route crossings: {len(crossings)}.",
    f"- Highest route sinuosity: {max_sinuosity['route_id']} at {max_sinuosity['sinuosity_ratio']:.3f}x direct distance.",
    "",
    "## Route geometry",
    "",
    "| Route | Class | Length cm | Sinuosity | Endpoints |",
    "|---|---|---:|---:|---|",
]
for row in sorted(route_rows, key=lambda row: row["route_id"]):
    lines.append(
        f"| {row['route_id']} | {row['route_class']} | {row['polyline_length_cm']:.0f} | "
        f"{row['sinuosity_ratio']:.3f} | {'match' if row['endpoint_match'] else 'DRIFT'} |"
    )
if errors:
    lines += ["", "## Errors", ""] + [f"- {error}" for error in errors]
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps({k: v for k, v in result.items() if k not in ("region_pairs", "route_geometry")}, indent=2))
if errors:
    raise SystemExit(1)
