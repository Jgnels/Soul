# Diplomacy V1 Integration: Prisoners, Memory & Relationship Effects

**Lane:** 04_PRISONERS_RANSOM_DIPLOMACY_V1
**Source snapshot:** `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
**Scope:** How the prisoner lifecycle plugs into the existing Diplomacy V0 relationship/memory model, and the exact files/symbols Codex should touch. **No second diplomacy authority.**

Tags: **[VERIFIED]** VERIFIED FROM CURRENT SNAPSHOT · **[PROPOSAL]** DESIGN PROPOSAL · **[LOCAL]** LOCAL RUNTIME-ASSET CHECK REQUIRED.

---

## 1. The existing Diplomacy V0 model (what we extend)

**[VERIFIED]** `Source/SoulCore/Public/SoulDiplomacy.h`:
- `enum ESoulDiplomaticStance { War, Peace, NonAggression }`.
- `enum ESoulDiplomaticAction { DeclareWar, Peace, NonAggression, Gift, BreakTreaty }`.
- `struct FSoulDiplomaticRelation { ESoulDiplomaticStance Stance=War; int32 RelationPermille=0, PactUntilDay=0, BetrayalDay=0, LastActionDay=0; }`.
- `struct FSoulDiplomaticDecision { bool bAccepted; int32 ScorePermille; TMap<FName,int32> Components; TArray<FString> Reasons; }` — the explainable payload shape prisoner actions should reuse.
- `class FSoulDiplomacyRules { EffectiveStance; Evaluate; Apply; Valid; StanceName; }`.

**[VERIFIED]** Relations are stored per-target-faction in `USoulFounderPlaytestStateSubsystem::HumanRelations : TMap<FName,FSoulDiplomaticRelation>` (`SoulFounderPlaytestStateSubsystem.h:44`), persisted via `CaptureDiplomacy`/`ValidateDiplomacy` (`SoulHeartlandDiplomacy.cpp:37-68`). Targets are exactly `dwarves`, `orcs`, `vikings` (`IsDiplomacyTarget`, `SoulHeartlandDiplomacy.cpp:4-5`). `RelationPermille` range is `[-1000, 1000]`.

**[VERIFIED]** The ONLY existing memory mechanism is `BetrayalDay`, decaying over 10 days (`SoulDiplomacy.cpp:27-28`). Relation deltas today: Gift `+100`, DeclareWar `-100`, BreakTreaty `-300` (`SoulDiplomacy.cpp:42-52`).

**[VERIFIED]** `RelationPermille` is persisted as `relation` and validated to `[-1000,1000]` on restore (`SoulHeartlandDiplomacy.cpp:60`). Save round-trip and adversarial rejection are already tested (`SoulHeartlandTests.cpp` `FHeartlandDiplomacySaveTest`).

---

## 2. Relationship effects of prisoner events (deterministic deltas)

**[PROPOSAL]** Prisoner outcomes adjust `RelationPermille` using the SAME fixed-scale integer style as existing actions, clamped to `[-1000,1000]`. These are the ONLY new relation deltas, chosen to be smaller than the structural actions so they tune relations without overriding war/peace swings:

| Event | RelationPermille delta (both captor<->owner) | Rationale |
|---|---|---|
| **Honorable ransom paid & hero released** | `+80` | Paying/accepting a fair ransom builds a working relationship. |
| **Prisoner exchange completed** | `+120` | A mutual exchange is the warmest prisoner outcome; it signals parity and good faith. |
| **AI ransom demand paid by player** | `+40` | Transactional goodwill, smaller than a negotiated ransom. |
| **Capturing an enemy hero in battle** | `-60` | Taking a prominent prisoner hardens the loser's stance. |
| **Breaking a treaty while holding the enemy's hero** | existing `-300` + records `BetrayalDay` (UNCHANGED) + cancels pending ransom process | Reuses existing BreakTreaty; no new penalty, just the process-cancel side effect. |

**[PROPOSAL]** All deltas apply via a small helper mirroring `FSoulDiplomacyRules::Apply`'s clamp pattern (`FMath::Clamp(R.RelationPermille + Delta, -1000, 1000)`), executed on a COPY of the relation and only committed on full transaction success.

---

## 3. Memory extension (minimal, optional)

**[PROPOSAL]** V0's only memory is `BetrayalDay`. For prisoners, V1 can either:
- **(A, preferred for V1):** add NO new relation field. Prisoner "memory" is implicit in the already-persisted `CaptorFaction` + `RelationPermille` + `BetrayalDay`. The fact that faction X holds your hero is itself the memory. This keeps the `FSoulDiplomaticRelation` struct and its save validator (`ValidateDiplomacy` expects exactly 5 fields — **[VERIFIED]** `SoulHeartlandDiplomacy.cpp:57`) UNCHANGED. **Strongly recommended.**
- **(B, deferred):** add an optional `LastRansomDay`/`PrisonerGrudge` field to `FSoulDiplomaticRelation`. This would require bumping the diplomacy field count check (`!=5`) and extending `CaptureDiplomacy`/`ValidateDiplomacy`. Recorded as a future option; NOT built in V1 to avoid touching the qualified diplomacy save contract.

**[PROPOSAL]** Decision: **adopt (A).** No change to `FSoulDiplomaticRelation` or its serialization. Prisoner state lives entirely on `FSoulHeroState` (per `prisoner_state_schema.json`), diplomacy relation effects are plain `RelationPermille` deltas.

---

## 4. New diplomacy-adjacent actions (UI + subsystem)

**[VERIFIED]** The Diplomacy panel today renders 5 actions (`DiplomacyAction:0..4`) and routes them through `ASoulFounderPlaytestHUD::NotifyHitBoxClick` -> `State->ExecuteDiplomacy(...)` (`SoulFounderPlaytestHUD.cpp:289-301`). Panel state (`bDiplomacyPanel`, `DiplomaticFaction`, `LastMessage`) lives on `ASoulFounderPlaytestCampaignActor` (`SoulFounderPlaytestCampaignActor.cpp:476-480`, etc.). Preview text comes from `S->PreviewDiplomacy(...)` (`SoulFounderPlaytestHUD.cpp:108`).

**[PROPOSAL]** Prisoner actions attach as a **Prisoners sub-section of the existing Diplomacy panel**, NOT a new panel — this reuses the panel's movement/battle/persistence guards and the hero-availability note. New subsystem methods, const-correct and mirroring the diplomacy pair:
- `FSoulPrisonerDecision PreviewRansom(FName Faction, FName HeroId) const`
- `bool ExecuteRansom(FName Faction, FName HeroId, FString& Message)`
- `FSoulPrisonerDecision PreviewExchange(FName Faction, FName OwnHeroId, FName TheirHeroId) const`
- `bool ExecuteExchange(FName Faction, FName OwnHeroId, FName TheirHeroId, FString& Message)`
- `bool AcceptRansomDemand(FName Faction, FName HeroId, FString& Message)` (player pays AI demand to free own hero)

Each follows `PreviewDiplomacy`/`ExecuteDiplomacy` exactly: game-thread guard, `bInitialized`, `IsDiplomacyTarget`, not-during-battle/AI-turn/persistence, `ActionPoints>=1`, affordability + overflow guard, then delegate scoring to `FSoulRansomRules`. (**[VERIFIED]** template: `SoulHeartlandDiplomacy.cpp:14-36`.)

**[PROPOSAL]** New HUD button names routed in `NotifyHitBoxClick` beside the existing `DiplomacyAction:`/`DiplomacyFaction:` parsing (`SoulFounderPlaytestHUD.cpp:299-300`): e.g. `RansomPay:<heroId>`, `RansomAcceptDemand:<heroId>`, `ExchangeProposeHero:<ownId>:<theirId>`. Preview strings reuse `FSoulDiplomaticDecision::Reasons`-style explainability.

---

## 5. Hostility / peace gating interactions

**[VERIFIED]** `DiplomacyAllowsHostility(A,D)` (`SoulHeartlandDiplomacy.cpp:8-13`) blocks attacks when the non-player party is a diplomacy target not at War. Movement/battle respect this (`SoulFounderPlaytestCampaignActor.cpp:171-172`).

**[PROPOSAL]** Prisoner V1 adds NO new hostility gate. Consequences:
- You can ransom/exchange with a faction regardless of stance (peace, pact, or war) — negotiating over prisoners is always diplomatically legal, only the *price/acceptance* changes with stance (war premium). This matches real prisoner-negotiation behavior and avoids a new gate on the qualified movement path.
- Because a captured hero cannot itself negotiate (**[VERIFIED]** `CanPerformDiplomacy` false, `SoulHero.h:37`), ALL prisoner actions are initiated by the owning faction's *available* hero/leadership, consistent with the existing `PreviewDiplomacy` hero-availability guard (`SoulHeartlandDiplomacy.cpp:20`). **[PROPOSAL]** EXCEPTION: freeing the player's OWN captured hero must NOT require an available hero (the whole point is that the hero is unavailable). So `AcceptRansomDemand`/`PreviewRansom` for recovering your own hero must SKIP the `CanPerformDiplomacy(Hero)` guard — this is the one deliberate divergence from the diplomacy template and must be unit-tested.

---

## 6. Save contract alignment

**[VERIFIED]** Diplomacy and hero state share ONE domain (`Soul.Campaign`, schema 1) owned by `USoulFounderPlaytestStateSubsystem`. Diplomacy serializes via `CaptureDiplomacy`/`ValidateDiplomacy` (`SoulHeartlandDiplomacy.cpp:37-68`); hero capture serializes in the `heartland_*` keys (`SoulFounderPlaytestStateSubsystem.cpp` ~lines 717-723, 837-861).

**[PROPOSAL]** V1 adds only the four optional `heartland_*` hero keys from `prisoner_state_schema.json`. The diplomacy block is UNCHANGED (decision 3A). No schema bump. Legacy and in-flight-capture saves restore byte-identically (tested).

---

## 7. Exact integration file/symbol map (for Codex)

| Concern | File | Symbol / anchor | Change kind |
|---|---|---|---|
| Hero prisoner fields | `Source/SoulCore/Public/SoulHero.h` | `FSoulHeroState` (14-30); add `ESoulPrisonerProcess` enum | **additive fields + enum** |
| Release transition | `Source/SoulCore/Public/SoulHero.h` + `Private/SoulHero.cpp` | add `FSoulHeroRules::Release`, `AdvanceCaptivity` | **new functions** |
| Ransom scoring engine | NEW `Source/SoulCore/Public/SoulRansom.h` + `Private/SoulRansom.cpp` | `FSoulRansomRules::{ValueGold, EvaluateRansomOffer, Valid}` | **new SoulCore unit (sibling of SoulDiplomacy)** |
| Prisoner ledger + preview/execute | `Source/Soul/Private/SoulHeartlandDiplomacy.cpp` (or new `SoulHeartlandPrisoners.cpp` in same module) | `PreviewRansom/ExecuteRansom/PreviewExchange/ExecuteExchange/AcceptRansomDemand`; add decls to `SoulFounderPlaytestStateSubsystem.h` | **new subsystem methods** |
| Daily captivity tick | `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` | `AdvanceDay` (~line 644, beside the two `AdvanceRecovery` calls) | **1-line addition per hero** |
| Optional immediate rescue | `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` | `ApplyBattleResult` after `FSoulWorldRules::Capture` (~line 615) | **optional, guarded** |
| Save capture | `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` | `CaptureRBSaveDomain_Implementation` heartland block (~717-723) | **4 optional keys + NPC mirror** |
| Save restore | `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp` | `RestoreRBSaveDomain_Implementation` heartland block (~837-861) | **4 optional keys + invariants** |
| UI panel | `Source/Soul/Private/SoulFounderPlaytestHUD.cpp` | diplomacy panel (96-115), `NotifyHitBoxClick` (289-301) | **Prisoners sub-section + button routing** |
| Panel state | `Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp` | `ToggleDiplomacy` (476-480), `DiplomaticFaction` | **reuse; no new panel** |
