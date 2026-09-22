"""Validate directed Soul overmap approach profiles against canonical source data."""
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT / "Data" / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
RUNTIME = json.loads((ROOT / "Data" / "soul_overmap_runtime_import_v1_20260922.json").read_text(encoding="utf-8"))
BATTLES = json.loads((ROOT / "Data" / "battlefield_recipes.json").read_text(encoding="utf-8"))
PROFILES = json.loads((ROOT / "Data" / "soul_overmap_approach_profiles_v1_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT / "Evidence" / "WorldOvermap" / "approach_profile_validation.json"

nodes = {n["id"]: n for n in WORLD["nodes"]}
recipes = {r["id"]: r for r in BATTLES["recipes"]}
founder = set(WORLD["founder_slice"]["region_ids"])
profiles = PROFILES["approaches"]
errors = []
seen_ids = set()
seen_pairs = Counter()

edge_lookup = {}
for edge in WORLD["edges"]:
    key = frozenset((edge["a"], edge["b"]))
    edge_lookup[key] = edge

required_tag_prefixes = ("route.", "entry.", "biome.", "landform.", "feature.", "elevation.")
for profile in profiles:
    pid = profile["id"]
    source = profile["source_region"]
    destination = profile["destination_region"]
    if pid in seen_ids:
        errors.append(f"duplicate profile id: {pid}")
    seen_ids.add(pid)
    if source not in nodes or destination not in nodes:
        errors.append(f"unknown region in {pid}")
        continue
    key = frozenset((source, destination))
    if key not in edge_lookup:
        errors.append(f"profile does not correspond to world edge: {pid}")
        continue
    seen_pairs[key] += 1
    edge = edge_lookup[key]
    route = profile["route"]
    for field in ("type", "action_cost", "logistics_movement_cost"):
        source_key = "route" if field == "type" else field
        if route[field] != edge[source_key]:
            errors.append(f"route {field} drift: {pid}")
    if route["road"] != bool(edge.get("road")):
        errors.append(f"road flag drift: {pid}")
    if route["chokepoint"] != bool(edge.get("chokepoint")):
        errors.append(f"chokepoint flag drift: {pid}")
    expected_direction = RUNTIME["regions"][destination]["approach_from_neighbor"].get(source)
    if profile["entry_direction"] != expected_direction:
        errors.append(f"entry direction drift: {pid}")
    node = nodes[destination]
    context = profile["destination_context"]
    for field in ("biome", "landform", "feature", "elevation_band"):
        if context[field] != node[field]:
            errors.append(f"destination {field} drift: {pid}")
    recipe_id = profile["battlefield"]["recipe_id"]
    if recipe_id not in recipes:
        errors.append(f"unknown battlefield recipe: {pid} -> {recipe_id}")
    elif recipe_id != node["battle_recipe_hint"]:
        errors.append(f"battlefield recipe hint drift: {pid}")
    elif profile["battlefield"]["recipe_status"] != recipes[recipe_id]["status"]:
        errors.append(f"battlefield recipe status drift: {pid}")
    if profile["battlefield"]["dynamic_context_required"] != ["weather", "time"]:
        errors.append(f"dynamic context drift: {pid}")
    tags = profile["approach_tags"]
    for prefix in required_tag_prefixes:
        if not any(tag.startswith(prefix) for tag in tags):
            errors.append(f"missing {prefix} tag: {pid}")
    expected_founder = source in founder and destination in founder
    if profile["founder_internal"] != expected_founder:
        errors.append(f"founder flag drift: {pid}")
expected_count = len(WORLD["edges"]) * 2
if len(profiles) != expected_count:
    errors.append(f"expected {expected_count} directed profiles, got {len(profiles)}")
for key in edge_lookup:
    if seen_pairs[key] != 2:
        names = sorted(key)
        errors.append(f"edge lacks exactly two directed profiles: {names}")

founder_count = sum(1 for p in profiles if p["founder_internal"])
expected_founder_count = 2 * sum(
    1 for e in WORLD["edges"] if e["a"] in founder and e["b"] in founder
)
if founder_count != expected_founder_count:
    errors.append(f"founder directed count {founder_count} != {expected_founder_count}")

required_context = BATTLES["rules"].get("strategic_context_required", [])
if "approach" not in required_context or "weather" not in required_context or "time" not in required_context:
    errors.append("battlefield recipe strategic-context contract lost approach/weather/time")

status_counts = Counter(p["battlefield"]["recipe_status"] for p in profiles)
result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "world_edges": len(WORLD["edges"]),
    "directed_approaches": len(profiles),
    "founder_internal_directed_approaches": founder_count,
    "recipe_status_counts": dict(sorted(status_counts.items())),
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
