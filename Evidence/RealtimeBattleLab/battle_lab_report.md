# Soul Real-Time Battle Lab - Synthetic Evidence

**CALIBRATION: UNCALIBRATED SYNTHETIC.** These results compare architecture and sensitivity only; they are not predicted player win rates or shipping performance.

The lab exercises the current Soul reinforcement rules, the released RB AI deterministic utility selector, and RB Combat's 20-member group ceiling. Physical navigation, animation, collision, Niagara cost, TCAT, and rendered frame time remain separate qualification gates.

| Scenario | Seeds | P wins | E wins | Deadlocks | Mean steps | Mean waves | Peak active | P attack share |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| waves_24_active | 400 | 169 | 231 | 0 | 22.66 | 9.32 | 48 | 65.1% |
| waves_32_active | 400 | 166 | 234 | 0 | 22.84 | 8.07 | 60 | 69.2% |
| waves_48_active | 400 | 168 | 232 | 0 | 22.57 | 4.89 | 92 | 79.3% |
| all_at_once | 400 | 213 | 187 | 0 | 22.04 | 0.00 | 124 | 89.6% |
| volcanic_fire_vs_ice_32 | 400 | 373 | 27 | 0 | 21.98 | 6.96 | 61 | 85.6% |
