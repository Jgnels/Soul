# Soul Campaign Mechanics + Balance Lab Report — 2026-09-20

## Executive summary

A deterministic non-UE Python lab now stress-tests the current Soul campaign rules plus the stronger Living Strategy campaign-reasoning/memory seam. The lab uses exact mirrors where rules already exist and labels all aggregate battle/recovery integration as lab-only glue.

The main conclusion is **do not scale expensive campaign content yet**. Several structural mechanics should be fixed first because tuning around them would produce misleading numbers:

- settlement repair is currently immediate and free;
- a per-settlement town-day API advances global campaign day/income;
- finite recruitment pools are global by unit id rather than local by settlement+unit;
- siege starvation bottoms out without changing any meaningful outcome;
- exposed-region seizure omits the recovery/readiness penalty used by other offensive choices;
- siege aftermath persists physical damage but not the economic cost that is supposed to make attacker/defender consequences matter.

The current commander-memory model is comparatively healthy: it is bounded, decays, becomes cautious after repeated grounded losses, and can still be overridden by a strong force advantage.

## Evidence quality and authority

**Observed live state:** current dirty checkout at D:\RefinedBadger\Games\Soul, read-only in this lane.

**Isolated lab branch:** astra/soul-strategy-balance-lab-20260920 from 05597a337f06bb1b01de3fc9dbe829b95b4e9e6a.

**Living Strategy reference:** revival/week-one-20260918 at d82ddcfdf8f76795909934a9983b43025c34f4c2.

The lab intentionally separates LIVE MIRROR (SoulCore integer rules), DONOR REFERENCE (Living Strategy strategic reasoning/memory), and LAB GLUE (generated maps, generic unit economics, aggregate battle resolution, provisional defeated-army recovery).

No production C++ was modified.
## Stabilized campaign results

The final bounded-replacement baseline used 1,000 seeds, a 2,000-run cumulative convergence pass, and an independent 1,000-seed replication.

| Metric | Primary 1,000 | Cumulative 2,000 | Independent 1,000 |
|---|---:|---:|---:|
| Runaway frequency (lab threshold: leader share > 42%) | 0.10% | 0.05% | 0.00% |
| Mean Gini of composite faction score | 0.0885 | 0.0881 | 0.0867 |
| Mean leader share | 30.58% | 30.54% | 30.45% |
| Mean battles / 56-day campaign | 24.79 | — | 24.53 |

Those values are stable. More importantly, **the result is not stable across different defeated-army replacement rules**:

| Recovery profile | Runaway | Mean Gini | Mean leader share | Mean battles |
|---|---:|---:|---:|---:|
| Generous: 2 days, full logistics, veterancy preserved, same-day recruiting | 29.3% | 0.2162 | 38.41% | 28.27 |
| Bounded lab center: 3 days, partial logistics, veterancy lost, recruit next day | 0.1% | 0.0885 | 30.58% | 24.79 |
| Harsh: 4 days, smaller retinue, weaker logistics | 0.1% | 0.0750 | 30.41% | 12.46 |

This is the strongest finding in the lab: **defeated-army replacement semantics are first-order campaign balance authority**. The generous profile produces roughly two orders of magnitude more runaway outcomes than the bounded profile under the same map/economy seeds. The harsh profile suppresses inequality but also cuts battle frequency roughly in half, risking a stalled campaign.

These percentages are lab diagnostics, not predicted shipping win rates. The composite score and aggregate battle layer are synthetic. Their value is comparative: the same deterministic sandbox exposes which mechanics amplify or suppress inequality.

## Map size and travel sensitivity

With the bounded recovery profile held constant:

| Scenario | Runaway | Mean Gini | Mean battles |
|---|---:|---:|---:|
| Small map | 5.7% | 0.1334 | 26.89 |
| Medium baseline | 0.1% | 0.0885 | 24.79 |
| Large map | 1.3% | 0.0720 | 14.94 |
| Medium, harsher travel cost (12 vs 8) | 0.1% | 0.0895 | 24.81 |

Small maps still produce the most inequality and contact. Large maps reduce average inequality and battle count but can occasionally create an isolated leader, hence the slightly higher runaway tail than medium despite lower Gini. Once replacement is bounded, moving travel cost from 8 to 12 matters far less than it did under the generous recovery profile; recovery semantics dominate logistics tuning.
## Day / action-point economy

