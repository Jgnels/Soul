# Soul World Overmap Runtime Import — 2026-09-22

## Purpose

This package turns the non-UE world structure into deterministic data a future UE overmap lane can consume without inventing geography again.

The source of truth remains `Data/soul_world_overmap_v1_20260922.json`.
The UE-facing derivative is `Data/soul_overmap_runtime_import_v1_20260922.json`.

## World-space convention

- map origin: structural coordinate `(500, 475)`
- scale: one structural map unit = 1,000 Unreal centimeters = 10 meters
- planned world envelope: approximately 9.35 km × 8.55 km
- occupied strategic-anchor footprint: approximately 8.0 km × 7.5 km
- map south maps to negative Unreal Y
- low / mid / high anchor Z bands are 0 / 12,000 / 30,000 cm

These are import anchors, not final landscape elevation samples.
## Region anchors

All 36 strategic regions export:
- stable region ID and display name;
- Unreal position in centimeters;
- presentation anchor type;
- owner and macro region;
- biome, landform, feature and elevation band;
- settlement/resource binding where present;
- battlefield recipe hint;
- neighbors, road neighbors and approach direction from each neighbor.

The six faction capitals export as `settlement_proxy` anchors.
Resource sites and chokepoints retain distinct presentation types so their gameplay purpose can be visible without permanent node UI.

## Route splines

All 51 strategic links export deterministic three-point spline data.
Roads are wider than trails; chokepoints remain explicit metadata.
The spline bend is stable from route ID, so regeneration does not reshuffle road shape.

This is intentionally presentation-only geometry.
Movement legality, AP cost and logistics cost remain canonical data, not spline-distance calculations.
## Founder-slice traversal gate

The headless simulation mirrors the current founder runtime rule: the player moves to a region adjacent to the Orc Stronghold, then spends one action point to commit to battle.

Current result:
- three equal-action attack approaches;
- three moves to a final approach + one battle action;
- fastest battle therefore begins early on Day 2 at the 3-AP baseline;
- Old Quarry detour still fits in six total actions / two days;
- Forest Edge is both a resource decision and a viable flank;
- Ancient Shrine is a deliberate longer hero/magic-progression detour;
- the Orc Stronghold is revealed from either final approach before battle commitment.

Evidence:
- `Evidence/WorldOvermap/founder_slice_simulation.json`
- `Evidence/WorldOvermap/founder_slice_simulation.md`
- `Evidence/WorldOvermap/runtime_import_validation.json`

## UE handoff order

1. Import only the nine founder regions and their runtime anchors.
2. Replace debug nodes with terrain/road/settlement proxies while retaining exact region IDs.
3. Drive click selection, fog, AP and AI from SoulCore state.
4. Draw route highlighting from the exported spline data; do not derive movement legality from the spline.
5. Prove River Ford / Forest Edge / North Pass predict their tactical battle context.
6. Expand toward the full 36-region world only after the founder loop survives a full battle-return cycle.

## Ownership semantics

The founder slice's populated Human and Orc ownership is scenario-start state.
Outside that nine-region slice, faction color is only a **homeland affinity / visual identity candidate**.

Do not initialize a full six-faction campaign by counting or copying those affinity regions.
A separate campaign-start-state pass must explicitly choose starting possessions, neutral expansion targets, army spawns and diplomacy.

Supporting audit:
- `Evidence/WorldOvermap/frontier_analysis.json`
- `Evidence/WorldOvermap/frontier_analysis.md`
