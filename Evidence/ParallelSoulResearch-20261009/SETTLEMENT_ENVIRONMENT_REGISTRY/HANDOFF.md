# HANDOFF — SETTLEMENT_ENVIRONMENT_REGISTRY

**Lane:** SETTLEMENT_ENVIRONMENT_REGISTRY (parallel research, read-only)
**For:** RefinedBadger Soul / Codex integration lane
**Authorities:** `Jgnels/Soul` (gameplay/source) + `Jgnels/Copperlight-Asset-Catalog` (owned-asset intelligence)
**Deliverables (this directory):** `settlement_environment_registry.json`, `settlement_environment_registry.csv`, `HUMAN_CAPITAL_DIAGNOSIS.md`, `HANDOFF.md`

**What this lane did NOT do:** no edits to Soul `Source/ Config/ Content/ .uproject`, no map/asset/save modification, no builds/cooks, no git commit/push/merge/reset/clean/stash. All exact on-disk `.uasset`/`.umap` confirmations are flagged for the Remote-Desktop lane in section 4.

**Global caveats (apply to every row):**
- **Licensing:** all six primary faction donors show `LicenseStatus = UNKNOWN` in `Copperlight-Asset-Catalog/catalog/products.json`. Every emitted `/Game/...` path is license-UNKNOWN pending Remote-Desktop license review.
- **Ownership/local split:** `Medieval Ruins` + `Fantasy Forest Village Kit` are `IMPORTED` / `LocallyAvailable=true` (match Soul LOCAL evidence). `Modular Castle`, `Modular Legendary Forge`, `Modular Water City`, `Fantasy Alien Castle` are `OWNERSHIP_CONFIRMED` but `LocallyAvailable=null` (match Soul `ACQUIRED_PAYLOAD_PENDING_UE_CACHE` / `PAYLOAD_PENDING`). This split is the strongest consistency signal between the two authorities and drives the confidence tiers below.
- **No broad terrain-shopping gap.** `Soul/Docs/ENVIRONMENT_ASSET_AUDIT_20260920.md` concludes the remaining risk is composition/qualification, not asset scarcity. The one plausible biome gap is **marsh/swamp**: the only wetland-adjacent owned record is `Coastal Wetland & Railroad Bridge` (Switchboard Studios, `LocallyAvailable=null`) — a coastal/rail asset, not a fantasy marsh. Flag for later, do **not** buy preemptively.

---

## 1. TOP 10 highest-impact environment integrations (ranked for Codex)

Ranking logic: local-availability + authored-map evidence + gameplay leverage (how many settlements/sieges/battles it unblocks) first; payload-pending but owned next; art-gated last.