Living Strategy Week One and Soul's campaign economy both center on 3 actions/day. With bounded recovery held constant:

- 2 AP: 0.2% runaway, Gini 0.0748, 21.29 battles;
- 3 AP: 0.1% runaway, Gini 0.0885, 24.79 battles;
- 4 AP: 0.8% runaway, Gini 0.0922, 23.87 battles.

The response is modest and non-monotonic because many days terminate on combat, recovery, or hold before all theoretical AP can be spent. Keep **3 AP as the center of a 2–4 test range** until real campaign path lengths exist. Route geometry, recovery, and candidate legality are higher-value levers than simply granting more actions.

## Recruitment buildings: loss must hurt but remain recoverable

The live rules correctly block recruitment and weekly growth when a required dwelling is ruined. The problem is the recovery path: RepairBuilding currently has no campaign resource, action, or time cost.

Focused 28-day destruction tests make the consequence visible:

| Dwelling | Current free repair | Costed 4-day lab repair |
|---|---:|---:|
| Ranged pool available by day 28 | 8 | 6 |
| Ranged repair cost | 0 | 216 lab gold |
| Beast/elephant pool available by day 28 | 3 | 2 |
| Beast/elephant repair cost | 0 | 202 lab gold |

In the 1,000-run campaign shock, **free repair was bit-for-bit indistinguishable from baseline** for both ranged and beast dwelling destruction. That fails the stated design goal: losing a recruitment building is not painful if a rational caller can restore it immediately for free.

Recommended test range after the runtime defect is fixed: **3–7 campaign days** and **25–60% of replacement/building value** to restore operational recruitment, with the existing 500–700 permille integrity band tested as the operational threshold. This creates one or more missed growth windows without turning one siege into permanent faction death.

## Dwarf capstone / town progression

The current settlement construction rules allow every building in `Building` state to progress in parallel and define no town construction slot/queue. In the lab's three-prerequisite capstone stress test, a wealthy town completed the capstone on day 9 with effectively unlimited parallel slots, versus day 12 with two slots and day 15 with one slot.

That is an exploit/rule-gap candidate rather than a confirmed tuning bug: parallel construction may be intentional. Before Dwarf capstone content is authored, choose and regression-test an explicit concurrency rule. The high-information range is **1–2 simultaneous major construction commitments**; unbounded parallelism should be an explicit design decision, not an accidental default.
## Commander memory: keep the current bounded shape

Soul's current C++ memory model uses a 7-turn rival horizon, defeat caution capped at -800, victory confidence capped inside the +300 total, and place-reclaim influence capped at +500.

Focused repeated-rival tests behaved correctly:

- one fresh 800-intensity defeat gives -640 rival bias;
- two fresh defeats saturate at -800 instead of stacking without bound;
- at readiness 550 and no force edge, two defeats flip ATTACK (1,050) below RECOVER (1,150);
- the same remembered defeats with a +300 force edge still choose ATTACK (1,350 > 1,150);
- once the 7-turn horizon expires, the rival bias returns to zero.

This is the desired pattern: history changes judgment without becoming deterministic obsession. Do **not** redesign memory before fixing the campaign mechanics around it. A reasonable tuning envelope is caution cap -500 to -800 and rival decay 5–8 turns.

## Low-readiness AI: actual scoring defect

`SEIZE_EXPOSED_REGION` currently lacks the readiness/recovery penalty applied to other aggressive choices. With a representative exposed region, its score is 1,850 at every readiness value. RECOVER scores 1,700 at readiness 0 and falls from there. Result: even at **zero readiness**, the current formula prefers the exposed-region seizure.

This violates the Living Strategy authority principle that recovery/defense should be considered before opportunistic aggression. It is not a reason to replace Soul strategic AI with RB AI; it is a narrow missing score component. The cheapest fix to test is to apply the existing recovery penalty pattern to seizure, with an optional offensive-readiness floor in the 250–400 range if scoring alone is insufficient.

## Siege starvation and assault

Current encirclement is mechanically incomplete. Defender supply loses 70 permille/day until it reaches 200; at day 12 it hits that floor and then remains 200 forever. No current SoulSiege rule turns that state into surrender pressure, combat degradation, attrition, readiness loss, or any other resolving consequence. The attacker also pays no canonical sustainment cost in this layer.

A starvation-only attacker therefore has no reason to assault and no way to finish by starvation. This is a mechanics defect, not a balance-number problem.

