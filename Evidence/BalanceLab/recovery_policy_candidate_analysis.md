# Soul Defeated-Army Replacement Candidate Sweep

Runs: **250 seeds per profile per scenario**. This is a lab comparison, not a shipping balance lock.

| Profile | Delay | Retinue | Readiness | Vet kept | Same-day recruit | Baseline runaway | Gini | Battles | Recovery to 70% |
|---|---:|---:|---:|---|---|---:|---:|---:|---:|
| bounded_center | 3d | 220 | 550 | False | False | 0.000 | 0.0857 | 24.88 | 6d |
| fast_fragile | 2d | 160 | 450 | False | False | 0.016 | 0.1153 | 32.72 | 4.0d |
| fast_bounded | 2d | 220 | 550 | False | False | 0.008 | 0.1066 | 35.26 | 4.5d |
| slow_bounded | 4d | 220 | 550 | False | False | 0.000 | 0.0856 | 17.76 | 8.0d |
| slow_stronger | 4d | 260 | 650 | False | False | 0.000 | 0.1014 | 21.30 | 8d |
| exploit_sentinel | 2d | 220 | 1000 | True | True | 0.208 | 0.2051 | 28.18 | 2d |

## Interpretation guardrails

- The useful question is whether a loss is painful but recoverable without becoming a beneficial reset.
- Delay, replacement strength, logistics state, veterancy identity, and same-day recruiting are separate levers.
- The exploit sentinel exists to keep the known failure mode visible; it is not a candidate recommendation.
