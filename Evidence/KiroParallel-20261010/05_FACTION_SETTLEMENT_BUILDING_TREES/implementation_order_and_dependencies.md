# Implementation Order, Dependencies & Conflicts — Lane 05

Audience: Codex (the Unreal integration lane) + Astra (integration owner).
Everything here is additive to the current snapshot and preserves the single settlement/save/combat authority.

---

## 0. Exact current symbols the smallest clean implementation touches

| Concern | Current symbol / file | Change class |
|---|---|---|
| Building definition | `FSoulBuildingDefinition` — `Source/SoulCore/Public/SoulSettlement.h:16` | REUSE unchanged |
| Settlement rules (prereq/construct/day/operational) | `FSoulSettlementRules` — `Source/SoulCore/Public/SoulSettlement.h:56`, impl `Source/SoulCore/Private/SoulSettlement.cpp` | REUSE unchanged |
| Authoring DataAsset + spec | `USoulSettlementScenarioData`, `FSoulBuildingDevelopmentSpec` — `Source/Soul/Public/SoulSettlementScenarioData.h` | REUSE; author new instances |
| DAG/validation | `USoulSettlementScenarioData::ValidateDefinition` — `Source/Soul/Private/SoulSettlementScenarioData.cpp:34` | REUSE unchanged |
| Day-range clamp (1-3) | `FSoulHeartlandContent::Load` — `Source/Soul/Private/SoulHeartlandContent.cpp` | MIRROR into a faction loader |
| Construction entry | `USoulFounderPlaytestStateSubsystem::BeginSettlementConstruction` — `Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp:420` | REUSE; it is already settlement-id generic |
| Service lookup | `USoulSettlementScenarioData::FindUniqueServiceDefinition` + `IsTavernOperational()` | REUSE unchanged (already generic over `service.tavern_hero`) |
| Recruitment gate | `FSoulRecruitmentPool.RequiredBuildingId`, `FSoulTownRules::RecruitFromBuilding` — `Source/SoulCore/Private/SoulTown.cpp:62` | REUSE; set RequiredBuildingId to faction building |
| Economy/barracks L2 effect | HARDCODED `human.market`/`human.barracks` in `USoulFounderPlaytestStateSubsystem::AdvanceDay` (~line 649-653) | GENERALIZE (additive) — see Phase 3 |
| Spell gating | `RefreshHeartlandSpellLearning` + `HeartlandDevelopment.json` spells | GENERALIZE (additive) — see Phase 3 |
| Save authority | `USoulSettlementStateSubsystem` ("Soul.Settlements", schema 1) — `Source/Soul/Private/SoulSettlementStateSubsystem.cpp` | REUSE unchanged (additive ids are save-safe) |
| Faction→profile hook | `FSoulFactionDefinition.SettlementProfileId` (currently UNUSED) — `Source/SoulCore/Public/SoulFaction.h:19` | ADOPT to bind faction→DataAsset instead of hardcoding |

**Smallest clean authority:** the faction trees live as DATA (`USoulSettlementScenarioData` + JSON). The only *code* additions are (a) a faction development loader that mirrors the Heartland 1-3 day clamp, and (b) generalizing two hardcoded `human.*` effect branches so they key off unlock ids. No new subsystem, no new save domain.

---

## Phase 1 — Pure data (no code), ships immediately. Dwarves first.

1. Resolve CONTRADICTION-D1 locally: confirm whether `/Game/DwarvenCitadel` is a real local asset or the Dwarf hold should be rebuilt on **Modular Legendary Forge** (CONFIRMED_OWNED). Update Copperlight either way.
2. Extend `DA_Soul_DwarfHold_DevelopmentProof` (and a new `DwarfHoldDevelopment.json`) with the civic/economy/first-military nodes from `dwarf_building_tree.json`, GAMEPLAY-ONLY (art later), exactly as `HeartlandDevelopment.json` shipped market/barracks before art.
3. Author `VikingHarbourDevelopment.json` + `OrcCampDevelopment.json` using `viking_building_tree.json` / `orc_building_tree.json`. These validate and construct without art.
4. Run acceptance group A, B, AT-E1, D. These should PASS.

Dependencies: none external. Conflicts: none (data under Data/ + new DataAssets; this lane only *designs* — Codex authors the runtime assets).

## Phase 2 — Recruitment bindings (minimal code)

