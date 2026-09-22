# Soul Overmap Visual Preview Handoff — 2026-09-22

## Purpose

This package turns the structural overmap data into a deterministic visual planning artifact without making UE or canon changes.

It previews the continuous strategic world, the 9-region founder slice, route readability, landmark identity, and recently admitted special-site candidates before any expensive terrain authoring.

Shipping rule remains unchanged: region nodes and route lines are logical helpers only. The final map uses continuous physical geography, terrain-following roads/trails, settlement silhouettes, fog and temporary selection overlays.

## Outputs

- `Evidence/WorldOvermap/soul_overmap_world_preview.svg` — all 36 regions and 51 routes.
- `Evidence/WorldOvermap/soul_overmap_founder_preview.svg` — founder 9-region / 10-route crop.
- `Data/UEImport/soul_overmap_world_markers_v1_20260922.csv` — 36 anchor/region rows.
- `Data/UEImport/soul_overmap_world_routes_v1_20260922.csv` — 51 route/cue rows.
- `Data/UEImport/soul_overmap_candidate_sites_v1_20260922.csv` — region-bound candidate special sites.
- `Data/soul_overmap_preview_bundle_v1_20260922.json` — combined presentation-only payload.
## Current read

The founder slice remains structurally strong: Human Capital feeds Crossroads, then three distinct resource/terrain approaches converge toward Orc territory without route crossings or selection overlap.

The new candidate-site layer adds **Lost Shrine** directly to Ancient Shrine while keeping the founder topology unchanged. Dragon Graveyard, Crystal Cave, Windmill, Red Canyon, Cave Camp, Mystic Dungeon and the neutral Arena city-state are staged on existing world regions rather than creating new nodes.

The full-world preview surfaced one non-blocking presentation risk: `orc_broken_bridge` and `dark_ruined_causeway` are only 60.8 planning-map units apart. That is acceptable structurally, but UE terrain/camera authoring should ensure the two landmarks read as distinct places rather than one crowded junction.

## UE handoff rule

Use the CSVs as staging/data-table inputs, not as authority for campaign outcomes. SoulCore remains authoritative for state and movement; RB Weather owns weather/sky selection; RB Optimization is the first performance layer; RB Save owns persistence.

The cheapest next overmap test in UE is the existing 9-region founder slice: reproduce the positions, roads/trails, visible ford/pass constraints, settlement silhouettes and Lost Shrine candidate, then exercise fog and movement from canonical state before scaling presentation to all 36 regions.
