"""Validate the deterministic founder campaign stress evidence."""
from __future__ import annotations

import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "Evidence" / "WorldOvermap" / "founder_campaign_stress.json"
CSV_PATH = ROOT / "Evidence" / "WorldOvermap" / "founder_campaign_stress.csv"
SEED = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
HANDOFF = ROOT / "Data" / "soul_overmap_battle_handoff_v1_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_campaign_stress_validation.json"

d = json.loads(EVIDENCE.read_text(encoding="utf-8"))
seed = json.loads(SEED.read_text(encoding="utf-8"))
handoff = json.loads(HANDOFF.read_text(encoding="utf-8"))
errors = []
warnings = []

expected_runs = 3 * 3 * 2 * 3 * 5
if d["run_count"] != expected_runs:
    errors.append(f"expected {expected_runs} stress runs, found {d['run_count']}")
if d["status"] != "pass" or d["errors"]:
    errors.append("stress analyzer did not report pass")

core = d["core_baseline_hostile_only"]
marked = d["core_baseline_all_marked"]
expected_core = {"road_gate", "forest_watch", "north_pass"}
if set(core) != expected_core:
    errors.append(f"core baseline strategies mismatch: {sorted(core)}")
if set(marked) != expected_core:
    errors.append("all-marked baseline strategy coverage mismatch")

for name, row in core.items():
    if row["ap_per_day"] != 3 or row["prep_days"] != 0:
        errors.append(f"{name}: baseline dimensions changed")
    if row["completion_day"] < 1:
        errors.append(f"{name}: invalid completion day")
    for key in ("supply_at_objective", "readiness_at_objective", "minimum_supply", "minimum_readiness"):
        if not 0 <= row[key] <= 1000:
            errors.append(f"{name}: {key} outside permille bounds")
    if row["recruit_pool_available_day14"] > row["recruit_pool_capacity"]:
        errors.append(f"{name}: recruit pool exceeds capacity")
    if row["strength_at_objective"] < row["initial_strength_after_recruit"]:
        warnings.append(f"{name}: strength fell despite attrition being disabled")

if not (
    core["road_gate"]["minimum_readiness"]
    > core["forest_watch"]["minimum_readiness"]
    > core["north_pass"]["minimum_readiness"]
):
    errors.append("expected road/forest/pass minimum-readiness gradient is absent")

if core["forest_watch"]["gold_projection_day14"] <= core["road_gate"]["gold_projection_day14"]:
    errors.append("Forest Edge resource route has no projected economic payoff")
if core["north_pass"]["gold_projection_day14"] != core["forest_watch"]["gold_projection_day14"]:
    errors.append("forest and north-pass resource payoff diverged unexpectedly")

if marked["road_gate"]["battles"] <= marked["forest_watch"]["battles"]:
    errors.append("marked River Ford encounter no longer creates additional road-route contact")
if marked["road_gate"]["hero_level_from_current_curve"] < marked["forest_watch"]["hero_level_from_current_curve"]:
    errors.append("extra marked encounter reduced hero progression unexpectedly")

ap = d["sensitivity"]["completion_day_by_ap_hostile_only"]
for name in expected_core:
    if not (ap[name]["2"] >= ap[name]["3"] >= ap[name]["4"]):
        errors.append(f"{name}: higher AP made route slower")
    if ap[name]["2"] == ap[name]["4"]:
        warnings.append(f"{name}: AP 2->4 has no completion-day sensitivity")

finding_ids = {x["id"] for x in d["findings"]}
required_findings = {
    "route_logistics_gradient",
    "forest_resource_tradeoff",
    "north_pass_incentive_gap",
    "encounter_progression_pressure",
    "seven_day_preparation_tradeoff",
    "reinforcement_timing",
}
missing_findings = sorted(required_findings - finding_ids)
if missing_findings:
    errors.append(f"missing expected findings: {missing_findings}")

handoff_pairs = {
    (h["source_region"], h["destination_region"])
    for h in handoff["handoffs"]
}
for name, row in core.items():
    final_src = row["path"][-2]
    if (final_src, row["path"][-1]) not in handoff_pairs:
        errors.append(f"{name}: final battle is not backed by a handoff")
    for option in row["reinforcement_options_for_final_battle"]:
        if (option["entry_source_region"], row["path"][-1]) not in handoff_pairs:
            errors.append(f"{name}: reinforcement option lacks handoff route")
        if option["days_at_ap"] < 1:
            errors.append(f"{name}: invalid reinforcement timing")

with CSV_PATH.open("r", encoding="utf-8", newline="") as handle:
    rows = list(csv.DictReader(handle))
if len(rows) != expected_runs:
    errors.append(f"CSV row count {len(rows)} != {expected_runs}")

if d["authority"]["battle_attrition"] != "NOT_MODELED":
    errors.append("stress lab must not invent realtime battle attrition")
if d["authority"]["town_construction_progression"] != "NOT_MODELED_UNRESOLVED_RUNTIME_RULE":
    errors.append("stress lab silently invented town progression")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "run_count": d["run_count"],
    "core_strategies": sorted(core),
    "finding_ids": sorted(finding_ids),
    "warnings": warnings,
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
