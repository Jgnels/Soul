# HANDOFF — Lane 05: Dwarf / Orc / Viking Settlement Development Trees

- **Lane:** 05_FACTION_SETTLEMENT_BUILDING_TREES
- **Branch:** `kiro/faction-settlement-building-trees-20261010`
- **Frozen source snapshot:** `handoff/soul-kiro-20261010` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c` (verified exact at session start)
- **Scope guard:** writes ONLY under `Evidence/KiroParallel-20261010/05_FACTION_SETTLEMENT_BUILDING_TREES/`. No runtime `Source/`, `Config/`, `Content/`, `.uproject`, maps or saves touched. No PR, no merge.
- **Authority sources inspected:** `Jgnels/Soul` (local snapshot) + `Jgnels/Copperlight-Asset-Catalog` (asset metadata, read-only).

## Artifacts in this directory

| File | Purpose |
|---|---|
| `dwarf_building_tree.json` | Dwarf development tree (11 nodes: 4 seed civic/econ/fort + caravan service + 6 proposed military/magic/beast). |
| `orc_building_tree.json` | Orc development tree (12 nodes; scar/war-camp identity, fast-tempo military). |
| `viking_building_tree.json` | Viking development tree (12 nodes; maritime economy gates elite capstone). |
| `physical_manifestation_map.md` | How each building ID becomes physical using `ASoulSettlementBuildingActor` + owned assets; contradictions. |
| `nature_dark_future_notes.md` | Lighter Nature/Dark skeleton sketches, explicitly NOT activated. |
| `acceptance_tests.md` | Test matrix extending existing automation tests; honest expected-fail register. |
| `implementation_order_and_dependencies.md` | Exact current symbols, 5-phase additive plan, cross-lane dependency matrix. |
| `HANDOFF.md` | This file. |

---

## What the proven Human system actually is (VERIFIED FROM CURRENT SNAPSHOT)

- Buildings are a **data-driven, presentation-independent** system. Canonical struct `FSoulBuildingDefinition` (`Source/SoulCore/Public/SoulSettlement.h:16`) has `Id, Category, MaxLevel, BuildDays, BuildCost(TMap<FName,int32>), Prerequisites, UnlockIds`. State machine `ESoulBuildingCondition {Unbuilt,Building,Intact,Damaged,Ruined}`.
- Rules live in `FSoulSettlementRules` (`SoulSettlement.h:56` / `SoulSettlement.cpp`): `PrerequisitesMet`, `CanBeginConstruction` (also enforces `Level<MaxLevel`, so upgrade tiers are re-construction), `BeginConstruction` (sets `ConstructionDaysRemaining=max(1,BuildDays)`), `AdvanceDay` (decrement → on 0, Level++, integrity 1000, Intact), `IsOperational` (Level>0, Intact/Damaged, integrity≥500).
- Authoring = `USoulSettlementScenarioData` + `FSoulBuildingDevelopmentSpec` (`Source/Soul/Public/SoulSettlementScenarioData.h`); `ValidateDefinition` (`SoulSettlementScenarioData.cpp:34`) enforces unique ids, prereqs present/non-self/non-repeated, DAG (no cycles), MaxLevel≥1, BuildDays≥1, nonnegative costs, unique unlock ids.
- Founder timing 1/2/3 is clamped by the Heartland JSON loader (`SoulHeartlandContent.cpp`: "must take 1-3 days"), NOT by SoulCore.
- Faction list `{humans,dwarves,orcs,vikings,nature,dark}` (`SoulCampaign.cpp:3`). `AdmittedStrategicUnit`: humans→human_knight, dwarves→dwarf_warrior, orcs→orc_hammer_warrior, vikings→viking_axe_warrior, nature→nature_bear_warrior, **dark→NAME_None**.
- Effects are dispatched by `UnlockIds` + some HARDCODED `human.*` branches: service (`service.tavern_hero` → generic `FindUniqueServiceDefinition`/`IsTavernOperational`), recruitment (`FSoulRecruitmentPool.RequiredBuildingId`), economy/barracks L2 (hardcoded `human.market`/`human.barracks` in `SoulFounderPlaytestStateSubsystem::AdvanceDay`), spells (`RefreshHeartlandSpellLearning` + HeartlandDevelopment.json `spells`).
- Save authority `USoulSettlementStateSubsystem` ("Soul.Settlements", schema 1). Additive building ids/levels are save-safe; new persisted per-building fields or new condition enum values need a schema bump.
- Physical layer: `ASoulSettlementBuildingActor` toggles four authored geometry branches to match canonical state; miniatures via `MiniatureBaseMesh/UpgradeMesh`. Binding data in per-settlement proof JSON + `environment_asset_bindings.json`; registry in `EnvironmentRegistry.json`.

## Key design decisions

1. **Reuse, don't rename.** Each faction tree uses the real system (`FSoulBuildingDefinition`/`FSoulSettlementRules`) with faction-prefixed ids (`dwarf.* / orc.* / viking.*`) and mechanical differentiation:
   - Orc core-fighter builds in **1 day (normal)** vs Dwarf **2 days (significant)** — tempo vs quality.
   - Viking **mandatory `viking.shipyard`** gates the elite `viking.huscarl_hall` — a trade-economy prerequisite no other faction has.
   - Distinct magic schools: Dwarf Earth/Rune, Orc Lightning/Blood, Viking Water/Storm (vs Human Frost). Lightning is explicitly human-forbidden in HeartlandDevelopment.json, so it is a clean cross-faction axis.
   - Orc "scar grammar": Orc buildings baseline to the `DamagedRoot` branch (reuses existing four-branch actor, no new code).
2. **Each faction gets the Human structural parity set**: one civic root (`control.settlement`), exactly one `service.tavern_hero` carrier (lights up existing generic code with ZERO new code), one `economy.market`, one `siege.defense`, plus a founder-timed 1/2/3 build-day spread. This is proven by acceptance group A + AT-D2.
3. **Dwarf tree is grounded in an existing proof** (`DwarfHoldDevelopmentProof.json`), so it extends real data rather than inventing. Orc/Viking are authored from `settlement_blueprints.json` seeds.
4. **Apex/beast and non-core units stay DESIGN PROPOSAL** — the snapshot wires only the one signature warrior per faction.

## Verified owned asset metadata (Copperlight)

- Modular Legendary Forge `fab_bf013eb27085437f9c071cf318019c26` — CONFIRMED_OWNED (Dwarf donor).
- Modular Water City `fab_cff1623a9db641e1bc2c0b948835b41d` + Modular Viking Village `fab_9a2467f766654e4fb5f3da9875105bab` (imported) — CONFIRMED_OWNED (Viking donors).
- Medieval Ruins `fab_41faeeed3014466d9861374190d4f438` (imported) — CONFIRMED_OWNED (Orc donor).
- Dwarf_Pack troop meshes already wired in Soul runtime (Bedvar/Broddi).

---

## Contradictions against current assumptions

- **C1 — `/Game/DwarvenCitadel` is NOT in the Copperlight catalog (ZERO references).** `DwarfHoldDevelopmentProof.json`/`EnvironmentRegistry.json` depend on it, but the only OWNED forge-city donor is Modular Legendary Forge. The proof's own `representation_scope` is a provisional "Caravan Hall cutaway plus authored gate," not a full citadel. **Downgrade `/Game/DwarvenCitadel` from assumed-owned to LOCAL RUNTIME-ASSET CHECK REQUIRED.** (physical_manifestation_map.md CONTRADICTION-D1.)
- **C2 — Dwarf caravan hall timing mismatch.** Proof says `build_days:2`; lane founder-timing says civic service = 1 (normal). Needs a founder ruling. (D2.)
- **C3 — No apex pipeline.** All beast dwellings are COMPOSITE with no confirmed apex mesh/animation; only `*_warrior` units are wired. Apex buildings must not be promised as buildable.
- **C4 — Dark has no strategic unit** (`AdmittedStrategicUnit(dark)==NAME_None`), so a Dark recruit tree is impossible today; Dark future notes are civic/economy/defense only.
- **C5 — Hardcoded Human effects.** Economy/barracks/spell effects are coded against literal `human.*` ids; faction trees need these generalized (Phase 3) or they silently do nothing for non-Human settlements. AT-E3/E4 are expected-fail until then.

## Explicit unknowns

- U1: Ownership/local presence of `/Game/DwarvenCitadel` and the Soul proxy meshes `SM_DwarfHold_*`.
- U2: Whether Modular Legendary Forge / Water City are imported into the Soul content root (Copperlight local evidence = None/partial).
- U3: Real unit ids + meshes for every non-core unit (dwarf_crossbow, orc_hunter, viking_shieldmaiden, …) and all apex beasts.
- U4: Faction magic schools (Earth/Lightning/Water-Storm) need real `Magic.Spell.*` gameplay tags confirmed against the owned tag set.
- U5: Orc/Viking authored maps + registry entries do not exist (Orc fully greenfield; Viking is Lane 6 scope).
- U6: Support donors (YI_BanditCamp, Coastal Ruins, Ancient Mountain) ownership not confirmed in Copperlight.

---

## Integration summary for Astra

The three trees are **data-authorable today** against the existing settlement system with no new authority and no save-schema change; validation, construction/timing, the generic hero-service unlock, and save round-trip all pass with pure authoring. Two small additive code generalizations (effect dispatch off unlock ids; faction-aware spell/recruit gating) unlock the economy/magic effects for non-Human factions. Physical art and non-core/apex rosters are explicitly deferred and labeled.

### Highest-priority integration action
**Resolve CONTRADICTION-C1 before any Dwarf physical work:** confirm locally whether `/Game/DwarvenCitadel` is a genuine owned/imported asset or whether the Dwarf Hold must be re-based on the CONFIRMED_OWNED **Modular Legendary Forge**. The Dwarf tree is the fastest faction to ship (it already has a registry entry, miniatures, and a DevelopmentProof), so this one asset-ownership fact is the gate on the whole lane's first playable deliverable. Pair it with Phase 1 data authoring (extend `DA_Soul_DwarfHold_DevelopmentProof` gameplay-only) and coordinate building IDs with **Lane 6** (walkable Dwarf Hold / Viking Harbour) and gold magnitudes with **Lane 9**.
