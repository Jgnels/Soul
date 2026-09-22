"""Validate the generated Soul overmap integrated import bundle."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUNDLE_PATH = ROOT / "Data" / "soul_overmap_integrated_import_v1_20260922.json"
OUT_JSON = ROOT / "Evidence" / "WorldOvermap" / "integrated_import_validation.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "integrated_import_validation.md"

bundle = json.loads(BUNDLE_PATH.read_text(encoding="utf-8"))
errors = []

for name, source in bundle["source_artifacts"].items():
    path = ROOT / source["path"]
    if not path.exists():
        errors.append(f"missing source artifact: {name} -> {source['path']}")
        continue
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != source["sha256"]:
        errors.append(f"source hash drift: {name}")

regions = bundle["world"]["regions"]
routes = bundle["world"]["routes"]
approaches = bundle["directed_approaches"]
slots = bundle["settlement_slots"]
recipes = {item["id"]: item for item in bundle["battlefield_recipes"]}
region_ids = set(regions)
if len(region_ids) != 36:
    errors.append(f"expected 36 regions, got {len(region_ids)}")
if len(routes) != 51:
    errors.append(f"expected 51 routes, got {len(routes)}")
if len(approaches) != 102:
    errors.append(f"expected 102 directed approaches, got {len(approaches)}")
if len(slots) != 14:
    errors.append(f"expected 14 settlement slots, got {len(slots)}")

route_pairs = set()
for route_id, route in routes.items():
    a, b = route["a"], route["b"]
    if a not in region_ids or b not in region_ids:
        errors.append(f"route endpoint missing: {route_id}")
    pair = tuple(sorted((a, b)))
    if pair in route_pairs:
        errors.append(f"duplicate route endpoints: {pair}")
    route_pairs.add(pair)

for approach in approaches:
    source = approach["source_region"]
    destination = approach["destination_region"]
    if source not in region_ids or destination not in region_ids:
        errors.append(f"approach endpoint missing: {approach['id']}")
        continue
    if tuple(sorted((source, destination))) not in route_pairs:
        errors.append(f"approach lacks route: {approach['id']}")
    recipe_id = approach["battlefield"]["recipe_id"]
    if recipe_id not in recipes:
        errors.append(f"approach recipe missing: {approach['id']} -> {recipe_id}")
    destination_recipe = regions[destination]["battle_recipe_hint"]
    if recipe_id != destination_recipe:
        errors.append(f"approach recipe drift: {approach['id']}")

for slot in slots:
    if slot["region_id"] not in region_ids:
        errors.append(f"settlement slot region missing: {slot['region_id']}")

macro_ids = {item["id"] for item in bundle["world"]["macro_regions"]}
surface_ids = set(bundle["surface_bindings"]["macro_regions"])
if macro_ids != surface_ids:
    errors.append(
        "surface binding macro-region drift: "
        f"missing={sorted(macro_ids-surface_ids)} extra={sorted(surface_ids-macro_ids)}"
    )

founder = bundle["founder_slice_projection"]
founder_ids = set(founder["scenario"]["region_ids"])
if len(founder_ids) != 9:
    errors.append(f"expected 9 founder regions, got {len(founder_ids)}")
if set(founder["regions"]) != founder_ids:
    errors.append("founder region projection does not match scenario region ids")
if len(founder["routes"]) != 10:
    errors.append(f"expected 10 founder routes, got {len(founder['routes'])}")
if len(founder["directed_approaches"]) != 20:
    errors.append(
        f"expected 20 founder directed approaches, got {len(founder['directed_approaches'])}"
    )
for route_id, route in founder["routes"].items():
    if route["a"] not in founder_ids or route["b"] not in founder_ids:
        errors.append(f"founder route escapes slice: {route_id}")
for approach in founder["directed_approaches"]:
    if approach["source_region"] not in founder_ids or approach["destination_region"] not in founder_ids:
        errors.append(f"founder approach escapes slice: {approach['id']}")

scenario = founder["scenario"]
if scenario["player_start_region"] != "human_capital":
    errors.append("founder player start drift")
if scenario["enemy_primary_region"] != "orc_camp":
    errors.append("founder enemy objective drift")
expected_owners = {
    "human_capital": "humans",
    "crossroads": "humans",
    "orc_watch": "orcs",
    "orc_camp": "orcs",
}
if scenario["owners"] != expected_owners:
    errors.append(f"founder ownership drift: {scenario['owners']}")

sandbox = bundle["start_states"]["six_faction_sandbox_candidate"]
if len(sandbox["factions"]) != 6:
    errors.append("six-faction sandbox does not contain six factions")
if len(sandbox["region_owners"]) != 12:
    errors.append(f"expected 12 sandbox-owned regions, got {len(sandbox['region_owners'])}")

presentation_authority = bundle["presentation_contract"]["authority"]
required_rb_authority = {
    "weather": "RB Weather",
    "optimization": "RB Optimization",
    "save": "RB Save",
}
for key, expected in required_rb_authority.items():
    if presentation_authority.get(key) != expected:
        errors.append(f"presentation authority drift: {key} != {expected}")

counts = bundle["counts"]
expected_counts = {
    "regions": 36,
    "routes": 51,
    "directed_approaches": 102,
    "settlement_slots": 14,
    "major_settlements": 6,
    "minor_settlement_candidates": 8,
    "factions": 6,
    "starting_owned_regions": 12,
    "founder_regions": 9,
    "founder_routes": 10,
    "founder_directed_approaches": 20,
}
for key, expected in expected_counts.items():
    if counts.get(key) != expected:
        errors.append(f"bundle count drift: {key}={counts.get(key)} expected={expected}")
result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "bundle": BUNDLE_PATH.name,
    "regions": len(region_ids),
    "routes": len(routes),
    "directed_approaches": len(approaches),
    "settlement_slots": len(slots),
    "founder_regions": len(founder_ids),
    "founder_routes": len(founder["routes"]),
    "founder_directed_approaches": len(founder["directed_approaches"]),
    "source_hashes_verified": len(bundle["source_artifacts"]),
    "rb_authorities_verified": sorted(required_rb_authority),
    "errors": errors,
}
OUT_JSON.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Overmap Integrated Import Validation",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- Regions: **{result['regions']}**",
    f"- Routes: **{result['routes']}**",
    f"- Directed approaches: **{result['directed_approaches']}**",
    f"- Settlement slots: **{result['settlement_slots']}**",
    f"- Founder slice: **{result['founder_regions']} regions / {result['founder_routes']} routes / {result['founder_directed_approaches']} approaches**",
    f"- Source hashes verified: **{result['source_hashes_verified']}**",
    "",
    "## Authority checks",
    "",
    "- Campaign/world state remains SoulCore-owned.",
    "- RB Weather owns dynamic weather presentation/context.",
    "- RB Optimization owns performance policy before bespoke overmap optimization.",
    "- RB Save owns persistence; this bundle is generated import data, not save authority.",
]
if errors:
    lines += ["", "## Errors", ""] + [f"- {error}" for error in errors]
OUT_MD.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")

print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
