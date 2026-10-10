# Acceptance Tests — Campaign Victory & Objectives

Lane: `01_CAMPAIGN_VICTORY_AND_OBJECTIVES`
Source snapshot: `Jgnels/Soul` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
Legend: **[VCS]** Verified from Current Snapshot · **[LCR]** Local Runtime/Asset Check Required · **[DP]** Design Proposal

These are the acceptance criteria Codex implements against. They are written as deterministic, state-only assertions so they can live as **SoulCore unit tests** (mirroring `Source/SoulCore/Private/Tests/SoulMechanicsTests.cpp`) plus a small number of **subsystem integration tests**. No RNG, no wall-clock, no presentation dependency.

Test tiers:
- **T-UNIT** — pure `FSoulVictoryRules` / `FSoulCampaignObjectiveRules` over a hand-built `FSoulVictoryWorldFacts`. No Unreal world.
- **T-SUB** — `USoulFounderPlaytestStateSubsystem` driven through `AdvanceDay` / `ApplyBattleResult`, asserting on `CampaignVictory`.
- **T-SAVE** — RBSave round-trip of the `Soul.Campaign` domain at schema 2.
- **T-MANUAL/[LCR]** — HUD/asset checks that require a local Unreal editor/runtime.

---

## A. Determinism & authority (gate on everything)

- **AC-D1 [T-UNIT]** `FSoulVictoryRules::Evaluate` called twice on an identical `FSoulVictoryWorldFacts` yields byte-identical `FSoulCampaignVictoryState`. No static mutable state, no RNG stream touched.
- **AC-D2 [T-UNIT]** All thresholds compared are integers/permille/millis; no `float`/`double` appears in the victory/objective structs or rules (grep assertion in test review).
- **AC-D3 [T-SUB]** No new `IRBSaveDomainProvider` is registered. `GetRBSaveDomainId_Implementation` still returns exactly `"Soul.Campaign"`. (Guards the "no second save authority" rule.) **[VCS]**
- **AC-D4 [T-SUB]** `FSoulWorldRules::Capture` remains the only call site that flips `OwnerFactionId`; the victory layer never writes region ownership. **[VCS]**

## B. V0 Military Domination (founder slice)

- **AC-V0-1 [T-SUB]** Starting the founder micro, `CampaignVictory.ActiveApproachMask` has **only** `MilitaryDomination` set; `AchievedApproach == None`.
- **AC-V0-2 [T-SUB]** Driving a winning `ApplyBattleResult` whose `TargetRegion == orc_camp` (the slice `enemy_primary_region`) sets `AchievedApproach == MilitaryDomination` and `AchievedDay == Economy.Day`. **[VCS]** this is the region `ApplyBattleResult` already captures.
- **AC-V0-3 [T-SUB]** Capturing a non-objective slice region (e.g. `orc_watch`) does **not** set `AchievedApproach` (progress ring may rise, but no win).
- **AC-V0-4 [T-UNIT]** With founder-slice facts where the player owns `orc_camp`, `Evaluate` returns `AchievedApproach == MilitaryDomination`; with ownership reverted, it returns `None` (idempotent, reversible before `AchievedDay` is latched).
- **AC-V0-5 [T-SUB]** Once `AchievedApproach` is latched, subsequent `AdvanceDay` calls do not clear or overwrite it.

## C. Loss & recovery

- **AC-L1 [T-SUB]** When the player's owned regions among its `starting_possessions` drop to zero **and** total owned regions == 0, `CampaignVictory.RecoveryDeadlineDay == Economy.Day + 5` and no final-loss flag is set yet (recoverable). **[DP]**
- **AC-L2 [T-SUB]** If the player captures **any** region before `Economy.Day` reaches `RecoveryDeadlineDay`, `RecoveryDeadlineDay` resets to 0 (recovered) and the campaign continues.
- **AC-L3 [T-SUB]** If `AdvanceDay` advances to `RecoveryDeadlineDay` with still-zero owned regions, a final-loss state is set exactly once and is stable across further `AdvanceDay` calls.
- **AC-L4 [T-SUB]** A Captured hero alone (seat still owned) does **not** trigger any loss or recovery state. **[VCS]** `FSoulHeroRules::AdvanceRecovery` continues to run independently.

## D. V1 approach conditions (six-faction, behind ActiveApproachMask)

