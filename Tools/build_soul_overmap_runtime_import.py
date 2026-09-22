"""Build UE-facing non-UE import data for the Soul overmap."""
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Data" / "soul_world_overmap_v1_20260922.json"
OUT = ROOT / "Data" / "soul_overmap_runtime_import_v1_20260922.json"

WORLD_SCALE_CM = 1000
ORIGIN_MAP = (500, 475)
BASE_Z_CM = 0
ELEVATION_Z = {"low": 0, "mid": 12000, "high": 30000}

def ue_xy(node):
    x = (node["x"] - ORIGIN_MAP[0]) * WORLD_SCALE_CM
    y = (ORIGIN_MAP[1] - node["y"]) * WORLD_SCALE_CM
    z = BASE_Z_CM + ELEVATION_Z.get(node["elevation_band"], 12000)
    return [int(x), int(y), int(z)]

def bend_sign(edge_id):
    h = hashlib.sha256(edge_id.encode("utf-8")).digest()
    return -1 if h[0] & 1 else 1
def route_spline(a_pos, b_pos, edge_id, road):
    ax, ay, az = a_pos
    bx, by, bz = b_pos
    dx, dy = bx - ax, by - ay
    length = math.hypot(dx, dy)
    if length <= 1:
        return [a_pos, b_pos]
    nx, ny = -dy / length, dx / length
    bend = min(65000, max(12000, length * (0.055 if road else 0.09)))
    bend *= bend_sign(edge_id)
    mx = (ax + bx) / 2 + nx * bend
    my = (ay + by) / 2 + ny * bend
    mz = int((az + bz) / 2)
    return [a_pos, [int(mx), int(my), mz], b_pos]

def anchor_type(node):
    if node["kind"] == "capital":
        return "settlement_proxy"
    if node.get("resource"):
        return "resource_site"
    if node["kind"] == "landmark":
        return "landmark"
    if node["feature"] in {"river_crossing","broken_bridge","narrow_pass","road_crossing"}:
        return "strategic_chokepoint"
    return "terrain_region"

d = json.loads(SRC.read_text(encoding="utf-8"))
nodes = {n["id"]: n for n in d["nodes"]}
runtime_regions = {}
for rid, node in nodes.items():
    pos = ue_xy(node)
    runtime_regions[rid] = {
        "id": rid,
        "display_name": node["name"],
        "ue_position_cm": pos,
        "anchor_type": anchor_type(node),
        "selection_radius_cm": 18000 if node["kind"] == "capital" else 12000,
        "macro_region": node["macro_region"],
        "owner": node["owner"],
        "biome": node["biome"],
        "landform": node["landform"],
        "feature": node["feature"],
        "elevation_band": node["elevation_band"],
        "settlement_id": node.get("settlement_id"),
        "resource": node.get("resource"),
        "battle_recipe_hint": node["battle_recipe_hint"],
        "neighbors": d["runtime_regions"][rid]["neighbors"],
        "road_neighbors": d["runtime_regions"][rid]["road_neighbors"],
        "approach_from_neighbor": d["runtime_regions"][rid]["approach_from_neighbor"],
    }

runtime_routes = {}
for edge in d["edges"]:
    edge_id = "route." + "_".join(sorted((edge["a"], edge["b"])))
    a_pos = runtime_regions[edge["a"]]["ue_position_cm"]
    b_pos = runtime_regions[edge["b"]]["ue_position_cm"]
    runtime_routes[edge_id] = {
        "id": edge_id,
        "a": edge["a"],
        "b": edge["b"],
        "route_class": edge["route"],
        "road": edge["road"],
        "chokepoint": edge["chokepoint"],
        "action_cost": edge["action_cost"],
        "logistics_movement_cost": edge["logistics_movement_cost"],
        "ue_spline_points_cm": route_spline(a_pos, b_pos, edge_id, edge["road"]),
        "spline_width_cm": 850 if edge["road"] else 420,
        "presentation": "road" if edge["road"] else "trail",
    }

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "source": SRC.name,
    "world_space": {
        "map_origin": list(ORIGIN_MAP),
        "map_unit_to_cm": WORLD_SCALE_CM,
        "approx_world_extent_km": [9.35, 8.55],
        "y_axis": "map south -> UE negative Y",
        "elevation_z_cm": ELEVATION_Z,
    },
    "founder_slice": d["founder_slice"],
    "terrain_features": d["terrain_features"],
    "regions": runtime_regions,
    "routes": runtime_routes,
}
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
print(f"WROTE {OUT}")
print(f"regions={len(runtime_regions)} routes={len(runtime_routes)}")
