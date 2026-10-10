# Dwarf Hold — Walkable Visit Implementation Plan

Lane 6. Source snapshot `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`.
Labels: **[VCS]** current snapshot · **[VOA]** owned asset metadata · **[LRC]** local runtime/asset check required · **[DP]** design proposal.

> Scope note: this is the *walkable visit* plan (Lane 6). Faction building trees are Lane 5; roster
> balance is Lane 2. Where this plan names building/service IDs it only reuses the IDs that already
> exist in the current Dwarf Hold development proof.

---

## 0. Headline: Dwarf Hold is the most advanced non-Human settlement in the snapshot

Dwarf Hold is **not greenfield**. The snapshot already contains a complete authored-environment proof
chain. The remaining work is strictly: (a) generalize the Human-only walk binder (see
`generic_visit_binder_spec.md`), (b) author/measure a Dwarf entry point **[LRC]**, and (c) wire the Dwarf
development scenario into the four-faction playtest selection.

---

## 1. Current bindings (VERIFIED FROM CURRENT SNAPSHOT)

### 1.1 Environment registry row [VCS]
`Data/SettlementEnvironments/EnvironmentRegistry.json`, entry `dwarf_hold`:
```json
{
  "settlement_id": "dwarf_hold",
  "faction": "dwarves",
  "settlement_type": "hold",
  "authored_environment_available": true,
  "visit_environment": "/Game/Soul/Maps/Settlements/L_DwarfHold_Authored",
  "city_battle_environment": "/Game/Soul/Maps/Settlements/L_DwarfHold_Authored",
  "siege_environment": null,
  "field_battle_environment": null,
  "arena_origin": null,
  "development_profile": "/Game/Soul/Data/Settlements/DA_Soul_DwarfHold_DevelopmentProof",
  "battle_enabled": false,
  "note": "Preserved authored proof; Alpha approach admission unset."
}
```
Interpretation: a visit environment **is bound**; battle is intentionally disabled
(`battle_enabled:false`) and no siege is set. The registry loader only requires origin/siege data when
`battle_enabled` is true, so this row loads cleanly as a visit-only binding
(`SoulSettlementEnvironmentRegistry.cpp:33-47`). [VCS]

### 1.2 Development scenario data asset [VCS]
`Content/Soul/Data/Settlements/DA_Soul_DwarfHold_DevelopmentProof.uasset` is **committed to git**
(confirmed via `git ls-files Content/`). Its authoring source is
`Data/SettlementEnvironments/DwarfHoldDevelopmentProof.json`:
- `settlement_id`/`region_id`: `dwarf_hold`
- `faction_id`: `humans` (a Human expedition owns the canonical Dwarven hold in the proof; architecture stays Dwarven)
- `owned_environment_map`: `/Game/Soul/Maps/Settlements/L_DwarfHold_Authored`
- `source_environment`: `/Game/DwarvenCitadel/Maps/DwarvenCitadel`
- `environment_listing`: `https://www.fab.com/listings/836bcc0d-025f-437c-bc0c-42c8b644b0eb`
- miniature meshes: `/Game/Soul/CampaignProxies/Dwarven/SM_DwarfHold_Base_r9`, `..._Upgrade_r2`
- initial buildings: `dwarf.kings_hall`, `dwarf.gates`, `dwarf.great_forge`, `dwarf.mine` (built), `dwarf.caravan_hall` (unbuilt)
- development definition: `dwarf.caravan_hall` → `category civic`, `build_days 2`, `cost {gold:200}`, `prerequisites [dwarf.kings_hall]`, `unlock_ids [service.tavern_hero]`

The `service.tavern_hero` binding to `dwarf.caravan_hall` is what satisfies
`InitializeSettlementDevelopment`'s requirement of exactly one `service.tavern_hero` building
(`SoulFounderPlaytestStateSubsystem.cpp:380-382`). So the Dwarf proof is already a legal development
scenario. [VCS]

### 1.3 Working runtime proof scenario [VCS]
`Data/SettlementEnvironments/DwarfHoldRuntimeProof.json` (loaded under `-SoulDwarfSettlementProof`,
`SoulFounderPlaytestStateSubsystem.cpp:155`) states: *"construction/visit/save and natural victory return
pass with real reserve arrivals on both sides; capital art remains provisional."* It uses
`battle_map: /Game/Soul/Maps/Settlements/L_DwarfHold_Authored`, `arena_origin [-13500,0,-750]`, player
start region `dwarf_hold` owned by `humans`. This proves the authored Dwarf map already streams, visits,
and returns in an isolated run. [VCS]

### 1.4 Canonical overmap node [VCS]
`Data/soul_world_overmap_v1_20260922.json` node `dwarf_hold`: `kind capital`,
`settlement_id city.dwarf_hold`, `biome mountain`, `landform fortified_hold`, `feature great_forge`,
`battle_recipe_hint dwarf.forge_approach`, notes "Dwarf faction seat / Automaton Colossus homeland".
Adjacency and region IDs (`dwarf_forge_approach`, `dwarf_snow_basin`, `dwarf_high_quarry`,
`dwarf_mountain_pass`) are the retained canonical Crownspine graph. [VCS]

