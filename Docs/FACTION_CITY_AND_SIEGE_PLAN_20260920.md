# Soul Faction City + Siege Construction Plan — 2026-09-20

## Shared rule
A settlement is one canonical object with four presentations:
1. strategic-map representation;
2. fixed-camera HOMM-style town view;
3. optional live 3D visit;
4. siege battlefield.

All four consume the same building, wall and damage state.
A destroyed recruitment dwelling remains unavailable until repaired.
A breached wall remains breached after combat.
Repair can restore function before the visual scar completely disappears.

## Human capital
Base: Hivemind PL_Fortress_Day.
Town-view composition: main gate foreground, military/market courtyard center, keep high/central, Griffon Roost and Witch Collegium visible on the skyline.

Recruit dwellings:
- Muster Yard -> militia/basic fighter.
- Archery Range -> ranged slot.
- Spear Guardhouse -> Knight_04 / spear fighter.
- Man-at-Arms Barracks -> swordsman fighter.
- Witch Collegium -> Witch Adventurer support/magic.
- Royal Chapterhouse -> Royal Guard / Knight_01 elite fighter.
- Griffon Roost -> Griffon beast.
Human civic structures:
- Royal Keep: hero services and settlement control.
- Forge: exact Hivemind Forge prefab; equipment/upgrades.
- Tavern: exact Tavern prefab; heroes/intelligence.
- Market: Hivemind market assets; resource exchange.
- Curtain walls/gate: defense and siege geometry.

Human siege layers:
outer fields -> gate/walls -> lower courtyard -> inner keep.
Physical objectives: Gatehouse, Forge, Witch Collegium, Keep.
The Griffon Roost should create a visually obvious aerial landmark without becoming the primary victory objective.
Generic Building A/B/C/D assignment is deliberately left for UE; their filenames do not tell us enough to choose responsibly.

## Dwarf hold
Base: Modular Legendary Forge, supported by Mountain/SnowyMountain terrain.
Town-view composition: lower workshops foreground, molten Great Forge central, King's Hold above, Dragon Eyrie highest.

Recruit dwellings:
- Stoneguard Hall -> Bedvar.
- Crossbow Workshop -> Orme.
- Hammer Hall -> Broddi.
- Rune Forge -> Agvid / rune-smith support.
- Construct Foundry -> Golem candidate.
- King's Guard Hall -> elite dwarf fighter.
- Mountain Dragon Eyrie -> Mountain Dragon.
Dwarf civic structures:
- Great Forge: settlement production and upgrade heart.
- King's Hall: hero services/control.
- Deep Mine: ore income.
- Caravan Hall: trade.
- Mountain Gate: defense.

Dwarf siege layers:
mountain approach -> outer works -> gate tunnel -> forge district -> inner hold.
Physical objectives: Mountain Gate, Great Forge, Rune Forge, King's Hall.
The Dragon Eyrie should be an upper optional tactical objective: capturing it removes aerial reinforcement/growth, but the city can still be won through the hold.
Persistent damage should lean on the Forge pack's damage decals, smoke and modular breakage before borrowing unrelated stone.

## Viking harbour
Base: Water City geography + Viking Village cultural kit.
Town-view composition: boats/piers foreground, lower harbour and bridge midground, settlement climbing toward the Great Hall.

Recruit dwellings:
- Raider Longhouse -> Viking Raider.
- Hunter Range -> hunter/skirmisher; use real archery target assets.
- Shield Hall -> Shieldmaiden.
- Berserker Mead Hall -> Viking Ulf / Barbarian.
- Shaman Lodge -> Shaman.
- Huscarl Hall -> heavy Customized Viking.
- Wolf Kennels -> Wolf.
Viking civic structures:
- Jarl's Great Hall: hero services/control.
- Shipyard & Docks: trade/coastal projection.
- Blacksmith: equipment/upgrades.
- Harbour Market: resource exchange.
- Palisade Watch: defense.

Viking siege layers:
shore/docks -> bridge or palisade -> lower village -> upper Great Hall.
Physical objectives: Bridge Watch, Shipyard, Shaman Lodge, Great Hall.
The layout should preserve multiple attack choices: landward road, shoreline/harbour pressure and at least one bridge/high route.
Timber damage should not reset to pristine after repair; fresh boards, scaffolding and fire scars are part of faction history.

## Orc ruinhold
Base: Medieval Ruins, reinforced with Ravenhold ruined pieces and BanditCamp tents/weapon racks.
Town-view composition: old stone ruin as permanent skeleton, crude Orc occupation growing through it, elephant yard large and visible near the outer gate.

Recruit dwellings:
- Grunt Barracks -> Orc grunt.
- Hunter Range -> Orc ranged hunter/skirmisher.
- Shield Pit -> shield/heavy Orc.
- Berserker Pit -> berserker.
- Brute Hall -> heavy brute.
- Shaman Totem Court -> shaman/drummer/caster support.
- War Elephant Yard -> War Elephant.
Orc civic structures:
- Warchief Hall: hero services/control.
- Salvage Forge: upgrades.
- Spoils Market: resource exchange.
- War Drum Tower: defensive/morale support.
- Patched Ruin Walls: siege defense.

Orc siege layers:
broken outer city -> patched palisade -> war camp -> ritual ruins -> scarred citadel.
Physical objectives: Elephant Gate, War Drum Tower, Shaman Court, Warchief Hall.
Repairs are deliberately asymmetric: Orcs add timber, stakes and scavenged material instead of restoring old masonry.
That means an Orc-held city becomes visually more Orc over time even though its ancient stone skeleton remains.

