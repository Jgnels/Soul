# Soul UE Environment Execution Queue — 2026-09-20

The non-UE design pass is complete enough that further high-value environment work is now editor-dependent.

## Gate 0 — donor qualification, no production edits
Open each donor/demo in isolation and capture standardized overview/town/siege screenshots:
1. Hivemind PL_Fortress_Day and PL_Fortress_Day1.
2. Water City LV_WaterVillage and LV_Overview.
3. Viking Village LV_MainVillage.
4. Ravenhold HM-FortCastle_Kit_Demo and Asset Gym.
5. LandscapePack Mountain, SnowyMountain, Grassland, Mesa and Desert maps.
6. Coastal Ruins 01–04.
7. When available: Legendary Forge, Medieval Ruins, Fantasy Alien Castle, Forest Village, Ancient Mountain and Kingdom Capital.

For every map record:
- useful camera anchors;
- scale relative to Soul units;
- performance baseline;
- walkable/tactical space;
- modular/prefab dependencies;
- lighting dependencies;
- obvious stylistic conflicts.

Do not migrate whole packs to Soul during this gate.
## Gate 1 — exact building assignment
Humans: visually classify Building A/A_02/B/C/D and lock Barracks/Range/Chapterhouse/Collegium assignments.
Vikings: choose the exact large/medium/small house shells for Great Hall, Raider Hall, Shield Hall, Huscarl Hall and Shaman Lodge.
Dwarves: identify exact Forge showcase map, workshop halls, gate/tunnel and upper eyrie space.
Orcs: identify largest surviving ruin, chapel/ritual area, open pit/courtyard and elephant-yard candidate.
Dark: identify barracks/gallery/court/spire/roost candidates and mark every prop/material that reads technological.
Nature candidate: locate the separate Treefort/Nature asset and decide whether the environment has enough verticality to justify a sixth faction.

Output: one contact sheet and one exact asset-path table per faction.

## Gate 2 — production city skeletons
Create Soul city maps using selected migrated content only.
Each city gets:
- one fixed town-view camera;
- named construction pads/zones;
- seven recruitment structures;
- civic structures;
- wall/defense state;
- siege objectives;
- damage-state hooks.
## Gate 3 — persistent damage proof
Prove one Human siege seam first:
intact wall -> breached wall -> save -> fresh process -> same breach -> repair/scaffold -> restored function.

Use Ravenhold ruined geometry only where the material fit is acceptable.
The proof must show the same canonical state in town view and siege view.

Then extend the damage grammar faction by faction:
- Humans: stone breach/scaffolding.
- Dwarves: cracked/disabled forge, smoke, rubble.
- Vikings: burned timber/fresh repair boards.
- Orcs: ever-scarred timber patches over old stone.
- Dark: shattered/corrupted architecture.
- Nature candidate: broken bridges/burned foliage/regrowth.

## Gate 4 — battlefield extraction
From each approved macro donor, select bounded combat zones rather than using whole maps.
Bake hidden hex metadata and validate:
- elevation;
- slope;
- LOS;
- movement cost;
- large-unit footprint;
- flying movement;
- deployment areas;
- at least two routes;
- signature landmark.
## Gate 5 — strategic-to-tactical causality
Build an automated recipe test:
campaign region -> biome/landform/feature/approach/weather -> selected authored battlefield.

Required first proofs:
- bridge/river region selects river/bridge battle;
- mountain pass selects pass battle;
- fortress outskirts selects fortress-edge battle;
- Viking harbour selects harbour/cliff battle;
- ruined Orc region selects ruined/camp battle.

## Gate 6 — visual/presentation pass
Only after mechanics fit:
- faction flags/material accents;
- town-view framing;
- RB Weather variants;
- population/dressing;
- damage smoke/fire;
- unit-card render points;
- hero portrait render points.

## Stop conditions
Do not import an entire asset pack merely because it exists.
Do not hand-author a bespoke replacement until RB/Hyper/Studio Full and owned modular content have been exhausted.
Do not lock Nature as faction six until Treefort/Forest Village passes the UE verticality test.
