"""Analyze candidate six-faction opening choices on Soul's overmap."""
import json
from collections import defaultdict, deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT/"Data"/"soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
START = json.loads((ROOT/"Data"/"soul_campaign_start_states_v1_20260922.json").read_text(encoding="utf-8"))
OUT_JSON = ROOT/"Evidence"/"WorldOvermap"/"campaign_opening_analysis.json"
OUT_MD = ROOT/"Evidence"/"WorldOvermap"/"campaign_opening_analysis.md"

nodes = {n["id"]:n for n in WORLD["nodes"]}
adj = defaultdict(set)
for e in WORLD["edges"]:
    adj[e["a"]].add(e["b"])
    adj[e["b"]].add(e["a"])

sandbox = START["scenarios"]["six_faction_sandbox_candidate"]
factions = sandbox["factions"]
owners = sandbox["region_owners"]
value_sites = sandbox["value_site_candidates"]

def bfs(start):
    dist={start:0}
    q=deque([start])
    while q:
        cur=q.popleft()
        for nxt in adj[cur]:
            if nxt not in dist:
                dist[nxt]=dist[cur]+1
                q.append(nxt)
    return dist
analysis = {}
capitals = {f:s["capital_region"] for f,s in factions.items()}
for faction,state in factions.items():
    capital = state["capital_region"]
    dist = bfs(capital)
    possessions = set(state["starting_possessions"])
    frontier = sorted({nxt for rid in possessions for nxt in adj[rid] if nxt not in possessions})
    neutral_frontier = [r for r in frontier if r not in owners]
    value_within_2 = sorted(
        rid for rid in value_sites if dist.get(rid,99) <= 2 and rid not in possessions
    )
    enemy_capitals = sorted(
        ((other,dist[other_cap]) for other,other_cap in capitals.items() if other!=faction),
        key=lambda x:(x[1],x[0])
    )
    enemy_owned = sorted(
        ((rid,dist[rid],owners[rid]) for rid in owners if owners[rid]!=faction),
        key=lambda x:(x[1],x[0])
    )
    analysis[faction] = {
        "capital": capital,
        "minor": state["secondary_settlement_region"],
        "neutral_frontier_choices": neutral_frontier,
        "neutral_frontier_count": len(neutral_frontier),
        "value_sites_within_2_moves": value_within_2,
        "nearest_enemy_owned_region": enemy_owned[0] if enemy_owned else None,
        "nearest_enemy_capital": enemy_capitals[0],
        "enemy_capital_distances": {k:v for k,v in enemy_capitals},
    }
errors=[]
for faction,a in analysis.items():
    if a["neutral_frontier_count"] < 2:
        errors.append(f"{faction}: less than two neutral frontier choices")
    if not a["value_sites_within_2_moves"]:
        errors.append(f"{faction}: no thematic value site within two moves")
    if a["nearest_enemy_owned_region"] and a["nearest_enemy_owned_region"][1] < 2:
        errors.append(f"{faction}: enemy-owned region directly adjacent to capital")
    if a["nearest_enemy_capital"][1] < 3:
        errors.append(f"{faction}: enemy capital less than three moves away")

result = {
    "schema": 1,
    "status": "pass" if not errors else "fail",
    "baseline_action_points_per_day": 3,
    "factions": analysis,
    "errors": errors,
    "notes": [
        "Hostile-region traversal is assumed to stop for battle/occupation; the current world graph itself does not enforce that.",
        "Value sites are thematic opening targets only; numeric economy mapping remains balance-owned.",
        "Three-region capital distance can still create early war, but owned minor settlements and neutral buffers prevent direct starting borders.",
    ],
}
OUT_JSON.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
lines=[
    "# Soul Six-Faction Sandbox - Opening Pressure Audit","",
    f"Status: **{result['status'].upper()}**","",
    "| Faction | Neutral frontier choices | Value sites <=2 moves | Nearest enemy-held region | Nearest enemy capital |",
    "|---|---:|---|---|---|",
]
for faction in sorted(analysis):
    a=analysis[faction]
    near=a["nearest_enemy_owned_region"]
    enemy_region = f"{nodes[near[0]]['name']} ({near[2]}, {near[1]} moves)" if near else "-"
    enemy_cap = f"{a['nearest_enemy_capital'][0]} ({a['nearest_enemy_capital'][1]} moves)"
    values=", ".join(nodes[r]["name"] for r in a["value_sites_within_2_moves"])
    lines.append(f"| {faction} | {a['neutral_frontier_count']} | {values} | {enemy_region} | {enemy_cap} |")

lines += ["","## Opening choice detail",""]
for faction in sorted(analysis):
    a=analysis[faction]
    lines.append(f"### {faction.title()}")
    lines.append("- Neutral frontier: " + ", ".join(nodes[r]["name"] for r in a["neutral_frontier_choices"]))
    lines.append("- Nearby value sites: " + ", ".join(nodes[r]["name"] for r in a["value_sites_within_2_moves"]))
    lines.append("")
if errors:
    lines += ["## Errors",""] + ["- "+e for e in errors]
OUT_MD.write_text("\n".join(lines)+"\n",encoding="utf-8")
print(json.dumps(result,indent=2))
if errors:
    raise SystemExit(1)
