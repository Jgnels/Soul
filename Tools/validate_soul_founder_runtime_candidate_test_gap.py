"""Validate the dirty-primary founder runtime test-gap audit snapshot."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANDIDATE_TEST = Path(r"D:\RefinedBadger\Games\Soul\Source\Soul\Private\Tests\SoulFounderPlaytestTests.cpp")
AUDIT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_candidate_test_gap.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_candidate_test_gap_validation.json"

audit = json.loads(AUDIT.read_text(encoding="utf-8"))
errors: list[str] = []
warnings: list[str] = []

if audit.get("status") != "READ_ONLY_RUNTIME_TEST_GAP_AUDIT":
    errors.append("test-gap audit status mismatch")
if audit.get("authority", {}).get("candidate_test_authority") != "UNTRACKED_DIRTY_PRIMARY_PROTOTYPE_NOT_CANONICAL":
    errors.append("candidate test authority boundary missing")

coverage = audit.get("coverage", {})
if coverage.get("required_behaviors") != 6:
    errors.append("expected six mapped runtime convergence behaviors")
critical = set(coverage.get("critical_missing", []))
expected_critical = {
    "hostile_nonsettlement_entry_blocks_occupation",
    "battle_result_applies_actual_destination",
}
if critical != expected_critical:
    errors.append(f"critical test-gap set drift: {sorted(critical)}")

checks = {x["id"]: x for x in audit.get("checks", [])}
for cid in expected_critical:
    if cid not in checks or checks[cid].get("covered"):
        errors.append(f"critical behavior unexpectedly marked covered: {cid}")

if not audit.get("hardcoded_stronghold_award_test_present"):
    warnings.append("candidate test no longer contains the known hardcoded stronghold award call; rerun runtime drift audit")

fixture_ids = {x.get("fixture_id") for x in audit.get("checks", [])}
required_fixture_ids = {
    "ue.fixture.founder_forest_contact",
    "ue.fixture.human_capital_east_assault",
    "ue.fixture.stronghold_reinforcement_retreat",
    "ue.fixture.river_ford_context",
}
if not required_fixture_ids <= fixture_ids:
    errors.append("test-gap audit is not mapped to all required acceptance fixtures")

if CANDIDATE_TEST.exists():
    current_hash = hashlib.sha256(CANDIDATE_TEST.read_bytes()).hexdigest()
    if current_hash != audit.get("candidate_test_sha256"):
        warnings.append("candidate test file changed since audit; rerun audit before relying on gap counts")
else:
    errors.append("candidate test file missing")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "audit_snapshot_valid": not errors,
    "candidate_snapshot_unchanged": not warnings,
    "master_gate_eligible": False,
    "critical_missing": sorted(critical),
    "warnings": warnings,
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
