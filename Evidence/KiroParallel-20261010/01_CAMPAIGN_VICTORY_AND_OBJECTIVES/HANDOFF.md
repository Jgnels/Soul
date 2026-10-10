# HANDOFF — Lane 1: Campaign Victory, Objectives, Quest Arc & Replayability

Lane dir: `Evidence/KiroParallel-20261010/01_CAMPAIGN_VICTORY_AND_OBJECTIVES/`
Source snapshot: `Jgnels/Soul` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (branch `handoff/soul-kiro-20261010`)
Lane branch (noncanonical): `kiro/campaign-victory-objectives-20261010`
Legend: **[VCS]** Verified from Current Snapshot · **[VOA]** Verified Owned Asset Metadata · **[LCR]** Local Runtime/Asset Check Required · **[DP]** Design Proposal

> This lane produces design + machine-readable specs only. Per shared rules it does **not** edit runtime `Source/`, `Config/`, `Content/`, `.uproject`, maps, or saves. It writes only under this directory. Codex is the integration lane.

---

## 1. Artifacts in this directory

| File | Purpose |
|---|---|
| `campaign_victory_schema.json` | Canonical shape of the victory evaluator + serialized state; 5 approaches, loss/recovery, save contract. |
| `campaign_progression_v0_v2.md` | The narrative: the gap, where the authority lives, V0→V2 roadmap, replayability, contradictions. |
| `early_mid_endgame_objectives.json` | Day 1-7 / 8-20 / endgame objective tracks + founder-slice V0 track + escalation model. |
| `starting_doctrines.md` | Per-faction start-doctrine overlays for replayability (no geography change, no invented IDs). |
| `acceptance_tests.md` | Deterministic acceptance criteria (UNIT/SUB/SAVE/MANUAL tiers) with the V0 sign-off matrix. |
| `HANDOFF.md` | This file: exact file/symbol references, implementation order, dependencies/conflicts, unknowns. |

---

## 2. Current-state audit (the facts Codex is building on) — all **[VCS]**

### 2.1 There is no campaign victory/objective/quest system
- Only win flag today: `FSoulSiegeState.bVictory` — `Source/SoulCore/Public/SoulSiege.h`, scoped to one siege.
- Tactical-only objectives: `FSoulBattleObjective` / `FSoulObjectiveRules` — `Source/SoulCore/Public/SoulObjectives.h` (`ESoulObjectiveKind{EliminateEnemy,HoldHexes,CaptureHex,ReachExit,SurviveRounds,CaptureKeep}`), evaluated over `TArray<FSoulRegimentState>`.
- No quest system anywhere in `Source/` or `Data/`.

### 2.2 The single campaign authority
`USoulFounderPlaytestStateSubsystem` — `Source/Soul/Public/SoulFounderPlaytestStateSubsystem.h` (impl `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp`). Key members/methods:
- `FSoulWorldState World;` · `FSoulCampaignEconomy Economy;` · `FSoulHeroState Hero;` · `FSoulHeroState DwarfCommander;`
- `TMap<FName,FSoulDiplomaticRelation> HumanRelations;` · `TMap<FName,FSoulFactionCampaignState> OtherFactionStates;`
- `FName PlayerFaction=TEXT("humans");` · `FName PlayerRegion, EnemyRegion;`
- `void AdvanceDay();` · `void AdvanceEnemyAI();` · `bool ApplyBattleResult(const FSoulCampaignBattleResult&);`
- RBSave: `GetRBSaveDomainId_Implementation()->"Soul.Campaign"`, `GetRBSaveSchemaVersion_Implementation()->1`, `CaptureRBSaveDomain_Implementation`, `RestoreRBSaveDomain_Implementation`.

### 2.3 The ownership mutation point
`FName FSoulWorldRules::Capture(FSoulWorldState&, FName RegionId, FName NewOwner)` — `Source/SoulCore/Public/SoulWorld.h` / `Source/SoulCore/Private/SoulWorld.cpp`. Called from `ApplyBattleResult` on `R.bPlayerWon`. **This is where territorial victory flips.**

### 2.4 The turn clock
`FSoulCampaignRules::AdvanceDay(FSoulCampaignEconomy&)` — `Source/SoulCore/Public/SoulCampaign.h` / `SoulCampaign.cpp`. `Economy.Day` starts 1, `MaxActionPoints=3`, weekly pool growth on `(Day-1)%7==0`. `CanonicalFactions()->{humans,dwarves,orcs,vikings,nature,dark}`.