---

## 2. Owned asset metadata (VERIFIED OWNED ASSET METADATA / CONTRADICTION)

- **CastleTown / "Medieval Kingdom"** (Human proof source, listing `42d4a792-...`) resolves in the
  Copperlight catalog as owned (`Owned=True`, `OwnedConfidence=HIGH`,
  `ProvenanceStatus=OWNERSHIP_CONFIRMED`). [VOA]
- **CONTRADICTION / UNKNOWN:** the Dwarf Hold's source environment Fab listing
  `836bcc0d-025f-437c-bc0c-42c8b644b0eb` (`/Game/DwarvenCitadel`) **does not appear anywhere in the
  Copperlight catalog** (`products.json`, `raw/epic_library.json`, `founder_ownership.json`,
  `local_asset_admissions.json` all negative). The only ownership assertion for DwarvenCitadel is inside
  Soul's own proof data file. Ownership is therefore *asserted by Soul but not corroborated by the
  catalog*. This is a **[LRC]** + ownership-verification item, not a blocker (the proof data and
  committed data asset indicate it has already been used locally). [VOA-negative]
- "Dwarfs Pack" is owned (`Owned=True`) in the catalog but is a character/other pack, not the citadel
  environment. [VOA]
- No Soul project appears in the catalog's `imported_unreal_summary.json` (23 projects, all
  `Copperlight-*`/donor), so the catalog cannot confirm what is imported into **Soul** specifically.
  Soul-side asset presence is **[LRC]**. [VOA-negative]

---

## 3. Visible persistent / sublevel structure & service anchors — LOCAL CHECK REQUIRED

The authored map `L_DwarfHold_Authored` is **not in git** (licensed-derived local content, consistent
with the `.gitignore` pattern for `/Content/Dwarf_Pack/`, `/Content/Kingdom_Capital/`, etc.). Everything
below is **[LRC]** — resolve with a local editor/PIE pass; local Unreal evidence outranks these guesses:

1. **[LRC]** Confirm `L_DwarfHold_Authored` exists under `/Game/Soul/Maps/Settlements/` locally and
   opens. The `DwarfHoldDevelopmentProof` representation note describes it as a "Native Caravan Hall
   cutaway plus authored gate with deterministic scale/spacing compression" — so the walkable interior
   footprint may be smaller than the Human capital.
2. **[LRC]** Enumerate persistent vs streaming sublevels (`UWorld::GetStreamingLevels()`), mirroring the
   survey logic in `SoulAuthoredEnvironmentSurvey.cpp:154-228`, to learn whether gate/forge/hall are
   persistent or streamed (affects `IsStreamingReady()` timing on entry).
3. **[LRC]** Confirm an `ASoulTownViewAnchor` with `SettlementId == dwarf_hold` is placed in the map.
   The visit GameMode requires a matching anchor to reach `bCameraReady`
   (`SoulSettlementVisitGameMode.cpp:107-127`); without it the overview visit times out at 20 s and
   fails. **This is the single highest-value local check for Dwarf Hold** because the overview visit
   works the moment that anchor exists.
4. **[LRC]** Confirm an `ASoulSettlementPresentationController` with `SettlementId == dwarf_hold` exists
   so construction state (built/absent/mid) is reflected (`RefreshPresentation`,
   `SoulSettlementVisitGameMode.cpp:88-96`).
5. **[LRC]** Measure a physically clear ground entry point near the Caravan Hall / gate forecourt
   (walkable V1) with a downward line trace and a standing-capsule sweep, exactly as `StartWalking`
   /`RefreshCompanion` already do. Record it in the Dwarf visit profile `EntryProbePoints`.

---

## 4. Likely spawn / return points

- **Return:** generic and unchanged. `HandleAction("Return")` → `OpenLevel(State->CampaignMap, ...)`
  with `game=/Script/Soul.SoulFounderPlaytestGameMode` once streaming settles
  (`SoulSettlementVisitGameMode.cpp:131-158`). Dwarf Hold needs no new return logic. [VCS]
- **Spawn (overview):** `ASoulTownViewAnchor::ActivateForPlayer` possesses the camera; already generic. [VCS]
- **Spawn (walk):** [DP] data-driven `EntryProbePoints` from the Dwarf visit profile, resolved by a
  measured gate/forecourt point from §3.5. V0 may ship `bWalkable=false` (overview only).

---

## 5. Battle / siege candidate status (do not expand V0)

- `battle_enabled:false`, `siege_environment:null`, `arena_origin:null` in the registry row. [VCS]
- The registry loader forbids a siege for anything but `human_capital`
  (`SoulSettlementEnvironmentRegistry.cpp:40-46`): *"Siege requires explicit native gate origin and owned
  Human Capital map."* So Dwarf Hold **cannot** carry a siege under the current loader. [VCS]
- **DESIGN PROPOSAL (defer):** a Dwarf field/approach battle (`dwarf.forge_approach` hint already exists
  in the overmap) could later be enabled by adding `arena_origin` + `authored_environment_available` +
  `battle_enabled:true`, but Lane 6 is visitation; sieges beyond Human Capital Siege V0 are Lane 7.
  Keep Dwarf Hold **visit-only** in this lane. [DP]

