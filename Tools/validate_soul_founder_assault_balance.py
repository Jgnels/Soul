"""Validate deterministic founder-slice campaign stress evidence."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STRESS = ROOT / "Evidence" / "BalanceLab" / "founder_assault_balance.json"
OUT = ROOT / "Evidence" / "BalanceLab" / "founder_assault_balance_validation.json"

d = json.loads(STRESS.read_text(encoding="utf-8"))
errors: list[str] = []
warnings: list[str] = []

if d.get("schema") != 1 or d.get("status") != "pass":
    errors.append("stress artifact must be schema 1 / pass")

summaries = d.get("summaries", [])
expected_cells = {
    (profile, ap, corridor)
    for profile in ("no_watch_reserve", "watch_reserve_500")
    for ap in (2, 3, 4)
    for corridor in ("river_watch", "forest_watch", "north_pass")
}
actual_cells = {
    (row["profile"], row["ap_per_day"], row["corridor"])
    for row in summaries
}
if actual_cells != expected_cells:
    errors.append("18-cell AP/reserve/corridor sweep is incomplete")

sensitivity = d.get("stronghold_sensitivity", [])
expected_sensitivity = {
    (stronghold, reserve, corridor)
    for stronghold in (650, 700, 750, 800, 900, 1050)
    for reserve in (0, 200, 500)
    for corridor in ("river_watch", "forest_watch", "north_pass")
}
actual_sensitivity = {
    (row["stronghold_strength"], row["reserve_strength"], row["corridor"])
    for row in sensitivity
}
if actual_sensitivity != expected_sensitivity:
    errors.append("54-cell stronghold/reinforcement sensitivity sweep is incomplete")

baseline = d.get("baseline_3ap_watch_reserve", {})
if set(baseline) != {"river_watch", "forest_watch", "north_pass"}:
    errors.append("baseline does not contain all three founder corridors")
else:
    for corridor, row in baseline.items():
        if row["camp_reach_rate"] < 0.99:
            errors.append(f"{corridor}: founder opening cannot reliably reach stronghold")
        if not 1 <= row["median_camp_day"] <= 3:
            errors.append(f"{corridor}: excessive or invalid opening travel time")
    if baseline["north_pass"]["reserve_arrival_rate"] != 1.0:
        errors.append("North Pass must expose the day-2 Orc Watch reserve timing case")
    if baseline["river_watch"]["reserve_arrival_rate"] != 0.0:
        errors.append("River/Watch corridor should cut the Orc Watch reserve before camp")
    if baseline["forest_watch"]["reserve_arrival_rate"] != 0.0:
        errors.append("Forest/Watch corridor should cut the Orc Watch reserve before camp")
    for corridor in ("river_watch", "forest_watch"):
        if baseline[corridor]["median_pre_camp_regiment_xp"] >= 100:
            errors.append(f"{corridor}: watch battle unexpectedly crosses Seasoned threshold")
        if baseline[corridor]["median_pre_camp_hero_xp"] >= 250:
            errors.append(f"{corridor}: watch battle unexpectedly crosses hero level-2 threshold")

    if baseline["river_watch"]["resource_sites"]:
        errors.append("River/Watch should not receive a founder resource site")
    for corridor in ("forest_watch", "north_pass"):
        if "forest_edge" not in baseline[corridor]["resource_sites"]:
            errors.append(f"{corridor}: Forest Edge resource decision missing")

recoveries = d.get("single_loss_recovery", {})
repeats = d.get("repeat_loss_stress", {})
for corridor in ("river_watch", "forest_watch", "north_pass"):
    if recoveries.get(corridor, {}).get("empty_pool_lines"):
        errors.append(f"{corridor}: single-loss stress unexpectedly exhausts recruitment line")
    if repeats.get(corridor, {}).get("empty_pool_lines"):
        errors.append(f"{corridor}: repeat-loss stress unexpectedly exhausts recruitment line")

if repeats:
    if repeats["forest_watch"]["final_strength"] <= repeats["river_watch"]["final_strength"]:
        errors.append("Forest resource income does not improve repeat-loss recovery as expected")
    if repeats["north_pass"]["final_strength"] <= repeats["river_watch"]["final_strength"]:
        errors.append("North-pass resource income does not improve repeat-loss recovery as expected")

candidate = {
    row["corridor"]: row["camp_victory_rate_all_runs"]
    for row in sensitivity
    if row["stronghold_strength"] == 700 and row["reserve_strength"] == 200
}
if set(candidate) != {"river_watch", "forest_watch", "north_pass"}:
    errors.append("candidate 700/200 topology sensitivity cell missing")
elif not all(0.25 <= rate <= 0.80 for rate in candidate.values()):
    warnings.append(
        "700/200 candidate cell moved outside broad comparative viability band; review tuning sensitivity"
    )

required_findings = {
    "reinforcement_first_order",
    "intermediate_battle_progression",
    "repeat_loss_recruitment",
    "weekly_growth_timing",
    "dominance_sensitivity",
    "stronghold_strength_placeholder",
    "viable_topology_band",
}
finding_ids = {item["kind"] for item in d.get("findings", [])}
missing = sorted(required_findings - finding_ids)
if missing:
    errors.append(f"required stress findings missing: {missing}")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "summary_cells": len(summaries),
    "sensitivity_cells": len(sensitivity),
    "candidate_700_200_rates": candidate,
    "warnings": warnings,
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