| # | Integration | Primary `/Game/...` or pack | Evidence tier | Why it is highest-impact | Key caveat |
|---|---|---|---|---|---|
| 1 | **Human Capital qualified slice** | `/Game/Medieval_Megapack/Levels/PL_Fortress_Day` | LOCAL_VERIFIED (load_ok=true) | Only settlement with hard UE load evidence; full modular prefab/level kit = real growth, not icons. Unblocks capital visit + siege + fortress-outskirts battle. | Strip 6 pathological diner props (~20 GB) + RB Optimization/HLOD. See `HUMAN_CAPITAL_DIAGNOSIS.md`. Do not use corrupt `PL_Fortress_Day1`. License UNKNOWN. |
| 2 | **Ravenhold siege-damage kit** | Ravenhold `CurtainWall_10m`/`_Ruined`, `Lrg_Wall_10m`/`_Ruined`, ruined bridges; `Scenes/HM-FortCastle_Kit_Demo.umap` | LOCAL_VERIFIED (prior audit) | Single donor that unlocks persistent breach grammar for **all** factions (clean->ruined) + neutral stronghold/bridge. Highest cross-faction leverage. | Copperlight `LocallyAvailable=null` -> confirm on-disk payload. Material-fit test before borrowing into Dark/Dwarf. License UNKNOWN. |
| 3 | **Viking Harbour** | `/Game/JustBStudios/Water_City/Levels/LV_WaterVillage` (+ `/Game/Viking_Village/Levels/MainVillage/LV_MainVillage`) | LOCAL_VERIFIED | Two authored maps + ready-made house/pier/bridge blueprints; strongest non-human environment. Unblocks visit+siege+harbour battle. | Two-pack composite (geography + culture). Water City Copperlight `LocallyAvailable=null` vs Soul LOCAL_VERIFIED — reconcile. License UNKNOWN. |
| 4 | **Orc Ruinhold showcase recovery** | `Medieval Ruins` showcase map (`SHOWCASE_TO_QUALIFY`) | PAYLOAD_PENDING but **IMPORTED/local** in Copperlight | Fastest payload-pending win: Copperlight marks it `Imported=true, LocallyAvailable=true`, so the exact showcase `/Game/...` map is recoverable immediately on Remote Desktop. | Soul flag says `ACQUIRED_PAYLOAD_PENDING` but Copperlight says IMPORTED — open the imported content and record the real map name. License UNKNOWN. |
| 5 | **Owned terrain battlefield family** | `LandscapePackOne/Two` Mountain/SnowyMountain/Grassland/Mesa/Desert `.umap`; `Elite_CoastalRuins/Maps/CoastalRuins_01..04.umap` | READY_FOR_UE / LOCAL_VERIFIED | 15 of 27 battlefield recipes are already `READY_FOR_UE` on owned local terrain — field battles for every faction are mostly unblocked today. | Crop bounded combat zones; do not ship whole 8x8km maps. License UNKNOWN. |
| 6 | **Dwarf Hold forge-city** | `Modular Legendary Forge` showcase (`PACK_SHOWCASE_TO_QUALIFY`) | PAYLOAD_PENDING (OWNERSHIP_CONFIRMED, local null) | Best-fit vertical forge-city with rich modular kit + damage decals/smoke/lava. Terrain family already READY_FOR_UE. | Needs import before qualification (not local). License UNKNOWN. |
| 7 | **Secondary human town** | `Townsmith: Modular Medieval Town` (`fab_0eba8defe69f4d4589c3f12022c34fcf`) | OWNERSHIP_CONFIRMED, **local=true** | Fills secondary-town/small-settlement need with an owned, locally-available kit — proves no purchase needed for non-fortress human settlements. | Interiors/walls need UE compose. License UNKNOWN. |
| 8 | **Dark Fortress (art-gated)** | `Fantasy Alien Castle` preassembled scene (`PREASSEMBLED_SCENE_TO_QUALIFY`) | PAYLOAD_PENDING (OWNERSHIP_CONFIRMED, local null) | Strong oppressive silhouette; terrain family READY_FOR_UE keeps Dark battles unblocked now. | Double-gated: needs import AND mandatory art filter to strip technological/sci-fi reads (`city_siege_blueprints.json` art_filter). License UNKNOWN. |
| 9 | **Ancient Mountain / Shrine donor** | `Mountain (Ancient Mountain, Shrine...)` (`fab_155f959c44724060b362df0ad8e295c2`) | OWNERSHIP_CONFIRMED, **local=true** | Resolves 2 of the shrine `PAYLOAD_PENDING` battlefield WARNs (`nature.ancient_shrine`, `neutral.mountain_shrine`) and the Nature `ancient_shrine` civic — with a locally-available owned pack. | Treat as landmark/shrine donor, not generic terrain. License UNKNOWN. |
| 10 | **Nature Treehold (candidate, deferred)** | `Fantasy Forest Village Kit` (`SHOWCASE_TO_QUALIFY`, IMPORTED/local) + unlocated Treefort map | PAYLOAD_PENDING | Lowest priority: faction `enabled:false`; showcase map is recoverable (imported) but the verticality-defining Treefort map is **not in either repo**. | Do NOT lock faction six until Treefort/Forest Village passes the UE verticality test. License UNKNOWN. |

---

## 2. Existing-Soul settlement environment reference map