---

## 6. Dependency risks (Dwarf-specific)

1. Scenario selection hardcodes the Human proof unless `-SoulDwarfSettlementProof` is set
   (`SoulFounderPlaytestStateSubsystem.cpp:341-344`): in the four-faction Heartland playtest a player who
   owns Dwarf Hold still gets the Human development scenario. **Blocks a four-faction Dwarf visit.** [VCS]
2. Walk auto-start is `human_capital`-only (B1/B2). Even with a Dwarf anchor placed, the walk never
   starts for Dwarf Hold today. [VCS]
3. DwarvenCitadel ownership not corroborated by the catalog (§2). [VOA-negative]
4. Authored map not in git → all geometry/anchor/collision facts are **[LRC]**.
5. `faction_id` in the Dwarf proof is `humans` (expedition-owns-hold fiction). A real four-faction Dwarf
   player owns the hold as `dwarves`; `CanVisit` checks `Settlement->FactionId == PlayerFaction`, so the
   production four-faction scenario must set Dwarf Hold's owner/faction coherently. [VCS]

---

## 7. Implementation order (Dwarf Hold)

1. **[LRC]** Local check #3 (place/verify `ASoulTownViewAnchor` id=`dwarf_hold`) → unlocks the overview
   visit immediately with zero code change. *(Verification + possibly a map edit done by the Unreal lane,
   not here.)*
2. **[DP]** Implement the generic visit binder (§3 of `generic_visit_binder_spec.md`): replace B1–B4/B6.
3. **[DP]** Add the per-settlement development-scenario map in scenario selection (binder spec §3.4) and
   register `dwarf_hold → DA_Soul_DwarfHold_DevelopmentProof`.
4. **[LRC]** Measure a Dwarf entry point (#5); add the Dwarf visit profile with `bWalkable=true` and a
   Dwarf-appropriate owned avatar mesh (candidate: Dwarfs Pack character **[VOA, LRC for rig/anim]**).
   Until measured, ship `bWalkable=false`.
5. Run acceptance tests (`acceptance_tests` section below).

---

## 8. Acceptance tests (Dwarf Hold)

Isolated-fixture style, matching existing `-SoulDwarfSettlementProof` runs. These are **[LRC]** (require
a local editor/PIE pass with the authored map):

- **DH-A1 Overview visit:** Launch the Dwarf development/runtime proof; from the owned `dwarf_hold`,
  trigger `VisitSettlement`. Expect `L_DwarfHold_Authored` to load, `SOUL_SETTLEMENT_VISIT_READY
  settlement=dwarf_hold` logged, overview camera active, management + return available, no walk.
- **DH-A2 Return:** From the visit, press Esc/Return. Expect clean `OpenLevel` back to the campaign map
  with no pending battle and streaming settled (no 15 s forced-return timeout hit under normal load).
- **DH-A3 Construction reflects in visit:** Build `dwarf.caravan_hall`; re-enter. Expect the presentation
  controller to reflect the newly built/mid state (`RefreshPresentation` revision bump).
- **DH-A4 Save/Load parity:** F5 during visit, F9; expect `CanVisit` re-validates and, if restored
  elsewhere, auto-returns (`SoulSettlementVisitGameMode.cpp:160-172`). No embodiment leak into stale map.
- **DH-A5 (only after walk profile):** With `bWalkable=true` and a measured entry, expect
  `SOUL_CITY_WALK_READY map=.../L_DwarfHold_Authored` and the avatar standing on physical ground, WASD
  movement blocked by authored walls (collision physical, never teleport-through).
- **DH-A6 Negative:** With no `dwarf_hold` town-view anchor present, expect the 20 s camera-missing
  failure path and a clean message, not a crash.

Unit-test hook (snapshot-local, no map needed): extend
`Source/Soul/Private/Tests/SoulSettlementSaveTests.cpp` / `SoulSettlementDevelopmentTests.cpp` to assert
the Dwarf development scenario validates (`ValidateDefinition`) and that `service.tavern_hero` resolves to
`dwarf.caravan_hall`. These run in the existing automation harness.

---

## 9. Explicit unknowns (Dwarf Hold)

- **[LRC]** Does `L_DwarfHold_Authored` currently contain a `dwarf_hold` `ASoulTownViewAnchor` and
  `ASoulSettlementPresentationController`? (Determines whether overview visit works today.)
- **[LRC]** Interior walkable footprint size and a clear entry point (the "cutaway" note hints it is
  compressed/partial).
- **[VOA-negative]** True Fab ownership/licence of `/Game/DwarvenCitadel` (not in catalog).
- **[LRC]** Whether a Dwarf-appropriate avatar + idle/jog animation set is importable/retargetable from
  owned packs (Dwarfs Pack) for a walkable V1; otherwise Dwarf Hold stays overview-only, which is fine.
- **[DP/open]** Whether the four-faction production scenario (not the proofs) will present Dwarf Hold with
  `faction_id: dwarves` ownership so a Dwarf-faction player can visit — needs campaign-lane coordination.
