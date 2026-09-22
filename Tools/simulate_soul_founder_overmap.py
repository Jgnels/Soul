"""Headless traversal/fog check for Soul's 9-region founder overmap."""
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SEED = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
OUT_JSON = ROOT / "Evidence" / "WorldOvermap" / "founder_slice_simulation.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "founder_slice_simulation.md"

d = json.loads(SEED.read_text(encoding="utf-8"))
regions = d["regions"]
routes = d["routes"]
start = d["start_region"]
goal = d["enemy_region"]
adj = defaultdict(list)
edge_by_pair = {}

for e in routes.values():
    a, b = e["a"], e["b"]
    adj[a].append(b)
    adj[b].append(a)
    edge_by_pair[frozenset((a, b))] = e
def all_simple_paths(start_id, goal_id, max_moves=7):
    out = []
    def walk(cur, path):
        if len(path) - 1 > max_moves:
            return
        if cur == goal_id:
            out.append(path[:])
            return
        for nxt in sorted(adj[cur]):
            if nxt not in path and nxt != goal:
                walk(nxt, path + [nxt])
    walk(start_id, [start_id])
    return out

def path_stats(path):
    logistics = roads = chokepoints = 0
    resources, encounters = set(), set()
    for a, b in zip(path, path[1:]):
        e = edge_by_pair[frozenset((a, b))]
        logistics += e["logistics_movement_cost"]
        roads += int(e["road"])
        chokepoints += int(e["chokepoint"])
    for rid in path:
        roles = set(regions[rid].get("site_roles", []))
        if "resource_site" in roles:
            resources.add(rid)
        if "encounter" in roles:
            encounters.add(rid)
    moves = len(path) - 1
    total_actions = moves + 1  # current runtime spends one AP to commit adjacent battle
    return {
        "path": path,
        "final_approach": path[-1],
        "moves": moves,
        "battle_commit_ap": 1,
        "total_actions_to_battle": total_actions,
        "logistics_cost": logistics,
        "road_edges": roads,
        "chokepoints": chokepoints,
        "resource_sites": sorted(resources),
        "encounter_regions": sorted(encounters),
        "days_at_3_ap": (total_actions + 2) // 3,
    }

approaches = sorted(adj[goal])
paths = []
for approach in approaches:
    for pth in all_simple_paths(start, approach, 7):
        paths.append(path_stats(pth))
paths.sort(key=lambda x:(x["total_actions_to_battle"], x["logistics_cost"], x["path"]))
min_actions = min(x["total_actions_to_battle"] for x in paths)
short = [x for x in paths if x["total_actions_to_battle"] == min_actions]
def reveal_sequence(path):
    explored = set(d["initial_knowledge"]["humans"]["explored_regions"])
    sequence = []
    for step, rid in enumerate(path):
        before = set(explored)
        explored.add(rid)
        explored.update(adj[rid])
        sequence.append({
            "step": step,
            "region": rid,
            "newly_explored": sorted(explored - before),
            "explored_total": len(explored),
        })
    return sequence

detours = {
    "quarry_resource_loop": [
        "human_capital","crossroads","old_quarry","crossroads","river_ford","orc_watch"
    ],
    "forest_resource_flank": [
        "human_capital","crossroads","forest_edge","north_pass"
    ],
    "shrine_exploration_loop": [
        "human_capital","crossroads","old_quarry","ancient_shrine",
        "old_quarry","crossroads","forest_edge","north_pass"
    ],
}
detour_stats = {k:path_stats(v) for k,v in detours.items()}
errors = []
if len(short) < 3:
    errors.append(f"expected >=3 shortest approaches, found {len(short)}")
if "old_quarry" not in detour_stats["quarry_resource_loop"]["resource_sites"]:
    errors.append("Old Quarry resource detour is broken")
if "forest_edge" not in detour_stats["forest_resource_flank"]["resource_sites"]:
    errors.append("Forest Edge resource flank is broken")
for x in short:
    seq = reveal_sequence(x["path"])
    if goal not in set(seq[-1]["newly_explored"]):
        errors.append(f"stronghold not revealed from final approach {x['final_approach']}")

result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "start": start,
    "goal": goal,
    "final_approach_regions": approaches,
    "shortest_moves_to_approach": short[0]["moves"],
    "minimum_actions_including_battle": min_actions,
    "shortest_approach_count": len(short),
    "shortest_approaches": short,
    "resource_detours": detour_stats,
    "representative_reveal_sequence": reveal_sequence(short[0]["path"]),
    "errors": errors,
}
result["interpretation"] = [
    "Three equal-action founder approaches prevent one mandatory attack lane.",
    "Road approach minimizes logistics cost; forest/pass routes trade efficiency for terrain.",
    "Old Quarry and Forest Edge are optional strategic resource decisions, not gates.",
    "Ancient Shrine is deliberately a deeper hero/magic progression detour.",
    "Stronghold becomes explored from either final approach before battle commitment.",
]
OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
OUT_JSON.write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")

lines = [
    "# Soul Founder Overmap — Headless Traversal Check","",
    f"Status: **{result['status'].upper()}**","",
    f"- Shortest approach distance: {result['shortest_moves_to_approach']} region moves.",
    f"- Battle commitment: +1 AP from an adjacent approach.",
    f"- Equal-action shortest approaches: {result['shortest_approach_count']}.","",
    "## Shortest approaches","",
]
for x in short:
    names = " -> ".join(regions[r]["name"] for r in x["path"])
    lines.append(
        f"- {names} -> BATTLE — {x['total_actions_to_battle']} actions, "
        f"logistics {x['logistics_cost']}, roads {x['road_edges']}, chokepoints {x['chokepoints']}."
    )
lines += ["","## Deliberate detours",""]
for key, x in detour_stats.items():
    names = " -> ".join(regions[r]["name"] for r in x["path"])
    lines.append(
        f"- **{key}**: {names} -> BATTLE — {x['total_actions_to_battle']} actions, "
        f"logistics {x['logistics_cost']}, ~{x['days_at_3_ap']} days at 3 AP/day."
    )
lines += ["","## Fog/exploration",""]
for step in result["representative_reveal_sequence"]:
    names = ", ".join(regions[r]["name"] for r in step["newly_explored"]) or "nothing new"
    lines.append(
        f"- Step {step['step']} at {regions[step['region']]['name']}: reveals {names}; "
        f"explored total {step['explored_total']}."
    )
lines += ["","## Interpretation",""]
for item in result["interpretation"]:
    lines.append("- " + item)
if errors:
    lines += ["","## Errors",""] + ["- " + e for e in errors]
OUT_MD.write_text("\n".join(lines)+"\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
