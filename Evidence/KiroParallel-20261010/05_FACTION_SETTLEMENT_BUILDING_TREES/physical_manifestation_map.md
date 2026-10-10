# Physical Manifestation Map — Faction Settlement Building Trees

Lane 05 — DWARF / ORC / VIKING SETTLEMENT DEVELOPMENT TREES
Source snapshot: `handoff/soul-kiro-20261010` @ `2c3a9055d71a87ed3ed8b8abf08503efa1218f7c`

This document maps each building ID in the three trees to how it physically manifests in an authored settlement, using the **existing** presentation system. It never invents `/Game/...` paths and labels every ownership claim.

---

## 1. How manifestation works today (VERIFIED FROM CURRENT SNAPSHOT)

The physical layer is **presentation-only**; SoulCore owns truth. The relevant code:

- `ASoulSettlementBuildingActor` — `Source/Soul/Private/SoulSettlementBuildingActor.cpp`, header `Source/Soul/Public/SoulSettlementBuildingActor.h`.
  - Bound to a building by `SettlementId` + `BuildingId` + `MinimumBuildingLevel`.
  - Holds four authored state branches: `ConstructionRoot / IntactRoot / DamagedRoot / RuinedRoot` plus parallel `ConstructionActors / IntactActors / DamagedActors / RuinedActors` arrays (externally-placed level-instance geometry).
  - When `bFollowSettlementState` is true it reads `USoulSettlementStateSubsystem::GetBuildingConditionName` and calls `ApplyConditionName` → `ShowOnly` to reveal only the matching branch (recurses into `ILevelInstanceInterface`).
  - `ConfigureMiniature` builds the strategic-map miniature from `MiniatureBaseMesh` + `MiniatureUpgradeMesh` (no collision, no nav).
- Binding data lives on `USoulSettlementScenarioData` (`OwnedEnvironmentMap`, `MiniatureBaseMesh`, `MiniatureUpgradeMesh`, `MiniatureTransform`) and in the per-settlement proof JSON (`environment_actor_group`, `campaign_miniature_group`, `miniature_*`, `owned_environment_map`), e.g. `Data/SettlementEnvironments/CrownsteadDevelopmentProof.json`.
- Mesh/structure bindings catalogue: `Data/environment_asset_bindings.json`.
- Registry of which settlements have authored environments: `Data/SettlementEnvironments/EnvironmentRegistry.json`.

**Design rule inherited from the Human proof:** a building ID becomes "physical" only when a `ASoulSettlementBuildingActor` is placed in the authored map and its four state-branch actor groups are populated with owned geometry, then the DataAsset is bound. Gameplay effects can precede art (`HeartlandDevelopment.json` art_status: "gameplay-only until approved authored groups are mapped"). The three trees here follow the same rule: **effects are designed now; physical groups require local Unreal placement + visual qualification.**

---

## 2. Manifestation readiness per faction

| Faction | Authored environment | Registry entry | DevelopmentProof | Readiness |
|---|---|---|---|---|
| Dwarves | `/Game/Soul/Maps/Settlements/L_DwarfHold_Authored` (VERIFIED FROM CURRENT SNAPSHOT — registry + proof) | YES (`dwarf_hold`, `battle_enabled=false`) | YES (`DwarfHoldDevelopmentProof.json`) | **Highest** — only faction with an existing visitable map + miniatures. Source env `/Game/DwarvenCitadel` is a CONTRADICTION (see §4). |
| Vikings | Candidate maps named in bindings (`JustBStudios/Water_City/.../LV_WaterVillage`, `Viking_Village/.../LV_MainVillage`) — VERIFIED OWNED ASSET METADATA for the packs, but no Soul-owned authored copy yet | NO | NO | **Medium** — strong owned donors + explicit per-structure bindings; Lane 6 owns the walkable binding. |
| Orcs | Donor `Medieval Ruins` (VERIFIED OWNED, imported) + Ravenhold ruins | NO | NO | **Lowest** — fully greenfield; no map, no registry, no proof. |

---

## 3. Per-building manifestation bindings

Legend for confidence: `LOCAL` = local evidence default, `OWNED` = verified owned Fab metadata, `COMPOSITE` = assembled from parts, `CHECK` = local runtime/asset check required.