Threshold sensitivity gives a useful design window once a real pressure rule exists: supply loss of 50–90/day combined with a pressure threshold of 300–500 reaches meaningful pressure in **6–14 days**. Test attacker sustainment simultaneously (roughly 30–80 supply and 10–40 readiness permille/day) so waiting is a strategic choice rather than a dominant free option.

`SoulSiegeAftermath` currently persists wall/building damage and scars, but does not yet express occupation cost, income disruption, attacker upkeep, or repair-resource burden. Those economic consequences should be connected before siege frequency is tuned.
## Veterancy snowball: direct bonus is bounded; morale interaction is the risk

Live rank bonuses are modest in isolation: +2.5%, +5%, +7.5%, and +10% combat from Seasoned through Legendary. Morale adds 0, +1, +1, +2.

`ESoulMoraleOutcome::ExtraAction` is defined and rolled inside SoulCombat, but the current SoulCore search shows no second integration site that consumes that outcome. Therefore the lab reports the following as a **conditional upper-bound interaction**, not as confirmed runtime DPS:

| Rank | Direct combat | Morale bonus | If a proc grants one full extra attack: expected action-damage index |
|---|---:|---:|---:|
| Recruit | 0% | 0 | 1.000 |
| Seasoned | +2.5% | 0 | 1.025 |
| Veteran | +5% | +1 | 1.155 |
| Elite | +7.5% | +1 | 1.1825 |
| Legendary | +10% | +2 | 1.320 |

If ExtraAction later becomes a full extra attack at base morale zero, Legendary veterancy can behave more like a 32% output premium than a 10% premium. That is the snowball surface to bound. Target a **5–20% total effective veterancy premium** after morale/initiative effects are included, rather than tuning only the damage scalar.

## Hero growth

Soul's generic XP threshold is `(level - 1) * level * 125`. Under the Living Strategy Week One reference reward of 100 XP per victory, level thresholds arrive after roughly 3, 8, 15, and 25 wins for levels 2–5.

SoulCore does not currently define a universal campaign-strength bonus per hero level. The lab's +55 permille/level aggregate strength is explicitly synthetic glue. Do not balance hero growth around that number; first decide which skills/stats actually project into campaign and tactical power, then rerun the lab with those real effects.

## Recovery after losing a major army

The current SoulCore mechanics reviewed here do not define a complete campaign-level defeated-army replacement rule. That missing rule is now the largest balance uncertainty found by the lab.

The bounded lab profile is deliberately conservative rather than canonical: a replacement retinue returns after 3 days at roughly 21% of starting field strength, with 700 supply, 550 readiness, 150 fatigue, no annihilated-regiment veterancy, and no same-day recruitment refill. Under that profile, the forced day-14 loss is **painful but recoverable**:

- median time to regain 70% of pre-loss field strength: **6 days**;
- faction-0 leader rate falls from **25.4% baseline to 21.5%**;
- mean faction-0 score share falls from **24.80% to 24.40%**;
- mean territory falls from **2.988 to 2.866**;
- median final field-strength / pre-loss-strength ratio is **0.618**, reflecting later battles after the initial recovery.

The earlier generous lab seam exposed a genuine exploit class: restoring full logistics, preserving destroyed-regiment veterancy, and allowing same-day recruitment could turn annihilation into a beneficial reset and raised the lab runaway rate to 29.3%. That result is not a runtime claim; it demonstrates why replacement semantics must be explicit and regression-tested before balance is locked.

Recommended first test range: **2–4 days** replacement delay, **15–25%** starting-strength retinue, and **450–700 readiness**. Default assumption should be that an annihilated regiment's veterancy dies with that regiment unless the design explicitly models survivors/reconstitution.

Do not solve comeback with hidden anti-leader income bonuses. The optional 50%/70% income floors did not change the median 6-day physical recovery in this shock and only modestly shifted aggregate inequality. Prefer visible, symmetric mechanics: surviving hero/commander, preserved recruitment capacity, local pools, retreat survivors, repairable infrastructure, and explicit replacement timing.

## Resource income and an early wealth lead

Resource-region income was tested at 6, 9, and 12 lab gold/day against a base income of 18. With bounded recovery held constant, mean Gini moved from **0.0829 -> 0.0885 -> 0.0928**. The direction is sensible but the effect is much smaller than changing defeated-army recovery or map compression.

