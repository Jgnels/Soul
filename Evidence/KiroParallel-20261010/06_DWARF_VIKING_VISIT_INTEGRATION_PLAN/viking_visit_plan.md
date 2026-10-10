# Viking Harbour — Walkable Visit Implementation Plan

Lane 6. Source snapshot `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`.
Labels: **[VCS]** current snapshot · **[VOA]** owned asset metadata · **[LRC]** local runtime/asset check required · **[DP]** design proposal.

> Scope note: walkable *visit* only (Lane 6). Building trees = Lane 5; roster/economy = Lanes 2/9.
> No invented `/Game/...` paths and no invented ownership below.

---

## 0. Headline: Viking Harbour is greenfield for environments, with strong owned candidates

Unlike Dwarf Hold, Viking Harbour has **no authored visit environment, no development-proof data asset,
and no environment-registry entry** in the snapshot. It exists only as world/geography data and as a unit
roster id. The design intent, however, is already written down and the required asset kinds are **owned**.
So the critical-path work is: produce a Soul-owned authored map + a development-proof data asset + a
registry entry, then reuse the generic visit binder.

---

## 1. Current bindings (VERIFIED FROM CURRENT SNAPSHOT)

### 1.1 Environment registry — ABSENT [VCS]
`Data/SettlementEnvironments/EnvironmentRegistry.json` has entries for `human_capital`, `dwarf_hold`,
`forest_edge`, `southern_crossing`. **There is no `viking_harbour` entry.** The registry's
`encounter_classes` map has no viking entry either. [VCS]

### 1.2 Development scenario / proof data — ABSENT [VCS]
No `DA_Soul_VikingHarbour_*` data asset in `git ls-files Content/` (only Dwarf Hold and Human Capital
development proofs are committed). No `Data/SettlementEnvironments/VikingHarbour*.json`. The only Viking
runtime data is `Data/CampaignComposition/VikingRuntimeProof.json`, which is an **isolated mountain
battle fixture** at `viking_snow_pass` using the Dragon Pass battle map — *not* a settlement/visit proof. [VCS]

### 1.3 Canonical overmap node — PRESENT [VCS]
`Data/soul_world_overmap_v1_20260922.json` node `viking_harbour`:
```json
{
  "id": "viking_harbour", "name": "Viking Harbour", "macro_region": "northern_fjords",
  "owner": "vikings", "biome": "cold_coast", "landform": "cliff_harbour",
  "feature": "shore_bridge", "elevation_band": "low", "kind": "capital",
  "settlement_id": "city.viking_harbour", "battle_recipe_hint": "viking.harbour_edge",
  "notes": ["Viking faction seat", "Kraken homeland access"]
}
```
Adjacency (edges + region-detail block ~L1575): `viking_harbour` ↔ `viking_fjord_ridge` (east) and
`viking_forest_track` (south_east); those two reach `viking_snow_pass`. So the authored map's approach
sides should read as "from fjord ridge (east)" and "from forest track (south-east)". The `shore_bridge`
feature and `cliff_harbour` landform confirm the water-edge + bridge motif. [VCS]

### 1.4 Roster / fixtures — PRESENT [VCS]
`viking_axe_warrior` is the Viking unit id; `city.viking_harbour` is the settlement slot;
`FourFactionBlockedFrontier.json` fixture shows "vikings: recruited 4 infantry at Viking Harbour",
confirming Viking Harbour is already treated as a recruitment-capable faction seat in alpha fixtures. [VCS]

---

## 2. Owned asset metadata (VERIFIED OWNED ASSET METADATA)

The established design (`Docs/FACTION_CITY_AND_SIEGE_PLAN_20260920.md`) says:
> **Viking harbour — Base: Water City geography + Viking Village cultural kit.**
> Town-view: boats/piers foreground, lower harbour and bridge midground, settlement climbing to the Great Hall.

Owned candidates in `Jgnels/Copperlight-Asset-Catalog` that satisfy that recipe (all `Owned=True`,
`OwnedConfidence=HIGH`):