Every `/Game/...`-grade environment reference currently asserted in the Soul repo, with file+key provenance. Full detail per settlement is in `settlement_environment_registry.json`.

| Settlement | Soul reference | Source file -> key | Tier |
|---|---|---|---|
| `city.human_capital` | `/Game/Medieval_Megapack/Levels/PL_Fortress_Day` | `environment_asset_bindings.json` humans.base_map; `city_siege_blueprints.json` city.human_capital.town_view.base | LOCAL_VERIFIED |
| Human prefabs | `/Game/Medieval_Megapack/Levels/Prefabs/{Building_A,A_02,B,C,D,Forge,Tavern}` | `environment_asset_bindings.json` humans.bindings; `hivemind_prefab_string_inventory.json` | LOCAL_EVIDENCE_DEFAULT / UE_VISUAL_CONFIRM |
| Human market | `/Game/Medieval_Megapack/Meshes/Props/BP_MarketStand`, `SM_market_01` | `environment_asset_bindings.json` human.market | UE_VISUAL_CONFIRM |
| `city.viking_harbour` | `/Game/JustBStudios/Water_City/Levels/LV_WaterVillage`; `/Game/Viking_Village/Levels/MainVillage/LV_MainVillage` | `environment_asset_bindings.json` vikings.base_maps | LOCAL_VERIFIED |
| Viking houses | `/Game/Viking_Village/.../BP_HouseBuilding_001/002/003`; `/Game/JustBStudios/Water_City/.../BP_Big_house_01/03/05`, `BP_Small_House_V04` | `environment_asset_bindings.json` vikings.bindings | LOCAL_EVIDENCE_DEFAULT / UE_VISUAL_CONFIRM |
| `ravenhold` (donor) | `CurtainWall_10m`/`_Ruined`, `Lrg_Wall_10m`/`_Ruined`, ruined bridges; `Scenes/HM-FortCastle_Kit_Demo.umap`, `..._Asset_Gym.umap` | `environment_asset_bindings.json` ravenhold; `ENVIRONMENT_ASSET_AUDIT_20260920.md` | LOCAL_EVIDENCE_DEFAULT |
| `city.dwarf_hold` | `PACK_SHOWCASE_TO_QUALIFY` (+ LandscapePackOne Mountain/SnowyMountain) | `settlement_blueprints.json` dwarves.donor | PAYLOAD_PENDING |
| `city.orc_ruinhold` | `SHOWCASE_TO_QUALIFY` (Medieval Ruins) | `settlement_blueprints.json` orcs.donor | PAYLOAD_PENDING |
| `city.dark_fortress` | `PREASSEMBLED_SCENE_TO_QUALIFY` (Fantasy Alien Castle) | `settlement_blueprints.json` dark.donor | PAYLOAD_PENDING |
| `city.nature_treehold` | `SHOWCASE_TO_QUALIFY` (Fantasy Forest Village) + Treefort map (unlocated) | `settlement_blueprints.json` nature_candidate.donor | PAYLOAD_PENDING |
| Battlefields (27) | `battlefield_recipes.json` donor map paths (Grassland_01/02, Mountain_01/04/05, SnowyMountain_01/02/03, Mesa_01, Desert_01, CoastalRuins_01..04, PL_Fortress_Day exterior, LV_WaterVillage, LV_MainVillage) | `battlefield_recipes.json` recipes[].donor | mixed (15 READY_FOR_UE, rest crop/composite/pending) |

---

## 3. Dragon Graveyard / placeholder fallback list

**Finding (explicit):** there is **no literal "Dragon Graveyard" asset reference** anywhere in `Jgnels/Soul` or `Jgnels/Copperlight-Asset-Catalog`. Case-insensitive grep of `Data/ Config/ Source/ Docs/ Evidence/` and the Copperlight catalog returned:
- `"dragon"` appears only as **faction concept names** (Mountain Dragon Eyrie, Apex Dragon Roost, dragon_roost objective) in `settlement_blueprints.json`, `city_siege_blueprints.json`, `FACTION_CITY_AND_SIEGE_PLAN_20260920.md`, `SETTLEMENT_RECRUITMENT_MATRIX_20260920.md`. None is an environment/map path.
- `"graveyard"` appears exactly **once**, unrelated: `Copperlight-Asset-Catalog/catalog/acquisition_candidates/humble_horrors_haunts_20261008.json:823` -> `"Religion & Memorial VOL.2 - Graveyard"` (an acquisition *candidate*, not an owned/used Soul environment).