### 2.5 World data (36 regions)
- Graph: `Data/soul_world_overmap_v1_20260922.json` — `nodes` (36), `edges` (51), `runtime_regions` (36), `founder_slice.region_ids` (9), macro_regions map to faction homelands (northern_fjords→viking, crownspine→dwarf, heartland→human, eastern_badlands→orc, greenwood→nature, ashen_south→dark). `owner` in nodes is non-canonical "homeland affinity".
- Canonical start ownership: `Data/soul_campaign_start_state_v1_20260922.json` — `factions[*].seat_region`, `region_control[*].{start_owner,control_class,homeland_affinity}`, 12 starting_possessions, 24 neutral, 2 neutral_minor_settlements (`northwest_march`, `coastal_ruins`).
- Scenarios: `Data/soul_campaign_start_states_v1_20260922.json` — `founder_human_orc_micro` = `LOCKED_TO_CURRENT_FOUNDER_PROTOTYPE`; `six_faction_sandbox_candidate` = `CANDIDATE_NONCANONICAL`.

### 2.6 Supporting systems the victory layer READS (never duplicates)
- Diplomacy V0: `Source/SoulCore/Public/SoulDiplomacy.h` — `ESoulDiplomaticStance{War,Peace,NonAggression}`, `FSoulDiplomaticRelation{Stance,RelationPermille,PactUntilDay,...}`, `FSoulDiplomacyRules::EffectiveStance/Apply`.
- Settlement/building: `Source/SoulCore/Public/SoulSettlement.h` — `FSoulBuildingDefinition{BuildDays(1/2/3),Prerequisites,UnlockIds}`, `FSoulSettlementRules::IsOperational/BeginConstruction/AdvanceDay`. Faction civic+recruit IDs in `Data/settlement_blueprints.json`.
- Hero: `Source/SoulCore/Public/SoulHero.h` — `FSoulHeroState{Condition(Healthy/Wounded/Captured),Level,Experience,KnownSpells,...}`, `FSoulHeroRules::AddExperience/AdvanceRecovery/LearnSpell`.
- Strategic sites: `FSoulHeartlandSite` in `Source/Soul/Public/SoulHeartlandContent.h` + overmap region `feature` (quarry/shrine/resource/crossroads/river_crossing); subsystem `InteractHeartlandSite`, `HeartlandSiteDays`.
- Strategic AI: `Source/SoulCore/Public/SoulStrategyAI.h` — `FSoulStrategyAI::Choose`. **[VCS]** wired only into tests + `SoulFoundationBridge.cpp`; `AdvanceEnemyAI()` is inert in the live loop.
- Siege: `Source/SoulCore/Public/SoulSiege.h`; live V0 in `Source/SoulRealtimeBattle/Private/SoulHumanCapitalSiege.cpp` + `Source/Soul/Private/SoulHumanCapitalSiegeQualification.cpp`. **Do not change (Lane 7 owns Siege V1).**

---

## 3. Where the smallest clean authority goes (exact)

