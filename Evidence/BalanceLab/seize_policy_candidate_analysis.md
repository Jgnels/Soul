# Soul SEIZE Readiness Candidate Audit

Runs per policy: **1000**; seed base: **20260922**.

| Policy | Gini | Leader share | Battles | SEIZE share |
|---|---:|---:|---:|---:|
| current_live_mirror | 0.0885 | 0.3058 | 24.81 | 0.2172 |
| floor400_only | 0.0884 | 0.3058 | 24.82 | 0.2173 |
| readiness_penalty | 0.1044 | 0.3131 | 29.12 | 0.2267 |

## Result

The 400-readiness floor is the smallest tested correction that directly prevents low-readiness opportunistic seizure while preserving the live-mirror score model above the floor.
The full recovery-style readiness penalty has a much broader campaign effect and should not be treated as a drop-in fix without further tuning.

This is lab evidence only; it does not change SoulCore production behavior.
