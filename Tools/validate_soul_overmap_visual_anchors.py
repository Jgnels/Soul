"""Validate Soul strategic-map visual-anchor coverage and founder-slice readability promises."""
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT / "Data" / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
ANCHORS = json.loads((ROOT / "Data" / "soul_overmap_visual_anchors_v1_20260922.json").read_text(encoding="utf-8"))
SLOTS = json.loads((ROOT / "Data" / "soul_overmap_settlement_slots_v1_20260922.json").read_text(encoding="utf-8"))
OUT = ROOT / "Evidence" / "WorldOvermap" / "visual_anchor_validation.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "visual_anchor_validation.md"

nodes = {node["id"]: node for node in WORLD["nodes"]}
anchors = {item["region_id"]: item for item in ANCHORS["anchors"]}
slots = {item["region_id"]: item for item in SLOTS["slots"]}
founder = set(WORLD["founder_slice"]["region_ids"])
errors = []
if len(ANCHORS["anchors"]) != len(anchors):
    errors.append("duplicate visual-anchor region ids")
if set(anchors) != set(nodes):
    errors.append(
        f"visual-anchor coverage drift: missing={sorted(set(nodes)-set(anchors))} "
        f"extra={sorted(set(anchors)-set(nodes))}"
    )

for region_id, anchor in anchors.items():
    if not anchor.get("source"):
        errors.append(f"anchor lacks source: {region_id}")
    if not anchor.get("anchor_label"):
        errors.append(f"anchor lacks readable label: {region_id}")
    if anchor.get("battle_recipe") != nodes[region_id]["battle_recipe_hint"]:
        errors.append(f"battlefield landmark binding drift: {region_id}")
    if bool(anchor.get("founder_slice")) != (region_id in founder):
        errors.append(f"founder-slice marker drift: {region_id}")

major = [s for s in slots.values() if s["tier"] == "major"]
minor = [s for s in slots.values() if s["tier"] == "minor"]
for slot in major:
    anchor = anchors.get(slot["region_id"], {})
    if anchor.get("anchor_type") != "major_settlement_silhouette":
        errors.append(f"major settlement lost silhouette anchor: {slot['region_id']}")
    if anchor.get("scale_class") != "major":
        errors.append(f"major settlement scale drift: {slot['region_id']}")
    if not anchor.get("persistent_state_projection"):
        errors.append(f"major settlement must project persistent condition: {slot['region_id']}")

for slot in minor:
    anchor = anchors.get(slot["region_id"], {})
    if anchor.get("anchor_type") != "minor_settlement_proxy":
        errors.append(f"minor settlement lost proxy anchor: {slot['region_id']}")
    if anchor.get("scale_class") != "medium":
        errors.append(f"minor settlement scale drift: {slot['region_id']}")

founder_expectations = {
    "human_capital": "major_settlement_silhouette",
    "orc_camp": "major_settlement_silhouette",
    "old_quarry": "resource_infrastructure",
    "forest_edge": "resource_landscape",
    "river_ford": "crossing",
    "north_pass": "chokepoint",
}
for region_id, expected_type in founder_expectations.items():
    actual = anchors.get(region_id, {}).get("anchor_type")
    if actual != expected_type:
        errors.append(f"founder readability drift: {region_id}={actual}, expected {expected_type}")

interactive_resource_regions = {
    node["id"] for node in nodes.values() if node.get("resource")
}
for region_id in interactive_resource_regions:
    if not anchors[region_id].get("interactive"):
        errors.append(f"resource site must remain interactable: {region_id}")

type_counts = Counter(item["anchor_type"] for item in ANCHORS["anchors"])
result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "anchors": len(ANCHORS["anchors"]),
    "major_settlement_silhouettes": type_counts["major_settlement_silhouette"],
    "minor_settlement_proxies": type_counts["minor_settlement_proxy"],
    "founder_anchors": sum(1 for item in ANCHORS["anchors"] if item["founder_slice"]),
    "interactive_anchors": sum(1 for item in ANCHORS["anchors"] if item["interactive"]),
    "anchor_types": dict(sorted(type_counts.items())),
    "errors": errors,
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
lines = [
    "# Soul Overmap Visual Anchor Validation", "",
    f"Status: **{result['status'].upper()}**", "",
    f"- Anchors: **{result['anchors']}**",
    f"- Major settlement silhouettes: **{result['major_settlement_silhouettes']}**",
    f"- Minor settlement proxies: **{result['minor_settlement_proxies']}**",
    f"- Founder-slice anchors: **{result['founder_anchors']}**", "",
    "## Founder slice", "",
    "| Region | Anchor type | Label | Interactive |",
    "|---|---|---|---|",
]
for region_id in WORLD["founder_slice"]["region_ids"]:
    anchor = anchors[region_id]
    lines.append(
        f"| {nodes[region_id]['name']} | {anchor['anchor_type']} | "
        f"{anchor['anchor_label']} | {anchor['interactive']} |"
    )
if errors:
    lines += ["", "## Errors", ""] + [f"- {error}" for error in errors]
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
