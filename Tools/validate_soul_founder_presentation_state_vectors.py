"""Validate Soul founder presentation QA state vectors."""
import hashlib
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Data" / "soul_founder_slice_presentation_import_v1_20260922.json"
VECTORS = ROOT / "Data" / "soul_founder_presentation_state_vectors_v1_20260922.json"
OUT_JSON = ROOT / "Evidence" / "WorldOvermap" / "founder_presentation_state_vector_validation.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "founder_presentation_state_vector_validation.md"

p = json.loads(SOURCE.read_text(encoding="utf-8"))
v = json.loads(VECTORS.read_text(encoding="utf-8"))
regions = {r["region_id"] for r in p["regions"]}
routes = {r["route_id"]: r for r in p["routes"]}
adj = defaultdict(set)
route_by_pair = {}
for route in routes.values():
    a, b = route["a"], route["b"]
    adj[a].add(b)
    adj[b].add(a)
    route_by_pair[frozenset((a, b))] = route["route_id"]
approaches = {
    (a["source_region"], a["destination_region"]): a
    for a in p["directed_approaches"]
}
errors = []
expected_hash = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
if v.get("source_sha256") != expected_hash:
    errors.append("source presentation import hash drift")
if len(v.get("corridors", [])) != 3:
    errors.append("expected exactly three shortest corridor fixtures")
if len(v.get("detours", [])) != 3:
    errors.append("expected exactly three deliberate detour fixtures")
if set(v.get("authority", {})) < {"campaign_state", "fixture_role", "weather", "save"}:
    errors.append("fixture authority declaration incomplete")

commit_ids = set()
for corridor in v.get("corridors", []):
    states = corridor.get("states", [])
    path = corridor.get("path", [])
    if len(states) != 4 or len(path) != 5:
        errors.append(f"{corridor.get('corridor_id')}: expected four states and five path regions")
        continue
    explored_previous = set()
    for index, state in enumerate(states):
        active = state["active_region"]
        visible = set(state["visible_regions"])
        explored = set(state["explored_regions"])
        memory = set(state["explored_not_visible_regions"])
        unexplored = set(state["unexplored_regions"])
        expected_visible = {active} | adj[active]
        if visible != expected_visible:
            errors.append(f"{corridor['corridor_id']} step {index}: visibility drift")
        if visible | memory | unexplored != regions:
            errors.append(f"{corridor['corridor_id']} step {index}: fog partition incomplete")
        if (visible & memory) or (visible & unexplored) or (memory & unexplored):
            errors.append(f"{corridor['corridor_id']} step {index}: fog partitions overlap")
        if explored != visible | memory:
            errors.append(f"{corridor['corridor_id']} step {index}: explored partition drift")
        if not explored_previous.issubset(explored):
            errors.append(f"{corridor['corridor_id']} step {index}: explored knowledge regressed")
        explored_previous = explored
        expected_routes = {
            route_by_pair[frozenset((active, nxt))] for nxt in adj[active]
        }
        if set(state["selectable_route_ids"]) != expected_routes:
            errors.append(f"{corridor['corridor_id']} step {index}: selectable routes drift")
        if set(state["selectable_destinations"]) != adj[active]:
            errors.append(f"{corridor['corridor_id']} step {index}: selectable destinations drift")
        for route_id in state["known_route_ids"]:
            route = routes[route_id]
            if route["a"] not in explored or route["b"] not in explored:
                errors.append(f"{corridor['corridor_id']} step {index}: unknown route remembered")
        target = state["planned_target_region"]
        planned_route = state["planned_route_id"]
        if route_by_pair.get(frozenset((active, target))) != planned_route:
            errors.append(f"{corridor['corridor_id']} step {index}: planned route mismatch")
        if not state["planned_target_visible_before_action"]:
            errors.append(f"{corridor['corridor_id']} step {index}: target not visible before action")
        if state["cumulative_travel_actions"] != index:
            errors.append(f"{corridor['corridor_id']} step {index}: action counter drift")
        if index < 3 and state["planned_action"] != "move":
            errors.append(f"{corridor['corridor_id']} step {index}: early battle commit")
    final = states[-1]
    if final["planned_action"] != "battle_commit" or "battle_commit" not in final:
        errors.append(f"{corridor['corridor_id']}: final state must commit battle")
        continue
    commit = final["battle_commit"]
    pair = (commit["source_region"], commit["destination_region"])
    approach = approaches.get(pair)
    if approach is None:
        errors.append(f"{corridor['corridor_id']}: missing directed final approach")
        continue
    if commit["directed_approach_id"] != approach["id"]:
        errors.append(f"{corridor['corridor_id']}: final approach id drift")
    if commit["battlefield_recipe_id"] != approach["battlefield"]["recipe_id"]:
        errors.append(f"{corridor['corridor_id']}: battlefield recipe drift")
    if "orc_camp" not in set(final["visible_regions"]):
        errors.append(f"{corridor['corridor_id']}: stronghold not visible before battle")
    commit_ids.add(commit["directed_approach_id"])

