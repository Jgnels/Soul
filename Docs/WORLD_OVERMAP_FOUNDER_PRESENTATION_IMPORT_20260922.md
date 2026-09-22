# Soul Founder-Slice Presentation Import — 2026-09-22

## Purpose

Provide the eventual Unreal founder-slice lane one compact, deterministic import surface instead of requiring Blueprint or editor scripting to re-join world, route, anchor, fog and battlefield data.

Primary artifact:
- `Data/soul_founder_slice_presentation_import_v1_20260922.json`

Build / validate:
- `python Tools/build_soul_founder_presentation_import.py`
- `python Tools/validate_soul_founder_presentation_import.py`

The artifact is derived presentation input. It does not become a new campaign-state authority.

## Founder slice preserved

The package contains exactly the current 9-region founder slice:
- Human Capital
- Crossroads
- Old Quarry
- River Ford
- Forest Edge
- Ancient Shrine
- Orc Watch
- North Pass
- Orc Stronghold
It also carries all 10 founder routes, 20 directed approaches, exact current UE blockout positions, selection radii, visual anchors, route spline points, route visual cues, initial fog state, battlefield recipes and battlefield-fidelity classification.

## Three attack corridors

The equal-action Human-to-Orc approaches remain mechanically identical in action count but visually distinct:

1. Human Capital -> Crossroads -> River Ford -> Orc Watch -> Orc Stronghold
   - `road -> road -> road -> road`
   - lowest logistics burden of the three.
2. Human Capital -> Crossroads -> Forest Edge -> North Pass -> Orc Stronghold
   - `road -> trail -> trail -> pass`
   - forest resource opportunity plus explicit pass/chokepoint identity.
3. Human Capital -> Crossroads -> Forest Edge -> Orc Watch -> Orc Stronghold
   - `road -> trail -> trail -> road`
   - woodland flank that rejoins the eastern road network.

This distinction is presentation evidence, not a new movement rule.

## Initial reveal contract

Humans begin with Human Capital and Crossroads visible/explored. The remaining seven founder regions begin unexplored.
The imported representative reveal sequence is the current deterministic traversal evidence, not an alternate fog authority.
## Fidelity review carried into the UE gate

Six founder destinations remain presentation-risk sites even though recipe binding is valid:
- Old Quarry
- River Ford
- Forest Edge
- Ancient Shrine
- Orc Watch
- North Pass

These require visual proof that the tactical transition preserves the campaign-map promise. They are not reasons to alter topology or add new routes.

## Authority

- SoulCore / the integrated overmap contracts own campaign state.
- RB Weather owns weather.
- RB Optimization is the first optimization authority.
- RB Save owns persistence.
- Unreal presentation consumes positions, splines, anchors and state; it does not invent ownership, fog, movement cost or battlefield selection.

## Next gate

When a UE lane is explicitly available, import this package into a bounded nine-region presentation proof. Measure route/anchor readability at strategic camera distance, verify fog state transitions, and exercise the six fidelity-risk transitions before expanding toward the 36-region world.
