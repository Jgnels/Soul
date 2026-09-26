"""Read-only audit of dirty-primary founder runtime tests against accepted overmap fixtures."""
from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANDIDATE = Path(r"D:\RefinedBadger\Games\Soul")
TEST_CPP = CANDIDATE / "Source/Soul/Private/Tests/SoulFounderPlaytestTests.cpp"
FIXTURES = ROOT / "Data" / "soul_overmap_ue_acceptance_fixtures_v1_20260922.json"
DRIFT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_candidate_drift.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_candidate_test_gap.json"
DOC = ROOT / "Docs" / "SOUL_FOUNDER_RUNTIME_TEST_GAPS_20260922.md"

source = TEST_CPP.read_text(encoding="utf-8")
fixtures = json.loads(FIXTURES.read_text(encoding="utf-8"))
drift = json.loads(DRIFT.read_text(encoding="utf-8"))
fixture_ids = {x["id"] for x in fixtures["fixtures"]}

test_names = re.findall(r'"(Soul\.Playtest\.[^"]+)"', source)
assertion_labels = re.findall(r'TEXT\("([^"]+)"\)', source)

checks = [
    {
        "id": "hostile_nonsettlement_entry_blocks_occupation",
        "fixture_id": "ue.fixture.founder_forest_contact",
        "required_behavior": (
            "Attempting Forest Edge -> Orc Watch must enter battle commitment without setting PlayerRegion "
            "or changing Orc Watch ownership before victory."
        ),
        "covered": (
            "orc_watch" in source
            and "MovePlayerTo" in source
            and ("hostile" in source.lower() or "battle" in source.lower())
        ),
        "severity": "critical",
    },
    {
        "id": "battle_result_applies_actual_destination",
        "fixture_id": "ue.fixture.founder_forest_contact",
        "required_behavior": (
            "A victory launched for Orc Watch must resolve/capture Orc Watch; battle result code must not "
            "hardcode orc_camp."
        ),
        "covered": (
            'AwardBattleVictory(TEXT("orc_watch")' in source
            or "battle result region" in source.lower()
        ),
        "severity": "critical",
    },
    {
        "id": "directed_handoff_selects_recipe",
        "fixture_id": "ue.fixture.founder_forest_contact",
        "required_behavior": (
            "Battle launch must carry directed approach/handoff and verify Orc Watch selects orc.badlands "
            "while Stronghold selects orc.war_camp."
        ),
        "covered": (
            "orc.badlands" in source and "orc.war_camp" in source
            and ("Approach" in source or "Handoff" in source)
        ),
        "severity": "high",
    },
    {
        "id": "settlement_assault_carries_siege_context",
        "fixture_id": "ue.fixture.human_capital_east_assault",
        "required_behavior": (
            "Hostile Human Capital entry must preserve settlement ID, approach direction, and siege-context fields."
        ),
        "covered": (
            "city.human_capital" in source
            and ("siege" in source.lower() or "wall" in source.lower())
        ),
        "severity": "high",
    },
    {
        "id": "stronghold_reinforcement_uses_real_adjacent_route",
        "fixture_id": "ue.fixture.stronghold_reinforcement_retreat",
        "required_behavior": (
            "Any Orc Watch reinforcement must be tied to the actual Orc Watch -> Orc Stronghold edge and timing; "
            "no duplicated reserve should appear."
        ),
        "covered": (
            "reinforce" in source.lower()
            and "orc_watch" in source
            and "orc_camp" in source
        ),
        "severity": "decision_blocked",
    },
    {
        "id": "river_ford_recipe_gate_is_explicit",
        "fixture_id": "ue.fixture.river_ford_context",
        "required_behavior": (
            "River Ford must report human.river_road and UE_CROP_REQUIRED until the crop is qualified."
        ),
        "covered": "human.river_road" in source and "UE_CROP_REQUIRED" in source,
        "severity": "medium",
    },
]

hardcoded_test = 'AwardBattleVictory(TEXT("orc_camp"), 300)' in source
for item in checks:
    if item["fixture_id"] not in fixture_ids:
        raise RuntimeError(f"fixture missing for test-gap item {item['id']}")

covered = [x for x in checks if x["covered"]]
missing = [x for x in checks if not x["covered"]]
critical_missing = [x["id"] for x in missing if x["severity"] == "critical"]

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "READ_ONLY_RUNTIME_TEST_GAP_AUDIT",
    "authority": {
        "candidate_test_file": str(TEST_CPP),
        "candidate_test_authority": "UNTRACKED_DIRTY_PRIMARY_PROTOTYPE_NOT_CANONICAL",
        "acceptance_fixtures": "Data/soul_overmap_ue_acceptance_fixtures_v1_20260922.json",
        "runtime_drift_audit": "Evidence/WorldOvermap/founder_runtime_candidate_drift.json",
    },
    "candidate_test_sha256": hashlib.sha256(TEST_CPP.read_bytes()).hexdigest(),
    "existing_test_names": test_names,
    "existing_assertion_count": source.count("TestTrue(") + source.count("TestEqual("),
    "existing_assertion_labels": assertion_labels,
    "hardcoded_stronghold_award_test_present": hardcoded_test,
    "coverage": {
        "required_behaviors": len(checks),
        "covered": len(covered),
        "missing": len(missing),
        "critical_missing": critical_missing,
    },
    "checks": checks,
    "interpretation": [
        "The candidate has a useful basic state-loop test, but it does not cover the accepted hostile-entry or directed battle-return contracts.",
        "The existing AwardBattleVictory(orc_camp) assertion validates a stronghold victory in isolation but cannot detect the current battle actor hardcoding every victory to orc_camp.",
        "The acceptance fixtures now provide concrete future runtime assertions; they should be exercised in committed C++/functional tests when the candidate is admitted.",
        "No candidate test or runtime file was modified by this audit.",
    ],
    "minimum_tests_before_runtime_admission": [
        "Orc Watch hostile entry does not change strategic location/ownership before battle victory.",
        "Battle result applies to the actual destination region carried by the handoff.",
        "Orc Watch and Orc Stronghold launch distinct accepted battlefield recipe contexts.",
        "Settlement attack preserves settlement/siege context.",
        "Reinforcement/retreat test follows actual adjacent routes after the founder chooses the reaction policy.",
    ],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Founder Runtime Test Gaps — 2026-09-22", "",
    "Status: **read-only audit; dirty-primary tests were not modified.**", "",
    f"- Existing automation tests found: {len(test_names)}.",
    f"- Existing TestTrue/TestEqual assertions: {payload['existing_assertion_count']}.",
    f"- Required convergence behaviors reviewed: {len(checks)}.",
    f"- Covered now: {len(covered)}.",
    f"- Missing: {len(missing)}.",
    f"- Critical missing: {len(critical_missing)}.", "",
]
if hardcoded_test:
    lines += [
        "The existing test explicitly calls AwardBattleVictory(orc_camp, 300). That proves the isolated award API, "
        "but it cannot catch the current battle actor bug where every victory is attributed to Orc Stronghold.", "",
    ]
lines += ["## Missing coverage", ""]
for item in missing:
    lines.append(
        f"- **{item['id']} ({item['severity']})** — {item['required_behavior']} "
        f"Fixture: {item['fixture_id']}."
    )
DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps({
    "status": payload["status"],
    "existing_tests": len(test_names),
    "covered": len(covered),
    "missing": len(missing),
    "critical_missing": critical_missing,
}, indent=2))
