"""Build deterministic directed campaign-to-battle approach profiles for Soul."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT / "Data" / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
RUNTIME = json.loads((ROOT / "Data" / "soul_overmap_runtime_import_v1_20260922.json").read_text(encoding="utf-8"))
BATTLES = json.loads((ROOT / "Data" / "battlefield_recipes.json").read_text(encoding="utf-8"))
OUT = ROOT / "Data" / "soul_overmap_approach_profiles_v1_20260922.json"
FOUNDER_MD = ROOT / "Evidence" / "WorldOvermap" / "founder_approach_matrix.md"

nodes = {n["id"]: n for n in WORLD["nodes"]}
recipes = {r["id"]: r for r in BATTLES["recipes"]}
founder = set(WORLD["founder_slice"]["region_ids"])

CHOKE_FEATURES = {
    "river_crossing", "broken_bridge", "narrow_pass", "road_crossing",
}

def transition_kind(node):
    if node.get("settlement_id"):
        return "settlement_approach"
    if node.get("resource"):
        return "resource_site"
    if node.get("kind") == "landmark":
        return "landmark"
    if node.get("feature") in CHOKE_FEATURES:
        return "strategic_chokepoint"
    return "field_encounter"

def build_tags(edge, node, entry_direction):
    tags = [
        f"route.{edge['route']}",
        f"entry.{entry_direction}",
        f"biome.{node['biome']}",
        f"landform.{node['landform']}",
        f"feature.{node['feature']}",
        f"elevation.{node['elevation_band']}",
    ]
    if edge.get("road"):
        tags.append("route.road_network")
    if edge.get("chokepoint"):
        tags.append("route.chokepoint")
    if node.get("settlement_id"):
        tags.append("destination.settlement")
    if node.get("resource"):
        tags.append(f"destination.resource.{node['resource']}")
    return sorted(set(tags))

approaches = []
for edge in sorted(WORLD["edges"], key=lambda e: tuple(sorted((e["a"], e["b"])))):
    a, b = edge["a"], edge["b"]
    for source, destination in ((a, b), (b, a)):
        node = nodes[destination]
        runtime = RUNTIME["regions"][destination]
        direction = runtime["approach_from_neighbor"][source]
        recipe = recipes[node["battle_recipe_hint"]]
        approaches.append({
            "id": f"approach.{source}.{destination}",
            "source_region": source,
            "destination_region": destination,
            "founder_internal": source in founder and destination in founder,
            "route": {
                "type": edge["route"],
                "road": bool(edge.get("road")),
                "chokepoint": bool(edge.get("chokepoint")),
                "action_cost": edge["action_cost"],
                "logistics_movement_cost": edge["logistics_movement_cost"],
            },
            "entry_direction": direction,
            "destination_context": {
                "biome": node["biome"],
                "landform": node["landform"],
                "feature": node["feature"],
                "elevation_band": node["elevation_band"],
                "transition_kind": transition_kind(node),
                "settlement_id": node.get("settlement_id"),
                "resource": node.get("resource"),
            },
            "battlefield": {
                "recipe_id": node["battle_recipe_hint"],
                "recipe_status": recipe["status"],
                "dynamic_context_required": ["weather", "time"],
            },
            "approach_tags": build_tags(edge, node, direction),
        })

approaches.sort(key=lambda x: x["id"])
founder_approaches = [a for a in approaches if a["founder_internal"]]
recipe_ids = sorted({a["battlefield"]["recipe_id"] for a in approaches})
payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "authority": {
        "geography": "Data/soul_world_overmap_v1_20260922.json",
        "entry_direction": "Data/soul_overmap_runtime_import_v1_20260922.json",
        "battle_recipe_hint": "destination region battle_recipe_hint",
        "dynamic_weather_time": "runtime-owned; intentionally not frozen here",
    },
    "rules": [
        "Every strategic edge exports two directed approach profiles.",
        "Campaign movement legality and AP/logistics costs remain owned by the world graph.",
        "Approach profiles carry context into battle presentation; they do not resolve combat.",
        "Weather and time are injected dynamically at battle launch.",
        "Battlefield recipe IDs must already exist in Data/battlefield_recipes.json.",
    ],
    "counts": {
        "world_edges": len(WORLD["edges"]),
        "directed_approaches": len(approaches),
        "founder_internal_directed_approaches": len(founder_approaches),
        "referenced_battlefield_recipes": len(recipe_ids),
    },
    "approaches": approaches,
}
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
print(f"WROTE {OUT} directed={len(approaches)} founder={len(founder_approaches)}")
lines = [
    "# Soul Founder Slice - Directed Approach Matrix",
    "",
    "Generated from the current world graph. Weather and time stay dynamic.",
    "",
    "| From | To | Route | Entry | Choke | Battle recipe | Transition |",
    "|---|---|---|---|---|---|---|",
]
for a in founder_approaches:
    src = nodes[a["source_region"]]["name"]
    dst = nodes[a["destination_region"]]["name"]
    route = a["route"]
    ctx = a["destination_context"]
    lines.append(
        f"| {src} | {dst} | {route['type']} | {a['entry_direction']} | "
        f"{'yes' if route['chokepoint'] else 'no'} | {a['battlefield']['recipe_id']} | "
        f"{ctx['transition_kind']} |"
    )
FOUNDER_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"WROTE {FOUNDER_MD}")
