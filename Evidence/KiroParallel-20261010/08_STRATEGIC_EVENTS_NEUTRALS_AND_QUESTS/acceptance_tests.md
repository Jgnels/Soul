# Acceptance Tests — Lane 8 (Strategic Events, Neutral Sites, Quests)

**Authority label:** DESIGN PROPOSAL
**Source snapshot:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
**Test style:** mirrors existing deterministic C++ test suites (e.g. `Source/Soul/Private/Tests/SoulSettlementSaveTests.cpp`, `SoulHeartlandTests.cpp`, `SoulMechanicsTests.cpp`). All tests are **pure/deterministic**, no RNG except seeded `FRandomStream`.

Each test lists: ID · intent · setup · action · expected · which VERIFIED authority it guards.

---

## A. Schema / data integrity (data-only, no engine)

- **T-A1 Region ids resolve.** Every `region` in `heartland_activity_expansion.json` and every spawned `scope_region` resolves to a node id in `soul_world_overmap_v1_20260922.json`. *(Guards: no invented regions.)*
- **T-A2 Backward-compat mapping.** The three existing `heartland.*` sites, when expressed via `activity_site_definition`, reproduce the exact current `FSoulHeartlandSite` fields (Id, Region, Effect→reward.kind, Amount→reward.amount, ActionCost→action_cost). *(Guards: current site behaviour unchanged.)*
- **T-A3 Effect whitelist for recurring collect.** Any activity with `interaction=collect` and `reward.kind` not in {gold, mana} is rejected by the loader (matches current `FSoulHeartlandContent::Load` validation). *(Guards: loader parity.)*
- **T-A4 No /Game path asserted.** No field in any Lane 8 JSON contains a `/Game/...` string. *(Guards: no invented asset paths.)*

## B. Site collection parity (generalised `InteractActivitySite`)

- **T-B1 Collect == legacy.** `InteractActivitySite("heartland.windmill")` produces byte-identical economy delta and message as the current `InteractHeartlandSite` on the same state. *(Guards: no regression in the existing feature.)*
- **T-B2 Ownership gate.** A `collect`/`toll` site with `requires_region_owned=true` is rejected when `World.Regions[r].OwnerFactionId != PlayerFaction` or `GetPlayerTroopCount()<=0`. *(Guards: existing gate at InteractHeartlandSite SoulFounderPlaytestStateSubsystem.cpp:988.)*
- **T-B3 Per-day throttle.** A `recurring` site collected on Day D is rejected again on Day D; allowed after `AdvanceDay()` increments to D+1. *(Guards: `activity_site_days` mirrors `HeartlandSiteDays`.)*
- **T-B4 One-shot depletion.** A `recurring=false` site transitions `available → depleted` after first success and is rejected thereafter, across a save/load. *(Guards: one-shot persistence.)*
- **T-B5 AP spend.** Collection fails with no state change when `Economy.ActionPoints < action_cost` (uses `FSoulCampaignRules::SpendAction`). *(Guards: no free actions.)*

## C. Clear-then-collect through the existing battle bridge

- **T-C1 Battle required.** A `contested` site (`interaction=clear_then_collect`/`intercept`) cannot be collected until a battle vs its occupant resolves as a win; the reward is granted only after `HandleBattleResolved` reports victory. *(Guards: no second combat authority — resolution is the existing bridge.)*
- **T-C2 Loss leaves site contested.** On a losing battle, the site stays `contested` and no reward is granted. *(Guards: no reward leakage.)*

## D. Event determinism & anti-spam

- **T-D1 Reproducibility.** With fixed `campaign_event_seed`, running `EvaluateStrategicEvents` across Days 1–30 twice yields identical event sequences. *(Guards: FRandomStream(FCrc::StrCrc32 ^ seed ^ Day) determinism.)*
- **T-D2 Save/load determinism.** Saving at Day K, reloading, and advancing to Day 30 yields the same events as an uninterrupted run. *(Guards: `campaign_event_seed` + `fired_events_by_day` persistence.)*
- **T-D3 Per-day cap.** No more than `max_events_per_day` events fire on any Day for the tested owned-region count. *(Guards: anti-spam.)*
- **T-D4 Cooldown.** The same event id cannot fire twice within `cooldown_days`. *(Guards: anti-spam.)*
- **T-D5 Earliest-day gate.** No event fires before its `earliest_day`. *(Guards: onboarding pacing.)*
- **T-D6 Decline is safe.** Declining any choice-event produces no negative economy/relation delta. *(Guards: Nature/Dark passivity compatibility.)*
- **T-D7 Transient expiry.** A spawned caravan/convoy with `window_days=W` transitions to `expired` exactly W days after spawn if not resolved; no reward. *(Guards: time-limited correctness.)*

## E. Quest advancement (deterministic)

- **T-E1 quest_progress persists.** A quest at step 2 reloads at step 2; an unknown quest id reloads as step 0. *(Guards: tolerant save restore.)*
- **T-E2 Step kinds.** Each of the six step kinds advances exactly when its VERIFIED condition is met and not before (interact_site, clear_occupant, hold_region, reach_region, relation_at_least, reclaim_region). 
- **T-E3 hold_region reset.** Losing the region before N consecutive days resets the hold counter. *(Guards: no partial-credit exploit.)*
- **T-E4 No RNG in quests.** Quest advancement is identical across two runs with DIFFERENT event seeds but identical player actions. *(Guards: quests are action-driven, not random.)*
- **T-E5 BLOCKED rewards absent.** No quest grants a `relic_item` (BLOCKED). *(Guards: no invented item authority.)*

## F. AI interest (deterministic scorer)

- **T-F1 Bonus applied.** Adding an activity with `resource_value_bonus=B` raises the hosting region's `FSoulStrategicRegion.ResourceValue` by exactly B in the assembled snapshot. *(Guards: data-fed AI integration.)*
- **T-F2 Scorer purity preserved.** `FSoulStrategyAI::Choose` output is identical for identical snapshots after Lane 8 wiring (no new nondeterminism). *(Guards: AI determinism.)*
- **T-F3 Threat events raise defence.** An event with `raise_threat` increases `DefendOwnedRegion` candidate score for the owning faction. 
- **T-F4 Passivity preserved.** Nature/Dark interest down-weighting leaves their chosen action unchanged vs the pre-Lane-8 baseline for a neutral opportunity. *(Guards: no accidental aggression.)*
- **T-F5 No synthetic armies.** No test path causes `AdvanceEnemyAI` to spawn a strategic army (preserves "garrisons hold; no synthetic strategic army bypasses encounter resolution").

## G. Save-domain safety

- **T-G1 Single domain.** Lane 8 adds NO new `IRBSaveDomainProvider`; all new fields live in the `Soul.Campaign` blob at schema 1. *(Guards: AGENTS.md single-save-authority rule.)*
- **T-G2 Old-save tolerance.** A pre-Lane-8 save (no activity/quest/event keys) loads successfully with all new state at defaults. *(Guards: additive, tolerant restore.)*

---

## Minimum green bar for V0 admission
T-A1..A4, T-B1..B3, T-D1..D5, T-E1, T-G1..G2 must pass. (C/F/E4+/D6-7 depend on the battle-bridge-for-neutrals and event-choice-UI unknowns and may be deferred with explicit LOCAL RUNTIME-ASSET CHECK notes.)
