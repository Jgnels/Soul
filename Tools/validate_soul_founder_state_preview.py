"""Validate the self-contained founder-state HTML preview against QA state vectors."""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Data"
OUT = ROOT / "Evidence" / "WorldOvermap"
SOURCE = json.loads((DATA / "soul_founder_presentation_state_vectors_v1_20260922.json").read_text(encoding="utf-8"))
WORLD = json.loads((DATA / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
HTML_PATH = OUT / "soul_founder_state_preview.html"
text = HTML_PATH.read_text(encoding="utf-8")
errors = []
warnings = []

match = re.search(r"const DATA = (\{.*?\});\nconst NS=", text, flags=re.S)
if not match:
    errors.append("embedded DATA payload not found")
    payload = {}
else:
    try:
        payload = json.loads(match.group(1))
    except json.JSONDecodeError as exc:
        errors.append(f"embedded DATA is not valid JSON: {exc}")
        payload = {}
founder = set(WORLD["founder_slice"]["region_ids"])
if set(payload.get("nodes", {})) != founder:
    errors.append("embedded node set does not exactly match founder slice")
expected_route_ids = {
    rid for rid, route in WORLD["runtime_routes"].items()
    if route["a"] in founder and route["b"] in founder
}
if set(payload.get("routes", {})) != expected_route_ids:
    errors.append("embedded route set does not exactly match founder internal routes")

source_corridors = {c["corridor_id"]: c for c in SOURCE["corridors"]}
preview_corridors = {c["corridor_id"]: c for c in payload.get("corridors", [])}
if set(preview_corridors) != set(source_corridors):
    errors.append("embedded corridor IDs drifted from source state vectors")

state_count = 0
for corridor_id, source in source_corridors.items():
    preview = preview_corridors.get(corridor_id, {})
    if preview.get("path") != source["path"] or preview.get("visual_signature") != source["visual_signature"]:
        errors.append(f"{corridor_id}: path/signature drift")
    if preview.get("states") != source["states"]:
        errors.append(f"{corridor_id}: state payload drift")
    state_count += len(source["states"])
for corridor in SOURCE["corridors"]:
    for state in corridor["states"]:
        visible = set(state["visible_regions"])
        memory = set(state["explored_not_visible_regions"])
        hidden = set(state["unexplored_regions"])
        if visible & memory or visible & hidden or memory & hidden:
            errors.append(f"{corridor['corridor_id']} step {state['step']}: fog sets overlap")
        if visible | memory | hidden != founder:
            errors.append(f"{corridor['corridor_id']} step {state['step']}: fog sets do not partition founder regions")
        if state["active_region"] not in visible:
            errors.append(f"{corridor['corridor_id']} step {state['step']}: active region not visible")
        if state["planned_target_region"] not in state["selectable_destinations"]:
            errors.append(f"{corridor['corridor_id']} step {state['step']}: target not selectable")
        if state["planned_route_id"] not in state["selectable_route_ids"]:
            errors.append(f"{corridor['corridor_id']} step {state['step']}: route not selectable")

if 'src="http' in text or "src='http" in text or '<link ' in text:
    errors.append("preview has external runtime dependencies")
for token in ('id="corridor"', 'id="step"', 'id="prev"', 'id="next"', 'function render()'):
    if token not in text:
        errors.append(f"missing interactive control/runtime token: {token}")
result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "regions": len(founder),
    "routes": len(expected_route_ids),
    "corridors": len(source_corridors),
    "states": state_count,
    "self_contained": not any('external runtime dependencies' in e for e in errors),
    "warnings": warnings,
    "errors": errors,
}
(OUT / "founder_state_preview_validation.json").write_text(
    json.dumps(result, indent=2) + "\n", encoding="utf-8")
lines = [
    "# Soul Founder State Preview Validation", "",
    f"Status: **{result['status'].upper()}**", "",
    f"- Regions: **{result['regions']}**.",
    f"- Internal routes: **{result['routes']}**.",
    f"- Corridors: **{result['corridors']}**.",
    f"- QA states: **{result['states']}**.",
    f"- Self-contained HTML: **{result['self_contained']}**.",
]
if errors:
    lines += ["", "## Errors", ""] + [f"- {e}" for e in errors]
(OUT / "founder_state_preview_validation.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
