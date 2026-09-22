# Soul UE Overmap Acceptance Fixtures — 2026-09-22

These are machine-readable future UE acceptance scenarios built from the current strategic contracts. They are not a second campaign ruleset.

## ue.fixture.founder_forest_contact

Exercise Human Capital -> forest corridor -> hostile Orc Watch contact without a generic-map fallback.

Pass conditions:
- The campaign pawn reaches Orc Watch through the authored road/trail graph with no teleport or disconnected spline.
- Entering hostile Orc Watch launches the orc.badlands context from the north-west entry.
- The tactical launch consumes the strategic handoff rather than choosing an unrelated generic arena.

## ue.fixture.human_capital_east_assault

Verify a walled major settlement approached from the east preserves its settlement/siege context.

Pass conditions:
- The east approach resolves to the Human Capital settlement rather than a field-only encounter.
- The settlement silhouette/gate context is visible from the east approach.
- The battle launch carries settlement/siege state fields and human.fortress_outskirts context.

## ue.fixture.broken_bridge_chokepoint

Verify the Orc-to-south broken bridge reads and launches as a ravine chokepoint, not generic badlands.

Pass conditions:
- The strategic route visibly converges on the broken-bridge chokepoint.
- The destination retains ravine/broken-bridge metadata.
- A contested entry selects orc.broken_bridge and the north entry direction.

## ue.fixture.dragon_graveyard_special_site

Exercise explicit Dragon Graveyard activation inside Orc Badlands without adding or rewiring a strategic graph node.

Pass conditions:
- Selecting Dragon Graveyard does not create, delete, or reconnect an overmap node.
- The tactical launch reports dragon_graveyard as the explicit special-site override.
- The strategic approach direction from Orc Camp remains west-facing context in the battle handoff.
- If the Dragon Graveyard battle payload is unavailable, fail explicitly instead of silently falling back to orc.badlands.

## ue.fixture.stronghold_reinforcement_retreat

Verify bypass/reinforcement/retreat timing uses actual adjacent regions and distinct strategic entry directions.

Pass conditions:
- An intact Orc Watch reserve is considered only through the real Orc Watch -> Stronghold road and timing rule.
- Primary North Pass attackers and Orc Watch reinforcements use distinct entry directions.
- Retreat to North Pass uses the reverse valid pass edge; no arbitrary destination teleport is allowed.

## ue.fixture.river_ford_context

Ensure the campaign river crossing selects a river-road battlefield context instead of a generic arena.

Pass conditions:
- River Ford is visually and logically a crossing before battle commitment.
- The launch selects human.river_road with the north-west approach.
- Until the crop is qualified, the fixture reports UE_CROP_REQUIRED rather than claiming presentation acceptance.

## Known gates

- River Ford is intentionally expected to report UE_CROP_REQUIRED until its battlefield crop is qualified.
- Dragon Graveyard requires the separate proof lane to provide the actual special-site battlefield payload/binding; this fixture defines how that payload must connect to campaign geography.
- Orc Stronghold remains UE_COMPOSITE; fixture acceptance must distinguish metadata correctness from final art qualification.