1. For each faction, add a core-fighter `FSoulRecruitmentPool` with `RequiredBuildingId = <faction>.<core_barracks>` and `UnitId = <faction>_<core_warrior>` (dwarf_warrior / orc_hammer_warrior / viking_axe_warrior — all wired). This mirrors the Human pool seeding.
2. Generalize `CanRecruitHumanCompany` into a faction-agnostic `CanRecruitCompany(FactionId, UnitId)` OR add parallel faction checks. Prefer the generic form.
3. Run AT-E2 for core units. Non-core units stay gated behind Phase 5 (roster).

Dependencies: Lane 2 (AI army composition) and Lane 10 (unit roles) should agree on faction unit ids before non-core units are added. Conflict risk: if Lane 2/Lane 3 also touch recruitment pools, coordinate the pool-seeding site.

## Phase 3 — Effect generalization (additive code, unblocks AT-E3/E4)

1. Replace the hardcoded `if (building == human.market ...) +100 gold` and `human.barracks ... +2 growth` in `AdvanceDay` with a loop that reads `UnlockIds` (`economy.market`, `recruitment.growth`) from the settlement's development definitions and applies the same bonuses to ANY faction building carrying those unlocks.
2. Generalize `RefreshHeartlandSpellLearning` to read spell→building bindings from the active faction's development JSON `spells` block (same shape as HeartlandDevelopment.json), not a human-only map.
3. Run AT-E3, AT-E4.

Dependencies: Lane 9 (economy balance) owns the actual +gold/+growth magnitudes — Phase 3 should read values Lane 9 sets, not invent them. Conflict risk: AdvanceDay is a hot path edited by several lanes; keep the change a self-contained helper.

## Phase 4 — Physical manifestation (authoring + local UE)

Per `physical_manifestation_map.md` §5: Dwarves (resolve D1) → Vikings (needs Lane 6 map) → Orcs (greenfield). Place `ASoulSettlementBuildingActor`s, populate state-branch actor groups with owned geometry, bind the DataAsset. All LOCAL RUNTIME-ASSET CHECK REQUIRED.

Dependencies: **Lane 6 owns the walkable Viking Harbour + Dwarf Hold visit maps** — Viking/Dwarf physical trees must bind to Lane 6's authored actors. Hard dependency; coordinate building IDs so Lane 6 binders target the same `viking.*`/`dwarf.*` ids used here.

## Phase 5 — Non-core rosters & apex (separate lane)

Every non-`*_warrior` unit and all apex/beast dwellings are DESIGN PROPOSAL only. They need a real unit id + mesh + animation pipeline. Do NOT ship these buildings as recruit-enabled until that roster lane lands. Keep their definitions in the tree (so the DAG is stable) but mark their pools absent.

---

## Cross-lane dependency matrix

| This lane needs / affects | Lane | Nature of coupling |
|---|---|---|
| Walkable Dwarf Hold + Viking Harbour maps to bind physical building actors | **Lane 6** | HARD — share building IDs; Lane 6 binders target `dwarf.*`/`viking.*` |
| Faction unit ids & composition targets for recruitment pools | Lane 2 | MEDIUM — agree unit ids before Phase 2 non-core |
| Hero effects from buildings (governor/stewardship, magic school) | Lane 3 | MEDIUM — magic nodes propose schools; hero lane owns affinity |
| Economy magnitudes (costs, income, build-time meaning as territory grows) | **Lane 9** | HARD — Lane 9 owns gold values; this lane owns structure/prereqs only |
| Siege effects of fortification buildings (walls/gates) | Lane 7 | MEDIUM — walls/gates feed Siege V1 defender depth; do not duplicate siege authority |
| Strategic sites vs settlement economy (don't double-count) | Lane 8 | LOW — keep site income (windmill/quarry/shrine) separate from building income |

## Conflicts to avoid (shared-rules compliance)

- Do NOT create a second settlement/town/save authority. All state stays in `USoulSettlementStateSubsystem` / SoulCore.
- Do NOT hardcode new `if (faction == ...)` effect branches when generalizing is cheap — the unlock-id channel already exists.
- Do NOT promote apex beasts or non-core units as buildable without a roster lane.
- Do NOT invent `/Game/...` paths; every mesh binding in the manifestation map is tagged OWNED / LOCAL / COMPOSITE / CHECK.
