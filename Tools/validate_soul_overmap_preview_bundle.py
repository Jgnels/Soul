"""Validate Soul's full-world/founder overmap preview and UE import tables."""
import csv
import json
import math
import xml.etree.ElementTree as ET
from itertools import combinations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
UE = DATA / "UEImport"
OUT = ROOT / "Evidence" / "WorldOvermap"
WORLD = json.loads((DATA / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
ANCHORS = json.loads((DATA / "soul_overmap_visual_anchors_v1_20260922.json").read_text(encoding="utf-8"))
ENV = json.loads((DATA / "soul_battlefield_environment_expansion_v1_20260922.json").read_text(encoding="utf-8"))
BUNDLE = json.loads((DATA / "soul_overmap_preview_bundle_v1_20260922.json").read_text(encoding="utf-8"))

nodes = {n["id"]: n for n in WORLD["nodes"]}
founder = set(WORLD["founder_slice"]["region_ids"])
errors = []
warnings = []
def read_csv(path):
    with path.open("r", newline="", encoding="utf-8-sig") as handle:
        return list(csv.DictReader(handle))

markers = read_csv(UE / "soul_overmap_world_markers_v1_20260922.csv")
routes = read_csv(UE / "soul_overmap_world_routes_v1_20260922.csv")
sites = read_csv(UE / "soul_overmap_candidate_sites_v1_20260922.csv")

if len(nodes) != 36:
    errors.append(f"expected 36 world regions, got {len(nodes)}")
if len(WORLD["runtime_routes"]) != 51:
    errors.append(f"expected 51 world routes, got {len(WORLD['runtime_routes'])}")
if len(founder) != 9:
    errors.append(f"expected 9 founder regions, got {len(founder)}")
if set(m["region_id"] for m in markers) != set(nodes):
    errors.append("world marker table does not exactly cover world regions")
if set(r["route_id"] for r in routes) != set(WORLD["runtime_routes"]):
    errors.append("world route table does not exactly cover runtime routes")
if set(a["region_id"] for a in ANCHORS["anchors"]) != set(nodes):
    errors.append("visual anchor registry does not exactly cover world regions")
for row in markers:
    node = nodes[row["region_id"]]
    if int(row["x"]) != node["x"] or int(row["y"]) != node["y"]:
        errors.append(f"{row['region_id']}: marker coordinates drifted")
    if row["founder_slice"].lower() == "true" and row["region_id"] not in founder:
        errors.append(f"{row['region_id']}: incorrect founder marker flag")

for row in routes:
    source = WORLD["runtime_routes"][row["route_id"]]
    if row["a"] != source["a"] or row["b"] != source["b"]:
        errors.append(f"{row['route_id']}: route endpoint IDs drifted")
    a, b = nodes[row["a"]], nodes[row["b"]]
    coords = tuple(map(int, (row["a_x"], row["a_y"], row["b_x"], row["b_y"])))
    if coords != (a["x"], a["y"], b["x"], b["y"]):
        errors.append(f"{row['route_id']}: route endpoint coordinates drifted")
    expected_founder = row["a"] in founder and row["b"] in founder
    if (row["founder_slice"].lower() == "true") != expected_founder:
        errors.append(f"{row['route_id']}: founder route flag drifted")
site_ids = {s["site_id"] for s in sites}
expected_sites = {
    a["id"] for a in ENV["active_assets"]
    if a.get("strategic_binding", {}).get("existing_region_candidate")
}
if site_ids != expected_sites:
    errors.append("candidate site table does not exactly cover region-bound admitted assets")
for site in sites:
    if site["region_id"] not in nodes:
        errors.append(f"{site['site_id']}: candidate site targets unknown region")
    if site["topology_change"].lower() != "false":
        errors.append(f"{site['site_id']}: preview pass attempted topology change")

site_by_id = {s["site_id"]: s for s in sites}
if site_by_id.get("arena_city_state", {}).get("region_id") != "southern_crossing":
    errors.append("arena city-state candidate is not staged at Southern Crossing")
if site_by_id.get("lost_shrine", {}).get("region_id") != "ancient_shrine":
    errors.append("Lost Shrine is not staged against Ancient Shrine")
if site_by_id.get("dragon_graveyard", {}).get("local_status") != "PROJECT_INSTALLED":
    errors.append("Dragon Graveyard local qualification status regressed")
svg_checks = [
    (OUT / "soul_overmap_world_preview.svg", set(nodes), set(WORLD["runtime_routes"])),
    (OUT / "soul_overmap_founder_preview.svg", founder,
     {rid for rid, r in WORLD["runtime_routes"].items() if r["a"] in founder and r["b"] in founder}),
]
for svg_path, expected_regions, expected_routes in svg_checks:
    try:
        root = ET.parse(svg_path).getroot()
    except Exception as exc:
        errors.append(f"{svg_path.name}: invalid SVG: {exc}")
        continue
    ids = {el.attrib.get("id") for el in root.iter() if el.attrib.get("id")}
    missing_regions = {f"region-{rid}" for rid in expected_regions} - ids
    missing_routes = expected_routes - ids
    if missing_regions:
        errors.append(f"{svg_path.name}: missing {len(missing_regions)} region markers")
    if missing_routes:
        errors.append(f"{svg_path.name}: missing {len(missing_routes)} route guides")

pair_distances = [
    (math.hypot(a["x"]-b["x"], a["y"]-b["y"]), a["id"], b["id"])
    for a, b in combinations(WORLD["nodes"], 2)
]
closest = min(pair_distances)
if closest[0] < 50:
    errors.append(f"world marker spacing below planning floor: {closest}")
elif closest[0] < 75:
    warnings.append(f"tight world marker pair: {closest[1]} / {closest[2]} at {closest[0]:.1f} map units")
founder_route_count = sum(
    1 for r in WORLD["runtime_routes"].values()
    if r["a"] in founder and r["b"] in founder
)
if founder_route_count != 10:
    errors.append(f"expected 10 founder internal routes, got {founder_route_count}")
if BUNDLE.get("status") != "PRESENTATION_PREVIEW_NONCANONICAL":
    errors.append("preview bundle lost noncanonical presentation-only status")
if len(BUNDLE.get("regions", [])) != 36 or len(BUNDLE.get("routes", [])) != 51:
    errors.append("preview bundle counts drifted from world graph")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "regions": len(markers),
    "routes": len(routes),
    "founder_regions": len(founder),
    "founder_internal_routes": founder_route_count,
    "candidate_sites": len(sites),
    "closest_world_marker_pair": {
        "a": closest[1], "b": closest[2], "distance_map_units": round(closest[0], 3)
    },
    "warnings": warnings,
    "errors": errors,
}
(OUT / "overmap_preview_validation.json").write_text(
    json.dumps(result, indent=2) + "\n", encoding="utf-8")
lines = [
    "# Soul Overmap Preview Validation",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- World markers: **{result['regions']}**.",
    f"- World routes: **{result['routes']}**.",
    f"- Founder slice: **{result['founder_regions']} regions / {result['founder_internal_routes']} internal routes**.",
    f"- Region-bound candidate sites: **{result['candidate_sites']}**.",
    f"- Closest world markers: **{closest[1]} / {closest[2]}** at **{closest[0]:.1f}** map units.",
    "",
    "SVGs are planning previews only. Visible route strokes do not authorize permanent board lines in the shipping overmap.",
]
if warnings:
    lines += ["", "## Warnings", ""] + [f"- {w}" for w in warnings]
if errors:
    lines += ["", "## Errors", ""] + [f"- {e}" for e in errors]
(OUT / "overmap_preview_validation.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
