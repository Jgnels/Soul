# Soul Founder Runtime Test Gaps — 2026-09-22

Status: **read-only audit; dirty-primary tests were not modified.**

- Existing automation tests found: 1.
- Existing TestTrue/TestEqual assertions: 16.
- Required convergence behaviors reviewed: 6.
- Covered now: 0.
- Missing: 6.
- Critical missing: 2.

The existing test explicitly calls AwardBattleVictory(orc_camp, 300). That proves the isolated award API, but it cannot catch the current battle actor bug where every victory is attributed to Orc Stronghold.

## Missing coverage

- **hostile_nonsettlement_entry_blocks_occupation (critical)** — Attempting Forest Edge -> Orc Watch must enter battle commitment without setting PlayerRegion or changing Orc Watch ownership before victory. Fixture: ue.fixture.founder_forest_contact.
- **battle_result_applies_actual_destination (critical)** — A victory launched for Orc Watch must resolve/capture Orc Watch; battle result code must not hardcode orc_camp. Fixture: ue.fixture.founder_forest_contact.
- **directed_handoff_selects_recipe (high)** — Battle launch must carry directed approach/handoff and verify Orc Watch selects orc.badlands while Stronghold selects orc.war_camp. Fixture: ue.fixture.founder_forest_contact.
- **settlement_assault_carries_siege_context (high)** — Hostile Human Capital entry must preserve settlement ID, approach direction, and siege-context fields. Fixture: ue.fixture.human_capital_east_assault.
- **stronghold_reinforcement_uses_real_adjacent_route (decision_blocked)** — Any Orc Watch reinforcement must be tied to the actual Orc Watch -> Orc Stronghold edge and timing; no duplicated reserve should appear. Fixture: ue.fixture.stronghold_reinforcement_retreat.
- **river_ford_recipe_gate_is_explicit (medium)** — River Ford must report human.river_road and UE_CROP_REQUIRED until the crop is qualified. Fixture: ue.fixture.river_ford_context.
