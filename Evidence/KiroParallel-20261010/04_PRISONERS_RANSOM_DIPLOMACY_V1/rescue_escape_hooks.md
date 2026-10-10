# Rescue & Deterministic Escape Hooks (V1)

**Lane:** 04_PRISONERS_RANSOM_DIPLOMACY_V1
**Source snapshot:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
**Scope:** Lightweight, deterministic ways a captured hero can leave captivity WITHOUT a paid ransom/exchange: battlefield rescue and timed escape. These are "hooks" — minimal surface in V1, extensible later.

Tags: **[VERIFIED]** VERIFIED FROM CURRENT SNAPSHOT · **[PROPOSAL]** DESIGN PROPOSAL · **[LOCAL]** LOCAL RUNTIME-ASSET CHECK REQUIRED.

---

## 1. Design stance

**[PROPOSAL]** Rescue and escape are the **non-transactional** release paths. They exist so a captured hero is never a pure "pay or lose forever" dead end, and so military action against the captor has a hero-recovery payoff. Both must be **deterministic** (no RNG), reuse the single release transition `FSoulHeroRules::Release` (see `exchange_rules.md` §1), and persist only through the existing save authority.

---

## 2. Deterministic escape

**[VERIFIED]** The capture record already carries everything needed for a timed escape: `CaptorFaction`, `CaptureRegion`, and (V1-added) `CaptureDay`. The per-day tick already exists: `AdvanceDay` calls `FSoulHeroRules::AdvanceRecovery(Hero)` and `AdvanceRecovery(DwarfCommander)` for the Heartland profile (`SoulFounderPlaytestStateSubsystem.cpp` ~line 644).

**[PROPOSAL]** Add a captured branch to the daily tick. The cleanest placement is to EXTEND `FSoulHeroRules::AdvanceRecovery` to also handle `Captured`, OR add a sibling `FSoulHeroRules::AdvanceCaptivity(Hero, int32 Day, bool bCaptorHoldsCaptureRegion)` called right next to it. Preference: a sibling function, so wound recovery semantics stay untouched and reviewable.

**Deterministic escape rule (no RNG):**
```
AdvanceCaptivity(Hero, Day, bCaptorHoldsCaptureRegion):
  if Hero.Condition != Captured: return
  daysHeld = Day - Hero.CaptureDay
  // Escape becomes possible only if the captor has LOST control of the capture region
  // (a deterministic, player-observable condition), AND enough time has passed.
  if !bCaptorHoldsCaptureRegion && daysHeld >= 5:
      // caller resolves release region; escape returns hero to nearest owned/home region
      Release(Hero, <resolved release region>)   // -> Wounded, 3-day clock
```

**[PROPOSAL]** Rationale for the gate:
- Escape keyed to the **captor losing the capture region** makes escape a *consequence of the campaign map*, not a dice roll. The player can engineer an escape by retaking/contesting the region (`FSoulWorldRules::Capture` already changes region ownership — **[VERIFIED]** `SoulWorld.h:38`).
- The `daysHeld >= 5` floor prevents instant escape and keeps ransom relevant in the early window.
- Fully deterministic: given the same save + the same region ownership timeline, the escape day is identical on every machine/reload.

**[PROPOSAL]** `bCaptorHoldsCaptureRegion` is computed at the call site as `World.Regions.FindChecked(Hero.CaptureRegion).OwnerFactionId == Hero.CaptorFaction` (**[VERIFIED]** region ownership lives in `World.Regions[...].OwnerFactionId`, used throughout `ApplyBattleResult`). No new world state.

---

## 3. Rescue hook (battlefield)

**[VERIFIED]** Battles resolve through `FSoulCampaignBattleResult` -> `ApplyBattleResult` (`SoulFounderPlaytestStateSubsystem.cpp:550-626`). Region ownership flips to the winner via `FSoulWorldRules::Capture` (lines ~615, ~597).

**[PROPOSAL]** V1 rescue is **implicit and emergent**, requiring no new battle-result field:
- When the owner (or an ally — see Unknowns) WINS a battle whose `TargetRegion == Hero.CaptureRegion`, the captor no longer holds the capture region after `Capture`. On the *next* `AdvanceDay`, the escape rule in §2 fires (region no longer captor-held). This gives a clean "liberate the holding region to free your hero" loop using only existing systems.
- **[PROPOSAL]** OPTIONAL immediate-rescue sugar (defer if risky): inside `ApplyBattleResult`, after `Capture` flips `TargetRegion` to the player, if any persisted hero has `Condition==Captured && CaptureRegion==TargetRegion && CaptorFaction==(defeated faction)`, call `Release` immediately with `ReleaseRegion=TargetRegion`. This is a 3-line addition guarded by `bHeartlandEnabled`. It is listed as OPTIONAL because it touches the qualified battle-apply path; **[LOCAL]** confirm it does not interfere with Human Capital Siege V0 apply ordering before enabling.

**[PROPOSAL]** No dedicated "rescue mission" minigame or new map objective in V1. Rescue == winning at the capture region. This keeps V1 lightweight (lane requirement: "lightweight rescue hooks").

---

## 4. Interaction with wounds and availability

**[PROPOSAL]** Escape/rescue both route through `FSoulHeroRules::Release`, so a freed hero is `Wounded(3)` and excluded from command/diplomacy via the existing `IsAvailable` gate (**[VERIFIED]** `SoulHero.h:35-37`). An escaped hero cannot immediately be recaptured (anti-churn, consistent with `ai_prisoner_behavior.md` §4.1).

**[PROPOSAL]** If a hero escapes/rescues while a `RansomProcess` was open, `Release` resets the process to `None` (built into the Release body) — a freed hero cannot still owe a ransom. Deterministic and atomic.

---

## 5. Persistence

**[PROPOSAL]** Escape/rescue mutate only `FSoulHeroState` fields that are ALREADY (or V1-additively) persisted via the single `Soul.Campaign` domain. No new save domain. `CaptureDay` is persisted (see `prisoner_state_schema.json`) so the escape clock survives save/load exactly. A save taken mid-captivity and reloaded yields the identical escape day — directly testable (see `HANDOFF.md` acceptance tests).

---

## 6. Explicit extensibility (post-V1, not built now)

- A future "rescue party" could reuse Lane 3 companion roles to raise the escape probability floor; V1 deliberately leaves escape as region-ownership-gated.
- A future dungeon/holding-site could use the capture region's settlement; V1 uses the abstract region only.
- Allied rescue (a faction at Peace freeing your hero) depends on multi-faction alliance mechanics that do not exist in Diplomacy V0; recorded in Unknowns.
