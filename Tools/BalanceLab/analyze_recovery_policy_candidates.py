"""Compare candidate defeated-army replacement policies in Soul's headless balance lab."""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from Tools.BalanceLab.balance_lab import aggregate, mkparams, run_scenario
OUT_JSON = ROOT / "Evidence" / "BalanceLab" / "recovery_policy_candidate_analysis.json"
OUT_MD = ROOT / "Evidence" / "BalanceLab" / "recovery_policy_candidate_analysis.md"
SEED_BASE = 20260922
RUNS = 250

PROFILES = {
    "bounded_center": mkparams(
        respawn_days=3, respawn_strength=220, respawn_supply=700,
        respawn_readiness=550, respawn_fatigue=150,
        respawn_preserve_veterancy=False, respawn_recruit_same_day=False),
    "fast_fragile": mkparams(
        respawn_days=2, respawn_strength=160, respawn_supply=600,
        respawn_readiness=450, respawn_fatigue=200,
        respawn_preserve_veterancy=False, respawn_recruit_same_day=False),
    "fast_bounded": mkparams(
        respawn_days=2, respawn_strength=220, respawn_supply=650,
        respawn_readiness=550, respawn_fatigue=150,
        respawn_preserve_veterancy=False, respawn_recruit_same_day=False),
}
PROFILES.update({
    "slow_bounded": mkparams(
        respawn_days=4, respawn_strength=220, respawn_supply=700,
        respawn_readiness=550, respawn_fatigue=150,
        respawn_preserve_veterancy=False, respawn_recruit_same_day=False),
    "slow_stronger": mkparams(
        respawn_days=4, respawn_strength=260, respawn_supply=700,
        respawn_readiness=650, respawn_fatigue=100,
        respawn_preserve_veterancy=False, respawn_recruit_same_day=False),
    "exploit_sentinel": mkparams(
        respawn_days=2, respawn_strength=220, respawn_supply=1000,
        respawn_readiness=1000, respawn_fatigue=0,
        respawn_preserve_veterancy=True, respawn_recruit_same_day=True),
})

rows = []
for name, params in PROFILES.items():
    baseline = aggregate(run_scenario(RUNS, SEED_BASE, 2, "", params))
    shock = aggregate(run_scenario(RUNS, SEED_BASE, 2, "day14_loss", params))
    rows.append({
        "profile": name,
        "params": {
            "respawn_days": params.respawn_days,
            "respawn_strength": params.respawn_strength,
            "respawn_supply": params.respawn_supply,
            "respawn_readiness": params.respawn_readiness,
            "respawn_fatigue": params.respawn_fatigue,
            "preserve_veterancy": params.respawn_preserve_veterancy,
            "recruit_same_day": params.respawn_recruit_same_day,
        },
        "baseline": {
            "runaway_rate": baseline["runaway_rate"],
            "mean_gini": baseline["mean_gini"],
            "mean_leader_share": baseline["mean_leader_share"],
            "mean_battles": baseline["mean_battles"],
        },
        "forced_loss": {
            "median_recovery70_days": shock["median_f0_recovery70_days"],
            "median_final_recovery_ratio": shock["median_f0_recovery_ratio"],
            "f0_leader_rate": shock["f0_leader_rate"],
            "mean_f0_score_share": shock["mean_f0_score_share"],
            "mean_f0_territory": shock["mean_f0_territory"],
            "mean_battles": shock["mean_battles"],
        },
    })

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "analyzed",
    "runs_per_profile_per_scenario": RUNS,
    "seed_base": SEED_BASE,
    "authority": "LAB_ONLY_CANDIDATE_ANALYSIS",
    "profiles": rows,
    "guardrails": [
        "Candidate profiles do not change Soul production runtime rules.",
        "The exploit sentinel intentionally preserves veterancy/full logistics/same-day recruiting as a regression warning.",
        "Prefer visible symmetric replacement rules over hidden comeback subsidies.",
    ],
}
OUT_JSON.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

lines = [
    "# Soul Defeated-Army Replacement Candidate Sweep",
    "",
    f"Runs: **{RUNS} seeds per profile per scenario**. This is a lab comparison, not a shipping balance lock.",
    "",
    "| Profile | Delay | Retinue | Readiness | Vet kept | Same-day recruit | Baseline runaway | Gini | Battles | Recovery to 70% |",
    "|---|---:|---:|---:|---|---|---:|---:|---:|---:|",
]
for row in rows:
    p = row["params"]
    b = row["baseline"]
    s = row["forced_loss"]
    recovery = s["median_recovery70_days"]
    lines.append(
        f"| {row['profile']} | {p['respawn_days']}d | {p['respawn_strength']} | "
        f"{p['respawn_readiness']} | {p['preserve_veterancy']} | {p['recruit_same_day']} | "
        f"{b['runaway_rate']:.3f} | {b['mean_gini']:.4f} | {b['mean_battles']:.2f} | {recovery}d |"
    )
lines += [
    "",
    "## Interpretation guardrails",
    "",
    "- The useful question is whether a loss is painful but recoverable without becoming a beneficial reset.",
    "- Delay, replacement strength, logistics state, veterancy identity, and same-day recruiting are separate levers.",
    "- The exploit sentinel exists to keep the known failure mode visible; it is not a candidate recommendation.",
]
OUT_MD.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
