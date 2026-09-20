# Soul Battlefield Library Plan — 2026-09-20

## Goal
Create Total-War-like perceived variety without authoring hundreds of unrelated maps.
Soul uses authored macro terrain, hidden deterministic hexes, and context-driven dressing.

Every battlefield must have:
- at least two meaningful routes, preferably three;
- one memorable landmark;
- a reason for high ground, forest, water, road or fortification to matter;
- approach-direction support;
- enough open space for large creatures;
- a clean tactical read when overlays are hidden.

The full 8x8 km Landscape Pack maps are source canvases, not combat boards.
UE should crop/author combat zones from strong ridges, bowls, passes and plains.

## Human homeland family
1. Grassland Crossroads — Grassland_01.
   Open flanks + faster road + modest cover.
2. Rolling Ridge — Grassland_02.
   Central high ground and reverse-slope approach.
3. Fortress Outskirts — Hivemind fortress exterior.
   Wall-side protected flank versus open road approach.
4. River Road — Coastal Ruins candidate crop.
   Water-limited flank, road lane, raised bank.
## Viking homeland family
1. Harbour Edge — Water City outer harbour crop.
   Water boundary, bridge choke, upper cliff route.
2. Snow Pass — SnowyMountain_01.
   Constrained large-unit lane, slope ambushes, ranged ledges.
3. Fjord Ridge — SnowyMountain_02 + water treatment.
   Exposed cliff flank, long ridge, brutal weather visibility.
4. Forest Track — Viking Village outskirts.
   Road lane, LOS-breaking woodland, close ambush route.

## Dwarf homeland family
1. Mountain Pass — Mountain_01.
   Switchback, ledges, narrow heavy-unit routes.
2. Snow Basin — SnowyMountain_03.
   Protected center versus dominant surrounding rim.
3. Forge Approach — Legendary Forge exterior.
   Industrial choke, heat/hazard spaces, elevated workshop route.
4. High Quarry — Mountain_04.
   Terraces, rough ground, large-unit bottlenecks.

The Dwarf family should feel engineered into difficult terrain, not merely 'snow maps.'
## Orc homeland family
1. Ruined Field — Medieval Ruins showcase crop.
   Broken LOS, multiple breaches, short cover routes.
2. Badlands — Mesa_01.
   Open War Elephant lane, gully cover, high shelf.
3. War Camp — Ruins/Coastal Ruins + BanditCamp tents.
   Camp obstacles, broad beast lane, flanking through old masonry.
4. Broken Bridge — Ravenhold ruined bridge + Coastal Ruins.
   Split approaches, bridge high ground, alternate ford/side route.

Orc terrain should visually show occupation and damage, not pristine purpose-built civilization.

## Dark homeland family
1. Castle Approach — Fantasy Alien Castle exterior.
   Narrow outer lanes, verticality, flying access.
2. Ash Plain — Desert_01 rematerial/dressing pass.
   Long sightlines, little cover, highly legible spell zones.
3. Ruined Causeway — Ravenhold/Coastal Ruins composite.
   Choke, broken side path, flying bypass.
4. Corrupted Valley — Mountain_05.
   Two valley lanes, elevated spell/ranged ledges, central hazard.

The Dark family should gain identity from corruption/atmosphere/landmarks rather than turning every terrain surface black.
## Optional Nature homeland family
1. Forest Clearing — Forest Village showcase.
   Dense tree line, open center, narrow side trails.
2. River Woodland — Forest Village river area.
   Shallow-water penalty, wooded flank, ranged crossing control.
3. Ancient Shrine — Ancient Mountain/Shrine.
   Terraced objective fight with vegetation flanks.
4. Grassland Edge — Grassland_02 dressed with groves/treefort pieces.
   Open center, protected forest flank, strong beast mobility.

## Neutral/world variety
- Coastal Ruins 01–04: neutral ruined/coastal encounters.
- Grassland 01–02: clean baseline and faction-occupation variants.
- Mountain/Snowy Mountain: neutral passes, shrines and quest battles.
- Desert/Mesa: arid campaign regions and scenario geography.
- Ravenhold pieces: bridges, fortified roads, neutral fortress encounters.
- Ancient Mountain: shrine/sanctuary landmarks.

A faction should be able to fight outside its homeland. Geography belongs to the world region, not the army skin.
## Variation multipliers
Each authored macro map can vary through:
- approach direction / deployment rotation;
- season;
- RB Weather;
- time of day;
- faction occupation dressing;
- road state;
- settlement proximity;
- prior-battle damage or fire;
- objective placement;
- bounded vegetation masks.

Do not randomize tactical fairness blindly.
Major elevation, chokepoints and deployment zones stay authored/validated.

## Hidden-hex bake target
The finished natural terrain is authoritative for presentation; Soul bakes logical cells onto it.
Cells record elevation, slope, terrain tag, movement cost, LOS blocking, large-unit legality, hazard and deployment legality.
The overlay appears for movement/targeting/spells and disappears when no decision needs it.

## Acceptance for a battlefield template
- no camera gymnastics required to understand the battlefield;
- large creatures have at least one viable route;
- ranged units have useful but contestable sightlines;
- at least one terrain feature changes the best tactical decision;
- the strategic map's promise is honored (bridge means bridge, pass means pass, forest means forest);
- landmark makes the map describable in ordinary language after play.
