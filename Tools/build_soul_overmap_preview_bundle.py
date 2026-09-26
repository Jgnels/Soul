"""Build deterministic full-world/founder overmap planning previews and UE-friendly tables."""
import csv
import html
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
OUT = ROOT / "Evidence" / "WorldOvermap"
UE = DATA / "UEImport"
WORLD = json.loads((DATA / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
ANCHORS = json.loads((DATA / "soul_overmap_visual_anchors_v1_20260922.json").read_text(encoding="utf-8"))
CUES = json.loads((DATA / "soul_overmap_route_visual_cues_v1_20260922.json").read_text(encoding="utf-8"))
ENV = json.loads((DATA / "soul_battlefield_environment_expansion_v1_20260922.json").read_text(encoding="utf-8"))

OUT.mkdir(parents=True, exist_ok=True)
UE.mkdir(parents=True, exist_ok=True)
nodes = {n["id"]: n for n in WORLD["nodes"]}
anchors = {a["region_id"]: a for a in ANCHORS["anchors"]}
cues = {c["route_id"]: c for c in CUES["cues"]}
founder = set(WORLD["founder_slice"]["region_ids"])
sites_by_region = defaultdict(list)
site_rows = []
for asset in ENV["active_assets"]:
    binding = asset.get("strategic_binding", {})
    region_id = binding.get("existing_region_candidate")
    if not region_id:
        continue
    row = {
        "site_id": asset["id"],
        "title": asset["title"],
        "region_id": region_id,
        "integration_class": asset["integration_class"],
        "local_status": asset["local_status"],
        "candidate_status": binding.get("status", "CANDIDATE"),
        "topology_change": bool(binding.get("topology_change", False)),
        "fab_listing_id": asset.get("fab_listing_id") or "",
    }
    site_rows.append(row)
    sites_by_region[region_id].append(row)

WORLD_W = 1000.0
WORLD_H = 900.0
MARKER_FIELDS = [
    "region_id", "display_name", "macro_region", "owner", "x", "y", "u", "v",
    "biome", "landform", "feature", "elevation_band", "settlement_id", "resource",
    "battle_recipe", "anchor_type", "anchor_label", "scale_class", "interactive",
    "interaction_scope", "founder_slice", "candidate_site_ids",
]
marker_rows = []
for region_id, node in sorted(nodes.items()):
    anchor = anchors[region_id]
    marker_rows.append({
        "region_id": region_id,
        "display_name": node["name"],
        "macro_region": node["macro_region"],
        "owner": node.get("owner") or "",
        "x": node["x"], "y": node["y"],
        "u": round(node["x"] / WORLD_W, 6),
        "v": round(node["y"] / WORLD_H, 6),
        "biome": node["biome"],
        "landform": node["landform"],
        "feature": node["feature"],
        "elevation_band": node["elevation_band"],
        "settlement_id": node.get("settlement_id") or "",
        "resource": node.get("resource") or "",
        "battle_recipe": node["battle_recipe_hint"],
        "anchor_type": anchor["anchor_type"],
        "anchor_label": anchor["anchor_label"],
        "scale_class": anchor["scale_class"],
        "interactive": anchor["interactive"],
        "interaction_scope": anchor["interaction_scope"],
        "founder_slice": region_id in founder,
        "candidate_site_ids": ";".join(s["site_id"] for s in sites_by_region[region_id]),
    })
ROUTE_FIELDS = [
    "route_id", "a", "b", "a_x", "a_y", "b_x", "b_y", "route_class",
    "render_family", "surface", "navigation_cue", "spline_width_cm", "road",
    "chokepoint", "chokepoint_cue", "action_cost", "logistics_movement_cost",
    "founder_slice",
]
route_rows = []
for route_id, route in sorted(WORLD["runtime_routes"].items()):
    cue = cues[route_id]
    a = nodes[route["a"]]
    b = nodes[route["b"]]
    route_rows.append({
        "route_id": route_id, "a": route["a"], "b": route["b"],
        "a_x": a["x"], "a_y": a["y"], "b_x": b["x"], "b_y": b["y"],
        "route_class": cue["route_class"], "render_family": cue["render_family"],
        "surface": cue["surface"], "navigation_cue": cue["navigation_cue"],
        "spline_width_cm": cue["spline_width_cm"], "road": cue["road"],
        "chokepoint": cue["chokepoint"],
        "chokepoint_cue": cue.get("chokepoint_cue") or "",
        "action_cost": cue["action_cost"],
        "logistics_movement_cost": cue["logistics_movement_cost"],
        "founder_slice": route["a"] in founder and route["b"] in founder,
    })

SITE_FIELDS = ["site_id", "title", "region_id", "integration_class", "local_status",
               "candidate_status", "topology_change", "fab_listing_id"]
def write_csv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

marker_csv = UE / "soul_overmap_world_markers_v1_20260922.csv"
route_csv = UE / "soul_overmap_world_routes_v1_20260922.csv"
site_csv = UE / "soul_overmap_candidate_sites_v1_20260922.csv"
write_csv(marker_csv, MARKER_FIELDS, marker_rows)
write_csv(route_csv, ROUTE_FIELDS, route_rows)
write_csv(site_csv, SITE_FIELDS, site_rows)

bundle = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "PRESENTATION_PREVIEW_NONCANONICAL",
    "world_canvas": {"width": WORLD_W, "height": WORLD_H},
    "regions": marker_rows,
    "routes": route_rows,
    "candidate_sites": site_rows,
    "terrain_features": WORLD["terrain_features"],
    "macro_regions": WORLD["macro_regions"],
    "rules": [
        "Preview route strokes are planning guides only; shipping map uses physical roads/trails.",
        "Candidate sites do not alter topology, ownership or canonical campaign state.",
        "Final ownership/battle legality remains SoulCore-owned; RB Weather and RB Optimization retain their existing authority.",
    ],
}
(DATA / "soul_overmap_preview_bundle_v1_20260922.json").write_text(
    json.dumps(bundle, indent=2) + "\n", encoding="utf-8")
MACRO_FILL = {
    "northern_fjords": "#dce8ee", "crownspine": "#d8d5cf", "heartland": "#dfe8c8",
    "eastern_badlands": "#e6cfb5", "greenwood": "#c9dfc1", "ashen_south": "#d2c7c3",
}
OWNER_STROKE = {
    "humans": "#315b8a", "vikings": "#4e6f83", "dwarves": "#73553b",
    "orcs": "#6d7131", "dark": "#5e405f", "nature": "#3e7250", "": "#555555",
}
ROUTE_STYLE = {
    "road": ("#8c6f48", "7", ""), "coast_road": ("#8c6f48", "7", ""),
    "forest_road": ("#7a6749", "7", ""), "trail": ("#776e60", "4", "9 7"),
    "mountain_trail": ("#6f6a63", "4", "7 6"), "pass": ("#60584d", "5", "5 4"),
    "causeway": ("#77634c", "6", "4 3"),
}


def esc(value):
    return html.escape(str(value), quote=True)


def points_text(points, ox=0, oy=0):
    return " ".join(f"{x+ox},{y+oy}" for x, y in points)


def candidate_code(site_id):
    return "".join(part[0].upper() for part in site_id.split("_")[:3])[:3]
def render_svg(path, included_regions, title, viewbox, ox=0, oy=0):
    inc = set(included_regions)
    route_subset = [r for r in route_rows if r["a"] in inc and r["b"] in inc]
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{viewbox}" role="img">',
        f'<title>{esc(title)}</title>',
        '<style>text{font-family:Segoe UI,Arial,sans-serif}.label{font-size:16px;font-weight:600;paint-order:stroke;stroke:#fff;stroke-width:4px}.small{font-size:11px}.candidate{font-size:9px;font-weight:700}</style>',
        '<rect x="0" y="0" width="100%" height="100%" fill="#f3f0e8"/>',
    ]
    for macro in WORLD["macro_regions"]:
        fill = MACRO_FILL.get(macro["id"], "#dddddd")
        pts = points_text(macro["polygon"], ox, oy)
        lines.append(f'<polygon points="{pts}" fill="{fill}" fill-opacity="0.72" stroke="#ffffff" stroke-width="3"/>')
    for forest in WORLD["terrain_features"].get("forest_belts", []):
        pts = points_text(forest["polygon"], ox, oy)
        lines.append(f'<polygon points="{pts}" fill="#5f8a55" fill-opacity="0.20" stroke="#4f7348" stroke-width="2" stroke-dasharray="6 5"/>')
    for river in WORLD["terrain_features"].get("rivers", []):
        pts = points_text(river["points"], ox, oy)
        lines.append(f'<polyline points="{pts}" fill="none" stroke="#4d88a8" stroke-width="8" stroke-linecap="round" stroke-linejoin="round" opacity="0.72"/>')
    for mountains in WORLD["terrain_features"].get("mountain_belts", []):
        pts = points_text(mountains["points"], ox, oy)
        lines.append(f'<polyline points="{pts}" fill="none" stroke="#726e69" stroke-width="16" stroke-linecap="round" stroke-linejoin="round" opacity="0.22"/>')
        lines.append(f'<polyline points="{pts}" fill="none" stroke="#66615d" stroke-width="3" stroke-dasharray="5 8" opacity="0.65"/>')
    for route in route_subset:
        stroke, width, dash = ROUTE_STYLE.get(route["route_class"], ("#786d5d", "4", "6 5"))
        dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
        a = nodes[route["a"]]; b = nodes[route["b"]]
        lines.append(
            f'<line id="{esc(route["route_id"])}" x1="{a["x"]+ox}" y1="{a["y"]+oy}" x2="{b["x"]+ox}" y2="{b["y"]+oy}" '
            f'stroke="{stroke}" stroke-width="{width}" stroke-linecap="round" opacity="0.72"{dash_attr}/>'
        )
        if route["chokepoint"]:
            mx=(a["x"]+b["x"])/2+ox; my=(a["y"]+b["y"])/2+oy
            lines.append(f'<circle cx="{mx}" cy="{my}" r="7" fill="#f3f0e8" stroke="#4a4139" stroke-width="3"/>')
    for region_id in sorted(inc, key=lambda rid: (nodes[rid]["y"], nodes[rid]["x"])):
        node = nodes[region_id]; anchor = anchors[region_id]
        x=node["x"]+ox; y=node["y"]+oy
        owner = node.get("owner") or ""
        stroke = OWNER_STROKE.get(owner, OWNER_STROKE[""])
        size = {"major": 14, "medium": 11, "small": 8}.get(anchor["scale_class"], 8)
        if anchor["anchor_type"] == "major_settlement_silhouette":
            lines.append(f'<rect id="region-{esc(region_id)}" x="{x-size}" y="{y-size}" width="{size*2}" height="{size*2}" rx="3" fill="#fffdf7" stroke="{stroke}" stroke-width="4"/>')
        elif anchor["anchor_type"] == "chokepoint":
            pts=f'{x},{y-size-3} {x+size+3},{y} {x},{y+size+3} {x-size-3},{y}'
            lines.append(f'<polygon id="region-{esc(region_id)}" points="{pts}" fill="#fffdf7" stroke="{stroke}" stroke-width="4"/>')
        else:
            lines.append(f'<circle id="region-{esc(region_id)}" cx="{x}" cy="{y}" r="{size}" fill="#fffdf7" stroke="{stroke}" stroke-width="4"/>')
        if region_id in founder:
            lines.append(f'<circle cx="{x}" cy="{y}" r="{size+7}" fill="none" stroke="#171717" stroke-width="2" opacity="0.75"/>')
        dx = -14 if node["x"] > 760 else 14
        anchor_text = "end" if dx < 0 else "start"
        lines.append(f'<text x="{x+dx}" y="{y-8}" text-anchor="{anchor_text}" class="label">{esc(node["name"])}</text>')
        lines.append(f'<text x="{x+dx}" y="{y+8}" text-anchor="{anchor_text}" class="small">{esc(anchor["anchor_label"])}</text>')
        for index, site in enumerate(sites_by_region.get(region_id, [])):
            sy = y + 23 + index * 13
            sx = x + (18 if node["x"] <= 760 else -18)
            text_anchor = "start" if node["x"] <= 760 else "end"
            code = candidate_code(site["site_id"])
            lines.append(f'<circle cx="{x}" cy="{y}" r="{size+12+index*4}" fill="none" stroke="#8e5f26" stroke-width="1.5" stroke-dasharray="4 3" opacity="0.9"/>')
            lines.append(f'<text x="{sx}" y="{sy}" text-anchor="{text_anchor}" class="candidate" fill="#704817">+{esc(code)} candidate</text>')
    lines.append(f'<text x="{viewbox.split()[0]}" y="{viewbox.split()[1]}" opacity="0">{esc(title)}</text>')
    lines.append('</svg>')
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")

