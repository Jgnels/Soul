"""Audit structural homeland/frontier pressure without treating it as locked start ownership."""
import json
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT/"Data"/"soul_world_overmap_v1_20260922.json"
OUT_JSON = ROOT/"Evidence"/"WorldOvermap"/"frontier_analysis.json"
OUT_MD = ROOT/"Evidence"/"WorldOvermap"/"frontier_analysis.md"

d = json.loads(SRC.read_text(encoding="utf-8"))
nodes = {n["id"]: n for n in d["nodes"]}
adj = defaultdict(set)
for e in d["edges"]:
    adj[e["a"]].add(e["b"])
    adj[e["b"]].add(e["a"])

factions = ["humans","vikings","dwarves","orcs","nature","dark"]
counts = Counter((n["owner"] or "neutral") for n in nodes.values())
report = {}
for faction in factions:
    owned = [rid for rid,n in nodes.items() if n["owner"] == faction]
    neutral_neighbors, foreign_neighbors, frontier_edges = set(), set(), []
    for rid in owned:
        for other in adj[rid]:
            owner = nodes[other]["owner"] or "neutral"
            if owner == faction:
                continue
            frontier_edges.append([rid, other, owner])
            if owner == "neutral":
                neutral_neighbors.add(other)
            else:
                foreign_neighbors.add((other, owner))
    report[faction] = {
        "homeland_affinity_regions": len(owned),
        "frontier_edges": len(frontier_edges),
        "neutral_frontier_regions": sorted(neutral_neighbors),
        "direct_foreign_frontiers": [
            {"region": rid, "faction": owner} for rid,owner in sorted(foreign_neighbors)
        ],
    }

founder = set(d["founder_slice"]["region_ids"])
founder_locked = {
    rid: nodes[rid]["owner"] for rid in sorted(founder)
    if nodes[rid]["owner"] is not None
}
result = {
    "schema": 1,
    "status": "pass",
    "ownership_semantics": {
        "founder_slice": "initial scenario ownership where explicitly populated",
        "full_world": "homeland affinity / art-direction candidate only",
    },
    "homeland_affinity_counts": dict(sorted(counts.items())),
    "factions": report,
    "founder_locked_owner_regions": founder_locked,
    "notes": [
        "Do not balance full-campaign starts from current owner counts.",
        "Humans having fewer affinity-colored regions is not yet a balance defect.",
        "A separate campaign-start-state pass should assign possessions, neutral sites and army spawns.",
    ],
}
OUT_JSON.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
lines = [
    "# Soul Overmap - Homeland / Frontier Audit","",
    "Current full-world faction coloring is **homeland affinity**, not locked campaign-start ownership.","",
    "| Faction | Affinity regions | Frontier edges | Neutral frontier regions |",
    "|---|---:|---:|---:|",
]
for faction in factions:
    r = report[faction]
    lines.append(
        f"| {faction} | {r['homeland_affinity_regions']} | {r['frontier_edges']} | "
        f"{len(r['neutral_frontier_regions'])} |"
    )
lines += ["","## Founder slice ownership actually locked by current scenario",""]
for rid,owner in sorted(founder_locked.items()):
    lines.append(f"- {nodes[rid]['name']}: {owner}")
lines += ["","## Guardrail","",
    "Do not infer a six-faction starting balance from the structural map's color count.",
    "The next campaign-start pass should explicitly choose starting possessions, neutral expansion targets, army spawns and diplomacy.",
]
OUT_MD.write_text("\n".join(lines)+"\n",encoding="utf-8")
print(json.dumps(result,indent=2))