### Dwarves (donor: Modular Legendary Forge — OWNED; Dwarf_Pack troops — OWNED/imported in Soul runtime)
| Building ID | State-branch donor concept | Miniature | Confidence |
|---|---|---|---|
| dwarf.kings_hall | inner hold interior (Caravan Hall cutaway, per proof `representation_scope`) | SM_DwarfHold_Base_r9 (proof) | VERIFIED FROM CURRENT SNAPSHOT |
| dwarf.gates | authored gate (proof confirms present) | shared base | VERIFIED FROM CURRENT SNAPSHOT |
| dwarf.great_forge | Legendary Forge centerpiece | SM_DwarfHold_Upgrade_r2 | OWNED (forge) + CHECK (import) |
| dwarf.mine | mountain/forge tunnel composition | COMPOSITE | CHECK |
| dwarf.caravan_hall | outer works (proof dev definition) | SM_DwarfHold_Upgrade_r2 | VERIFIED FROM CURRENT SNAPSHOT |
| dwarf.stoneguard_hall | Forge modular hall | COMPOSITE | OWNED (forge) + CHECK |
| dwarf.crossbow_workshop | workshop + crossbow dressing | COMPOSITE | CHECK |
| dwarf.hammer_hall | forge-side barracks hall | COMPOSITE | OWNED (forge) + CHECK |
| dwarf.rune_forge | interior forge + lava/smoke FX | COMPOSITE | CHECK |
| dwarf.kings_guard_hall | inner-hold modular hall | COMPOSITE | CHECK |
| dwarf.dragon_eyrie | upper mountain perch | COMPOSITE | CHECK (apex pipeline) |

### Vikings (donors: Water City — OWNED; Viking Village — OWNED/imported; Viking boats — OWNED)
Per-structure donors are already enumerated in `Data/environment_asset_bindings.json` (`vikings`). Reproduced here as the manifestation plan (all require the Soul-owned authored copy Lane 6 will build):
| Building ID | Donor blueprint (from environment_asset_bindings.json) | Confidence |
|---|---|---|
| viking.great_hall | `BP_Big_house_05` (Water City) | OWNED + CHECK |
| viking.watch | `BP_WoodTower` / `BP_Watchtower` | OWNED + CHECK |
| viking.market | `BP_HouseBuilding_001` / `BP_StorageMarket` | OWNED/imported + CHECK |
| viking.shipyard | WaterCity piers + Gislinge/Longboat boats | OWNED + COMPOSITE |
| viking.smithy | VikingVillage anvil/tool dressing | OWNED/imported + CHECK |
| viking.raider_longhouse | `BP_HouseBuilding_003` | OWNED/imported + CHECK |
| viking.hunter_range | `BP_StrawArcheryTarget_01a/01b` + lodge | OWNED/imported |
| viking.shield_hall | `BP_Big_house_03` + shield dressing | OWNED + CHECK |
| viking.shaman_lodge | `BP_Small_House_V04` relocated | OWNED + CHECK |
| viking.berserker_mead_hall | `BP_Big_house_01` + fire/barrels | OWNED + CHECK |
| viking.huscarl_hall | largest house | OWNED + CHECK |
| viking.wolf_kennels | fenced yard (COMPOSITE) | COMPOSITE + CHECK |

### Orcs (donor: Medieval Ruins — OWNED/imported; Ravenhold ruined walls — LOCAL)
| Building ID | State-branch donor concept | Confidence |
|---|---|---|
| orc.warchief_hall | largest surviving ruin/citadel | OWNED/imported + CHECK |
| orc.patched_walls | Ravenhold `CurtainWall_10m` + `_Ruined` + timber | LOCAL + CHECK |
| orc.spoils_market | tent/market composition (YI_BanditCamp) | CHECK (donor ownership) |
| orc.salvage_forge | ruin + forge/anvil props | COMPOSITE + CHECK |
| orc.grunt_barracks | occupied ruin + tents | OWNED/imported + CHECK |
| orc.hunter_range | collapsed courtyard + weapon racks | COMPOSITE + CHECK |
| orc.shield_pit | open ruined courtyard | COMPOSITE + CHECK |
| orc.berserker_pit | sunken/ruined court | CHECK |
| orc.war_drum_tower | ruined tower + banners | COMPOSITE + CHECK |
| orc.shaman_court | reappropriated ritual ruin + totems | COMPOSITE + CHECK |
| orc.brute_hall | reinforced broken tower/hall | CHECK |
| orc.elephant_yard | largest outer courtyard pen | COMPOSITE + CHECK |

