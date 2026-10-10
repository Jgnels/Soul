# Campaign Progression V0 → V2 — Victory, Objectives & Replayability

Lane: `01_CAMPAIGN_VICTORY_AND_OBJECTIVES`
Source snapshot: `Jgnels/Soul` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (branch `handoff/soul-kiro-20261010`)
Status label legend: **[VCS]** = Verified from Current Snapshot · **[VOA]** = Verified Owned Asset Metadata · **[LCR]** = Local Runtime/Asset Check Required · **[DP]** = Design Proposal

---

## 0. The gap this lane closes

**[VCS]** Soul today has a complete *tactical* loop and a one-shot *founder micro-campaign*, but **no campaign macro-structure**:

- The only "win" concept in the codebase is `FSoulSiegeState.bVictory` (`Source/SoulCore/Public/SoulSiege.h`), scoped to a single siege encounter.
- `FSoulObjectiveRules` / `FSoulBattleObjective` (`Source/SoulCore/Public/SoulObjectives.h`) are **hex-battle objectives only** (`EliminateEnemy`, `HoldHexes`, `CaptureHex`, `ReachExit`, `SurviveRounds`, `CaptureKeep`) evaluated over `TArray<FSoulRegimentState>` within one battle.
- A repo-wide search found **no** campaign victory, loss, game-over, or quest system in `Source/` or `Data/`.
- The de-facto campaign "goal" today is the founder slice's `enemy_objective: "orc_camp"` (`Data/soul_world_overmap_v1_20260922.json`): capture that region and the micro-campaign is effectively over, but nothing in code *declares* a win.

So the player can conquer regions, develop a settlement, recruit, use magic, do Diplomacy V0, wound/capture heroes, and run a siege — but the campaign never **resolves**. This lane designs the smallest clean authority that gives the four-faction game a reason to end and multiple viable ways to get there.

---

## 1. Where the authority lives (smallest clean seam)

**[VCS]** The campaign truth already has a single owner: **`USoulFounderPlaytestStateSubsystem`** (`Source/Soul/Public/SoulFounderPlaytestStateSubsystem.h`, impl `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp`). It:

- owns `FSoulWorldState World`, `FSoulCampaignEconomy Economy`, `FSoulHeroState Hero`, `TMap<FName,FSoulDiplomaticRelation> HumanRelations`, `TMap<FName,FSoulFactionCampaignState> OtherFactionStates`;
- is the **single turn loop**: `AdvanceDay()` → `FSoulCampaignRules::AdvanceDay(Economy)` + settlement/hero/other-faction advance + `AdvanceEnemyAI()`;
- is the **single battle→campaign sink**: `ApplyBattleResult()` → `FSoulWorldRules::Capture(World, TargetRegion, PlayerFaction)` + XP/gold rewards + `bBattleWon`;
- is the **Soul.Campaign RBSave domain provider** (`GetRBSaveDomainId_Implementation` = `"Soul.Campaign"`, schema version `1`), serializing to JSON in `CaptureRBSaveDomain_Implementation` / `RestoreRBSaveDomain_Implementation`.

### Proposed new canonical rules (SoulCore, pure/static — mirrors `FSoulSiegeRules`/`FSoulObjectiveRules`)

| New file (SoulCore/Public) | Contents |
|---|---|
| `SoulCampaignVictory.h` | `enum class ESoulVictoryApproach`, `struct FSoulCampaignVictoryState`, `struct FSoulVictoryWorldFacts`, `class FSoulVictoryRules` (static `Evaluate(const FSoulVictoryWorldFacts&, FSoulCampaignVictoryState&)`). |
| `SoulCampaignObjectives.h` | `enum class ESoulCampaignGoalKind`, `struct FSoulCampaignGoal`, `struct FSoulCampaignGoalProgress`, `class FSoulCampaignObjectiveRules` (static `EvaluateGoal(const FSoulCampaignGoal&, const FSoulVictoryWorldFacts&)`). |

**Why SoulCore, static, struct-only:** AGENTS.md mandates deterministic, presentation-independent canonical rules with integer/permille values and stable IDs. `FSoulSiegeRules`, `FSoulObjectiveRules`, `FSoulWorldRules`, `FSoulDiplomacyRules` are all exactly this shape. The victory evaluator reads a **facts snapshot** (`FSoulVictoryWorldFacts`) assembled by the subsystem from `World`, `Economy`, `Hero`, `HumanRelations`, `OtherFactionStates`, and the settlement authority — so SoulCore never depends on the Soul module or Unreal types.

### The two evaluation hooks (no third authority)

1. **`ApplyBattleResult`** — after `FSoulWorldRules::Capture` mutates ownership (territorial/elimination conditions can flip here).
2. **`AdvanceDay`** — after economy/settlement/hero advance (time-sustained conditions, recovery timers, endgame escalation).