if len(commit_ids) != 2:
    errors.append(f"expected two distinct final stronghold approaches, found {len(commit_ids)}")

for detour in v.get("detours", []):
    states = detour.get("states", [])
    path = detour.get("path", [])
    detour_id = detour.get("detour_id", "unknown_detour")
    if not states or len(states) != len(path) - 1:
        errors.append(f"{detour_id}: state/path length mismatch")
        continue
    explored_previous = set()
    for index, state in enumerate(states):
        active = state["active_region"]
        visible = set(state["visible_regions"])
        explored = set(state["explored_regions"])
        memory = set(state["explored_not_visible_regions"])
        unexplored = set(state["unexplored_regions"])
        if visible != ({active} | adj[active]):
            errors.append(f"{detour_id} step {index}: visibility drift")
        if visible | memory | unexplored != regions:
            errors.append(f"{detour_id} step {index}: fog partition incomplete")
        if not explored_previous.issubset(explored):
            errors.append(f"{detour_id} step {index}: explored knowledge regressed")
        explored_previous = explored
        target = state["planned_target_region"]
        if route_by_pair.get(frozenset((active, target))) != state["planned_route_id"]:
            errors.append(f"{detour_id} step {index}: planned route mismatch")
        if not state["planned_target_visible_before_action"]:
            errors.append(f"{detour_id} step {index}: planned target not visible")
    if states[-1]["planned_action"] != "battle_commit":
        errors.append(f"{detour_id}: final state must commit battle")

fixture_active = set(v.get("fixture_active_regions", []))
goal = p["scenario"]["enemy_primary_region"]
if fixture_active | {goal} != regions:
    errors.append("fixture does not exercise every founder region as active or final battle destination")
if "old_quarry" not in fixture_active or "ancient_shrine" not in fixture_active:
    errors.append("resource/shrine detours are not exercised as active presentation states")

decision = {
    row["region_id"]: set(row["shortest_corridor_next_choices"])
    for row in v.get("decision_points", [])
}
expected_decisions = {
    "crossroads": {"forest_edge", "river_ford"},
    "forest_edge": {"north_pass", "orc_watch"},
}
if decision != expected_decisions:
    errors.append(f"decision-point drift: {decision}")
result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "corridors": len(v.get("corridors", [])),
    "corridor_states": sum(len(c["states"]) for c in v.get("corridors", [])),
    "detours": len(v.get("detours", [])),
    "detour_states": sum(len(d["states"]) for d in v.get("detours", [])),
    "active_fixture_regions": len(fixture_active),
    "decision_points": len(v.get("decision_points", [])),
    "distinct_final_approaches": len(commit_ids),
    "source_sha256": expected_hash,
    "errors": errors,
}
OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
OUT_JSON.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder Presentation State-Vector Validation",
    "",
    f"Status: **{result['status'].upper()}**",
    "",
    f"- Corridors: **{result['corridors']}** with **{result['corridor_states']}** states.",
    f"- Deliberate detours: **{result['detours']}** with **{result['detour_states']}** states.",
    f"- Founder regions exercised as active states: **{result['active_fixture_regions']}/8** (stronghold is the battle destination).",
    f"- Branch decision points: **{result['decision_points']}**.",
    f"- Distinct final stronghold approaches: **{result['distinct_final_approaches']}**.",
    "",
    "The fixture verifies fog partitions, persistent exploration memory, selectable adjacency,",
    "route-memory exposure, and the final directed battlefield handoff without changing SoulCore authority.",
]
if errors:
    lines += ["", "## Errors", ""] + [f"- {err}" for err in errors]
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
