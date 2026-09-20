# Soul engineering authority

## Product
Soul is a PC-first Unreal Engine 5.8 turn-based fantasy strategy game with a living 3D adventure map, hidden-hex tactical battles, persistent commanders, regiment veterancy, faction armies, sieges, and high-impact magic.

## Tool hierarchy
Use existing systems in this order:
1. RefinedBadger / RB Foundation products.
2. Hyper tools where installed and materially useful.
3. Other qualified Studio Full tools.
4. Project-owned bespoke code only when no earlier layer owns the required domain.

Do not create a second authority for Save, Item Economy, Routine, Weather, Optimization, or an installed RB domain.

## Donor policy
Living Strategy is a PROJECT_OWNED donor/reference implementation. Port mechanics and tests selectively; do not embed Godot or GDScript runtime dependencies.
Mosaic is REFERENCE_ONLY unless the founder explicitly admits a bounded component. Do not copy Mosaic source into Soul.
Proprietary Heroes/Total War material is REFERENCE_ONLY: mechanics research only, never content/code/assets.

## Architecture
Canonical rules must be deterministic and presentation-independent.
Unreal Actors, animation, VFX, UI, physics, and camera present canonical outcomes; they do not invent them.
Use integer/fixed-scale values where practical, explicit RNG streams, stable tie-breaks, and stable content IDs.
Heroes/commanders may have subjective memory. Ordinary unit groups have veterancy/history, not individual psychological memory.

## Current mechanics direction
- turn-based tactical combat;
- hidden axial hex grid;
- initiative timeline rather than side-wide turns;
- wait, defend, retaliation, bounded morale/luck;
- 1-hex normal groups, multi-hex large creatures;
- flying as a movement capability, not immunity;
- seven core unit families per faction, heroes separate;
- finite recruitment/growth pools;
- strategic day/action commitment;
- layered sieges prepared before combat;
- adventure-map geography must causally select battlefield recipes.

## Environment interaction budget
Default to **interactive space, non-interactive clutter**.
Preserve tactical architecture, traversal, persistent structures, siege objectives and gameplay-relevant NPCs as separate interactive entities.
Merge/instance/bake decorative prop clusters instead of simulating individual furniture/tableware/shelf contents.
Taverns may host physically present recruitable heroes/companions; those NPCs remain interactive while surrounding clutter stays static unless a prop has explicit gameplay purpose.
Use RB Optimization before bespoke representation/culling systems.

