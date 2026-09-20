# Soul Environment Asset Audit — 2026-09-20

This is a non-Unreal audit. It separates verified local payload facts from concepts that still require UE visual qualification.

## Status language
- LOCAL VERIFIED: files/maps/prefabs inspected on disk.
- ACQUIRED / PAYLOAD PENDING: Fab ownership/download metadata exists, but the UE payload is not expanded locally enough for map-level inspection.
- UE VISUAL PICK: multiple real candidates exist; only an editor view can determine the correct visual assignment.
- COMPOSITE: intended to be assembled from owned pieces; no new modeled asset is implied.

## Human capital — Hivemind Modular Castle
Status: LOCAL VERIFIED.
Primary donor root: Medieval_Megapack.
Primary authored maps: Levels/PL_Fortress_Day.umap and PL_Fortress_Day1.umap.
The pack exposes 22 map/level assets locally and roughly 3,000 uassets in the cached payload.

Most useful exact pieces:
- Levels/Prefabs/Building_A, Building_A_02, Building_B, Building_C, Building_D.
- Levels/Prefabs/Forge.umap.
- Levels/Prefabs/Tavern.umap.
- BP_MarketStand and SM_market_01.
- Courtyard tower, wall, gate, portcullis and wall-walk modular pieces.
- Scaffolding and a damaged retaining-wall variant.
Interpretation: this is the best current Human production base because town growth can be represented by actual level instances/prefabs rather than decorative icons.
Unknown until UE: which generic Building A/B/C/D reads best as barracks, range, chapterhouse and collegium.
No new building model is required for the Griffon Roost: use an existing tower/wall-walk and dress it as a perch.

## Viking city — Water City + Modular Viking Village
Status: BOTH LOCAL VERIFIED.
Water City authored maps: LV_WaterVillage, LV_Overview, LV_DayLighting, LV_NightLighting plus three BigCliff level prefabs.
Water City exposes many ready-made large/medium/small house blueprints, BP_Watchtower, BP_Bridge and modular wooden pier pieces.

Viking Village authored map: Levels/MainVillage/LV_MainVillage.umap.
Useful exact Viking pieces:
- BP_HouseBuilding_001, 002, 003.
- BP_WoodTower variants.
- BP_StorageMarket variants.
- BP_StrawArcheryTarget_01a/01b.
- BP_VikingBoat and BP_SmallBoat variants.
- blacksmith anvil/tool props, shields, bows/arrows, flags, hay, cages and fire camps.

Interpretation: use Water City for geography, cliffs, bridges, piers and settlement density; use Viking Village to overwrite cultural language.
The likely winning city silhouette is docks in the foreground, bridge/village in the middle, Great Hall on the high ground.
## Ravenhold
Status: LOCAL VERIFIED.
Primary authored scene: Scenes/HM-FortCastle_Kit_Demo.umap.
Asset gym: Scenes/HM-FortCastle_Kit_Asset_Gym.umap.
The local payload has very deep gatehouse, tower, curtain-wall, bridge, cliff and foliage coverage.

Critical Soul value: Ravenhold includes explicit clean and ruined variants such as:
- CurtainWall_10m and CurtainWall_10m_Ruined.
- Lrg_Wall_10m and Lrg_Wall_10m_Ruined.
- ruined bridge variants.
- multiple ruined medium/small wall sets.
- gatehouse and portcullis pieces.

Recommendation: do not spend Ravenhold as a routine faction town yet.
Use it as the persistent-siege damage donor, legendary neutral fortress kit, and source for campaign strongholds/bridges where its materials fit.
Its ruined variants are the cleanest current route to a breach remaining visibly breached after combat.

## Dwarf city — Modular Legendary Forge
Status: ACQUIRED / PAYLOAD PENDING.
Fab listing confirms 106 meshes, modular walls/floors/roofs, indoor and outdoor forge spaces, mountain vistas, lava/molten-metal treatment, smoke/fog/snow and damage decals.
Interpretation: very strong Dwarf fit. The Great Forge should be the town's visual center, not merely a blacksmith side building.
Use the Forge as a vertical settlement: approach -> outer works -> workshops -> Great Forge -> inner hold -> Mountain Dragon eyrie.
The pack's existing damage decals and smoke are especially valuable for persistent siege consequences.
Need UE to identify exact showcase map names, large halls, tunnel/gate candidates and upper exterior perch.

## Orc city — Medieval Ruins + support donors
Primary pack status: ACQUIRED / PAYLOAD PENDING.
Official pack language identifies an abandoned medieval/gothic/nature environment.
Support donors already local:
- Ravenhold ruined architecture.
- YI_BanditCamp tents / weapon-rack environment.
- Elite Coastal Ruins terrain.

