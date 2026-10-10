# LANE 4 HANDOFF — Prisoners, Ransom, Exchange, Rescue & Diplomacy V1

**Lane:** `04_PRISONERS_RANSOM_DIPLOMACY_V1`
**Branch:** `kiro/prisoners-ransom-diplomacy-v1-20261010`
**Frozen source snapshot:** `Jgnels/Soul @ 2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (branch `handoff/soul-kiro-20261010`) — **verified exact match at session start.**
**Asset catalog:** `Jgnels/Copperlight-Asset-Catalog` (read-only, authenticated).
**Author lane:** Kiro frontier parallel lane 4. NOT the Unreal integration lane. No runtime `Source/`/`Config/`/`Content/`/`.uproject`/maps/saves were edited. All writes are under this Evidence directory only.

Evidence labels used throughout: **VERIFIED FROM CURRENT SNAPSHOT** · **VERIFIED OWNED ASSET METADATA** · **LOCAL RUNTIME-ASSET CHECK REQUIRED** · **DESIGN PROPOSAL**.

---

## 1. Executive summary

Soul already ships: hero `HEALTHY/WOUNDED/CAPTURED` state (`FSoulHeroState`/`FSoulHeroRules`), a deterministic, explainable **Diplomacy V0** (`FSoulDiplomacyRules` + subsystem driver + save + UI panel), and a single save authority (`USoulFounderPlaytestStateSubsystem`, domain `Soul.Campaign` v1). **The one true gap: a captured hero can never leave captivity** — `FSoulHeroRules::AdvanceRecovery` ignores `Captured` (VERIFIED, `SoulHero.cpp:63-68`). There is no ransom, exchange, rescue, or escape anywhere in the snapshot.

This lane designs a coherent, deterministic prisoner lifecycle that **extends** those systems with **zero new save authority and zero changes to the qualified diplomacy save contract**:

1. **Captured-hero ledger** = a derived view over the existing persisted heroes (no new container).
2. **Deterministic ransom valuation** (`FSoulRansomRules::ValueGold`) + **offer/acceptance scoring** (`EvaluateRansomOffer`), both fixed-scale integer, no RNG — identical determinism to `FSoulDiplomacyRules`.
3. **Release transition** `FSoulHeroRules::Release` returning a freed hero as `Wounded(3)` so the existing availability gate and recovery tick keep working.
4. **Prisoner exchange** with deterministic value balancing + gold top-up.
5. **Deterministic escape** keyed to the captor LOSING the capture region (map-driven, not dice) + **lightweight rescue** = winning at the capture region.
6. **Relationship effects** as small clamped `RelationPermille` deltas; **no change** to `FSoulDiplomaticRelation` or its serializer.
7. **UI** as a Prisoners sub-section of the EXISTING diplomacy panel.

**Artifacts in this directory:**
- `prisoner_state_schema.json` — additive hero fields, ledger view, save-contract delta.
- `ransom_scoring.json` — deterministic valuation + acceptance formulas with worked examples and VERIFIED economic anchors.
- `exchange_rules.md` — release primitives, release locations, exchange, siege-capture, peace-with-prisoners, admission-before-mutation.
- `ai_prisoner_behavior.md` — reactive AI captor/owner behavior + anti-degeneracy guarantees.
- `rescue_escape_hooks.md` — deterministic escape clock + battlefield rescue hook.
- `diplomacy_v1_integration.md` — exact relation deltas, memory decision, and file/symbol integration map.
- `HANDOFF.md` — this file.

---

## 2. VERIFIED FROM CURRENT SNAPSHOT — the systems we build on

| Thing | Where | Note |
|---|---|---|
| `ESoulHeroCondition { Healthy, Wounded, Captured }` | `Source/SoulCore/Public/SoulHero.h:12` | uint8; 0/1/2 relied on by save validation |
| `FSoulHeroState { HeroId, Condition, RecoveryDays, CaptorFaction, CaptureRegion, Kind, Level, Experience, ... }` | `SoulHero.h:14-30` | no owning-faction/location field |
| `FSoulHeroRules::ApplyBattleInjury` | `SoulHero.cpp:54-62` | capture iff `ArmySurvivors==0`; wound else, `RecoveryDays=3` |
| `FSoulHeroRules::AdvanceRecovery` | `SoulHero.cpp:63-68` | **ignores Captured — the gap** |
| `FSoulHeroRules::IsAvailable / CanCommandSiege / CanPerformDiplomacy` | `SoulHero.h:35-37` | all require `Healthy` |
| Diplomacy enums/structs/rules | `Source/SoulCore/Public/SoulDiplomacy.h` | `Stance{War,Peace,NonAggression}`, `Action{DeclareWar,Peace,NonAggression,Gift,BreakTreaty}`, `FSoulDiplomaticRelation`, `FSoulDiplomaticDecision`, `FSoulDiplomacyRules` |
| Diplomacy scoring (fixed-scale, no RNG, 500 threshold) | `SoulDiplomacy.cpp:10-37` | base 400 + relation/2 + betrayal memory |
| Diplomacy apply deltas | `SoulDiplomacy.cpp:38-53` | Gift +100, DeclareWar -100, BreakTreaty -300 + BetrayalDay |
| Subsystem diplomacy driver | `Source/Soul/Private/SoulHeartlandDiplomacy.cpp:4-36` | `IsDiplomacyTarget / DiplomaticRelation / DiplomacyAllowsHostility / PreviewDiplomacy / ExecuteDiplomacy` |
| Diplomacy serialization | `SoulHeartlandDiplomacy.cpp:37-68` | `CaptureDiplomacy / ValidateDiplomacy`; 5-field rows; targets dwarves/orcs/vikings |
| Single save authority | `Source/Soul/Public/SoulFounderPlaytestStateSubsystem.h:26` + `.cpp:686-903` | `IRBSaveDomainProvider`, domain `Soul.Campaign`, schema 1 |
| Hero capture serialization | `SoulFounderPlaytestStateSubsystem.cpp:717-723` (capture), `837-861` (restore+invariants) | Captured => recovery 0, canonical captor != owner, valid region |
| Hero wound/capture wiring | `ApplyBattleResult` lambda `ApplyHeroInjury` | `.cpp:564-577`, invoked 583/604 |
| Daily recovery tick | `AdvanceDay` | `.cpp:644` (two `AdvanceRecovery` calls) |
| Battle exclusion of injured hero | `ConfigureHeartlandMagic` | `.cpp:969-981` |
| Diplomacy UI panel + routing | `SoulFounderPlaytestHUD.cpp:96-115`, `289-301`; panel state `SoulFounderPlaytestCampaignActor.cpp:476-480` | reuse, do not fork |
| Economy constants | `SoulFounderPlaytestStateSubsystem.cpp:290-307,504,616-617,671` | gold 3000 start / 450 income / 140-220 unit / 250 gift / 600 reward / 1200 hire |
| Test harness | `Source/Soul/Private/Tests/SoulHeartlandTests.cpp:11-16` | `FHeartlandFixture`, `HeartlandSnapshot` equality |

---

## 3. VERIFIED OWNED ASSET METADATA (optional UI only)

Lane 4 is logic-first; UI is text today (Kenney UI is already imported — **VERIFIED FROM CURRENT SNAPSHOT**, `Data/UI/Kenney/provenance.json`). If icons are wanted for a Prisoners panel, these are **VERIFIED OWNED ASSET METADATA** from `Copperlight-Asset-Catalog/catalog/founder_ownership.json` (founder-explicit, OWNERSHIP_CONFIRMED, 2026-09-16):
- "Ribbon pearchment" — NKNW_Lab (parchment/scroll framing for ransom notes).
- "2D Icons - Casual Icon Pack", "2D Icons - Reward Chest Pack" — LayerLab (gold/coin, chest for ransom payment).
- "Modern Casual Ui Pack" — 34IB Studio (panels/buttons).

**No purchases required.** **No invented `/Game/...` paths** — these are catalog ownership records, not confirmed imported `/Game` assets. Whether any is imported into Soul's content is **LOCAL RUNTIME-ASSET CHECK REQUIRED**. V1 can ship with the existing text panel and zero new assets.

---

## 4. Implementation order (smallest clean authority first)

Each step is independently compilable/testable and leaves the build green.

1. **SoulCore: additive hero fields + enum.** Add `ESoulPrisonerProcess` and the four fields (`CaptureDay`, `RansomProcess`, `RansomAskGold`, `ExchangeCounterpartHeroId`) to `FSoulHeroState` with safe defaults. Pure data; no behavior change. *(file: `SoulHero.h`)*
2. **SoulCore: `FSoulHeroRules::Release` + `AdvanceCaptivity`.** The single release transition and the deterministic escape clock. Unit-testable with no engine. *(files: `SoulHero.h/.cpp`)*
3. **SoulCore: new `FSoulRansomRules` unit** (`SoulRansom.h/.cpp`), sibling of `SoulDiplomacy`: `ValueGold`, `EvaluateRansomOffer`, `Valid`. Pure, deterministic, headless-testable. Reproduce the worked examples in `ransom_scoring.json` as unit assertions.
4. **Soul subsystem: ledger view + preview/execute methods.** `PreviewRansom/ExecuteRansom/PreviewExchange/ExecuteExchange/AcceptRansomDemand`, built on the `ExecuteDiplomacy` admission-before-mutation template. *(file: `SoulHeartlandDiplomacy.cpp` or new `SoulHeartlandPrisoners.cpp` in the Soul module; decls in `SoulFounderPlaytestStateSubsystem.h`)*
5. **Soul subsystem: daily captivity tick.** Add `AdvanceCaptivity` calls beside the two `AdvanceRecovery` calls in `AdvanceDay` (~line 644), passing `bCaptorHoldsCaptureRegion` computed from `World.Regions`.
6. **Soul subsystem: save capture/restore delta.** Add the four optional `heartland_*` keys (+ NPC mirror) with full adversarial invariants in `RestoreRBSaveDomain_Implementation`. Keep schema at 1 (optional keys).
7. **Soul subsystem (optional): immediate battlefield rescue** in `ApplyBattleResult` after `FSoulWorldRules::Capture`. Guarded by `bHeartlandEnabled`; defer if it risks the qualified siege-apply ordering.
8. **UI: Prisoners sub-section** in the existing diplomacy panel + button routing in `NotifyHitBoxClick`. Preview strings reuse `FSoulDiplomaticDecision::Reasons` style.

**Rule:** steps 1-3 are SoulCore and carry zero risk to the qualified runtime. Steps 4-6 are the real integration. Step 7 is optional. Step 8 is presentation.

---

## 5. Acceptance tests (deterministic, headless)

Use the existing `FHeartlandFixture` + `HeartlandSnapshot(S)` byte-equality style (`SoulHeartlandTests.cpp`). All tests must pass on repeated runs identically (no RNG).

**A. Valuation determinism (SoulCore unit)**
- `ValueGold` reproduces every worked example in `ransom_scoring.json` exactly.
- `ValueGold` is monotone non-increasing in `daysHeld` and floors at 60% after 10 days.
- `EvaluateRansomOffer` accepts a full-value offer (score >= 500) and rejects a low-ball at war; threshold is exactly 500.

**B. Release transition (SoulCore unit)**
- `Release` on a non-captured hero is a no-op (atomic).
- `Release` on a captured hero yields `Wounded`, `RecoveryDays==3`, cleared captor/region/process fields.
- After `Release`, `IsAvailable==false`; after 3 `AdvanceRecovery` calls, `Healthy` and available.

**C. Escape clock (integration)**
- A captured hero with `CaptureDay` set does NOT escape while captor holds `CaptureRegion`, for any number of days.
- After the capture region flips away from the captor AND `daysHeld>=5`, the next `AdvanceDay` releases the hero to a valid owned/home/fallback region (verified `World.Regions.Contains`).
- Escape is identical across a save/reload taken mid-captivity (snapshot equality of release day).

**D. Ransom transaction atomicity (integration)**
- `ExecuteRansom` with insufficient gold / wrong faction / non-captured hero returns false and leaves `HeartlandSnapshot(S)` byte-identical (mirror `FHeartlandDiplomacyTest` atomic-rejection assertions).
- A successful ransom: owner gold decreases by `ValueGold`, captor gold increases by same, 1 movement spent, hero becomes `Wounded(3)`, relation delta applied and clamped.
- Recovering the player's OWN captured hero via `AcceptRansomDemand` succeeds even though `CanPerformDiplomacy(Hero)` is false (the deliberate guard exception).

**E. Exchange (integration)**
- Even swap (|Vp-Va|<=300) costs 0 gold; both heroes released to their own owners' regions.
- Unequal swap requires the correct top-up and is affordability/overflow-gated like Gift; relation `+120` applied.

**F. Save contract (integration, adversarial — mirror `FHeartlandDiplomacySaveTest`)**
- A legacy pre-V1 checkpoint (no prisoner keys) restores with all prisoner fields at defaults and `HeartlandSnapshot` byte-stable.
- Save->restore of an active captivity/ransom process is exact (snapshot equality).
- Restore REJECTS (atomic): `process!=None && condition!=Captured`; `capture_day>day`; `ransom_ask` out of `[0,100000]`; `exchange_counterpart` set with `process!=ExchangeProposed`. Invalid save leaves authority intact.

**G. Peace-with-prisoners (integration)**
- Offering Peace does NOT release any prisoner.
- At Peace, `EvaluateRansomOffer` drops the war premium (price falls vs. the War case for identical inputs).
- `BreakTreaty` while holding the enemy hero cancels any pending ransom process on that hero and records `BetrayalDay` (existing behavior unchanged).

---

## 6. Dependencies & conflicts (cross-lane)

**Depends on / coordinates with:**
- **Lane 3 (Hero XP/levels/classes):** `ValueGold` reads `Hero.Level`/`Kind`. If Lane 3 adds a hero registry or an `OwningFaction` field, migrate the ledger derivation (currently resolves owner at call site). COORDINATE the additive-field ordering in `FSoulHeroState` so both lanes' fields coexist cleanly. **No conflict today** — both are purely additive to the same struct.
- **Lane 1 (Campaign victory):** an "alliance/federation" or "military domination" victory may want captured-hero counts or ransom income as objective inputs. The derived ledger view is the clean read surface. No shared mutable state.
- **Lane 2 (AI army composition/recovery):** anti-recapture-churn and "stranded commander" concerns overlap. Lane 4 guarantees a freed hero returns `Wounded` (not immediately re-fightable) and always to a valid region — consistent with Lane 2's anti-degeneracy goals. If Lane 2 adds autonomous AI spending, that lane (not this one) should decide whether AI spontaneously ransoms back its own commander.
- **Human Capital Siege V0 (qualified, DO NOT CHANGE):** Lane 4 only READS siege results (`bCourtyardCaptured`, survivors) already surfaced in `FSoulCampaignBattleResult`. The optional immediate-rescue hook (step 7) is the only touch of `ApplyBattleResult` and is deferrable.

**Hard non-conflicts (by construction):**
- No second save authority (reuses `Soul.Campaign` v1, optional keys only).
- No second diplomacy authority (reuses `FSoulDiplomacyRules`; prisoner relation effects are plain `RelationPermille` deltas).
- No change to `FSoulDiplomaticRelation` or its serializer (memory decision 3A).
- No change to combat resolution authority.

**Explicitly rejected (per shared rules):** no torture/execution/dynasty; no Titan/R10/terrain/encounter-matrix reopen; no Dragon Graveyard fallback; no invented `/Game` paths or ownership.

---

## 7. Explicit unknowns (LOCAL RUNTIME-ASSET CHECK REQUIRED / open)

1. **Owner home-region FNames.** Release-location rule 2 needs the human capital region FName and the dwarf home region FName from the ACTIVE scenario. Do not hardcode invented FNames — resolve from scenario data at integration. *(LOCAL)*
2. **World adjacency helper signature.** Release rule 1 and the escape computation assume a `FSoulWorldRules` adjacency query; confirm the exact symbol (`CanMove`/neighbor accessor) in `SoulWorld.h` at integration. *(LOCAL)*
3. **Commander<->settlement linkage** for the siege ransom-leverage bonus (`exchange_rules.md` §4): confirm how a settlement's defending commander maps to a hero id. *(LOCAL)*
4. **Autonomous AI ransom spending.** V1 AI does not spontaneously pay to recover its own hero (strategic AI is intentionally off in these profiles — VERIFIED). Whether a later lane turns this on is open. *(OPEN)*
5. **Allied rescue.** Diplomacy V0 has no alliance/military-access concept, so a faction at Peace cannot free your hero. Deferred until alliances exist. *(OPEN)*
6. **Prisoner-panel icons.** Whether any VERIFIED OWNED catalog icon is actually imported into Soul `/Game` content is unverified from the repo; V1 ships text-only safely. *(LOCAL)*
7. **Multi-prisoner UI scaling.** Today only 2 heroes exist (player + DwarfCommander), so the ledger is tiny. If Lane 3 adds many heroes, the Prisoners sub-section needs a scroll/list; out of V1 scope. *(OPEN)*

---

## 8. Contradictions against current assumptions

- **"CAPTURED is a terminal, decorative state."** The snapshot treats capture as a dead end (no exit path in `AdvanceRecovery`). This lane asserts capture must be a **lifecycle with a guaranteed finite exit** (monotone-decaying ransom + region-driven escape). Codex should treat the current terminal behavior as an unfinished stub, not a design intent.
- **"Diplomacy actions are a fixed set of 5."** The UI hardcodes `DiplomacyAction:0..4` (VERIFIED). Prisoner actions must be added as a **sub-section with distinct button names**, not by overloading the 5 enum actions — do not widen `ESoulDiplomaticAction`.
- **"A hero can only act when Healthy."** `PreviewDiplomacy` blocks injured heroes (VERIFIED). This is correct for OFFENSIVE diplomacy, but **recovering your own captured hero must bypass that guard** — otherwise a captured hero can never be ransomed back. This single, deliberate exception is the subtlest correctness point in the lane.
- **"Capture requires total army loss."** VERIFIED (`ArmySurvivors==0`). So heroes are captured rarely and only in catastrophic defeats; ransom values are therefore tuned as high-stakes, infrequent events, not routine income.

---

## 9. Highest-priority integration action for Astra

**Land SoulCore steps 1-3 first (additive `FSoulHeroState` fields + `FSoulHeroRules::Release`/`AdvanceCaptivity` + the new `FSoulRansomRules` unit), with the SoulCore unit tests from §5.A/B green, BEFORE touching the subsystem.** This establishes the deterministic prisoner/ransom primitives in the dependency-free core module (zero risk to the qualified runtime or save contract), gives Lane 3 a stable additive-field contract to coordinate against, and makes every later integration step (save keys, UI, escape tick) a thin, independently testable wiring layer. The single correctness watch-item to carry into step 4 is the **`AcceptRansomDemand` guard exception**: freeing the player's OWN captured hero must NOT require an available hero.
