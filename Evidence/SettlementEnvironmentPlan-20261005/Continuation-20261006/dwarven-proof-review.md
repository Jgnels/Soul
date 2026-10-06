# Authored Dwarf proof - verified state at 12:34 UTC

The approved DwarvenCitadel now runs through Soul's existing construction, visit, save and physical battle systems. **The first authored-environment functional proof passes.** It is not final Crownspine capital art, a complete siege system or a cooked-package acceptance.

Donor: `D:/Unreal Projects/AoEAssetRenderLab/Content/DwarvenCitadel`, native `/Game/DwarvenCitadel/Maps/DwarvenCitadel`. Owned map: `/Game/Soul/Maps/Settlements/L_DwarfHold_Authored`. Its native architecture, section instances, materials and approaches remain substantially intact. The campaign uses the retained evil-waterfront terrain and an explicit three-region opt-in fixture. No terrain package, canonical adjacency, construction balance or save schema was changed.

## Physical groups

| Class | Content |
|---|---|
| STARTING | Excavated hall enclosure/floor, perimeter masonry, adjoining monumental halls, exterior cliff gate and approach. |
| BUILDABLE | Caravan Hall inner colonnade, gathering furnishings, chests and fixtures: 56 persistent native roots plus regenerated children. |
| DECORATION | Native rocks, masonry dressing, flags, ordinary props and lights outside the state group. |
| OMITTED IN SOUL WRAPPER | Posed cinematic dragon, 25 sequence actors and nine dragon-flame cards. The donor retains all of them. |

This is construction/fitting-out within an existing mountain hall, not a new freestanding city. Two broader hide groups were rejected because they exposed unmodeled cave surfaces. Eleven unstable serialized child references were replaced by their already-present persistent parents; runtime children follow the same authority. No duplicate development or persistence owner was introduced.

## Native-1080p functional acceptance

`Local/authored-runtime-r9-native1080` passes all 26 native steps and the separate actual-reinforcement gate, then exits cleanly. The run uses real existing input actions: U buys Caravan Hall for 200 gold; one Space leaves construction in progress; the second completes it. Native colonnade and furnishings appear, and the companion service unlocks. H hires the existing companion for 1,200 gold. F5/F9 restores exact independently recorded `Soul.Campaign` and `Soul.Settlements` strings, including rollback to mid-construction and the matching hidden physical group. Repeated visits/returns retain the same state. The completed miniature is restored in the campaign.

The real encounter bridge loads the owned Citadel at the measured exterior approach `[-13500,0,-750]` cm. All 30 deployment probes and six reserve-entry corridors pass native collision checks. Runtime r9 delivers 20 allied and 15 enemy reserve bodies over five/four waves, then a natural victory after 196.48 seconds. It returns 21 player / 0 enemy strategic survivors to Forge Approach, with settlement state unchanged and final F5/F9 passing. Independent log audit is CONSISTENT, with 728 contacts, 77/77 PBIL queries and no orphan events. r8 previously qualified the same corrected origin with a natural victory and 26 survivors.

Earlier r5-r7 natural defeats returned correctly, but their old Z=0 origin blocked allied reserve entries. They are preserved failure evidence, not corrected-origin defeat acceptance. A new corrected-origin defeat/recruit/retry path is not qualified; the small Dwarf fixture has no Human-capital recruitment destination. No outcome was fabricated to clear a gate.

## Strategic representation

`SM_DwarfHold_Base_r9` is a deterministic 180,000-triangle derivative of the native gate and Caravan Hall. `SM_DwarfHold_Upgrade_r2` adds 14,840 triangles from the same authored colonnade. One gate-group transform compresses strategic scale/spacing; no generated replacement architecture is used. Full visit/battle geometry remains at native scale.

Reviewed r9 starting/completed campaign images show the colonnade appear behind the gate. Selection via Home, town/visit controls, construction, movement into the real encounter and return all work. The current representation remains a rectangular cutaway on a temporary retained quarry anchor. It lacks final mountain embedding, hinterland and capital composition. **Accept shared state and source correspondence; do not promote it as finished capital art.** r5 fallback, r8 intermediate and rejected r6/r7 derivatives remain preserved locally.

## Performance and rendered quality

See `dwarven-shadow-review.md` and `dwarven-performance-r6-shadows-native1080.json`. At native 1920x1080, uncapped after 20s warmup with 30s samples:

| View | Mean / P95 / P99 ms | Average FPS | Below 30 / 40 / 60 | Frames |
|---|---|---:|---|---:|
| Caravan Hall | 27.070 / 32.876 / 36.941 | 36.94 | 49 / 815 / 1108 | 1108 |
| Three-region campaign proof | 8.781 / 9.811 / 10.386 | 113.88 | 1 / 1 / 1 | 3417 |

Peak sampled GPU is 84 C; device-wide VRAM 2,551 MiB, process working set 4,227 MiB and private commit 5,805 MiB. City native LoadMap is 5.54s, excluding asset preparation. The fresh process restores independently expected domains and both representations, visits and returns cleanly. **City 40+ FPS remains unmet.** These fixed views do not qualify the whole campaign or battle performance.

Earlier r2/r3/r4 results upscaled 1400x788 into 1080p. They are not native-1080p evidence. r5-native1080 completed both samples but hit 85 C at the end, so its overall run is thermally stopped. r6 explicitly checks 100% primary/secondary scale and dynamic resolution off; its GPU capture confirms native scene dimensions.

Twelve previously unbaked stationary lights now match the donor's movable-light model. Forty tiny fire effects use native 40m culling. Half-resolution shadows on 205 small fixtures improve mean city time by about 10%; all lights, colors, intensities, attenuation and shadows remain present. Reviewed images preserve masonry, relief, furnishings and the authored hall's light balance. Broader moving-camera review remains needed. The autobattle camera still excludes the monumental gate and is weak context for the city; the actual fight is on its authored approach.

## Evidence and preservation

Raw screenshots/logs/checkpoints: `Local/authored-runtime-r9-native1080/`; fresh native profile: `Local/authored-fresh-r6-shadows-native1080/`. Reviewed r9 images include `_miniature_start`, `_miniature_completed`, `_city_start_clean`, `_city_completed_clean`, `_authored_battle_warm` and `_campaign_after_battle` under `User/Saved/Screenshots`.

`dwarven-donor-after-r9.json` exactly matches all 2,569 baseline file records, including SHA256 for 1,117 maps/external packages. Other records are size/mtime, not full-byte hashes. The original map remains SHA256 `14a026d0ba49f546ca1d08206e97d14d2012702bc01f8878564302597f7b6580`. Licensed maps/meshes/backups remain local-only; source commits do not contain vendor payload. Current handoff records build/test versions and further Human work.
