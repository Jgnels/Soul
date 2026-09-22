"""Generate deterministic siege waiting-cost candidate evidence for Soul."""
import json
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from Tools.BalanceLab.balance_lab import siege_commitment_stress

OUT_JSON = ROOT / "Evidence" / "BalanceLab" / "siege_commitment_candidate_analysis.json"
OUT_MD = ROOT / "Evidence" / "BalanceLab" / "siege_commitment_candidate_analysis.md"

rows = siege_commitment_stress()
for row in rows:
    pressure = row["pressure_day"]
    row["supply_exhausted_before_pressure"] = bool(
        row["supply_exhausted_day"] and pressure and row["supply_exhausted_day"] <= pressure
    )
    row["below_assault_readiness_before_pressure"] = bool(
        row["below_assault_readiness_day"] and pressure and row["below_assault_readiness_day"] <= pressure
    )
profile_summary = {}
for profile in ("light", "medium", "heavy"):
    group = [row for row in rows if row["attacker_sustainment_profile"] == profile]
    profile_summary[profile] = {
        "cases": len(group),
        "supply_exhausted_before_pressure": sum(row["supply_exhausted_before_pressure"] for row in group),
        "below_assault_readiness_before_pressure": sum(row["below_assault_readiness_before_pressure"] for row in group),
        "pressure_days": dict(sorted(Counter(row["pressure_day"] for row in group).items())),
    }

payload = {
    "schema": 1,
    "generated": "2026-09-22",
    "authority": "LAB_ONLY_CANDIDATES_NOT_PRODUCTION_RULES",
    "assumptions": {
        "attacker_assault_readiness_reference": 600,
        "defender_floor": 200,
        "starting_supply_and_readiness": 1000,
    },
    "profile_summary": profile_summary,
    "cases": rows,
}
OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
OUT_JSON.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
lines = [
    "# Soul Siege Commitment Candidate Audit", "",
    "This audit does not define surrender or assault resolution. It only tests whether waiting can carry symmetric logistics cost before defender pressure is reached.", "",
    "| Attacker sustainment | Cases | Supply exhausted before pressure | Below 600 readiness before pressure |",
    "|---|---:|---:|---:|",
]
for profile in ("light", "medium", "heavy"):
    s = profile_summary[profile]
    lines.append(
        f"| {profile} | {s['cases']} | {s['supply_exhausted_before_pressure']} | "
        f"{s['below_assault_readiness_before_pressure']} |"
    )

center = next(row for row in rows if row["defender_loss"] == 70 and row["defender_threshold"] == 400 and row["attacker_sustainment_profile"] == "medium")
lines += ["", "## Center candidate", "",
    f"At defender loss 70/day, pressure threshold 400, and medium attacker cost 50 supply + 25 readiness/day, pressure arrives on day **{center['pressure_day']}**.",
    f"Attacker state then is supply **{center['state_at_pressure']['attacker_supply']}**, readiness **{center['state_at_pressure']['attacker_readiness']}**; defender supply is **{center['state_at_pressure']['defender_supply']}**.",
    "", "## Interpretation", "",
    "- Light and medium sustainment preserve room to wait for pressure in the tested range.",
    "- Heavy sustainment creates cases where the attacker runs out of supply or falls below the existing 600 siege-entry readiness reference before deep starvation pressure.",
    "- That is useful: waiting stops being a free dominant action without requiring a hidden comeback subsidy.",
    "- A real surrender/attrition/assault-pressure rule is still required in SoulCore; this lab does not choose it.",
]
OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(json.dumps(profile_summary, indent=2))