Both call `FSoulVictoryRules::Evaluate(Facts, CampaignVictory)` and store the result on a new member `FSoulCampaignVictoryState CampaignVictory;`.

### Save contract

Persist `CampaignVictory` inside the **existing** `Soul.Campaign` domain. Bump `GetRBSaveSchemaVersion_Implementation` **1 → 2**. In `RestoreRBSaveDomain_Implementation`, treat a missing `"victory"` object as default-constructed (schema-1 saves load clean). **No second save authority** (AGENTS.md).

---

## 2. The victory approaches (and why five)

Keeping Nature/Dark passive and compatible with the four-faction game, the enabled approaches for a *playable* faction (humans/dwarves/orcs/vikings) are:

| Approach | One-line | Primary state read | Owning dependency |
|---|---|---|---|
| **Military Domination** | Hold ≥55-60% of 36 regions + all rival seats | `FSoulWorldState` ownership + seat set | self-contained |
| **Alliance / Federation** | Bind ≥3 playable realms at Peace ≥750‰ for N days | `HumanRelations` + `FSoulDiplomacyRules` | **Lane 4** Diplomacy V1 |
| **Magical Supremacy** | Hold ≥2 arcane shrines + operational arcane building + high-level mage hero | ownership + `FSoulSettlementRules::IsOperational` + `FSoulHeroState` | **Lane 8** (sites), **Lane 3** (hero) |
| **Economic / Site Control** | Hold ≥4 strategic sites + operational market + treasury threshold | ownership + `Economy.Resources['gold']` + settlement | **Lane 8** (sites), **Lane 9** (gold) |
| **Last Realm Standing** | Every rival playable faction has lost its seat | ownership + seat set | self-contained |

### Why a fifth approach is justified

The prompt allows a fifth "only if justified". **Last Realm Standing** is justified by data and determinism, not flavour:

1. **The data already distinguishes seats.** `Data/soul_campaign_start_state_v1_20260922.json` carries `factions[*].seat_region` and `region_control[*].control_class == "starting_possession"`. Elimination (seat loss) is a *free* read; no new data authoring.
2. **It gives domination a decisive terminal without a 100% map sweep.** Pure Military Domination at 60% can stall into a boring mop-up. Last Realm Standing resolves the game the moment the last rival capital falls, even if neutral/passive regions remain.
3. **It is the AI's natural win too (Lane 2).** A shared, verifiable "all rival seats captured" predicate lets the strategic AI pursue the same end the player can, which is what makes an *AI loss* for the player legible.

Military Domination (share-based) and Last Realm Standing (seat-based) are deliberately *both* present: the former rewards broad control, the latter rewards decapitation. They converge in the limit but reward different play, which is replayability, not redundancy.

### Nature/Dark compatibility **[VCS/DP]**

**[VCS]** `AdvanceEnemyAI()` reports six-faction strategic AI **OFF** and non-player garrisons simply hold. **[DP]** Nature and Dark therefore:

