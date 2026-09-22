# Soul World Overmap Visual Anchors — 2026-09-22

## Purpose

Give every strategic region one readable physical landmark so the final campaign map reads as geography rather than exposed graph nodes.

This is a presentation layer only. SoulCore still owns region state and movement; RB Weather owns weather; RB Optimization remains the first performance authority; RB Save owns persistence.

Data:
- `Data/soul_overmap_visual_anchors_v1_20260922.json`

Validation:
- `Evidence/WorldOvermap/visual_anchor_validation.json`
- `Evidence/WorldOvermap/visual_anchor_validation.md`

## Reference synthesis

The plan uses Heroes III for glance-readable adventure-map sites, Total War: Warhammer for exaggerated strategic landmarks/chokepoints, and Bannerlord for settlements physically embedded in continuous terrain. No proprietary map content or code is copied.
## Anchor classes

- Major settlements: recognizable city silhouettes derived from the actual visitable settlement identity, with persistent damage/repair state projected onto the proxy.
- Minor settlements: medium-scale bounded proxies rather than capital-sized scenes.
- Resource sites: visible infrastructure or landscape signatures that communicate why the location matters before selection.
- Crossings/chokepoints: bridge, ford, ravine, pass, gate, or equivalent physical silhouette.
- Landmarks: shrine, altar, monument, great tree, mesa pillar, or other navigation object shared with the battlefield promise.
- Terrain landmarks: small non-interactive navigation cues where a region has no settlement/resource interaction.

Every anchor carries its region ID, macro region, source/donor reference, battlefield recipe binding, scale class, interaction scope, and founder-slice marker.

## Founder-slice readability contract

The nine-region slice now has explicit anchor expectations:
- Human Capital and Orc Stronghold: major settlement silhouettes.
- Crossroads: bounded market-town proxy.
- Old Quarry: readable resource infrastructure.
- Forest Edge: readable woodland/resource edge.
- River Ford: visible crossing geometry.
- Ancient Shrine: shrine landmark.
- Orc Watch: badlands/mesa navigation landmark.
- North Pass: explicit chokepoint silhouette.

These cues must remain legible at regional zoom and survive fog/weather presentation strongly enough that route choice still predicts tactical context.
## Performance rule

Do not load full settlement scenes onto the strategic map. Proxies should be simplified static/HLOD/instanced representations, with RB Optimization applied before bespoke culling or representation systems. Campaign armies stay party-level actors rather than rendered formations.

## Current qualification

The generated layer contains 36 anchors: 6 major settlement silhouettes, 8 minor settlement proxies, and 22 non-settlement navigation/site anchors. Nineteen anchors are interactive; seventeen are navigation-only.

The validator requires complete one-anchor-per-region coverage, source references, battlefield-recipe alignment, all settlement tier rules, founder-slice readability classes, and interaction for resource sites.

This closes the non-UE question of **what the player should visually navigate toward**. It does not qualify final mesh choice, silhouette readability at camera distance, fog/weather visibility, or performance; those belong to the later bounded UE presentation proof.