## Dark fortress
Base: Fantasy Alien Castle preassembled scene, pending UE qualification.
Town-view composition: dead approach foreground, ritual/guard structures rising toward an inner sanctum, Dragon Roost highest.

Recruit dwellings:
- Black Guard Bastion -> Dark Knight.
- Dread Gallery -> Devil / ranged spell attacker.
- Execution Court -> Executioner.
- Demon Gate -> Demon.
- Fallen Hall -> Troll / brute.
- Befouler Sanctum -> Befouler support.
- Apex Dragon Roost -> Fantasy Dragon.
Dark civic structures:
- Throne Sanctum: hero services/control.
- Occult Armory: upgrades.
- Crypt Exchange: resource exchange.
- Ward Spire: magical settlement defense.
- Outer Works: siege defense.

Dark siege layers:
dead approach -> outer buttresses -> guard gallery -> sacrificial court -> inner sanctum.
Physical objectives: Ward Spire, Demon Gate, Dragon Roost, Throne Sanctum.
UE art filter is mandatory: preserve otherworldly silhouettes but remove or hide anything that reads as machinery/technology.
Use magical light as an event or ward, not permanent neon character-action styling.

## Optional Nature / Animal / Primitive / Centaur treehold
Base: Fantasy Forest Village + separate Treefort/Nature donor when located.
This remains a candidate faction, not part of the current five-faction requirement.

Recruit dwellings:
- Tribal Hall -> primitive fighter.
- Centaur Range -> Centaur Archer.
- Animal Warrior Lodge -> Animal Warrior fighter.
- Canopy Guard Platform -> fighter.
- Guardian Circle -> elite fighter.
- Druid Circle -> support/magic.
- Beast Grove -> beast slot TBD.

Civic spaces: Great Tree, Healing Spring, Trade Clearing, Ancient Shrine and Root/Timber defenses.
Several recruitment sites should be glades/platforms/groves rather than conventional buildings.
Nature siege layers:
forest approach -> root barrier -> ground village -> canopy bridges -> Great Tree core.
Physical objectives: Root Barrier, Druid Circle, Canopy Bridge, Great Tree.
Damage language should be broken platforms, burned/stripped foliage and damaged bridges, with gradual regrowth/patching during repair.

## Cross-faction construction rule
Every visible recruitment dwelling should answer three questions without UI:
1. what kind of unit comes from here;
2. whether the building is healthy enough to recruit;
3. whether the settlement has grown since the player's previous visit.

Use props and silhouettes to reinforce function:
bows/targets for ranged, shields/weapons for fighters, cages/pens/groves for beasts, ritual objects for support/magic, monumental elevation for elite/capstone structures.

Do not force identical layouts or identical building counts beyond the seven core recruitment hooks.
Shared systems should be symmetrical; the cities themselves should not be.

## Interior interaction scope — locked 2026-09-20
Soul city interiors are tactical spaces, not object-simulation spaces.

Required for siege-relevant buildings:
- navigable entry/exit routes;
- usable stairs/upper floors where the donor supports them;
- reliable collision;
- doorways/windows/corners that create defensible positions;
- enough interior room for Soul combat groups that are allowed indoors;
- optional building-control / hold-point anchors where tactically useful.

Not required:
- individual interaction with furniture, food, bottles, crates, cups or decorative clutter;
- per-prop destruction or physics;
- bespoke civilian-use logic for every room.

Optimization implication:
- preserve architectural shells/interiors and tactical openings;
- simplify or remove pathological micro-props;
- use Nanite/instancing/static presentation where appropriate without sacrificing collision/nav;
- only objects with gameplay consequences need separate interactive actors.

The Hivemind town can therefore remain visually dense and fully enterable while Soul strips the extremely expensive diner-detail meshes that add no siege gameplay.

## Global environment interaction rule — locked 2026-09-20
Apply this across all Soul settlements, interiors, battlefields and adventure-map locations.

### Keep individually interactive
- heroes/companions and other gameplay-relevant NPCs;
- doors, gates, ladders, siege mechanisms and tactical traversal pieces when gameplay uses them;
- settlement buildings and fortification segments whose state persists;
- siege objectives, destructible/breachable structures and explicitly authored hazards;
- rare props only when they have a concrete gameplay action or consequence.

### Keep tactically solid but not individually interactive
- walls, floors, stairs, windows, balconies, counters and large furniture that shape movement/cover;
- room-scale obstacles that soldiers must path around or use defensively.

### Merge / instance / bake as non-interactive dressing
- table + chairs + dishes;
- shelf + books/jars;
- market stall + goods;
- bed + bedside clutter;
- forge corner + loose tools;
- barrels/crates grouped as scenery;
- food, cups, plates, bottles, cutlery and other micro-props;
- distant or unreachable interior dressing.

The default implementation is one static/instanced/merged 3D clutter cluster rather than a literal 2D card when units can see it from multiple angles.
A flat impostor/image is appropriate only for distant/unreachable presentation where parallax will not expose it.

### Tavern exception
Taverns may support Bannerlord-style in-world hero/companion recruitment.
Recruitable heroes remain real NPC actors with dialogue/identity/recruitment state.
The tavern's furniture and decorative clutter still follows the normal non-interactive clustering rule.
A companion does not require the mug, chair, table and shelf around them to become individually simulated.

### Optimization authority
Use RB Optimization first for eligible repeated/simple static representations.
Do not duplicate RB Optimization with a bespoke city-wide representation system.
Author-level cleanup still applies to pathological source assets whose raw geometry/build cost is unreasonable.

