# Soul Founder Slice Stress Test — 2026-09-22

This uses the actual 9-region founder graph plus live-mirrored Soul logistics, veterancy, hero thresholds, finite recruitment, and campaign-economy rules. Numeric army/garrison combat values remain BalanceLab glue, so percentages below are comparative diagnostics rather than shipping win-rate predictions.
Reserve-arrival percentages in this report use the original coarse day>1 sensitivity shortcut; exact AP/day reaction windows are now reported separately in SOUL_FOUNDER_REACTION_WINDOWS_20260922.md.

## Baseline: 3 AP/day, Orc Watch can reinforce the stronghold

| Corridor | Camp day | Reach | Win diagnostic | Reserve arrives | Pre-camp strength | Supply | Readiness | Reg XP | Hero XP |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| river_watch | 2 | 100.0% | 0.0% | 0.0% | 764 | 890 | 964 | 85 | 100 |
| forest_watch | 2 | 100.0% | 0.0% | 0.0% | 753 | 785 | 960 | 85 | 100 |
| north_pass | 2 | 100.0% | 0.0% | 100.0% | 1050 | 722 | 916 | 0 | 0 |

## Findings

- **pass / no_excessive_walking** — All three 3-AP openings reach the stronghold decision by day 2-3 in the live-mirrored action model.
- **decision_required / reinforcement_first_order** — North Pass bypasses Orc Watch but gives its one-action reserve a full reaction window before a day-2 stronghold assault at 3 AP/day.
- **risk / intermediate_battle_progression** — The Orc Watch victory grants 85 lab regiment XP and 100 hero XP, leaving both below their next power threshold before the stronghold; the extra battle is attrition without an immediate progression payoff.
- **pass / resource_income_timing** — Forest Edge adds one resource-income site, but the field army is already away from the capital, so the extra income cannot improve the immediate day-2 assault under current finite-recruitment semantics.
- **pass / single_loss_recruitment** — A single forced early annihilation does not exhaust the seven-line finite recruitment pool in the bounded recovery model.
- **pass / repeat_loss_recruitment** — Even three forced early annihilations do not exhaust a recruitment line in this bounded stress window; recovery is gold/income-limited before it is pool-limited.
- **pass / weekly_growth_timing** — The first weekly recruitment growth pulse occurs on day 8, well after the day-2 baseline stronghold decision, so weekly settlement growth cannot rescue or distort the opening assault.
- **decision_required / dominance_sensitivity** — Route ranking is not robust until Orc Watch reserve/reaction semantics are fixed; bypassing the outpost changes the North Pass result materially when its 500-strength lab reserve is enabled.
- **risk / stronghold_strength_placeholder** — With the noncanonical 1050-vs-1050 field-strength placeholder, travel wear plus the optional outpost fight makes an immediate founder stronghold rush effectively nonviable; topology is not the blocker, the still-unset army/garrison profile is.
- **pass / viable_topology_band** — The same topology produces three live opening corridors in the 700-stronghold / 200-reserve sensitivity cell, showing that the map itself is not a dead opening once defender and reinforcement values are in a plausible tuning band.

## AP sensitivity

- **2 AP/day:** river_watch: day 3, reserve 0%, win 0.0%; forest_watch: day 3, reserve 0%, win 0.0%; north_pass: day 2, reserve 100%, win 0.0%.
- **3 AP/day:** river_watch: day 2, reserve 0%, win 0.0%; forest_watch: day 2, reserve 0%, win 0.0%; north_pass: day 2, reserve 100%, win 0.0%.
- **4 AP/day:** river_watch: day 2, reserve 0%, win 0.0%; forest_watch: day 2, reserve 0%, win 0.0%; north_pass: day 1, reserve 0%, win 0.0%.
- **Timing correction:** AP2 reserve=100% above is a coarse sensitivity artifact; exact graph timing has no overnight window between North Pass reveal and Stronghold contact at 2 AP/day. See SOUL_FOUNDER_REACTION_WINDOWS_20260922.md.

## Stronghold / reinforcement sensitivity

- **Equal 1050 field strengths, no reserve:** river_watch 0.0%, forest_watch 0.0%, north_pass 3.6%.
- **700 stronghold + 200 one-action reserve:** river_watch 62.6%, forest_watch 43.8%, north_pass 54.4%. This is a sensitivity example, not recommended shipping tuning.

## Recovery / finite recruitment

- **river_watch:** income 18/day; single-loss recovery70 [7], final strength 1165; repeat-loss final strength 1325, gold 10, remaining pool 42, empty lines none.
- **forest_watch:** income 27/day; single-loss recovery70 [7], final strength 1475; repeat-loss final strength 1660, gold 20, remaining pool 38, empty lines none.
- **north_pass:** income 27/day; single-loss recovery70 [7], final strength 1475; repeat-loss final strength 1660, gold 20, remaining pool 38, empty lines none.

## Decision exposed by the test

The cheapest high-information product decision is the Orc Watch reserve rule. If a player bypasses the outpost through North Pass, decide whether that outpost can reinforce the stronghold before the next-day assault. The route balance changes materially either way; tuning army numbers before fixing that timing rule would be premature.