| Product | AssetCatalogId / Fab id | Owned | LocalAvail | Imported | Role |
|---|---|---|---|---|---|
| Modular Viking Village (…Viking Village, Viking) | `fab_9a2467f766654e4fb5f3da9875105bab` / `9a2467f766654e4fb5f3da9875105bab` (listing `a72879ea-577a-4125-bde8-25fdd51060cf`) | ✅ | ✅ | ⚠️ into Copperlight only | Cultural kit (longhouses, palisade) |
| Viking Village Modular kit | `fab_ca87fe290bad4a2fb9548fc1b8ae7b87` | ✅ | ✅ | ⚠️ Copperlight only | Cultural kit (modular) |
| Viking Village | `fab_cce6557756174a938fbdf47cb613d255` | ✅ | — | ⚠️ Copperlight only | Cultural kit |
| Modular Water City Environment (Water Village, Town, Seaside) | `fab_cff1623a9db641e1bc2c0b948835b41d` | ✅ | — | ❌ | Harbour/water geography base |
| [Nanite] Medieval Harbor Kit — Harbor Props (Docks, Harbor) | `fab_6e225753c9084660bee4194b873ddb9f` | ✅ | — | ❌ | Docks/piers props |
| Nordic Fishing Hut — Coastal Lakeside Cabin | `fab_4698479bf1cf47209f541a29f1d3dfb5` | ✅ | ✅ | ❌ | Shore dressing |
| Viking Warrior (character) | catalog `character`, `Owned=True` | ✅ | — | ❌ | Walk avatar / defenders candidate |
| Viking / Viking (Customized) | `Owned=True`, LocalAvail | ✅ | ✅ | — | Avatar / companion candidate |
| Gislinge Viking Boat / Viking Longboat / Fishing Boat | `Owned=True` | ✅ | — | ❌ | Foreground boats |
| Viking War Horn Sounds (audio) | `Owned=True` | ✅ | — | ❌ | Ambience |

**Critical caveat (CONTRADICTION against a naive "Imported=True" reading):** the three Viking-Village
products all map to a *single* content root `/Game/Viking_village` imported only into
`Copperlight-first-night-living-hearth` (a sibling project), **not Soul**. The catalog itself flags the
association as token-matched and ambiguous. **No Soul project appears in the catalog's imported-Unreal
summary at all.** Therefore "owned and importable" is **[VOA]**, but "present in Soul's Content" is
**[LRC]** — must be verified/imported locally into Soul. [VOA + LRC]

Supporting local evidence already in Soul: `Evidence/viking_house_blueprint_inventory.json` enumerates a
"water_city" Viking house blueprint set (BP_Big_house_01..03, beds/tables as instanced clutter), which
matches AGENTS.md "interactive space, non-interactive clutter" guidance for a visitable harbour. [VCS]

---

## 3. Authored map candidates & recommended composition [DP, grounded in §2]

**DESIGN PROPOSAL** (consistent with the existing faction-city plan; final art is the Unreal lane's):
- Base geography: **Modular Water City Environment** (water + pier geography). [VOA]
- Overlay cultural kit: **Modular Viking Village** / **Viking Village Modular kit** longhouses + palisade. [VOA]
- Docks/piers foreground: **[Nanite] Medieval Harbor Kit**; shore cabins: **Nordic Fishing Hut**. [VOA]
- Foreground boats: **Gislinge Viking Boat / Viking Longboat**. [VOA]
- Produce a **Soul-owned copy** named `/Game/Soul/Maps/Settlements/L_VikingHarbour_Authored` (mirrors the
  existing `L_HumanCapital_Authored` / `L_DwarfHold_Authored` naming so the `/Game/Soul/` registry guard
  passes). This map is **[LRC]** to author locally; do **not** invent its contents here. [DP]
- Place one `ASoulTownViewAnchor` (SettlementId=`viking_harbour`) and one
  `ASoulSettlementPresentationController` (SettlementId=`viking_harbour`) — the two actors the generic
  visit flow requires. [DP, mirrors Human/Dwarf proofs]

Spatial reading from the overmap (§1.3): harbour/piers to the water (low elevation), Great Hall uphill;
bridge on the `shore_bridge` feature; two land approaches (east fjord ridge, south-east forest track).

---

## 4. Service anchors, spawn / return points [DP]

- **Service binding:** the development scenario must bind exactly one building to `service.tavern_hero`
  (hard requirement, `SoulFounderPlaytestStateSubsystem.cpp:380-382`). Reuse the design doc's
  **Jarl's Great Hall** as the hero-services building and bind `service.tavern_hero` there (mirrors
  Dwarf `dwarf.caravan_hall` and Human tavern). Use `viking.*` building ids (Lane 5 owns the full tree;
  Lane 6 needs only the single service building to make visits legal). [DP]
- **Return:** generic/unchanged (`OpenLevel(State->CampaignMap,...)`). [VCS]
- **Spawn (overview):** generic via the town-view anchor. [VCS]
- **Spawn (walk):** [DP] data-driven `EntryProbePoints` measured at the dock/harbour-front approach
  **[LRC]**; until measured, ship `bWalkable=false` (overview camera only).

---

## 5. Battle / siege candidate status (do not build V0)

- Overmap `battle_recipe_hint: viking.harbour_edge` exists but there is **no** owned, measured arena
  origin and **no** registered field/city battle environment for Viking Harbour. [VCS]
- The registry loader forbids a siege for any non-`human_capital` settlement
  (`SoulSettlementEnvironmentRegistry.cpp:40-46`). [VCS]
- **DESIGN PROPOSAL (defer):** a `viking.harbour_edge` field battle could be added later with a measured
  arena origin, but that is a battle-variety/Lane-7 concern. Keep Viking Harbour **visit-only** in V0,
  `battle_enabled:false`. The design doc's multi-approach siege (shore/docks → bridge/palisade → lower
  village → upper Great Hall) is explicitly future work. [DP]