I did **not** fabricate a Dragon Graveyard path. If one exists it is a known-local-only reference the Remote-Desktop lane must confirm (see section 4).

**The references that functionally serve as fallbacks / placeholders today:**

| Fallback kind | Exact value | File:line / key | Meaning |
|---|---|---|---|
| Engine default map | `/Engine/Maps/Entry` | `Soul/Config/DefaultEngine.ini:2` `GameDefaultMap=/Engine/Maps/Entry` | The literal default map Soul boots to; the only true "default_map" in the repo. Engine stub, not a settlement. |
| Sentinel placeholder (dwarves) | `PACK_SHOWCASE_TO_QUALIFY` | `settlement_blueprints.json` dwarves.donor.primary_map | Stand-in for the not-yet-qualified Legendary Forge showcase map. |
| Sentinel placeholder (orcs, nature) | `SHOWCASE_TO_QUALIFY` | `settlement_blueprints.json` orcs.donor.primary_map, nature_candidate.donor.primary_map | Stand-in for Medieval Ruins / Forest Village showcase maps. |
| Sentinel placeholder (dark) | `PREASSEMBLED_SCENE_TO_QUALIFY` | `settlement_blueprints.json` dark.donor.primary_map | Stand-in for Fantasy Alien Castle preassembled scene. |
| Corrupt map to AVOID | `Levels/PL_Fortress_Day1.umap` | `settlement_blueprints.json` humans.donor.alternate_map; `ENVIRONMENT_ASSET_AUDIT_20260920.md` | Locally corrupt/unloadable; must not be used as a human fallback. |
| Damage-donor fallback | Ravenhold ruined geometry | `ENVIRONMENT_ASSET_AUDIT_20260920.md:89`; `settlement_blueprints.json` dark/orc support donors | "fallback damage donor only if materials can be made visually coherent." |
| Unlocated donor | user Treefort/Nature map | `settlement_blueprints.json` / `city_siege_blueprints.json` nature | Referenced but not present in either repo. |

**Recommendation to Codex:** the four `*_TO_QUALIFY` sentinels are the real "generic placeholders that should eventually be replaced" with the exact qualified showcase `/Game/...` map names (recoverable per section 4). Treat `/Engine/Maps/Entry` as boot default only.

---

## 4. REMAINING EXACT-LOCAL-PATH CHECKS FOR CODEX / REMOTE DESKTOP LANE

GitHub proves ownership, confidence tiers, policy and every reference above. It cannot expose the exact on-disk `.uasset`/`.umap` paths for payload-pending content or confirm NavMesh. The following must be confirmed on the Remote Desktop / in-editor lane. Each item is keyed to its evidence.

### 4a. Payload-pending primary maps (resolve the `*_TO_QUALIFY` sentinels to real `/Game/...` names)
Keyed off `Soul/Evidence/environment_plan_validation_20260920.txt` WARN lines + `settlement_blueprints.json`:
- **Dwarves:** exact `Modular Legendary Forge` showcase map name (replaces `PACK_SHOWCASE_TO_QUALIFY`). WARN: "dwarves: primary environment payload still needs UE/cache qualification". *(OWNERSHIP_CONFIRMED, local=null -> import first.)*
- **Orcs:** exact `Medieval Ruins` showcase map name (replaces `SHOWCASE_TO_QUALIFY`). WARN: "orcs: primary environment payload still needs UE/cache qualification". *(Copperlight IMPORTED/local=true -> should be immediately recoverable; reconcile the Soul PAYLOAD flag.)*
- **Dark:** exact `Fantasy Alien Castle` preassembled scene name (replaces `PREASSEMBLED_SCENE_TO_QUALIFY`). WARN: "dark: primary environment payload still needs UE/cache qualification". *(OWNERSHIP_CONFIRMED, local=null -> import first; then art filter.)*
- **Nature:** exact `Fantasy Forest Village` showcase map name + locate the **user Treefort/Nature map** (not in either repo). WARNs: `nature.forest_clearing`, `nature.river_woodland`, `nature.ancient_shrine`.