- are **never** victory-seekers (excluded from `playable_victory_factions`);
- **can** be conquered (their regions and seats count toward a player's Military Domination / Last Realm Standing);
- **can** hold arcane shrines (`nature_shrine`) and strategic sites, so they are meaningful obstacles for Magical Supremacy / Economic Site Control;
- are **excluded** from Alliance/Federation counts (you federate playable realms, not the wild/corrupt);
- trigger a player loss only by the universal rule (capturing the player's seat in a battle the player loses) — which, with AI off, cannot happen autonomously in V0.

This keeps the four-faction game whole while leaving Nature/Dark as flavourful, conquerable edges of the 36-region world.

---

## 3. V0 → V1 → V2 roadmap

### V0 — "Declare the win that already happens" (minimal, founder-slice-safe)

Goal: make the campaign *resolve* without changing any shipped mechanic.

- Add `SoulCampaignVictory.h` + `FSoulVictoryRules::Evaluate` (SoulCore).
- Add `FSoulCampaignVictoryState CampaignVictory` to the subsystem; evaluate in `ApplyBattleResult` + `AdvanceDay`.
- **Enabled approach in V0 = Military Domination only**, parameterised for the founder slice as *"own `enemy_primary_region` (`orc_camp`)"* — **[VCS]** this is exactly what `ApplyBattleResult` already produces when the player captures `orc_camp`; V0 only reads the flag and raises `AchievedApproach`.
- Add the capital-loss **loss condition** + 5-day **recovery grace** (`RecoveryDeadlineDay`).
- Persist in `Soul.Campaign` (schema 2).
- HUD: a single "Objective: take the Orc stronghold" line + a "Victory!" / "Capital Lost — N days to recover" banner, extending the existing Kenney HUD (`SoulHUDArt.h`). **[VOA]** optional icon from the owned LayerLab/34IB UI packs after an **[LCR]** import check.
- Objectives surfaced: the founder-slice V0 track in `early_mid_endgame_objectives.json`.

**V0 acceptance:** capturing `orc_camp` raises `AchievedApproach = MilitaryDomination`; a save/F9 reload preserves it; losing the capital enters recovery and recapturing any region within 5 days clears it. (See `acceptance_tests.md`.)

### V1 — "Five ways to win, six-faction sandbox"

- Enable all five approaches, parameterised for the 36-region / six-faction start (`soul_campaign_start_state_v1_20260922.json`).
- Alliance/Federation reads **Lane 4** Diplomacy V1 (`HumanRelations` ledger — no second authority).
- Magical Supremacy / Economic Site Control read the authoritative site list from **Lane 8** and gold thresholds from **Lane 9**.
- Add sustained-day conditions (`SustainStartDayByApproach`) evaluated in `AdvanceDay`.
- Add the Day 1-7 / 8-20 objective tracks (`early_mid_endgame_objectives.json`).
- Add per-approach HUD progress rings (0-1000‰ from `ProgressByApproach`).
- Add starting doctrines (`starting_doctrines.md`) as scenario overlays on the start-state file.

### V2 — "Diplomacy-assisted terminals + escalation"

- Vassal/federated seats count toward Military Domination and remove rivals from Last Realm Standing (`allow_vassal_*`), rewarding mixed conquest+diplomacy.
- Endgame escalation (`EndgameEscalationDay`): occupation unrest, rival rally (**Lane 2**), seeded site contests (**Lane 8**).
- Score victory at day 60 stalemate (weights pending **Lane 9**).
- Lower raw-conquest thresholds since diplomacy now contributes.

---

## 4. Replayability levers

1. **Approach choice** — five enabled approaches × four playable factions = distinct win paths; the HUD shows all enabled approaches from day 3 so the player chooses deliberately.
2. **Starting doctrine** — see `starting_doctrines.md`: each faction gets 2-3 start-state overlays (e.g. Human "Crown Militant" vs "Mercantile League") that re-weight starting army, pools, and which approach is cheapest.
3. **Start position asymmetry** — **[VCS]** the start state already gives each faction a distinct seat, secondary settlement, safe-expansion target, and frontier neighbours (e.g. humans border `coastal_ruins`/`northwest_march`; orcs sit in the eastern badlands ruins). Geography is **not** redesigned (prompt rule); it is *leveraged*.
4. **Passive-faction pressure** — Nature shrines and Dark fortress sit on different approaches' critical paths, so the "neutral" map edges reward different openings per approach.

---

## 5. Integration order (summary — full in HANDOFF.md)

1. SoulCore: `SoulCampaignVictory.h` + `FSoulVictoryRules` + unit tests (mirror `SoulMechanicsTests.cpp`).
2. Subsystem: `FSoulCampaignVictoryState` member + facts assembly + two evaluation hooks.
3. Save: schema bump 1→2, capture/restore `"victory"`.
4. SoulCore: `SoulCampaignObjectives.h` + `FSoulCampaignObjectiveRules`.
5. Data: objective/victory parameter block appended as a *new* data file (never editing locked start-state/overmap files).
6. HUD: objective + victory widgets on the existing Kenney HUD.
7. V1/V2 wiring to Lanes 4/8/9/2/3 behind the `ActiveApproachMask`.

---

## 6. Contradictions against current assumptions

- **"The founder slice has an objective (orc_camp)"** — true in *data* (`enemy_objective`) but **[VCS] there is no code that declares a win when it is captured.** The campaign silently continues. V0 fixes exactly this.
- **"Diplomacy V0 exists, so an alliance victory is close"** — **[VCS]** Diplomacy V0 (`FSoulDiplomacyRules`) only mutates stance/relation; there is **no** federation/vassal concept and no AI acceptance pressure. Alliance victory is **blocked on Lane 4** and must ship LOCKED in V0.
- **"Six-faction sandbox is the campaign"** — **[VCS]** `soul_campaign_start_states_v1_...json` marks it `CANDIDATE_NONCANONICAL`; the only `LOCKED` scenario is the Human-Orc founder micro. V0 victory must target the founder micro; six-faction victory is V1.
- **"Strategic AI will contest victory"** — **[VCS]** `FSoulStrategyAI` exists but is wired only into tests + `SoulFoundationBridge`; `AdvanceEnemyAI()` is inert. Rival-win loss conditions are therefore dormant until **Lane 2**.