### 3.1 New SoulCore headers (pure/static, struct-only — mirror `FSoulSiegeRules`)
**`Source/SoulCore/Public/SoulCampaignVictory.h`** (+ `Private/SoulCampaignVictory.cpp`):
```cpp
enum class ESoulVictoryApproach : uint8 { None, MilitaryDomination, AllianceFederation, MagicalSupremacy, EconomicSiteControl, LastRealmStanding };

struct FSoulVictoryWorldFacts {            // assembled by the subsystem; SoulCore stays Unreal/Soul-module free
    FName PlayerFaction;
    int32 Day = 1;
    int32 TotalRegions = 0;
    int32 PlayerOwnedRegions = 0;
    TMap<FName,int32> OwnedRegionsByFaction;        // faction -> count
    TSet<FName> PlayerOwnedRegionIds;
    TMap<FName,FName> SeatRegionByFaction;          // from start-state seat_region
    TMap<FName,FName> SeatOwnerByFaction;           // current owner of each seat
    TSet<FName> OwnedArcaneSiteRegions;             // intersection(player regions, shrine regions)
    TSet<FName> OwnedStrategicSiteRegions;
    bool bArcaneBuildingOperational = false;
    bool bMarketOperational = false;
    int32 HeroLevel = 1; int32 HeroKnownSpells = 0;
    int32 TreasuryGold = 0;
    int32 FederatedPlayableCount = 0;               // from HumanRelations predicate
    bool bAnyPlayableAtWarWithPlayer = false;
    TSet<FName> EliminatedPlayableFactions;
};

struct FSoulCampaignVictoryState {         // SERIALIZED in Soul.Campaign (schema 2)
    int32 ActiveApproachMask = 0;          // bitmask of ESoulVictoryApproach
    ESoulVictoryApproach AchievedApproach = ESoulVictoryApproach::None;
    int32 AchievedDay = 0;
    int32 RecoveryDeadlineDay = 0;
    int32 EndgameEscalationDay = 0;
    bool bFinalLoss = false;
    TMap<int32,int32> ProgressByApproach;  // approach -> 0..1000 permille
    TMap<int32,int32> SustainStartDayByApproach;
};

class SOULCORE_API FSoulVictoryRules {
public:
    static void Evaluate(const FSoulVictoryWorldFacts& Facts, FSoulCampaignVictoryState& State); // deterministic, idempotent until latched
    static int32 ApproachProgressPermille(ESoulVictoryApproach, const FSoulVictoryWorldFacts&);
};
```
**`Source/SoulCore/Public/SoulCampaignObjectives.h`** (+ `.cpp`): `enum class ESoulCampaignGoalKind{...}` (see `early_mid_endgame_objectives.json`), `struct FSoulCampaignGoal`, `struct FSoulCampaignGoalProgress`, `class FSoulCampaignObjectiveRules { static FSoulCampaignGoalProgress EvaluateGoal(const FSoulCampaignGoal&, const FSoulVictoryWorldFacts&); };`.

### 3.2 Subsystem changes (`USoulFounderPlaytestStateSubsystem`)
- Add member `FSoulCampaignVictoryState CampaignVictory;` and `TArray<FSoulCampaignGoal> ActiveGoals;` (goals are data-loaded, not serialized as truth — only completion flags are, if needed).
- Add private `FSoulVictoryWorldFacts BuildVictoryFacts() const;` assembling facts from `World`, `Economy`, `Hero`, `HumanRelations`, `OtherFactionStates`, `SettlementAuthority`, and the loaded start-state seat/site data.
- In `ApplyBattleResult`, **after** `FSoulWorldRules::Capture(...)` and reward block, call `FSoulVictoryRules::Evaluate(BuildVictoryFacts(), CampaignVictory);`.
- In `AdvanceDay`, **after** economy/settlement/hero advance (before/after `AdvanceEnemyAI()` is fine; must be deterministic), call the same `Evaluate`. Also update `RecoveryDeadlineDay` / `bFinalLoss` / `EndgameEscalationDay` here.
- Save: in `CaptureRBSaveDomain_Implementation` add a `"victory"` JSON object (shape in `campaign_victory_schema.json`); in `RestoreRBSaveDomain_Implementation` read it (absent → default). **Bump `GetRBSaveSchemaVersion_Implementation` 1 → 2.**

### 3.3 Data (new files only — never edit locked start-state/overmap)
- `Data/soul_campaign_victory_params_v0.json` — per-scenario `ActiveApproachMask` + thresholds + site/shrine region lists (sourced from overmap features; final site list from **Lane 8**).
- `Data/soul_campaign_objectives_v0.json` — the objective tracks from `early_mid_endgame_objectives.json`.
- `Data/soul_starting_doctrines_v0.json` — doctrine overlays from `starting_doctrines.md`.

### 3.4 HUD (presentation only)
Extend the existing Kenney HUD (`Source/SoulCore/Public/SoulHUDArt.h` / `SoulHUDTheme.h` and the founder HUD/PlayerController). Objective line + victory/recovery banner + per-approach progress rings reading `CampaignVictory.ProgressByApproach`. **[VOA]** optional icons from owned LayerLab/34IB packs **after** an **[LCR]** import check — **no `/Game/...` path asserted here.**

---

## 4. Implementation order (dependency-ordered)

