"""Build deterministic presentation QA states for Soul's founder overmap corridors."""
import hashlib
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESENTATION = ROOT / "Data" / "soul_founder_slice_presentation_import_v1_20260922.json"
SIMULATION = ROOT / "Evidence" / "WorldOvermap" / "founder_slice_simulation.json"
OUT = ROOT / "Data" / "soul_founder_presentation_state_vectors_v1_20260922.json"

p = json.loads(PRESENTATION.read_text(encoding="utf-8"))
sim = json.loads(SIMULATION.read_text(encoding="utf-8"))
regions = {r["region_id"]: r for r in p["regions"]}
routes = {r["route_id"]: r for r in p["routes"]}
region_ids = set(regions)
adj = defaultdict(list)
route_by_pair = {}
for route in routes.values():
    a, b = route["a"], route["b"]
    adj[a].append(b)
    adj[b].append(a)
    route_by_pair[frozenset((a, b))] = route["route_id"]

approach_by_pair = {
    (a["source_region"], a["destination_region"]): a
    for a in p["directed_approaches"]
}
def visible_from(region_id):
    return set([region_id]) | set(adj[region_id])

def route_memory(explored):
    return sorted(
        route_id for route_id, route in routes.items()
        if route["a"] in explored and route["b"] in explored
    )

def state_for(region_id, explored, step, target_region, route_id, final_commit):
    visible = visible_from(region_id)
    explored = set(explored) | visible
    explored_not_visible = explored - visible
    unexplored = region_ids - explored
    selectable = sorted(
        route_by_pair[frozenset((region_id, nxt))]
        for nxt in adj[region_id]
    )
    state = {
        "step": step,
        "active_region": region_id,
        "camera_focus_region": region_id,
        "camera_focus_cm": regions[region_id]["ue_position_cm"],
        "cumulative_travel_actions": step,
        "visible_regions": sorted(visible),
        "explored_regions": sorted(explored),
        "explored_not_visible_regions": sorted(explored_not_visible),
        "unexplored_regions": sorted(unexplored),
        "live_dynamic_regions": sorted(visible),
        "memory_only_regions": sorted(explored_not_visible),
        "hidden_anchor_regions": sorted(unexplored),
        "known_route_ids": route_memory(explored),
        "selectable_route_ids": selectable,
        "selectable_destinations": sorted(adj[region_id]),
        "planned_target_region": target_region,
        "planned_route_id": route_id,
        "planned_action": "battle_commit" if final_commit else "move",
        "planned_target_visible_before_action": target_region in visible,
    }
    if final_commit:
        approach = approach_by_pair[(region_id, target_region)]
        state["battle_commit"] = {
            "source_region": region_id,
            "destination_region": target_region,
            "route_id": route_id,
            "directed_approach_id": approach["id"],
            "entry_direction": approach["entry_direction"],
            "route_type": approach["route"]["type"],
            "road": approach["route"]["road"],
            "chokepoint": approach["route"]["chokepoint"],
            "battlefield_recipe_id": approach["battlefield"]["recipe_id"],
            "battlefield_recipe_status": approach["battlefield"]["recipe_status"],
            "dynamic_context_required": approach["battlefield"]["dynamic_context_required"],
        }
    return state, explored

corridor_vectors = []
for corridor in p["shortest_attack_corridors"]:
    path = corridor["path"]
    explored = set(p["scenario"]["initial_knowledge"]["humans"]["explored_regions"])
    states = []
    for step, current in enumerate(path[:-1]):
        target = path[step + 1]
        route_id = route_by_pair[frozenset((current, target))]
        final_commit = step == len(path) - 2
        state, explored = state_for(
            current, explored, step, target, route_id, final_commit
        )
        states.append(state)
    corridor_vectors.append({
        "corridor_id": corridor["id"],
        "path": path,
        "visual_signature": corridor["visual_signature"],
        "total_actions_to_battle": corridor["total_actions_to_battle"],
        "states": states,
    })

detour_vectors = []
goal = p["scenario"]["enemy_primary_region"]
for detour_id, detour in sorted(sim["resource_detours"].items()):
    path = list(detour["path"]) + [goal]
    explored = set(p["scenario"]["initial_knowledge"]["humans"]["explored_regions"])
    states = []
    for step, current in enumerate(path[:-1]):
        target = path[step + 1]
        route_id = route_by_pair[frozenset((current, target))]
        final_commit = step == len(path) - 2
        state, explored = state_for(current, explored, step, target, route_id, final_commit)
        states.append(state)
    detour_vectors.append({
        "detour_id": detour_id,
        "path": path,
        "total_actions_to_battle": detour["total_actions_to_battle"],
        "states": states,
    })

next_choices = defaultdict(set)
for corridor in p["shortest_attack_corridors"]:
    for a, b in zip(corridor["path"], corridor["path"][1:]):
        next_choices[a].add(b)
decision_points = []
for region_id, choices in sorted(next_choices.items()):
    if len(choices) > 1:
        decision_points.append({
            "region_id": region_id,
            "shortest_corridor_next_choices": sorted(choices),
            "choice_count": len(choices),
            "visible_from_region": sorted(visible_from(region_id)),
        })

fixture_active_regions = sorted({
    state["active_region"]
    for group in corridor_vectors + detour_vectors
    for state in group["states"]
})

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "PRESENTATION_QA_FIXTURE_NONAUTHORITATIVE",
    "source": PRESENTATION.name,
    "source_sha256": hashlib.sha256(PRESENTATION.read_bytes()).hexdigest(),
    "authority": {
        "campaign_state": "SoulCore remains authoritative",
        "fixture_role": "expected presentation states for UE QA only",
        "fog_basis": "current founder visibility pattern: active region plus adjacent regions",
        "weather": "RB Weather injects runtime state; not frozen here",
        "save": "RB Save owns persistence",
    },
    "presentation_rules": [
        "Visible regions may show current armies, interactables, and settlement condition.",
        "Explored-not-visible regions retain terrain/anchor memory but hide current dynamic state.",
        "Unexplored regions remain obscured.",
        "Physical routes become remembered when both endpoints have been explored.",
        "Selectable movement comes only from routes incident to the active region.",
        "The final adjacent action is a battle commitment, not movement into the enemy stronghold.",
    ],
    "corridors": corridor_vectors,
    "detours": detour_vectors,
    "decision_points": decision_points,
    "fixture_active_regions": fixture_active_regions,
    "acceptance_targets": {
        "corridors": len(corridor_vectors),
        "states_per_corridor": sorted({len(c["states"]) for c in corridor_vectors}),
        "detours": len(detour_vectors),
        "detour_state_counts": sorted(len(d["states"]) for d in detour_vectors),
        "decision_points": len(decision_points),
        "founder_regions": len(region_ids),
        "active_fixture_regions": len(fixture_active_regions),
        "founder_routes": len(routes),
    },
}
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
print(f"WROTE {OUT}")
print(json.dumps(payload["acceptance_targets"], indent=2))
