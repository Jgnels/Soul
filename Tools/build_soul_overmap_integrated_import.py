"""Build one deterministic non-UE import bundle for Soul's strategic overmap."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Data" / "soul_overmap_integrated_import_v1_20260922.json"

SOURCES = {
    "world": "Data/soul_world_overmap_v1_20260922.json",
    "runtime": "Data/soul_overmap_runtime_import_v1_20260922.json",
    "route_visual_cues": "Data/soul_overmap_route_visual_cues_v1_20260922.json",
    "approaches": "Data/soul_overmap_approach_profiles_v1_20260922.json",
    "settlements": "Data/soul_overmap_settlement_slots_v1_20260922.json",
    "presentation": "Data/soul_overmap_presentation_contract_v1_20260922.json",
    "surfaces": "Data/soul_overmap_surface_bindings_v1_20260922.json",
    "start_states": "Data/soul_campaign_start_states_v1_20260922.json",
    "visual_anchors": "Data/soul_overmap_visual_anchors_v1_20260922.json",
    "battlefields": "Data/battlefield_recipes.json",
}

def read_json(relative_path):
    return json.loads((ROOT / relative_path).read_text(encoding="utf-8"))

def sha256(relative_path):
    return hashlib.sha256((ROOT / relative_path).read_bytes()).hexdigest()
loaded = {name: read_json(path) for name, path in SOURCES.items()}
world = loaded["world"]
runtime = loaded["runtime"]
route_visual_cues = loaded["route_visual_cues"]
approaches = loaded["approaches"]
settlements = loaded["settlements"]
presentation = loaded["presentation"]
surfaces = loaded["surfaces"]
start_states = loaded["start_states"]
visual_anchors = loaded["visual_anchors"]
battlefields = loaded["battlefields"]

node_by_id = {node["id"]: node for node in world["nodes"]}
slot_by_region = {slot["region_id"]: slot for slot in settlements["slots"]}
anchor_by_region = {item["region_id"]: item for item in visual_anchors["anchors"]}
route_cue_by_id = {item["route_id"]: item for item in route_visual_cues["cues"]}
recipe_by_id = {recipe["id"]: recipe for recipe in battlefields["recipes"]}
founder_ids = list(world["founder_slice"]["region_ids"])
founder_set = set(founder_ids)
founder_scenario = start_states["scenarios"]["founder_human_orc_micro"]
sandbox = start_states["scenarios"]["six_faction_sandbox_candidate"]

founder_routes = {
    route_id: route for route_id, route in runtime["routes"].items()
    if route["a"] in founder_set and route["b"] in founder_set
}
founder_route_visual_cues = {
    route_id: route_cue_by_id[route_id] for route_id in founder_routes
}
founder_approaches = [
    item for item in approaches["approaches"]
    if item["source_region"] in founder_set and item["destination_region"] in founder_set
]
founder_regions = {}
for region_id in founder_ids:
    node = node_by_id[region_id]
    recipe_id = node["battle_recipe_hint"]
    founder_regions[region_id] = {
        "runtime": runtime["regions"][region_id],
        "start_owner": founder_scenario["owners"].get(region_id),
        "settlement_slot": slot_by_region.get(region_id),
        "visual_anchor": anchor_by_region[region_id],
        "battlefield_recipe": recipe_by_id[recipe_id],
    }

source_artifacts = {
    name: {
        "path": path,
        "sha256": sha256(path),
    }
    for name, path in sorted(SOURCES.items())
}

bundle = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "INTEGRATED_NON_UE_HANDOFF",
    "source_artifacts": source_artifacts,
    "authority": {
        "world_graph": SOURCES["world"],
        "runtime_positions_routes": SOURCES["runtime"],
        "route_presentation": SOURCES["route_visual_cues"],
        "start_state_overlay": SOURCES["start_states"],
        "visual_anchor_plan": SOURCES["visual_anchors"],
        "directed_approaches": SOURCES["approaches"],
        "settlement_density": SOURCES["settlements"],
        "battlefield_selection": SOURCES["battlefields"],
        "presentation": SOURCES["presentation"],
        "surface_binding_plan": SOURCES["surfaces"],
        "dynamic_weather_time": "runtime-injected; not authored into geography",
    },
    "import_order": [
        "world_space_and_terrain",
        "regions_and_routes",
        "settlement_slots",
        "start_state_overlay",
        "directed_approaches",
        "battlefield_recipe_bindings",
        "presentation_contract",
        "route_visual_cues",
        "visual_anchors",
        "surface_bindings",
    ],
    "world": {
        "world_space": runtime["world_space"],
        "terrain_features": runtime["terrain_features"],
        "macro_regions": world["macro_regions"],
        "regions": runtime["regions"],
        "routes": runtime["routes"],
    },
    "settlement_slots": settlements["slots"],
    "start_states": start_states["scenarios"],
    "directed_approaches": approaches["approaches"],
    "route_visual_cues": route_visual_cues["cues"],
    "visual_anchors": visual_anchors["anchors"],
    "battlefield_recipes": battlefields["recipes"],
    "presentation_contract": presentation,
    "surface_bindings": surfaces,
    "founder_slice_projection": {
        "scenario": founder_scenario,
        "regions": founder_regions,
        "routes": founder_routes,
        "route_visual_cues": founder_route_visual_cues,
        "directed_approaches": founder_approaches,
    },
    "counts": {
        "regions": len(runtime["regions"]),
        "routes": len(runtime["routes"]),
        "directed_approaches": len(approaches["approaches"]),
        "settlement_slots": len(settlements["slots"]),
        "major_settlements": settlements["counts"]["major"],
        "minor_settlement_candidates": settlements["counts"]["minor_candidates"],
        "factions": len(sandbox["factions"]),
        "starting_owned_regions": len(sandbox["region_owners"]),
        "founder_regions": len(founder_ids),
        "founder_routes": len(founder_routes),
        "route_visual_cues": len(route_visual_cues["cues"]),
        "founder_route_visual_cues": len(founder_route_visual_cues),
        "founder_directed_approaches": len(founder_approaches),
        "visual_anchors": len(visual_anchors["anchors"]),
        "founder_visual_anchors": sum(1 for item in visual_anchors["anchors"] if item["founder_slice"]),
        "battlefield_recipes": len(battlefields["recipes"]),
    },
}

OUT.write_text(json.dumps(bundle, indent=2) + "\n", encoding="utf-8")
print(f"WROTE {OUT}")
print(json.dumps(bundle["counts"], indent=2))
