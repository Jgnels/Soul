"""Build presentation-only route cues for Soul's strategic overmap."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = json.loads((ROOT / "Data" / "soul_overmap_runtime_import_v1_20260922.json").read_text(encoding="utf-8"))
WORLD = json.loads((ROOT / "Data" / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT / "Data" / "soul_overmap_route_visual_cues_v1_20260922.json"

STYLE = {
    "road": ("road", "packed earth / local stone", "ruts, verge, sparse waymarkers"),
    "trail": ("trail", "narrow earth path", "worn ground, minimal signage"),
    "pass": ("mountain_pass", "rock-cut mountain path", "rock walls, cairn or gate"),
    "coast_road": ("coastal_road", "packed coastal road", "water/cliff exposure, posts"),
    "ridge_trail": ("ridge_trail", "narrow exposed path", "ridge edge and drop-off"),
    "trade_road": ("trade_road", "broad maintained road", "signpost, cart wear, roadside stop"),
    "mountain_trail": ("mountain_trail", "rough stone/earth path", "cairns and grade change"),
    "high_pass": ("high_pass", "snow/rock pass", "snow banks, rock gate, exposed summit"),
}
STYLE.update({
    "old_dwarf_road": ("engineered_old_road", "worn fitted stone", "dwarf retaining walls / broken milestones"),
    "ridge_road": ("ridge_road", "rough maintained road", "ridge edge, stakes, war traffic"),
    "gully_trail": ("gully_trail", "dry badlands track", "gully walls, dust, bone/rock markers"),
    "forest_road": ("forest_road", "packed woodland road", "tree tunnel, cut verge, signpost"),
    "river_trail": ("river_trail", "damp riverside path", "visible water edge, roots, bank markers"),
    "meadow_trail": ("meadow_trail", "grass-worn path", "flower/grass edge, sparse stones"),
    "causeway": ("causeway", "stone causeway", "raised edges, bridge masonry, broken span cues"),
})

founder = set(WORLD["founder_slice"]["region_ids"])
cues = []
for route_id, route in sorted(RUNTIME["routes"].items()):
    route_class = route["route_class"]
    if route_class not in STYLE:
        raise RuntimeError(f"No visual style for route class: {route_class}")
    family, surface, navigation_cue = STYLE[route_class]
    cue = {
        "route_id": route_id,
        "a": route["a"],
        "b": route["b"],
        "route_class": route_class,
        "render_family": family,
        "surface": surface,
        "navigation_cue": navigation_cue,
        "spline_width_cm": route["spline_width_cm"],
        "road": route["road"],
        "chokepoint": route["chokepoint"],
        "chokepoint_cue": navigation_cue if route["chokepoint"] else None,
        "founder_slice": route["a"] in founder and route["b"] in founder,
        "selection_overlay": "brighten / edge-highlight existing surface only",
        "permanent_map_line": False,
        "action_cost": route["action_cost"],
        "logistics_movement_cost": route["logistics_movement_cost"],
    }
    cues.append(cue)

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "PRESENTATION_ONLY_DERIVED_ROUTE_CUES",
    "rules": [
        "Route cues never change adjacency, AP cost, logistics cost or movement legality.",
        "Permanent board-game route lines are forbidden; selected-route overlay is temporary.",
        "Passes, causeways and terrain-specific roads/trails must remain visually distinguishable.",
        "Chokepoint routes require a physical constraint cue in addition to path surface.",
    ],
    "cues": cues,
}
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
print(f"WROTE {OUT} cues={len(cues)} classes={len(STYLE)}")
