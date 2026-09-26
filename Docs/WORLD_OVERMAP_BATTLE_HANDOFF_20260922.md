# Soul Overmap -> Battle Handoff Contract — 2026-09-22

This is a deterministic non-UE contract between Soul campaign geography and tactical battle launch.
It does not resolve combat and does not replace RB Weather, settlement, siege, or battle runtime authority.

## Coverage

- Founder regions: 9.
- Directed founder approaches: 20.
- Generated battle handoffs: 20.
- Stronghold directed entries: 2 (north_pass, orc_watch).
- Handoffs carrying candidate special-site overrides: 1.

## Runtime contract

- Destination biome/landform/feature/elevation and route/entry direction select the battlefield recipe context.
- RB Weather owns the weather snapshot; the campaign clock owns time-of-day.
- Settlement/siege state is passed through from Soul runtime and cannot be synthesized by presentation.
- Reinforcement entry directions are constrained to actual adjacent strategic routes and runtime timing.
- Candidate special sites are opt-in overrides only; they do not modify the campaign graph.
- Stable battle seeds are SHA-256-derived from scenario, campaign seed, turn, source, destination, and encounter ordinal.

## Current recipe readiness across founder directed handoffs

- PAYLOAD_PENDING: 1
- READY_FOR_UE: 15
- UE_COMPOSITE: 2
- UE_CROP_REQUIRED: 2

## Important founder observations

- Orc Stronghold can be entered from Orc Watch or North Pass; those produce distinct entry directions and reinforcement slots.
- River Ford preserves the river-crossing context rather than falling back to a generic field.
- Ancient Shrine carries the Lost Shrine candidate as an inactive special-site override; activation must be explicit.
- Recipe readiness is an asset-production concern. A valid handoff may point to a recipe that still needs UE crop/composite work.