- **AC-V1-MIL [T-UNIT]** Facts with player owning ≥ 60‰·36 regions (≥22) **and** every rival seat → `MilitaryDomination` achieved; owning 21 regions with one rival seat still held → not achieved.
- **AC-V1-LRS [T-UNIT]** Facts where every rival *playable* faction owns zero regions → `LastRealmStanding` achieved; passive factions (nature/dark) still owning regions do **not** block it. **[DP]**
- **AC-V1-MAG [T-UNIT]** Facts: player owns ≥2 arcane-shrine regions, arcane building operational, `Hero.Level>=8`, `KnownSpells>=6`, sustained ≥7 days → `MagicalSupremacy` achieved. Dropping any single input below threshold on day N resets `SustainStartDayByApproach[MagicalSupremacy]` to 0.
- **AC-V1-ECO [T-UNIT]** Facts: ≥4 strategic sites owned, market operational, `gold>=8000`, sustained ≥7 days → `EconomicSiteControl` achieved. Treasury dipping below threshold mid-sustain resets the sustain clock.
- **AC-V1-ALL [T-UNIT]** Facts: ≥3 playable factions at Peace with RelationPermille≥750 and no playable faction at War with player, sustained ≥10 days → `AllianceFederation` achieved. **Depends on Lane 4**; test uses a stubbed `HumanRelations` ledger and asserts the predicate only (no diplomacy mechanics invented here).
- **AC-V1-SUSTAIN [T-UNIT]** For every sustained condition, achievement fires on the day `Economy.Day - SustainStartDay >= required_days`, not before.

## E. Objectives (soft goals)

- **AC-O1 [T-UNIT]** `FSoulCampaignObjectiveRules::EvaluateGoal` for `OwnRegionCount{min=3}` returns complete iff facts show ≥3 owned regions; never ends the game.
- **AC-O2 [T-SUB]** Completing/failing any Day 1-7 or 8-20 objective never mutates `AchievedApproach` and never sets a loss state (objectives are guidance only). **[DP]**
- **AC-O3 [T-UNIT]** `ReachHeroLevel{5, spells>=3}` reads `FSoulHeroState.Level` and `KnownSpells.Num()`; completion is monotonic (can't un-complete by losing a battle). **[VCS]** hero fields.
- **AC-O4 [T-UNIT]** `WinSiege{min=1}` increments only on an `ApplyBattleResult` with `bSiege==true && bPlayerWon`; founder Human Capital Siege V0 is unchanged. **[VCS]** `LastBattleResult.bSiege`.

## F. Endgame escalation (V2)

- **AC-E1 [T-SUB]** `EndgameEscalationDay` is set exactly once, on the first `AdvanceDay` where `Economy.Day >= 21` **or** any faction crosses 400‰ region share.
- **AC-E2 [T-UNIT]** Occupation-unrest reduces income contribution only for regions where `OwnerFactionId != homeland_affinity`; homeland regions are unaffected. **[DP]** (homeland_affinity from `soul_campaign_start_state_v1_20260922.json`).

## G. Save / schema (T-SAVE)

- **AC-S1 [T-SAVE]** `GetRBSaveSchemaVersion_Implementation() == 2`. **[VCS]** currently returns `1`.
- **AC-S2 [T-SAVE]** Capture→Restore of the `Soul.Campaign` domain preserves every `FSoulCampaignVictoryState` field exactly (approach mask, achieved, achieved day, recovery deadline, progress map, sustain map, escalation day).
- **AC-S3 [T-SAVE]** Loading a **schema-1** save (no `"victory"` object) yields a default-constructed `CampaignVictory` with no error and no false win/loss. **[VCS]** back-compat.
- **AC-S4 [T-SAVE]** F5 checkpoint / F9 reload during the founder slice reproduces `AchievedApproach` bit-for-bit (parity with the existing siege save-exactness proof pattern in `SoulHumanCapitalSiegeQualification.cpp`).

## H. Presentation (manual / local)

- **AC-H1 [T-MANUAL/LCR]** Objective line and victory/recovery banner render on the existing Kenney HUD (`SoulHUDArt.h`/`SoulHUDTheme.h`) without introducing a second UI stack.
- **AC-H2 [T-MANUAL/LCR]** If owned icon packs (LayerLab Casual/Reward Chest, 34IB Modern Casual UI) are used for approach icons, their runtime import is confirmed by a local asset-registry check; **no `/Game/...` path is asserted until that check passes.** **[LCR]**

---

## Minimum test matrix for V0 sign-off

| ID | Tier | Must pass for V0 |
|---|---|---|
| AC-D1..D4 | UNIT/SUB | yes |
| AC-V0-1..5 | SUB | yes |
| AC-L1..L4 | SUB | yes |
| AC-O1..O4 | UNIT/SUB | yes |
| AC-S1..S4 | SAVE | yes |
| AC-V1-* | UNIT | **no** (V1) |
| AC-E1..E2 | UNIT/SUB | **no** (V2) |
| AC-H1..H2 | MANUAL/LCR | **no** (needs local Unreal) |
