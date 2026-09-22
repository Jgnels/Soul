# Soul Real-Time Battle Lab - Synthetic Evidence

**CALIBRATION: UNCALIBRATED SYNTHETIC.** These results compare architecture and sensitivity only; they are not predicted player win rates or shipping performance.

The lab exercises the current Soul reinforcement rules, the released RB AI deterministic utility selector, and RB Combat's 20-member group ceiling. Physical navigation, animation, collision, Niagara cost, TCAT, and rendered frame time remain separate qualification gates.

| Scenario | Seeds | P wins | E wins | Deadlocks | Mean steps | Mean waves | Peak active | P attack share |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| waves_24_active | 400 | 118 | 282 | 0 | 23.30 | 9.80 | 48 | 59.0% |
| waves_32_active | 400 | 106 | 294 | 0 | 23.35 | 8.48 | 61 | 62.4% |
| waves_48_active | 400 | 81 | 319 | 0 | 23.00 | 5.48 | 93 | 69.6% |
| all_at_once | 400 | 36 | 364 | 0 | 21.80 | 0.00 | 127 | 76.0% |
| volcanic_fire_vs_ice_32 | 400 | 295 | 105 | 0 | 23.82 | 8.56 | 61 | 78.6% |
