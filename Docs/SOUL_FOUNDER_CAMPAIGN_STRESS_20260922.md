# Soul Founder Campaign Stress — 2026-09-22

Deterministic non-UE stress test using the actual 9-region founder topology.
This is a decision aid, not production balance authority.

## Baseline: 3 AP/day, no intentional prep, resource income 9

| Corridor | Encounters | Day | Logistics | Supply | Readiness | Day-14 gold | Hero level | Regiment |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| road_gate | 2 | 2 | 24 | 720 | 916 | 237 | 1 | Seasoned |
| forest_watch | 2 | 2 | 28 | 615 | 860 | 354 | 1 | Seasoned |
| north_pass | 1 | 2 | 32 | 552 | 816 | 354 | 1 | Seasoned |

## If every marked encounter fires

- **road_gate**: 3 battles; day 3; hero level 2; regiment Veteran; readiness 976.
- **forest_watch**: 2 battles; day 2; hero level 1; regiment Seasoned; readiness 860.
- **north_pass**: 2 battles; day 2; hero level 1; regiment Seasoned; readiness 816.

## Findings

- **equal_day_core_corridors (info)** — All three core founder corridors reach the stronghold on the same campaign day at 3 AP/day under hostile-only encounters.
- **route_logistics_gradient (info)** — Road -> forest -> pass produces a clean readiness gradient; route geometry is mechanically legible rather than cosmetic.
- **forest_resource_tradeoff (info)** — Forest Edge gives the forest routes an economic payoff that offsets their worse logistics.
- **north_pass_incentive_gap (action)** — With every marked encounter active, North Pass has no systemic strategic reward in the current model: same battle count/resource payoff, no faster completion, and worse logistics than Forest Watch.
  - Next test: Give North Pass a concrete tactical/intelligence/avoidance benefit before changing topology; validate it in battle rather than compensating with arbitrary AP discounts.
- **encounter_progression_pressure (watch)** — Marked encounters make route choice alter hero/regiment progression before the stronghold.
- **seven_day_preparation_tradeoff (info)** — A full weekly-growth wait is measurable against its seven-day opportunity cost; this is the first founder-slice finite-pool preparation check.
- **reinforcement_timing (info)** — Final-battle reinforcement entries are now measured from Crossroads using real graph distance, hostile-clear actions, and the battle-entry action.

## Boundaries

- No realtime-battle attrition or casualty feedback is invented here.
- Settlement-local recruitment identity remains a known runtime gap; the seven-line pool model is existing Balance Lab glue.
- Town construction progression remains intentionally excluded until its concurrency/day semantics are decided.
- Special-site rewards (including Lost Shrine) are not assigned synthetic power values.
