# Controlled action admission

Full autonomous strategy is OFF. A caller supplies faction, exact army, expected source and canonical destination. Preparation and execution use the existing Soul state subsystem, SoulCore graph/AP/capture rules and campaign battle bridge.

1. Reject off-game-thread preparation/execution before reading live state. Require initialized isolated six-faction state, no persistence operation or pending battle.
2. Validate canonical faction, exact living owned army, expected source, valid source/destination and canonical destination owner.
3. Require ownership of the source and an existing legal edge. Test the normal AP spend against a copy. Existing Human Adventure skill movement discounts remain normal authority, not an AI exemption.
4. For hostile destinations, require a living defending army at that region, an explicit ordered exact-roster pair, an existing playable battlefield recipe matching the region biome, landform or feature, and an available existing bridge.
5. Preparation captures the current campaign save-domain snapshot, save-profile identity and load revision without mutation.
6. Execution repeats admission and rejects any changed profile, load revision or campaign snapshot before spending AP, capture or encounter creation. Proposals cannot be replayed after an action.
7. Neutral/owned movement delegates to existing MoveFactionArmy. Human hostile execution delegates to BeginBattle. Non-Human hostile execution prepares the normal AP spend on a copy, admits the existing bridge, then commits AP and pending descriptor together on the game thread.

Explicit rejection reasons are returned. No save is written by a rejected proposal. Battle winners, damage, morale and reserve production remain existing RBCombat authority. Results map attacker and defender onto their actual faction armies; the Human founder state is not temporarily relabeled.

Qualification uses supplied deterministic actions, not personalities, diplomacy, recruitment optimization or autonomous target scoring. The battle observer stages each army through legal moves and ordinary day advancement. It never teleports armies or assigns ownership directly. Automated battles currently control attacker side; interactive Human defensive-side control is not claimed by this milestone.

Admitted pair count and runtime evidence are reported separately in ordered-pair-matrix.json. Pair admission alone never guarantees that an arbitrary target has a legal edge, living defender or geographic recipe.
