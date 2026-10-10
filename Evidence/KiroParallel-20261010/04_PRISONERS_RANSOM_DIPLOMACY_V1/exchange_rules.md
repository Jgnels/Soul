# Prisoner Exchange, Release Locations & Peace-With-Prisoners Rules (V1)

**Lane:** 04_PRISONERS_RANSOM_DIPLOMACY_V1
**Source snapshot:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
**Scope:** How captured heroes change hands and where they return. Extends existing `ESoulHeroCondition::Captured` + Diplomacy V0.

Every statement is tagged:
- **[VERIFIED]** = VERIFIED FROM CURRENT SNAPSHOT
- **[PROPOSAL]** = DESIGN PROPOSAL
- **[LOCAL]** = LOCAL RUNTIME-ASSET CHECK REQUIRED

---

## 1. Transaction primitives (how a captured hero leaves `Captured`)

**[VERIFIED]** Today there is exactly ONE way out of `Captured`: none. `FSoulHeroRules::AdvanceRecovery` (`Source/SoulCore/Private/SoulHero.cpp:63-68`) returns immediately unless `Condition==Wounded`. A captured hero is permanently captured in the current build. This is the gap V1 closes.

**[PROPOSAL]** V1 adds **three** release transactions, each a pure state transition on `FSoulHeroState` plus a gold/movement cost settled through the existing `FSoulCampaignRules::SpendAction` / treasury pattern used by `ExecuteDiplomacy` (`Source/Soul/Private/SoulHeartlandDiplomacy.cpp:25-36`):

| Transaction | Initiator | Cost | Effect on prisoner |
|---|---|---|---|
| **Ransom (owner pays captor)** | Owner | `ValueGold` to captor treasury + 1 movement | `Captured -> Wounded(RecoveryDays=3)` at a release region |
| **Pay AI demand** | Owner (player) | `demandGold` to captor + 1 movement | same as ransom |
| **Prisoner exchange** | Either side | 1 movement, no gold (or gold top-up if values differ) | BOTH prisoners `Captured -> Wounded` at their own release regions |

**[PROPOSAL]** A released hero returns as **`Wounded` with `RecoveryDays=3`**, NOT `Healthy`. Rationale: (a) a freed prisoner should not instantly command/cast — this reuses the existing availability gate `FSoulHeroRules::IsAvailable` (**[VERIFIED]** `SoulHero.h:35`, which already blocks siege command and diplomacy for non-Healthy heroes); (b) it keeps the recovery clock machinery (`AdvanceRecovery`) as the single healing authority, so release introduces no new per-day tick. On release we clear `CaptorFaction`, `CaptureRegion`, `CaptureDay`, `RansomProcess`, `RansomAskGold`, `ExchangeCounterpartHeroId` to their default/None values.

**[PROPOSAL]** The release transition belongs in a new `FSoulHeroRules::Release(FSoulHeroState& Hero, FName ReleaseRegion)` so it sits beside `ApplyBattleInjury` / `AdvanceRecovery` and is unit-testable headless. Body:
```
void FSoulHeroRules::Release(FSoulHeroState& Hero, FName ReleaseRegion)
{
    if (Hero.Condition != ESoulHeroCondition::Captured) return;   // atomic no-op guard
    Hero.Condition      = ESoulHeroCondition::Wounded;
    Hero.RecoveryDays   = 3;
    Hero.CaptorFaction  = NAME_None;
    Hero.CaptureRegion  = NAME_None;
    // V1 additive fields reset:
    Hero.CaptureDay     = 0;
    Hero.RansomProcess  = ESoulPrisonerProcess::None;
    Hero.RansomAskGold  = 0;
    Hero.ExchangeCounterpartHeroId = NAME_None;
}
```
This keeps the post-release invariants identical to a normal wounded hero, which the save-restore validator already accepts (**[VERIFIED]** non-captured => no captor/region, `RecoveryDays>0` for wounded — `SoulFounderPlaytestStateSubsystem.cpp` ~line 859-861).

---

## 2. Release locations (where the freed hero appears)

**[VERIFIED]** `FSoulHeroState` has no residence field; current region is implicit. For the player Hero, the campaign tracks `PlayerRegion` (**[VERIFIED]** `SoulFounderPlaytestStateSubsystem.h:88`).

**[PROPOSAL]** Deterministic release-region selection, in priority order (first match wins, no RNG):
1. **Owner's current staging region** if owner has an owned region adjacent to `CaptureRegion` (reuse `FSoulWorldRules` adjacency; **[LOCAL]** confirm the adjacency helper name/signature in `SoulWorld.h` at integration time).
2. Else the owner's **capital/home region**. For the human player this is the Heartland capital region used as `PlayerRegion` start; for the Dwarf commander, the dwarf home region. **[LOCAL]** confirm the exact home-region FName from the active scenario at integration (do not hardcode an invented FName).
3. Else `CaptureRegion` itself (fallback; the hero is freed on the spot).

**[PROPOSAL]** The chosen release region is validated against `World.Regions.Contains(...)` before use, exactly like the save validator checks `CaptureRegion` (**[VERIFIED]** `SoulFounderPlaytestStateSubsystem.cpp` ~line 860). An invalid computed region falls back to rule 3.