Interpretation: do not rebuild the ruins into a clean Orc castle.
The faction's visual grammar should be occupation: old masonry + timber patches + tents + trophies + beast pens + fires.
Repairs should remain visibly crude/scarred.
This makes War Elephant infrastructure believable because the outer city can contain a genuinely large yard and reinforced elephant gate.

## Dark city — Fantasy Alien Castle
Status: ACQUIRED / PAYLOAD PENDING.
Official pack: 117 meshes plus a preassembled scene, designed to straddle dark fantasy and alien/sci-fi architecture.
Interpretation: strong Dark candidate only if a UE art-direction pass can remove technological reads.
Keep strange silhouettes and oppressive scale; remove machinery language, bright sci-fi emissives and anything that reads as technology.
Use restrained occult emissive, dark stone/metal, fog and corruption instead.
The highest terrace/spire should become the Fantasy Dragon Roost.
Ravenhold ruined geometry is a fallback damage donor only if materials can be made visually coherent.

## Optional Nature / Animal / Primitive / Centaur city
Primary pack: Fantasy Forest Village Kit, ACQUIRED / PAYLOAD PENDING.
Official pack: showcase level, asset showcase, 68 static meshes, architecture/modules/props/plants/rocks/nature assets, procedural foliage, falling leaves and editable prefab level actors.
The user-owned Treefort/Nature map still needs to be located and identified.

Interpretation: do not make this another walled castle.
Use clearings, platforms, bridges, a Great Tree, beast groves and ritual spaces.
Centaur recruitment should come from a range/glade rather than a house.
This environment is worth promoting to a sixth faction only if UE confirms strong vertical/tree architecture or the separate Treefort map supplies it.

## Human alternates
Fantasy Kingdom Capital Kit is acquired but payload-pending. Officially it has a showcase capital, 76 static meshes, procedural vegetation, waterfall/leaves FX and modular building pieces.
Use as a landmark/campaign capital donor, not as a reason to replace the more mechanically modular Hivemind fortress.
CastleTown/Townsmith remain useful secondary medieval donors for houses, halls and town dressing.
## Terrain library
LandscapePack413 is LOCAL VERIFIED and unusually useful:
- Mountain_01 through Mountain_05.
- SnowyMountain_01 through SnowyMountain_05.
- Grassland_01 and Grassland_02.
- Mesa_01 and Mesa_02.
- Desert_01 and Desert_02.
- lake/ocean/translucent water examples.
Official pack metadata states 8x8 km / 64 km2 source maps, 8K heightfields/splatmaps, rocks and editable materials.

Use this as macro-geography source material, not literal whole tactical levels.
Crop authored combat zones out of ridges, bowls, passes, terraces and plains, then bake Soul's hidden hex topology across the chosen play space.

Elite Coastal Ruins is LOCAL VERIFIED in ProgramData with four authored maps and 8K source terrain data.
Use for neutral/coastal/ruined battlefields and as an Orc/bridge donor.
Elite Desert II is acquired and can add arid scenarios; do not create a desert faction merely because the terrain exists.
Ancient Mountain/Shrine is acquired and is best treated as a landmark/shrine donor rather than redundant generic mountain terrain.

## Broad conclusion
There is no broad terrain-shopping problem left.
The remaining environment risk is composition/qualification, not asset scarcity.
The one plausible biome gap to keep an eye on is a convincing marsh/swamp if no owned map already covers it.

## Hivemind UE qualification update
The full PL_Fortress_Day map loads and passes MapCheck, but its first DDC build exposed several pathological diner-decoration meshes that are inappropriate for Soul's production capital.

Largest observed required-memory estimates during the real UE load included:
- SM_Cheese_Var1: ~5193 MB.
- SM_WoodCup_SM_WoodCup: ~4455 MB.
- SM_WineBottles_Var3: ~3403 MB.
- SM_SilverCandle: ~3256 MB.
- SM_SilverCup: ~2159 MB.
- SM_Cheese_Board: ~1534 MB.

Observed one-time build times included ~186 s for the wood cup, ~157 s for wine bottles and ~134 s for the cheese prop.

Conclusion: the castle architecture remains a strong Human-capital donor, but Soul should not inherit the showcase's full micro-prop population. Keep walls/gates/towers/prefab buildings/Forge/Tavern and selectively redress. Remove or replace pathological micro-props and let RB Optimization handle representation/HLOD for the retained architecture.

The alternate PL_Fortress_Day1 package is locally corrupt/unloadable (failed package name-table seek) and should not be used. PL_Fortress_Day is the qualified source map.