1. **SoulCore `SoulCampaignVictory.h/.cpp`** + unit tests (AC-D1,D2,V0-4,V1-*). No engine/world needed.
2. **Subsystem: facts assembly + member + two hooks** for V0 Military Domination only; mask = `{MilitaryDomination}` for founder slice (AC-V0-*). (AC-D3,D4 guard authority.)
3. **Loss + recovery** in `AdvanceDay`/`ApplyBattleResult` (AC-L1..L4).
4. **Save schema 1→2** capture/restore `"victory"` + back-compat (AC-S1..S4).
5. **SoulCore `SoulCampaignObjectives.h/.cpp`** + objective data load; wire Day 1-7 founder track (AC-O1..O4).
6. **HUD** objective line + victory/recovery banner (AC-H1; AC-H2 after local asset check).
7. **V1**: enable all approaches behind mask; wire Alliance (**Lane 4**), site lists (**Lane 8**), gold thresholds (**Lane 9**); sustained-day logic (AC-V1-*).
8. **V2**: endgame escalation (AC-E*), vassal/federation terminals, day-60 score victory; rival-win loss once **Lane 2** drives AI.
9. **Doctrines**: `soul_starting_doctrines_v0.json` overlay + save of selected `doctrine_id`.

---

## 5. Dependencies & conflicts

**Depends on (reads, never duplicates):**
- **Lane 4** (Prisoners/Diplomacy V1): Alliance/Federation + vassal terminals read the `HumanRelations` ledger and `FSoulDiplomacyRules`. Lane 1 must NOT invent diplomacy mechanics. Alliance ships LOCKED in V0.
- **Lane 8** (Strategic events/sites): authoritative strategic-site and arcane-site region lists for Economic/Magical approaches and site-contest escalation.
- **Lane 9** (Economy/balance): treasury gold thresholds, per-site/building income, score-victory weights, doctrine start values (all `BALANCE_PENDING` here).
- **Lane 2** (AI army/recruitment): rival-win loss conditions and `esc.rival_rally` are inert until strategic AI drives armies.
- **Lane 3** (Hero progression): Magical Supremacy reads `Hero.Level`/`KnownSpells`; confirm additive fields are save-safe on `FSoulHeroState`.
- **Lane 6** (Walkable Dwarf Hold / Viking Harbour): any doctrine/objective implying visitation is gated here.
- **Lane 7** (Siege V1): `WinSiege` objective reads results only; Siege V0 stays frozen.

**Conflict guards (must hold):**
- Single Save authority: reuse `Soul.Campaign` domain, bump schema; never register a new provider. **[VCS]**
- Single combat/diplomacy authority: read `FSoulWorldRules::Capture` output and `FSoulDiplomacyRules`; never re-implement.
- No geography redesign: use the 36-region graph + 6 macro-region homelands as-is.
- No invented `/Game/...` paths or asset ownership; owned-asset refs are metadata-only until a local import check.

---

## 6. Explicit unknowns (need local Unreal evidence or another lane)

1. **[LCR]** Exact `FSoulCampaignBattleResult` fields for siege outcomes beyond `bSiege`/`SiegeGateRemaining`/`bCourtyardCaptured` (seen in `CaptureRBSaveDomain`) — confirm against the live struct before `WinSiege` counting.
2. **[LCR]** Whether the six-faction start-state is actually *loaded* into `World.Regions` at runtime today, or only the 9-region founder slice. V0 targets the slice; V1 needs the 36-region load path confirmed.
3. **[LCR]** Which settlement building IDs are *operational-gated* for non-Human factions (dwarf.mine, viking.market, orc.spoils_market) — `Data/settlement_blueprints.json` lists them, but only Human `human.market`/`human.barracks` have **[VCS]** live income/pool effects in `AdvanceDay`.
4. **Lane 8** must confirm the canonical strategic-site and arcane-site region sets; Lane 1 only derived candidates from overmap `feature` tags.
5. **Lane 9** must supply all gold thresholds and income values (every number here is `BALANCE_PENDING`).
6. **[LCR]** Local asset-registry check for the owned LayerLab/34IB UI packs before any objective/victory icon is bound to a content path.
7. **Lane 2** must exist before rival factions can *win*; until then the only loss path is player-seat elimination, which cannot occur autonomously (AI off).

---

## 7. One-line integration summary for Codex

Add a pure `FSoulVictoryRules` (SoulCore) + a `FSoulCampaignVictoryState` member on `USoulFounderPlaytestStateSubsystem`, evaluated in `ApplyBattleResult` (after `FSoulWorldRules::Capture`) and `AdvanceDay`, persisted in the existing `Soul.Campaign` save domain at schema 2 — shipping V0 as "capture `orc_camp` = Military Domination win" with a 5-day capital-loss recovery, then unlocking the other four approaches behind `ActiveApproachMask` as Lanes 4/8/9/2/3 land.
