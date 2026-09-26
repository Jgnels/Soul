# Soul Founder Runtime Candidate Drift Audit — 2026-09-22

Status: **read-only audit of an untracked dirty-primary prototype; no candidate files modified.**

- Candidate regions: 9 / accepted 9.
- Candidate edges: 10 / accepted 10.
- Region metadata drift items: 4.
- Edge drift items: 0.

## Findings

- **candidate_is_uncommitted (boundary)** — Founder playtest runtime is untracked in the dirty primary and has no committed Git authority.
- **topology_matches_current_founder_ids (pass)** — The candidate currently uses the same nine region IDs.
- **edge_graph_matches (pass)** — Adjacency/road flags should converge on the accepted graph rather than remain separately hardcoded.
- **region_metadata_matches (medium)** — The candidate has region biome/landform/feature/owner drift from accepted overmap data.
- **hostile_nonsettlement_entry_gated_before_occupation (critical)** — Current candidate can move the player into an Orc-owned non-settlement such as Orc Watch before battle. Accepted start-state policy requires hostile entry to resolve encounter/battle before occupation or traversal.
- **battle_recipe_uses_directed_handoff (high)** — StartBattle opens one hardcoded Orc Badlands overlay instead of consuming the directed battle handoff. The accepted Stronghold destination currently resolves to orc.war_camp.
- **battle_victory_returns_to_actual_region (critical)** — Every candidate battle victory currently awards/captures orc_camp. A battle launched from Orc Watch can therefore resolve the wrong strategic region.
- **neutral_rewards_are_data_driven (decision_required)** — The candidate grants the same first-capture gold/XP reward to generic neutral regions. Accepted overmap data distinguishes resource, landmark, crossing and pass roles and does not authorize a universal reward.
- **enemy_force_model_matches_reinforcement_analysis (model_gap)** — The current candidate models one movable Orc field army, not an Orc Watch reserve plus Stronghold force. Reserve strength in the balance lab remains synthetic sensitivity glue and must not be mistaken for runtime state.

## Safe convergence order

1. Preserve the candidate files; do not reset or overwrite the dirty primary.
2. Use accepted overmap/import data for region IDs, positions, adjacency, metadata and directed battle context.
3. Gate hostile non-settlement entry before changing PlayerRegion or ownership.
4. Carry actual source/destination/approach/handoff into battle launch and return.
5. Remove hardcoded orc_camp victory attribution; persist the battle's actual strategic destination.
6. Keep reserve/garrison numbers outside runtime until a founder-approved force/reaction rule exists.
7. Only then decide explicit rewards for resource sites, landmarks and neutral captures.