The adversarial "wealthy faction captures two resource regions quickly" shock also remained recoverable: faction-0 leader rate increased from **25.4% to 27.3%**, mean score share from **24.80% to 25.43%**, while the overall runaway diagnostic moved only from **0.1% to 0.2%**.

So early resource ownership creates a real advantage in the present sandbox without automatically becoming a rich-get-richer lock. Keep resource-region daily income in an initial **33–67% of base-income** test band, but tune it only after settlement-local recruitment pools and siege/occupation economics are defined.
## Actual mechanics defects vs tuning-only problems

### Fix before broad content production

| Problem | Classification | Why |
|---|---|---|
| Free, immediate building repair | Mechanics defect | Erases intended recruitment-growth consequence; free-repair shock exactly matched baseline |
| Zero repair can change Ruined -> Damaged at zero integrity | State defect | Condition changes without meaningful restoration |
| Per-settlement Town.AdvanceDay advances global economy/day | Structural defect | Multi-town iteration can multiply day/income/weekly effects |
| RecruitmentPools keyed globally by UnitId | Structural defect | Cannot represent independent finite local pools for same unit family across towns |
| Damaged prerequisite can unlock while below operational integrity | State/rule defect | Near-zero-integrity prerequisite can satisfy construction even though it is not operational |
| Siege supply floors at 200 with no outcome linkage | Mechanics defect | Starvation-only siege cannot resolve |
| No attacker sustainment / siege economic aftermath link | Missing mechanic | Waiting is free and damage lacks campaign-economic consequence |
| SeizeExposedRegion omits recovery penalty | AI mechanics defect | Opportunistic seize beats recover even at zero readiness in representative case |

### Tune only after those are fixed

| Surface | Current conclusion |
|---|---|
| 2–4 AP/day | Real tuning range; 3 remains sensible center |
| Map spacing | Highly sensitive; small maps snowball much more |
| Travel cost | Secondary pacing lever once recovery is bounded; tested 8 -> 12 barely changed the medium-map baseline |
| Memory caution/decay | Current bounded shape works; tune within narrow envelope |
| Repair cost/days | Values are tuning; absence of any cost/time is the defect |
| Siege supply threshold/loss | Values are tuning only after low supply changes meaningful state |
| Veterancy | Direct bonus is fine; total premium must include morale/action effects |
| Construction slots | Explicit design decision first, then tune 1–2 |
| Resource-region income | Tune after local pool and siege economy semantics are correct |

### Still unknown / requires an explicit product rule

- What campaign power hero levels/skills actually add beyond tactical abilities.
- Exact defeated-army replacement/recovery rule.
- Whether construction is intentionally parallel and, if so, what capacity constrains it.
- How siege occupation transfers income, repair liability, and recruitment disruption.
- Representative shipping map route lengths and road density.
## Recommended implementation order

1. **Fix campaign-state semantics before balance tuning:** global day advancement, settlement-local recruitment pool identity, zero/instant repair behavior.
2. **Define defeated-army replacement authority:** what survives (hero, units, veterancy, equipment), delay, replacement retinue, and logistics state. The lab shows this rule can dominate campaign inequality.
3. **Complete siege campaign consequences:** defender low-supply pressure, attacker sustainment, repair/economic aftermath.
4. **Repair the narrow AI scoring omission:** exposed-region seizure must price low readiness/recovery.
5. **Choose town construction concurrency and prerequisite-operational semantics.**
6. **Then rerun balance sweeps using real faction roster costs/growth and representative campaign maps.** Only after those results stabilize should faction-specific dwelling/capstone quantities be locked.

Cheapest high-information test after each mechanics change: rerun this headless suite with the same seeds. Do not spend UE time or build additional campaign content merely to answer balance questions the deterministic model can falsify first.

## Evidence

- `Evidence/BalanceLab/scenario_results.csv` — 1,000-seed adversarial scenarios.
- `Evidence/BalanceLab/sensitivity.csv` — map/AP/travel/resource/recovery sensitivity.
- `Evidence/BalanceLab/convergence.json` — cumulative 100/250/500/1,000/2,000 baseline convergence.
- `Evidence/BalanceLab/replication.json` — independent-seed replication.
- `Evidence/BalanceLab/focused_results.json` — memory, readiness, siege, building loss, capstone, hero, veterancy micro-stresses.
- `Evidence/BalanceLab/source_audit.json` — read-only live-source defect observations.
- `Data/balance_lab_tuning_ranges.json` — proposal-only parameter ranges; not production tuning data.