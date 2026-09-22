# Soul Battlefield Environment Expansion — 2026-09-22

## Decision

Owned, prebuilt environment maps are **battlefield donors first**, not raw material we automatically rebuild.

If a showcase map already provides strong tactical geometry, the first question is whether Soul can safely wrap/crop and fight on it. Recomposition is the fallback when the authored map fails navigation, deployment, performance or strategic-context fidelity.

This preserves the production value already paid for while keeping Soul's combat rules independent of vendor content.
## Admitted this round

**Direct battlefield candidates**
- Dragon Graveyard — first qualification target and signature mythic landmark.
- Lost Shrine — preferred replacement candidate for the current Ancient Shrine fidelity gap.
- Crystal Cave — Dwarf/resource-site crystal-mine variant.
- Old Windmills Modular — rural landmark battle where the windmill remains visible and meaningful.
- Red Canyon Desert Biome — Orc/badlands alternate battlefield.
- Medieval Fantasy Cave Camp — fortified/raider/Orc camp variant.
- Mystic Dungeon — arcane special-site battle.

**Tactical overlay**
- Medieval Defense Spikes/Fences/Walls — fieldworks modifier for prepared camps and siege outskirts.
**Special venue**
- Modular Gladiator Arena — one neutral independent city-state venue, not one arena per faction.

**Atmosphere palettes**
- Aurora Skies
- Surreal Skies
- Chaotic Skies II

Sky assets are qualified through **RB Weather** as selectable presentation palettes. They do not become hard-coded battlefield recipes.

Generic dungeon, Jungle Ruins, Desert Temple, Babylon Temple and the second Fantasy Cave set remain owned reserves rather than admitted production work in this pass.
## Arena city-state scope

The arena is staged as a **candidate special site at Southern Crossing**, an existing neutral node outside the 9-region founder slice. This requires no world-topology change.

Initial feature scope:
- enter the independent city-state;
- register for an arena bout;
- fight personally using Soul's existing battle combat;
- support a duel and a small-team bout profile;
- optionally connect simple payout/reputation later through existing campaign authority.

Explicitly out of scope:
- owning gladiators;
- a separate gladiator roster;
- training/management simulation;
- a parallel gladiator economy/campaign;
- duplicating an arena across every faction.
## Direct-map qualification

A prebuilt environment is promoted into battlefield_recipes.json only after it passes:

1. usable combat footprint and at least two deployment zones;
2. collision/navigation/group steering;
3. camera and ranged-sightline viability;
4. the strategic landmark remains recognizable during battle;
5. the accepted 48-active-combatant baseline;
6. RB Optimization before bespoke optimization.

Vendor maps remain untouched. Soul should wrap, stream, crop, or duplicate into Soul-owned content.
## First visually representative battle

Target: **Dragon Graveyard — Humans vs Dwarves**.

Already available:
- Dragon Graveyard: 2 local maps and 93 local assets in AoEAssetRenderLab;
- accepted Soul realtime battle: 24v24, 16 combat groups, RB Combat + RBAI/PBIL + RB Magic;
- Human Knights_Pack payload is local;
- Dwarf Dwarf_Pack payload is local.

Important distinction: the accepted realtime battle currently renders combatants as cube proxies. The combat/AI is real; the visual-representation bridge to skeletal character meshes is the remaining unit-side integration.
## First UE proof gates

The first new-environment playtest should do only this:

1. create a Soul-owned Dragon Graveyard wrapper/crop;
2. mark valid battle floor, bounds and Human/Dwarf deployment anchors;
3. prove collision/nav/group steering;
4. replace cube bodies with a bounded Human/Dwarf skeletal-mesh representation set plus basic locomotion;
5. run the existing 24v24 battle/magic proof unchanged;
6. capture pathing, readability and performance evidence.

Do not wait for every faction roster, every animation or the whole 36-region campaign presentation before running this proof. It is specifically intended to falsify the environment + real-unit representation seam early.
