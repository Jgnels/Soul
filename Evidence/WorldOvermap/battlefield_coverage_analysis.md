# Soul Overmap - Battlefield Coverage Audit

Hard integrity: **PASS**.
Regions covered: **36/36**.
Directed approaches checked: **102**.

## Recipe readiness by destination region

| Status | Regions |
|---|---:|
| LOCAL_VERIFIED | 2 |
| PAYLOAD_PENDING | 11 |
| READY_FOR_UE | 18 |
| UE_COMPOSITE | 4 |
| UE_CROP_REQUIRED | 1 |

## Context fidelity

Exact metadata equality is diagnostic only; settlement approaches may intentionally transform a capital/hold into its outskirts.

| Class | Regions |
|---|---:|
| context_transform_review | 8 |
| exact_context | 23 |
| settlement_approach_transform | 5 |

## Priority context-transform review

These are not hard failures. They are founder-slice or identity-sensitive destinations where the selected recipe changes campaign context metadata.

| Region | Recipe | Status | Differing context |
|---|---|---|---|
| Ancient Shrine | neutral.mountain_shrine | PAYLOAD_PENDING | biome: mountain_forest -> mountain; landform: shrine_terrace -> terrace; feature: ancient_shrine -> shrine |
| Forest Edge | nature.grassland_edge | READY_FOR_UE | biome: forest -> meadow_edge; landform: woodland -> rolling_plain; feature: grove -> forest_edge |
| North Pass | dwarf.mountain_pass | READY_FOR_UE | feature: narrow_pass -> switchback |
| Northwest March | human.rolling_ridge | READY_FOR_UE | landform: rolling_plain -> ridge; feature: trade_road -> high_ground |
| Old Quarry | human.rolling_ridge | READY_FOR_UE | feature: resource -> high_ground |
| Orc Watch | orc.badlands | READY_FOR_UE | feature: watch -> dry_gully |
| River Ford | human.river_road | UE_CROP_REQUIRED | feature: river_crossing -> river_or_lake_road |
| Southern Crossing | neutral.open_grassland | LOCAL_VERIFIED | landform: river_valley -> plain; feature: road_crossing -> open |

## Interpretation

- Missing recipe IDs or profile drift are hard failures; none should be accepted.
- PAYLOAD_PENDING / UE_COMPOSITE / UE_CROP_REQUIRED are production-readiness gates, not topology failures.
- Context transforms require design/UE review only where the chosen battlefield stops communicating a campaign-map feature that mattered to the route decision.
- Weather and time remain runtime-owned and are intentionally outside this static audit.
