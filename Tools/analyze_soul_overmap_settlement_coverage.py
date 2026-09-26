"""Measure how well major + candidate minor settlements cover Soul's overmap graph."""
import heapq
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD = json.loads((ROOT / "Data" / "soul_world_overmap_v1_20260922.json").read_text(encoding="utf-8"))
SLOTS = json.loads((ROOT / "Data" / "soul_overmap_settlement_slots_v1_20260922.json").read_text(encoding="utf-8"))
OUT_JSON = ROOT / "Evidence" / "WorldOvermap" / "settlement_coverage_analysis.json"
OUT_MD = ROOT / "Evidence" / "WorldOvermap" / "settlement_coverage_analysis.md"

nodes = {n["id"]: n for n in WORLD["nodes"]}
settlements = {s["region_id"]: s for s in SLOTS["slots"]}
adj = {rid: [] for rid in nodes}
for edge in WORLD["edges"]:
    cost = edge["logistics_movement_cost"]
    adj[edge["a"]].append((edge["b"], cost))
    adj[edge["b"]].append((edge["a"], cost))

def nearest(start, allowed):
    heap = [(0, 0, start)]
    best = {start: (0, 0)}
    while heap:
        cost, hops, current = heapq.heappop(heap)
        if (cost, hops) != best[current]:
            continue
        if current in allowed:
            return hops, cost, current
        for nxt, edge_cost in adj[current]:
            candidate = (cost + edge_cost, hops + 1)
            if nxt not in best or candidate < best[nxt]:
                best[nxt] = candidate
                heapq.heappush(heap, (candidate[0], candidate[1], nxt))
    raise RuntimeError(f"No settlement reachable from {start}")

def nearest_hops(start, allowed):
    queue = [(start, 0)]
    seen = {start}
    for current, hops in queue:
        if current in allowed:
            return hops, current
        for nxt, _ in adj[current]:
            if nxt not in seen:
                seen.add(nxt)
                queue.append((nxt, hops + 1))
    raise RuntimeError(f"No settlement reachable from {start}")

all_sites = set(settlements)
major_sites = {rid for rid, slot in settlements.items() if slot["tier"] == "major"}
rows = []
for rid in sorted(nodes):
    hops, nearest_by_hops = nearest_hops(rid, all_sites)
    cost_hops, logistics_cost, nearest_by_cost = nearest(rid, all_sites)
    major_hops, nearest_major = nearest_hops(rid, major_sites)
    _, major_logistics_cost, nearest_major_by_cost = nearest(rid, major_sites)
    rows.append({
        "region_id": rid,
        "display_name": nodes[rid]["name"],
        "nearest_settlement_hops": hops,
        "nearest_settlement_by_hops": nearest_by_hops,
        "nearest_settlement_logistics_cost": logistics_cost,
        "nearest_settlement_by_cost": nearest_by_cost,
        "nearest_major_hops": major_hops,
        "nearest_major_by_hops": nearest_major,
        "nearest_major_logistics_cost": major_logistics_cost,
        "nearest_major_by_cost": nearest_major_by_cost,
    })
hop_distribution = {}
for row in rows:
    key = str(row["nearest_settlement_hops"])
    hop_distribution[key] = hop_distribution.get(key, 0) + 1

max_hops = max(r["nearest_settlement_hops"] for r in rows)
max_cost = max(r["nearest_settlement_logistics_cost"] for r in rows)
max_major_hops = max(r["nearest_major_hops"] for r in rows)
coverage_deserts = [r["region_id"] for r in rows if r["nearest_settlement_hops"] > 2]

result = {
    "schema": 1,
    "status": "analyzed",
    "settlement_slots": len(settlements),
    "major_slots": len(major_sites),
    "minor_candidate_slots": len(settlements) - len(major_sites),
    "regions": len(nodes),
    "nearest_settlement_hop_distribution": hop_distribution,
    "max_hops_to_any_settlement": max_hops,
    "max_logistics_cost_to_any_settlement": max_cost,
    "max_hops_to_major_settlement": max_major_hops,
    "regions_over_two_hops_from_any_settlement": coverage_deserts,
    "regions_detail": rows,
}
OUT_JSON.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps({k: v for k, v in result.items() if k != "regions_detail"}, indent=2))
lines = [
    "# Soul Overmap - Settlement Coverage Audit",
    "",
    f"Settlement slots: **{len(settlements)}** ({len(major_sites)} major + {len(settlements)-len(major_sites)} candidate minor).",
    f"Maximum topological distance to any settlement: **{max_hops} hops**.",
    f"Maximum logistics cost to the cheapest settlement: **{max_cost}**.",
    f"Maximum topological distance to a major seat: **{max_major_hops} hops**.",
    "",
    "## Nearest-settlement hop distribution",
    "",
    "| Hops | Regions |",
    "|---:|---:|",
]
for hops in sorted(hop_distribution, key=int):
    lines.append(f"| {hops} | {hop_distribution[hops]} |")
lines += ["", "## Coverage deserts (>2 hops)", ""]
if coverage_deserts:
    for rid in coverage_deserts:
        lines.append(f"- {nodes[rid]['name']} (`{rid}`)")
else:
    lines.append("None. Every strategic region is within two graph hops of a major or candidate minor settlement.")
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"WROTE {OUT_MD}")
