# Soul Overmap Integrated Non-UE Handoff — 2026-09-22

## Purpose

This branch reconciles the committed overmap start-state and battlefield-fidelity descendants without touching either source worktree's uncommitted files.

Primary generated artifact:
- `Data/soul_overmap_integrated_import_v1_20260922.json`

One-command acceptance:
- `python Tools/run_soul_overmap_nonue_acceptance.py`

The bundle is generated data, not a new gameplay authority. Its job is to give the later Unreal lane one coherent import surface while preserving the source contracts underneath it.

## Current structural contract

- 36 strategic regions.
- 51 undirected travel routes.
- 102 directed approach profiles.
- 14 settlement slots: 6 major + 8 candidate minor.
- 6 factions in the candidate full-world opening overlay.
- 12 starting-owned regions and 24 neutral regions in that candidate overlay.
- Founder slice: 9 regions, 10 routes, 20 directed approaches.
- 36 strategic-map visual anchors, including all 9 founder regions.
- 27 battlefield recipes referenced by the world.
## Import order

The integrated artifact declares this import sequence:
1. world space and terrain features;
2. regions and travel routes;
3. settlement slots;
4. campaign start-state overlay;
5. directed approach profiles;
6. battlefield recipe bindings;
7. presentation contract;
8. strategic-map visual anchors;
9. macro-region surface bindings.

This order keeps deterministic campaign state ahead of Unreal presentation. Weather and time are still injected dynamically at battle commitment rather than authored into geography.

## RB authority preserved

The presentation contract remains explicit:
- SoulCore owns world/campaign state.
- RB Weather owns dynamic weather.
- RB Optimization is the first optimization authority.
- RB Save owns persistence.
- Unreal overmap actors/materials present state; they do not invent campaign outcomes.

The validator fails if those RB authority bindings drift in the generated bundle.
## Acceptance evidence

`Evidence/WorldOvermap/nonue_acceptance_manifest.json` records 13 deterministic non-UE checks covering topology, runtime import, settlements, approaches, start state, founder traversal, settlement coverage, battlefield fidelity, opening pressure, visual-anchor generation/validation, bundle generation, and bundle validation.

`Evidence/WorldOvermap/integrated_import_validation.json` additionally verifies:
- all nine source-artifact SHA-256 hashes;
- route and approach referential integrity;
- settlement-region references;
- macro-region surface-binding coverage;
- founder ownership/start/objective invariants;
- six-faction opening counts;
- RB Weather / RB Optimization / RB Save authority bindings.

## Remaining gates

The current hard structural contract is green. The main open work is presentation qualification, not more graph architecture.

Eight destinations need deliberate geography-to-battlefield fidelity review: Ancient Shrine, Forest Edge, North Pass, Northwest March, Old Quarry, Orc Watch, River Ford, and Southern Crossing. These are not topology failures; they are places where the selected battlefield recipe transforms campaign context enough that the UE lane must prove the visible route decision still matters.

The visual-anchor question is now closed at non-UE level: all 36 regions have explicit physical navigation targets. Remaining visual-anchor gates are mesh/proxy qualification, readability at strategic camera distance, fog/weather legibility, and performance in the bounded UE founder-slice proof.
