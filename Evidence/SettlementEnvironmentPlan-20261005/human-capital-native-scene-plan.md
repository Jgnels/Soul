# Human Capital: native scene inspection and bounded integration

**Use Jeff's exact Medieval Kingdom environment, listing `42d4a792-2b66-423d-9b20-84d6b2c578d8`. Its local package is `CastleTown`; the current transfer does not contain the complete authored demo.** Recover the missing closure and render the intact scene before selecting the starting/buildable groups. No scene conversion, donor edit or replacement composition is justified by the present evidence.

## Identity and present evidence

[Identity receipt](human-capital-local-identity.json) links the owned Hivemind catalog record `fab_879a536feaa0407d88d5cf92f68b9add` to Macbeth's existing native King's Hall provenance. The correct demo is `/Game/CastleTown/Levels/Persistant/PL_CastleTown`, from `D:/RefinedBadger/Games/Macbeth/Content/CastleTown`. Preserve the donor's spelling `Persistant`.

Two similarly named environments are **different products**: LAYA DESIGN's `Kingdom_Capital` / Fantasy Kingdom Capital Kit (`09a1119d…`), and Hivemind's `Medieval_Megapack` / Modular Castle (`826f1b55…`). The older Soul `human_hivemind_*.png` captures were independently viewed; their capture manifest names `PL_Fortress_Day`, so they do not prove the selected CastleTown demo's appearance. Reuse older Soul state/presentation mechanics, not their donor choice or arbitrary actor placements.

Root's [704-package baseline](castletown-donor-maps-before.json) hashes the 43 local maps and 661 available external actor packages. [Dependency gaps](castletown-dependency-gaps.json) identifies a material completeness problem:

- `PL_CastleTown` references ten native AlwaysLoaded sublevels: `SL_BridgesAndWalls`, `SL_Castle`, `SL_Castle_Walls`, `SL_Courtyard`, `SL_Houses`, `SL_Landscape`, `SL_Lighting`, `SL_Mountain`, `SL_Roads`, `SL_Town_Props`.
- Fifteen inspected map packages contain external-actor metadata but have no corresponding local external package directory. This includes `LI_Full_Castle`, gate/corner/tower/front/side/top/main halls, two older building maps, log stores and `SL_Landscape`. Only `LI_Kings_Hall` has its 661 external actors. The `*_Fix` house maps have substantial embedded content; their presence cannot compensate for missing castle/landscape data.
- `SL_Landscape` explicitly contains World Partition metadata and a missing external actor reference. External actors on the other LevelInstance maps do not by themselves imply World Partition. Do not convert the whole demo to a new world format merely to support state.
- The earlier base transfer also omitted `T_EuropeanBeech_01_Snow_M` and `SM_BrickI`. The snow mask is directly referenced by both native Beech atlas material instances. No BrickI reference was found in the bounded Levels/externalactors/Materials/Scanned_Foliage scan; that does not establish it is unused elsewhere.

The prior HTTP transfer source at `10.0.0.10:8899` did not answer two bounded three-second HEAD probes. Recovery is being investigated separately; no replacement geometry should conceal this gap.

## Read-only editor inspection

Root mounts `/Game/CastleTown` and its exact external actor namespace. A junction is a reference to writable donor files, not filesystem write protection: no save, autosave or asset fixup may target those packages. Do not use the old prototype script as an inspector; it mutates assets and uses historical bulk save paths.

[inspect-loaded-settlement.py](inspect-loaded-settlement.py) is prepared and Python syntax checked. It is not yet executed by this worker. Root loads the correct map in the sole editor and runs:

```python
SOUL_SCENE_EXPECTED_MAP = '/Game/CastleTown/Levels/Persistant/PL_CastleTown'
exec(open(r'D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929/Evidence/SettlementEnvironmentPlan-20261005/inspect-loaded-settlement.py').read())
```

