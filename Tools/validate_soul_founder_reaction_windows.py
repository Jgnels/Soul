"""Validate founder reaction-window evidence without setting campaign policy."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ANALYSIS = ROOT / "Evidence" / "WorldOvermap" / "founder_reaction_window_analysis.json"
SEED = ROOT / "Data" / "soul_founder_slice_runtime_seed_20260922.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_reaction_window_validation.json"

analysis = json.loads(ANALYSIS.read_text(encoding="utf-8"))
seed = json.loads(SEED.read_text(encoding="utf-8"))
errors: list[str] = []

if analysis.get("status") != "POLICY_NEUTRAL_REACTION_EVIDENCE":
    errors.append("reaction analysis lost policy-neutral status")
if analysis["authority"].get("canonical_reinforcement_policy") != "UNDECIDED_REQUIRES_FOUNDER_APPROVAL":
    errors.append("reaction analysis incorrectly asserts canonical reinforcement policy")
candidate = analysis.get("candidate_for_next_prototype", {})
if candidate.get("status") != "PROPOSAL_ONLY_REQUIRES_FOUNDER_DECISION":
    errors.append("candidate rule is not clearly proposal-only")

route = None
for value in seed["routes"].values():
    if {value["a"], value["b"]} == {"orc_watch", "orc_camp"}:
        route = value
        break
if route is None:
    errors.append("Orc Watch <-> Orc Stronghold route missing")
else:
    if route["action_cost"] != 1:
        errors.append("Orc Watch reserve route action cost drift")
    if not route["road"]:
        errors.append("Orc Watch reserve route is no longer a road")

rows = {x["ap_per_day"]: x for x in analysis.get("reaction_windows", [])}
if set(rows) != {2, 3, 4}:
    errors.append("expected AP 2/3/4 reaction windows")
expected = {
    2: ((2, 1), (2, 2), False, "order_dependent"),
    3: ((1, 3), (2, 1), True, "yes"),
    4: ((1, 3), (1, 4), False, "order_dependent"),
}
for ap, values in expected.items():
    if ap not in rows:
        continue
    threat, battle, overnight, same_day = values
    row = rows[ap]
    actual_threat = (row["threat_reveal"]["day"], row["threat_reveal"]["action_slot"])
    actual_battle = (row["stronghold_contact"]["day"], row["stronghold_contact"]["action_slot"])
    if actual_threat != threat:
        errors.append(f"AP{ap}: threat window drift {actual_threat} != {threat}")
    if actual_battle != battle:
        errors.append(f"AP{ap}: battle window drift {actual_battle} != {battle}")
    if row["reaction_gap"]["campaign_day_boundary"] != overnight:
        errors.append(f"AP{ap}: overnight reaction drift")
    if row["policy_outcomes"]["same_day_interleaved_reaction"]["certainty"] != same_day:
        errors.append(f"AP{ap}: same-day reaction classification drift")

overstated = [
    x["ap_per_day"]
    for x in analysis.get("previous_coarse_comparison", [])
    if x["coarse_overstates_overnight_reaction"]
]
if overstated != [2]:
    errors.append(f"expected only AP2 coarse overnight overstatement, got {overstated}")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "ap_windows": sorted(rows),
    "coarse_overstatement_ap": overstated,
    "canonical_policy_changed": False,
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
