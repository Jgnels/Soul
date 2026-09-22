# Soul World Overmap — Structural V1 — 2026-09-22

## Purpose

This is the first concrete campaign/adventure overmap for Soul.
It is deliberately **non-UE** so the active realtime-battle lane can continue independently.

The overmap has two scales:
- a **9-region founder slice** for the first Human-vs-Orc 30–60 minute loop;
- a **35-region structural world** that places all six current faction seats in one connected geography.

Macro-region names are temporary. Region IDs already used by the founder playtest are preserved exactly.

## Design reference synthesis

Soul does not copy another game's map. It combines three useful patterns:
- **Heroes III:** fog/exploration, meaningful roads, mines/resources, landmarks and readable adventure-map objectives;
- **Total War: Warhammer:** settlement-centered territorial pressure, terrain-constrained movement, road/pass chokepoints and geography that matters strategically;
- **Bannerlord:** a continuous-looking physical campaign world where settlement silhouettes, resources and terrain are visible on the map itself.

The logical region graph is implementation data. It should not look like a board of exposed nodes in final presentation.

## Founder slice — exact current prototype topology

The current playable founder scenario is embedded unchanged:

Human Capital -> Crossroads

From Crossroads:
- north/east to **Old Quarry -> Ancient Shrine**;
- east/south to **River Ford -> Orc Watch**;
- direct wild route to **Forest Edge**.

Forest Edge branches to **Orc Watch** or **North Pass**.
Orc Watch and North Pass provide two distinct approaches to **Orc Stronghold**.

This produces ten links across nine regions, enough for:
- one obvious safe road;
- one resource detour;
- one magic-landmark detour;
- a forest flanking line;
- two final approaches to the enemy stronghold.

The overmap data keeps the founder prototype's region IDs so the presentation can later replace the blockout without rewriting campaign state.

## Full structural world

The full v1 places the faction seats as geographic anchors rather than menu destinations:

- **Vikings — northwest coast:** harbour, fjord ridge, forest track and snowy pass.
- **Dwarves — northeast mountain spine:** quarry, snow basin, forge approach, mountain pass and fortified hold.
- **Humans — west-central heartland:** capital, crossroads, quarry/river/resource routes and frontier forest.
- **Orcs — east-central badlands:** stronghold, watch, ruined field, war camp, dry mesa and broken bridge.
- **Nature — southwest forest/river country:** Treehold, shrine, forest clearing, river woodland and meadow edge.
- **Dark — southeast corrupted country:** valley, ruined causeway, ash plain, castle approach and fortress.

Neutral landmarks and crossings join the homelands so geography creates fronts rather than each faction living in an isolated theme park.

The structural map currently contains **35 strategic regions, 48 links, 25 road links and 10 explicit chokepoints**.
Those are starting topology numbers, not shipping balance locks.

## Presentation contract

The final Unreal overmap should look like a place, not a graph editor.

Keep:
- visible roads, bridges, passes, rivers and terrain barriers;
- small but recognizable settlement silhouettes derived from the actual city scenes;
- resource-site dressing that communicates output without opening a tooltip;
- army/hero actors travelling over the terrain;
- explored terrain memory plus current visibility/fog;
- strategic overlays only when needed for selection, ownership, threat or route planning.

Avoid:
- permanent giant node circles;
- arbitrary province borders that contradict terrain;
- perfectly straight route lines;
- terrain that looks decorative but has no relationship to travel or battlefield selection.

The logical graph remains deterministic underneath the physical presentation.

## Battlefield handshake

Every overmap region already carries:
- biome;
- landform;
- feature;
- preferred battlefield-recipe hint.

Later UE integration should also derive attacker approach from the selected connecting edge.
This preserves Soul's rule that a bridge, pass, forest, road or settlement visible on the campaign map must be honored by the tactical battle.

## Next UE integration gate

Do not consume the active realtime-battle lane for this yet.

When a UE lane is free:
1. import the founder-slice positions and links into the existing founder campaign presentation;
2. replace debug-node presentation with terrain/road/settlement proxies while preserving the same region IDs;
3. verify click selection, fog, AP spending and AI movement still use the canonical SoulCore world state;
4. make Human Capital and Orc Stronghold recognizable as their actual city donors;
5. prove River Ford, Forest Edge and North Pass visually predict their tactical battlefield context;
6. only then expand presentation from the 9-region slice toward the 35-region structural world.

## Generated artifacts

- Data/soul_world_overmap_v1_20260922.json — machine-readable geography/topology.
- Evidence/WorldOvermap/soul_world_overmap_v1.svg — full structural-map schematic.
- Evidence/WorldOvermap/soul_founder_slice_overmap_v1.svg — focused founder-slice schematic.
- Evidence/WorldOvermap/validation.json — deterministic graph/settlement/topology checks.
- Tools/build_soul_world_overmap.py — deterministic generator.
- Tools/validate_soul_world_overmap.py — non-UE acceptance.