all_regions = list(nodes)
render_svg(
    OUT / "soul_overmap_world_preview.svg", all_regions,
    "Soul strategic overmap planning preview - 36 regions",
    "0 0 1080 1020", ox=40, oy=60,
)
founder_nodes = [nodes[rid] for rid in WORLD["founder_slice"]["region_ids"]]
min_x=min(n["x"] for n in founder_nodes)-85; max_x=max(n["x"] for n in founder_nodes)+125
min_y=min(n["y"] for n in founder_nodes)-85; max_y=max(n["y"] for n in founder_nodes)+90
render_svg(
    OUT / "soul_overmap_founder_preview.svg", founder,
    "Soul founder slice planning preview - 9 regions",
    f"{min_x} {min_y} {max_x-min_x} {max_y-min_y}",
)
manifest = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "BUILT",
    "regions": len(marker_rows),
    "routes": len(route_rows),
    "founder_regions": len(founder),
    "candidate_sites": len(site_rows),
    "outputs": [
        str(marker_csv.relative_to(ROOT)), str(route_csv.relative_to(ROOT)),
        str(site_csv.relative_to(ROOT)), "Data/soul_overmap_preview_bundle_v1_20260922.json",
        "Evidence/WorldOvermap/soul_overmap_world_preview.svg",
        "Evidence/WorldOvermap/soul_overmap_founder_preview.svg",
    ],
}
(OUT / "overmap_preview_build_manifest.json").write_text(
    json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print(json.dumps(manifest, indent=2))
