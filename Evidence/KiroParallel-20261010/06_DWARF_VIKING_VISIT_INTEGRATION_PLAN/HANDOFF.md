# HANDOFF — Lane 6: Next Walkable Settlements (Dwarf Hold + Viking Harbour)

- **Lane:** `06_DWARF_VIKING_VISIT_INTEGRATION_PLAN` (prompt pack LANE 6 — "NEXT WALKABLE SETTLEMENTS: DWARF HOLD + VIKING HARBOUR")
- **Repo:** `Jgnels/Soul`
- **Source snapshot (frozen, verified EXACT):** `handoff/soul-kiro-20261010` = `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`
- **Prompts source:** `handoff/soul-kiro-prompts-20261010` = `dbdeb082146ffbe9e66e5b69a4ca8edec44afb2d`
- **This branch:** `kiro/dwarf-viking-visit-integration-plan-20261010` (created from the frozen source commit)
- **Asset metadata source:** `Jgnels/Copperlight-Asset-Catalog` (read-only)
- **Boundary honoured:** No runtime `Source/`, `Config/`, `Content/`, `Soul.uproject`, maps, or saves edited. Writes confined to `Evidence/KiroParallel-20261010/06_DWARF_VIKING_VISIT_INTEGRATION_PLAN/`. No PR, no merge, no purchases, no invented `/Game/...` paths or ownership.

Evidence labels: **[VCS]** verified current snapshot · **[VOA]** verified owned asset metadata · **[LRC]** local runtime/asset check required (outranks cloud guesses) · **[DP]** design proposal.

## Artifacts in this directory
| File | Purpose |
|---|---|
| `dwarf_visit_plan.md` | Exact Dwarf Hold visit plan: current bindings, local checks, order, acceptance tests, unknowns |
| `viking_visit_plan.md` | Exact Viking Harbour visit plan: owned-asset recipe, net-new authoring, order, tests, unknowns |
| `visit_registry_extensions.json` | Proposed `viking_harbour` registry entry + `FSoulSettlementVisitProfile` shapes + per-settlement profiles |
| `dependency_checklist.json` | Code/data/asset/cross-lane dependencies with exact files, symbols, risk, owners |
| `generic_visit_binder_spec.md` | The smallest generic visit-binder architecture that removes Human-only visitation |
| `HANDOFF.md` | This file |

---

## Key conclusions

1. **The visit *entry* and *gate* are already faction-generic. [VCS]**
   `ASoulFounderPlaytestCampaignActor::VisitSettlement()` (`SoulFounderPlaytestCampaignActor.cpp:331-339`)
   opens the bound scenario's `OwnedEnvironmentMap` with `game=/Script/Soul.SoulSettlementVisitGameMode`,
   and `ASoulSettlementVisitGameMode::CanVisit()` (`SoulSettlementVisitGameMode.cpp:44-67`) validates by
   scenario/region/ownership — **neither contains a `human_capital` literal.** Visitation is *not*
   Human-only at the architecture level.

2. **Human-only coupling is isolated to exactly six code sites, all presentation. [VCS]**
   Walk auto-start gated on `human_capital` (`SoulSettlementVisitGameMode.cpp:113-115, 122-123`),
   hardcoded Paragon-Aurora avatar + four Human forecourt entry probes in `StartWalking()` (245-279),
   hardcoded Rowan Knight companion in `RefreshCompanion()` (144-220), and the compile-time Human
   development-scenario default in `InitializeScenario()` (`SoulFounderPlaytestStateSubsystem.cpp:341-344`).

3. **Dwarf Hold is already overview-visit-ready in data; the walk + four-faction selection are the gaps. [VCS]**
   `dwarf_hold` has a registry entry, a committed `DA_Soul_DwarfHold_DevelopmentProof` (with
   `service.tavern_hero` on `dwarf.caravan_hall`), miniature meshes, and a passing isolated runtime proof
   (`DwarfHoldRuntimeProof.json`: visit/save/return pass). It is `battle_enabled:false` (visit-only), which
   is correct for this lane.

4. **Viking Harbour is greenfield for environments but well-supported by owned assets. [VCS + VOA]**
   No registry entry, no development proof, no authored map in the snapshot — only world/overmap data
   (`city.viking_harbour`, `cliff_harbour`/`shore_bridge`, `viking.harbour_edge`) and roster ids
   (`viking_axe_warrior`). The established design (`Docs/FACTION_CITY_AND_SIEGE_PLAN_20260920.md`) specifies
   **Water City geography + Viking Village cultural kit**, and both are **owned** (plus a Nanite Medieval
   Harbor Kit, Nordic Fishing Hut, Viking Warrior avatar, longboats).

5. **The smallest generic binder is a data-driven `FSoulSettlementVisitProfile`** (immutable presentation
   input, no new subsystem/save/encounter authority) that the already-generic transition/gate consume,
   with graceful degradation to the existing overview camera when walk data is absent. See
   `generic_visit_binder_spec.md`.

---

## Contradictions against current assumptions

- **"Visitation is Human-only" is only half true.** The *overview* visit is already generic; a Dwarf Hold
  overview visit should work today the moment a `dwarf_hold` `ASoulTownViewAnchor` is confirmed in the
  authored map. Only *walking embodiment* and *four-faction scenario selection* are Human-locked. [VCS]
- **"Viking kits are imported" is misleading.** The Copperlight catalog marks the Viking-village products
  `Imported=True`, but they are imported into a **sibling** project (`Copperlight-first-night-living-hearth`,
  `/Game/Viking_village`), and **no Soul project appears in the catalog's imported-Unreal summary at all.**
  Soul-side presence is **[LRC]**, not confirmed. [VOA + LRC]
- **Dwarf Hold's source environment ownership is uncorroborated.** Soul's own data asserts ownership of
  `/Game/DwarvenCitadel` (Fab listing `836bcc0d-...`), but that listing is **absent from the entire
  Copperlight catalog**, whereas the Human source (CastleTown / listing `42d4a792`) resolves to the owned
  "Medieval Kingdom" product. Flag for ownership verification. [VOA-negative]
- **Design-doc vs snapshot drift (minor):** `FACTION_CITY_AND_SIEGE_PLAN` lists the Dwarf base as "Modular
  Legendary Forge", but the current proof uses `/Game/DwarvenCitadel`. Not a blocker; note for art lineage.

---

## Highest-priority integration action for Astra

**Make the development-scenario selection data-driven per owned settlement** (dependency `D2`,
`SoulFounderPlaytestStateSubsystem.cpp:341-344`), replacing the compile-time
`DA_Soul_HumanCapital_DevelopmentProof` default with a `{settlementId -> FSoftObjectPath}` lookup seeded
with the existing Human + Dwarf proofs.

Rationale: Dwarf Hold already has a complete, committed, test-passing authored proof chain, yet the
four-faction Heartland playtest can never bind it (or any non-Human settlement) because scenario selection
is hardcoded. Fixing D2 unlocks a **Dwarf Hold overview visit end-to-end with essentially zero new art**
(pending the one `ASoulTownViewAnchor` local check, A1), and is the shared prerequisite that lets the
generic visit binder (D1/D3) and the future Viking Harbour map (A2) actually reach players. It is the
smallest change with the largest unlock, and it touches a single, well-isolated function.

Immediately after D2: implement the generic `FSoulSettlementVisitProfile` binder (D3 then D1) so walking
embodiment, entry points, and companions become data — reproducing today's exact Human behaviour via the
`human_capital` profile while enabling Dwarf/Viking overview-first visits.
