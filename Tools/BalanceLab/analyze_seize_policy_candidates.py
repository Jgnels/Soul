"""Compare bounded low-readiness SEIZE fixes in the deterministic Soul balance lab."""
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from Tools.BalanceLab.balance_lab import (
    Params, aggregate, mkparams, run_scenario, seize_policy_stress,
)
OUT_JSON = ROOT / "Evidence" / "BalanceLab" / "seize_policy_candidate_analysis.json"
OUT_MD = ROOT / "Evidence" / "BalanceLab" / "seize_policy_candidate_analysis.md"

POLICIES = {
    "current_live_mirror": Params(),
    "floor400_only": mkparams(seize_min_readiness=400),
    "readiness_penalty": mkparams(seize_readiness_penalty=True),
}

KEYS = ("runaway_rate", "mean_gini", "mean_leader_share", "mean_battles")

def compact(summary):
    return {key: summary[key] for key in KEYS} | {"action_share": summary["action_share"]}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=1000)
    ap.add_argument("--seed-base", type=int, default=20260922)
    args = ap.parse_args()

    summaries = {}
    for name, params in POLICIES.items():
        results = run_scenario(args.runs, args.seed_base, 2, "", params)
        summaries[name] = compact(aggregate(results))

    baseline = summaries["current_live_mirror"]
    deltas = {}
    for name, summary in summaries.items():
        deltas[name] = {
            key: summary[key] - baseline[key] for key in KEYS
        }
        deltas[name]["seize_action_share"] = (
            summary["action_share"]["SEIZE"] - baseline["action_share"]["SEIZE"]
        )

    payload = {
        "schema": 1,
        "generated": "2026-09-22",
        "runs_per_policy": args.runs,
        "seed_base": args.seed_base,
        "authority": "LAB_ONLY_CANDIDATES_NOT_PRODUCTION_RULES",
        "focused_choice_matrix": seize_policy_stress(),
        "summaries": summaries,
        "deltas_vs_live_mirror": deltas,
        "interpretation": [
            "The readiness floor is the narrower intervention: it blocks opportunistic seizure below the floor without changing scores above it.",
            "The full readiness penalty changes strategic behavior beyond the zero-readiness defect and therefore requires broader balance review.",
            "These aggregates are synthetic comparative diagnostics, not shipping win-rate predictions.",
        ],
    }
    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    lines = [
        "# Soul SEIZE Readiness Candidate Audit", "",
        f"Runs per policy: **{args.runs}**; seed base: **{args.seed_base}**.", "",
        "| Policy | Gini | Leader share | Battles | SEIZE share |",
        "|---|---:|---:|---:|---:|",
    ]
    for name in POLICIES:
        s = summaries[name]
        lines.append(
            f"| {name} | {s['mean_gini']:.4f} | {s['mean_leader_share']:.4f} | "
            f"{s['mean_battles']:.2f} | {s['action_share']['SEIZE']:.4f} |"
        )
    lines += ["", "## Result", "",
        "The 400-readiness floor is the smallest tested correction that directly prevents low-readiness opportunistic seizure while preserving the live-mirror score model above the floor.",
        "The full recovery-style readiness penalty has a much broader campaign effect and should not be treated as a drop-in fix without further tuning.",
        "", "This is lab evidence only; it does not change SoulCore production behavior."]
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(json.dumps(payload, indent=2))

if __name__ == "__main__":
    main()