**[PROPOSAL]** For a **prisoner exchange**, each side's freed hero uses ITS OWN owner's release region (two independent resolutions). Neither hero teleports into enemy territory.

---

## 3. Prisoner exchange rules

**[PROPOSAL]** An exchange is proposed only when BOTH sides currently hold a captured hero of the other (derived from the ledger view in `prisoner_state_schema.json`). V1 supports the player exchanging with one AI faction at a time.

**Value balancing (deterministic):**
- Let `Vp` = `ValueGold` of the player's captured hero (held by AI), `Va` = `ValueGold` of the AI's captured hero (held by player).
- **Even swap** if `abs(Vp - Va) <= 300` (one unit-cost band): 0 gold, 1 movement each.
- Otherwise the side receiving the MORE valuable prisoner **tops up** `abs(Vp - Va)` gold to the other side. Top-up is clamped and affordability-gated like a Gift.
- AI acceptance uses `EvaluateRansomOffer` with `offeredGold = Va + topUp` against `valueGold = Vp` (treat the traded prisoner's value as payment-in-kind). Threshold 500, fully previewable.

**[PROPOSAL]** On acceptance (admission-before-mutation, see §6): both `FSoulHeroRules::Release` calls fire, gold top-up transfers, 1 movement spent, and a relationship bump applies (see `diplomacy_v1_integration.md`). On any failure, nothing mutates.

---

## 4. Siege-capture consequences

**[VERIFIED]** Siege results already flow through `FSoulCampaignBattleResult` (`bCourtyardCaptured`, `SiegeGateRemaining`) and `ApplyBattleResult` -> `ApplySiegeAftermath` (`SoulFounderPlaytestStateSubsystem.cpp` ~lines 553-562), and hero wound/capture already flows through the same result via `ApplyHeroInjury` (~lines 564-577). **[VERIFIED]** Hero capture occurs iff the losing side has `ArmySurvivors==0` (`SoulHero.cpp:58`).

**[PROPOSAL]** Siege-capture needs **no new capture path** — a defender hero whose army is annihilated during a siege is already captured by the existing `ApplyBattleInjury(ArmySurvivors==0)` rule. V1 only adds the *consequence*: if the captured hero was the settlement's commander, the captor gains a **ransom-leverage bonus** (`+10%` to `ValueGold` while the captor also holds the hero's home settlement region). This is a deterministic multiplier applied in `EvaluateRansomOffer`, not a new state. **[LOCAL]** confirm the commander<->settlement linkage at integration (Human Capital Siege V0 is qualified and MUST NOT be changed — this only reads its result).

---

## 5. Peace-with-prisoners interactions

**[PROPOSAL]** Making peace while prisoners are held must not silently free or orphan them. Rules:
- **Offering Peace does NOT release prisoners.** Peace and prisoner status are orthogonal. (A captured hero's `CanPerformDiplomacy` is already false — **[VERIFIED]** `SoulHero.h:37` — so a captured hero cannot itself negotiate.)
- **Peace unlocks a discounted ransom:** while `EffectiveStance==Peace` or `NonAggression`, `EvaluateRansomOffer` drops the `war_premium` penalty (it is `0` for non-War, **[PROPOSAL]** per `ransom_scoring.json`). So peace makes freeing prisoners easier, giving the player a reason to pursue peace.
- **Breaking a treaty while holding the enemy's hero** records betrayal exactly as today (**[VERIFIED]** `BreakTreaty` sets `BetrayalDay`, `SoulDiplomacy.cpp:52`) and, **[PROPOSAL]**, cancels any `OfferOpen`/`AgreedPendingPayment` ransom process on that hero (reset to `None`) — a betrayer cannot keep a pending honorable ransom. Deterministic, atomic.
- **A standing AI ransom demand persists across peace changes** (it is a per-hero field, not a relation field), but its *acceptance score* is recomputed from current stance each preview, so peace immediately changes the price.

---

## 6. Admission-before-mutation (non-negotiable)

**[VERIFIED]** The entire codebase uses "validate into temporaries, mutate authority only when fully valid" — see `ExecuteDiplomacy` (`SoulHeartlandDiplomacy.cpp:25-36`: it applies to COPIES `Relation`/`Paid` and only commits with `Economy=MoveTemp(Paid); HumanRelations.Add(...)`) and `RestoreRBSaveDomain_Implementation` (`SoulFounderPlaytestStateSubsystem.cpp:731-903`).

**[PROPOSAL]** Every ransom/exchange transaction MUST follow the same shape:
1. `PreviewRansom(...)` / `PreviewExchange(...)` — pure, const, returns an explainable decision; mutates nothing.
2. `ExecuteRansom(...)` / `ExecuteExchange(...)` — re-previews; if accepted, applies to copies of `Economy`/`Relation` and the hero(es); only on full success commits with `MoveTemp`. Any mid-step failure returns false and leaves `HeartlandSnapshot(S)` byte-identical (this is directly testable — see `acceptance_tests` in `HANDOFF.md`).