The probe does not load or save maps/assets, spawn actors, query triangles/LODs, expand foliage instances or run simulation. It records only loaded actor paths/GUIDs/labels/folders, transforms/bounds, attachment parents, LevelInstance references, mesh/material overrides, spline components, collision, instance counts and loaded sublevels. Wrong-map and overwrite checks prevent ambiguous receipts. Unavailable Python bindings are recorded, not worked around with mutations. A parent map with zero/sparse child actors is incomplete evidence. Compare the loaded-level list with the ten expected sublevels and confirm actual geometry before choosing any group.

First render gates: an intact environment overview; the entrance/approach; a courtyard showing the actual candidate building; its native frontage/contact; and matching source/proxy views. Preserve original light, terrain, roads, water and building relationships while judging this donor. Do not judge it from the first unresolved textures or an incomplete external-actor load.

## Smallest integration after that inspection

Keep one authored environment for visits and battle presentation. The existing Soul settlement ID and `Soul.Settlements` saved state remain the authority. The campaign contains only a corresponding lightweight mesh-derived base proxy plus a separate upgrade group; it does not stream the full environment, landscape, foliage, lights or interior clutter.

For editable scene binding, prefer one owned persistent map that retains the authored persistent actors/transforms and references unchanged donor visual assets/sublevels. Duplicate only state-bearing sublevel/LevelInstance packages that need saved wrapper bindings or removals, and remap those references to the owned copies. Native meshes/materials/textures continue to be referenced from licensed donor packages. This avoids copying the 8GB asset family or three full environments for campaign/visit/battle. Use UE's native world/package duplication so external actor identities are remapped; raw filesystem map copies are not sufficient.

Existing `EditorAssetSubsystem.duplicate_asset` and `save_loaded_asset` are the native path; Macbeth's `banquet-map-duplicate.json` verifies the earlier King's Hall duplicate, but does not certify duplication of the entire capital. Recheck owned destination, external package remapping, native sublevel paths and source hashes for this scene. `EditorLevelUtils.add_level_to_world` is available if an owned level must reference a native sublevel. The original persistent map contains meaningful authored actors too, so an empty wrapper with only the ten sublevels would lose part of the environment.

Use existing `ASoulSettlementBuildingActor` actor arrays and `ASoulSettlementPresentationController` before adding Data Layers. Its current direct-actor visibility/collision toggles require explicit loaded-child handling for nested LevelInstances and must preserve native collision defaults. Keep a wrapper with its affected actors in the same owned state-bearing level where possible; cross-level hard actor references may be invalid to save. Data Layers can organize an owned copy if inspection proves that helpful, but are not a new save/state authority or a reason to repartition the original demo.

## Starting group and one upgrade

Actor classification must be a reviewed membership list of stable original GUID/path/transform, not a name-based automatic deletion pass:

| Classification | Initial scope |
| --- | --- |
| Starting / always present | Authored terrain, approach, roads, bridge/wall/keep relationships, lighting and sufficient original settlement fabric to preserve the demo's identity. Exact members await complete scene inspection. |
| Buildable | One recognizable existing authored building and its associated props/attachments. Keep its native position, facade and geometry. |
| Optional dressing | Noninteractive props, ambience or far set dressing; initially preserve. Any later removal requires visible/context/performance evidence. |
| Remove / replace | Only verified demo-only gameplay helpers or incompatible utility logic after inspection. Do not remove architecture merely to simplify extraction. |

Integration's smallest suggested proof is existing `human.tavern`: start Unbuilt, complete the existing canonical construction transition to Intact, and verify its existing `HireTavernHero` service gate. This remains a candidate until a suitable authored building is actually identified. The same state must reveal the native building group in the visit/battle scene and its corresponding mesh-derived miniature group. Do not bake that group permanently into the always-present proxy. No new currency, construction queue or settlement ledger is needed.

Acceptance is one successful paid/authorized upgrade, persistence through save/reload, visible change in both representations, correct service availability, and unchanged route/gameplay authority. A read-only intact donor render alone is not that acceptance; neither is a valid canonical-state test without the corresponding visual change.

This preparation changes evidence files only. Root owns editor loads, source work, asset duplication, renders and all builds.