### 4b. Payload-pending battlefield donors (from `environment_plan_validation_20260920.txt` WARN lines)
- `dwarf.forge_approach` — Legendary Forge showcase exterior.
- `orc.ruined_field` — Medieval Ruins showcase crop.
- `dark.castle_approach` — Fantasy Alien Castle exterior.
- `nature.forest_clearing`, `nature.river_woodland`, `nature.ancient_shrine` — Forest Village / Ancient Mountain crops.
- `neutral.mountain_shrine` — Ancient Mountain/Shrine.

### 4c. Gate 0 in-editor qualification screenshots (from `Soul/Docs/UE_ENVIRONMENT_EXECUTION_QUEUE_20260920.md`)
Open each donor in isolation and capture overview/town/siege shots; record camera anchors, Soul-unit scale, perf baseline, walkable/tactical space, modular/prefab deps, lighting deps, stylistic conflicts:
1. Hivemind `PL_Fortress_Day` **and** `PL_Fortress_Day1` (confirm the latter's corruption).
2. Water City `LV_WaterVillage` and `LV_Overview`.
3. Viking Village `LV_MainVillage`.
4. Ravenhold `HM-FortCastle_Kit_Demo` and Asset Gym.
5. LandscapePack Mountain/SnowyMountain/Grassland/Mesa/Desert maps.
6. Coastal Ruins 01-04.
7. When available: Legendary Forge, Medieval Ruins, Fantasy Alien Castle, Forest Village, Ancient Mountain, Kingdom Capital.

### 4d. Exact building/house assignment (Gate 1)
- Humans: visually classify `Building_A/A_02/B/C/D` -> lock Barracks/Range/Chapterhouse/Collegium (`settlement_blueprints.json` confidences are `UE_VISUAL_PICK`).
- Vikings: pick exact large/medium/small shells for Great Hall / Raider / Shield / Huscarl / Shaman (`BP_Big_house_01/03/05`, `BP_Small_House_V04`, `BP_HouseBuilding_001/002/003`).
- Dwarves/Orcs/Dark/Nature: identify exact halls/courts/spires per Gate 1.

### 4e. Ownership/local reconciliation (Copperlight vs Soul)
- `Modular Water City` & `Ravenhold`: Copperlight `LocallyAvailable=null` but Soul audit marks Water City `LOCAL_VERIFIED` and Ravenhold `LOCAL VERIFIED` — confirm actual on-disk payload.
- `Medieval Ruins` & `Fantasy Forest Village`: Copperlight `IMPORTED/local=true` — confirm the imported `/Game/...` roots and showcase maps.

### 4f. Nav/pathing + exact mesh paths not in repo
- No baked NavMesh dump exists in either repo for any settlement — all `nav_pathing_evidence` fields require in-editor confirmation.
- `SM_SilverCandle`, `SM_SilverCup` full `/Game/...` paths were "recorded by UE audit when available" (`environment_qualification_rules.json`) — capture exact paths in-editor.

### 4g. Licensing
- All six primary donors are `LicenseStatus=UNKNOWN` — resolve license terms before shipping any `/Game/...` path to production.

### 4h. Possible biome gap
- Confirm whether any owned pack covers a convincing **marsh/swamp** biome. Current catalog evidence: none beyond the coastal `Coastal Wetland & Railroad Bridge` (local=null). Do not purchase until the gap is proven real.

---

## 5. Reproduction
`settlement_environment_registry.json` and `.csv` were generated by `_build_registry.py` (in this directory) from the cited Soul and Copperlight files. Re-run with `python3 _build_registry.py`.