---

## 6. Dependency risks (Viking-specific)

1. **No authored map yet** — the single largest dependency. All of §3 is net-new local authoring. [LRC]
2. **Asset import boundary** — Viking kits are owned but imported only into a *sibling* Copperlight
   project; they must be re-imported into Soul (licence permitting) and placed under `/Game/Soul/...`
   to pass the registry/visit `/Game/Soul/` and `/Game/` guards. [VOA + LRC]
3. **No development-proof data asset** — must author `DA_Soul_VikingHarbour_DevelopmentProof` with a
   `service.tavern_hero` building, validated by `USoulSettlementScenarioData::ValidateDefinition`. [VCS→DP]
4. **Scenario selection hardcode** (same as Dwarf, B5 in binder spec): four-faction playtest cannot pick
   a Viking development scenario until selection is data-driven. [VCS]
5. **Walk binder is Human-only** (B1–B4/B6). [VCS]
6. **Water/collision risk** — a harbour map has water volumes; the walk spawn uses a strict
   `ImpactNormal.Z > .75f` ground check that will reject water/pier gaps, so entry points must be
   measured on solid dock planking. [VCS reasoning + LRC]

---

## 7. Implementation order (Viking Harbour)

1. **[VOA→LRC]** Verify/import the owned Viking + Water City + Harbor Kit assets into Soul (licence
   check) under `/Game/Soul/...`. *(Asset/Unreal lane; not done here.)*
2. **[LRC/DP]** Author `/Game/Soul/Maps/Settlements/L_VikingHarbour_Authored` using the §3 composition;
   place `ASoulTownViewAnchor` + `ASoulSettlementPresentationController` with `SettlementId=viking_harbour`.
3. **[DP]** Author `DA_Soul_VikingHarbour_DevelopmentProof` (one building → `service.tavern_hero`) and a
   `VikingHarbourDevelopmentProof.json` authoring source mirroring the Dwarf proof's shape.
4. **[DP]** Add the `viking_harbour` **Environment Registry** entry (see `visit_registry_extensions.json`)
   with `battle_enabled:false`, `visit_environment` set, siege/arena null.
5. **[DP]** Implement the generic visit binder + per-settlement development-scenario selection (shared
   with Dwarf; see binder spec §3.4) and register `viking_harbour → DA_Soul_VikingHarbour_DevelopmentProof`.
6. **[LRC]** Measure a dock-front entry point; add a Viking visit profile (`bWalkable` only once
   measured; owned Viking Warrior avatar candidate). Until then overview-only.
7. Run acceptance tests.

---

## 8. Acceptance tests (Viking Harbour)

All **[LRC]** (need the authored map + local import):

- **VH-A1 Registry loads:** With a `viking_harbour` entry added, `FSoulSettlementEnvironmentRegistry::Load`
  succeeds (no "Enabled encounter requires an owned map" error, since `battle_enabled:false`).
  *Can be unit-tested headlessly* by pointing a test at a fixture registry — add to
  `Source/Soul/Private/Tests/SoulSettlementSaveTests.cpp` style harness.
- **VH-A2 Scenario validates:** `DA_Soul_VikingHarbour_DevelopmentProof` passes `ValidateDefinition` and
  exposes exactly one `service.tavern_hero` (headless unit test).
- **VH-A3 Overview visit:** In an isolated Viking settlement proof run, from owned `viking_harbour`,
  `VisitSettlement` loads `L_VikingHarbour_Authored`, logs `SOUL_SETTLEMENT_VISIT_READY
  settlement=viking_harbour`, overview camera active, management + return available.
- **VH-A4 Return / save-load parity:** same as DH-A2/A4.
- **VH-A5 (after walk profile):** walkable entry on physical dock planking; water/edges block movement.
- **VH-A6 Negative — missing anchor:** clean 20 s camera-missing failure, no crash.
- **VH-A7 No-asset guard:** if the authored map is absent locally, `CanVisit` rejects with "owned
  settlement environment map is missing or not bound" (existing guard, `SoulSettlementVisitGameMode.cpp:63-65`).

---

## 9. Explicit unknowns (Viking Harbour)

- **[VOA/LRC]** Licence terms for redistributing the owned Viking/Water-City/Harbor kits into a Soul-owned
  map copy (the Human/Dwarf proofs did exactly this with CastleTown/DwarvenCitadel, so precedent exists).
- **[LRC]** Which specific Viking-village content root actually lands in Soul (three catalog products map
  to one ambiguous `/Game/Viking_village` folder in a sibling project).
- **[LRC]** Whether the Viking Warrior / Viking (Customized) owned characters have usable idle+jog
  animations for a walk avatar, or whether the Human Paragon avatar is reused temporarily.
- **[LRC]** Water-volume collision layout — where solid dock planking provides a legal spawn/entry.
- **[DP/open]** Final `viking.*` building ids and the harbour's single `service.tavern_hero` building name
  (coordinate with Lane 5's `viking_building_tree.json`).