**Orc scar grammar (DESIGN PROPOSAL):** because the Orc theme is "repairs remain visibly scarred," Orc buildings should prefer the `DamagedRoot` state branch as their *baseline intact* appearance and reserve `RuinedRoot` for destruction — i.e. an Orc "intact" building is a Human "damaged-looking" building. This reuses `ASoulSettlementBuildingActor`'s existing four-branch system with no new code; it is purely an authoring choice in which actor group is populated.

---

## 4. CONTRADICTIONS against current assumptions

- **CONTRADICTION-D1 (`/Game/DwarvenCitadel`):** `DwarfHoldDevelopmentProof.json` and `EnvironmentRegistry.json` imply a working Dwarven hold environment sourced from `/Game/DwarvenCitadel/Maps/DwarvenCitadel`. A full-text search of the **Copperlight Asset Catalog returns ZERO references to "DwarvenCitadel" / "Dwarven Citadel"** — it is not a cataloged owned Fab product. The only forge-city OWNED donor in the catalog is **Modular Legendary Forge** (`fab_bf013eb27085437f9c071cf318019c26`), which `settlement_blueprints.json` already names as the Dwarf donor. The proof's `representation_scope` describes a "Native Caravan Hall cutaway plus authored gate" (i.e. a provisional, partial anchor), not a full citadel. **Resolution required locally:** either `/Game/DwarvenCitadel` is an un-cataloged local asset (then update Copperlight), or the authored Dwarf hold must be rebuilt on Modular Legendary Forge. Treat `/Game/DwarvenCitadel` as **LOCAL RUNTIME-ASSET CHECK REQUIRED**, not VERIFIED OWNED.

- **CONTRADICTION-D2 (caravan hall build days):** proof authors `dwarf.caravan_hall` `build_days: 2`, but the lane's founder-timing rule classifies a civic service node as **normal = 1 day**. The trees here set `days: 1` and flag the mismatch; a founder decision is needed on whether to re-tag the proof or treat the Dwarf service as "significant."

- **CONTRADICTION-GEN (apex beasts):** every faction's apex/beast dwelling (dwarf.dragon_eyrie, orc.elephant_yard, viking.wolf_kennels) is marked COMPOSITE in `settlement_blueprints.json` and has **no confirmed apex mesh/animation pipeline** in the current snapshot. The battle roster wires only the signature *warrior* per faction (dwarf_warrior / orc_hammer_warrior / viking_axe_warrior). Apex nodes must stay DESIGN PROPOSAL and gated behind an explicit roster/animation lane; do NOT promise them as buildable in a first pass.

- **CONTRADICTION-MINI (miniature meshes):** Dwarf proof references `SM_DwarfHold_Base_r9` / `SM_DwarfHold_Upgrade_r2` under `/Game/Soul/CampaignProxies/Dwarven/`. These are Soul-authored proxies, not Fab products; their actual presence is a LOCAL CHECK.

---

## 5. Minimum physical path to a first playable faction tree (implementation order)

1. **Dwarves first** (lowest risk): it already has a registry entry, a DevelopmentProof, miniatures, and an authored map. Resolve CONTRADICTION-D1 locally, then extend the proof's `development_definitions` with the civic/economy/first-military nodes (gameplay-only, art later), exactly as `HeartlandDevelopment.json` added market/barracks "gameplay-only until approved authored groups are mapped."
2. **Vikings second**: owned donors + explicit bindings exist; blocked only on Lane 6's authored visit map. Author the development JSON now so Lane 6 can bind actors to real building IDs.
3. **Orcs last**: greenfield; needs a map, registry entry, and proof before any physical manifestation. Gameplay tree can still ship first.
