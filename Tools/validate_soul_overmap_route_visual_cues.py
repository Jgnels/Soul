"""Validate presentation-only route cues against Soul's canonical runtime route import."""
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = json.loads((ROOT / "Data" / "soul_overmap_runtime_import_v1_20260922.json").read_text(encoding="utf-8"))
CUES = json.loads((ROOT / "Data" / "soul_overmap_route_visual_cues_v1_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT / "Evidence" / "WorldOvermap" / "route_visual_cue_validation.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "route_visual_cue_validation.md"

routes = RUNTIME["routes"]
cues = {item["route_id"]: item for item in CUES["cues"]}
errors = []
if len(CUES["cues"]) != len(cues):
    errors.append("duplicate route cue ids")
if set(cues) != set(routes):
    errors.append(f"route cue coverage drift: missing={sorted(set(routes)-set(cues))} extra={sorted(set(cues)-set(routes))}")
for route_id, route in routes.items():
    cue = cues.get(route_id, {})
    for key in ("route_class", "road", "chokepoint", "action_cost", "logistics_movement_cost", "spline_width_cm"):
        if cue.get(key) != route.get(key):
            errors.append(f"route cue semantic drift: {route_id} field={key}")
    if cue.get("permanent_map_line") is not False:
        errors.append(f"route cue must not become permanent board line: {route_id}")
    if route["chokepoint"] and not cue.get("chokepoint_cue"):
        errors.append(f"chokepoint lacks physical cue: {route_id}")
    if not cue.get("surface") or not cue.get("navigation_cue"):
        errors.append(f"route cue lacks readable surface/navigation language: {route_id}")

founder_cues = [item for item in CUES["cues"] if item["founder_slice"]]
if len(founder_cues) != 10:
    errors.append(f"expected 10 founder route cues, got {len(founder_cues)}")
class_counts = Counter(route["route_class"] for route in routes.values())
family_counts = Counter(cue["render_family"] for cue in CUES["cues"])
if len(class_counts) != 15:
    errors.append(f"expected 15 canonical route classes, got {len(class_counts)}")
if len(family_counts) != len(class_counts):
    errors.append(
        f"route classes collapsed in presentation: classes={len(class_counts)} families={len(family_counts)}"
    )

chokepoints = [item for item in CUES["cues"] if item["chokepoint"]]
if len(chokepoints) != 11:
    errors.append(f"expected 11 chokepoint route cues, got {len(chokepoints)}")

pair_to_cue = {
    tuple(sorted((item["a"], item["b"]))): item
    for item in CUES["cues"]
}
founder_paths = {
    "road_ford_watch": ["human_capital", "crossroads", "river_ford", "orc_watch", "orc_camp"],
    "forest_pass": ["human_capital", "crossroads", "forest_edge", "north_pass", "orc_camp"],
    "forest_watch": ["human_capital", "crossroads", "forest_edge", "orc_watch", "orc_camp"],
}

def path_signature(path):
    signature = []
    for a, b in zip(path, path[1:]):
        cue = pair_to_cue.get(tuple(sorted((a, b))))
        if cue is None:
            errors.append(f"founder path lacks route cue: {a}<->{b}")
            signature.append("MISSING")
        else:
            signature.append(cue["render_family"])
    return signature

founder_signatures = {
    name: path_signature(path) for name, path in founder_paths.items()
}
if len({tuple(value) for value in founder_signatures.values()}) != 3:
    errors.append(f"founder approach visual signatures are not distinct: {founder_signatures}")
result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "route_cues": len(CUES["cues"]),
    "route_classes": len(class_counts),
    "render_families": len(family_counts),
    "founder_route_cues": len(founder_cues),
    "chokepoint_route_cues": len(chokepoints),
    "road_routes": sum(1 for route in routes.values() if route["road"]),
    "nonroad_routes": sum(1 for route in routes.values() if not route["road"]),
    "route_class_counts": dict(sorted(class_counts.items())),
    "render_family_counts": dict(sorted(family_counts.items())),
    "founder_approach_signatures": founder_signatures,
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Overmap Route Visual Cue Validation", "",
    f"Status: **{result['status'].upper()}**", "",
    f"- Route cues: **{result['route_cues']}**",
    f"- Semantic route classes: **{result['route_classes']}**",
    f"- Render families: **{result['render_families']}**",
    f"- Founder route cues: **{result['founder_route_cues']}**",
    f"- Chokepoint cues: **{result['chokepoint_route_cues']}**", "",
]
lines += [
    "## Founder approach signatures", "",
    "| Approach | Visual sequence |",
    "|---|---|",
]
for name, signature in founder_signatures.items():
    lines.append(f"| {name} | {' -> '.join(signature)} |")
lines += ["", "## Route classes", "", "| Route class | Render family | Count |", "|---|---|---:|"]
for route_class in sorted(class_counts):
    sample = next(cue for cue in CUES["cues"] if cue["route_class"] == route_class)
    lines.append(f"| {route_class} | {sample['render_family']} | {class_counts[route_class]} |")
if errors:
    lines += ["", "## Errors", ""] + [f"- {error}" for error in errors]
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
