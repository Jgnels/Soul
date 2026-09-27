# Soul mechanics foundation v0.1

## Tactical
- Axial hidden hex grid with deterministic distance, pathing, radius queries and LOS.
- Natural terrain metadata drives movement cost, LOS and occupancy rather than visible board-game tiles.
- 1–3 hex footprints support humanoid groups, elephants and apex creatures.
- Ground and flying movement modes are distinct.
- Enemy melee groups exert zone of control; flying movement ignores ground ZOC.
- Elevation, forest protection and facing/flanking expose bounded tactical modifiers.
- Initiative is speed-based and interleaves both sides.
- Wait moves a unit into a late initiative lane rather than ending its opportunity.
- Retaliation is once per round by default.
- Morale and luck are bounded and seeded.
- Large groups use stabilized damage variance so 50 soldiers do not swing as wildly as one creature.

## Factions and hero entities
Each finished faction has exactly seven core slots: four melee, one ranged,
one support/magic, and one apex. Apex is inside the seven, never an eighth slot.
Exactly one core family uses the quadruped direction; body plan is independent
of role, so that family need not be the apex. Nature's apex remains undecided.
An unresolved roster must not validate as final content.

Heroes and paragons are distinct persistent hero entities. Both use the existing
hero recruitment authority; neither consumes a core slot. Hero kind, hero level,
unit role, and regiment rank are separate concepts. Presentation should preserve
non-anime, HOMM-style silhouette and role readability.

## Regiments
A combat group is persistent. Rank-and-file individuals are not persistent psychological agents.
Ranks: Recruit, Seasoned, Veteran, Elite, Legendary.
Veterancy bonuses are intentionally bounded to avoid irreversible snowballing.
Unit tier/family and regiment veterancy remain separate systems.

## Commanders
Commanders carry subjective memory.
Current memory contexts:
- remembered victory/defeat against a rival;
- loss of a place;
- recency/intensity decay;
- bounded caution/confidence;
- bounded reclaim motive.
A replacement elephant does not remember being defeated; its commander can.

## Strategy
Campaign AI evaluates explicit candidates and records score components/reason tags.
Current candidates:
- hold;
- recover;
- defend threatened owned region;
- capture resource region;
- attack visible army;
- seize exposed region;
- siege adjacent settlement.
Memory can change the selected action without becoming an absolute script.

## Campaign
- explicit day/action budget;
- recurring income;
- finite recruitment pools;
- weekly growth/capacity;
- resource affordability;
- army readiness, supply and fatigue;
- roads make movement/logistics cheaper;
- hostile territory increases supply pressure;
- force march trades future readiness for immediate reach.

## World and battle maps
Adventure regions retain explored terrain separately from current visibility.
Campaign biome/landform/feature/road/approach becomes tactical battle context.
Battlefield templates are authored skeletons selected by a deterministic recipe scorer.
A campaign-map river crossing should produce a river-crossing battle rather than a random arena.

## Siege
Sieges begin before combat.
No pocket ladders or mid-battle magical construction.
Preparation can include ladders, ram, tower or breach; defenders can reinforce gates, stock ammunition and establish wards.
Siege layers: outer field -> walls -> inner settlement -> keep.
Named objectives have physical meaning: gatehouse, armory, mage tower, keep.
Encirclement creates supply pressure before assault.

## Magic
Spells use deterministic range/shape/effect definitions.
Area effects operate on hidden hex topology while Unreal VFX remains presentation.
The effect vocabulary already includes damage, healing, status/stat changes, displacement, summon, terrain change and reveal.
