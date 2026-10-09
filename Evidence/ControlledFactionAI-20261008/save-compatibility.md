# Save compatibility and isolated qualification

RBSave `Soul.Campaign` schema1 and the existing settlement domain remain unchanged. The retained nine-region slot is not migrated into the 36-region composition. No existing qualification save is overwritten.

Older six-faction saves with `None` for Viking or Nature retain that exact unavailable-roster state. Loading does not silently replace their armies. Such armies may make legal neutral moves, but hostile exact-roster admission rejects them. New isolated fixtures initialize admitted exact units.

Preparation records the current profile, load revision and complete campaign snapshot. Execution rejects stale profiles, loads or state changes before mutation. F5/F9 invokes the existing controller/save path. Separate-process checks compare saved bytes and restored state; camera pose persistence is not claimed.

| Purpose | Slot |
|---|---|
| default | `Soul.VerticalCampaign` |
| composition_founder | `Soul.Composition3500.Founder` |
| composition_human_proof | `Soul.Composition3500.HumanProof` |
| composition_orc_proof | `Soul.Composition3500.OrcProof` |
| composition_six_faction_proof | `Soul.Composition3500.SixFactionProof` |
| composition_viking_proof | `Soul.Composition3500.VikingProof` |
| controlled_dwarves | `Soul.Composition3500.Controlled.dwarves` |
| controlled_orcs | `Soul.Composition3500.Controlled.orcs` |
| controlled_vikings | `Soul.Composition3500.Controlled.vikings` |
| controlled_nature | `Soul.Composition3500.Controlled.nature` |
| controlled_human_nature | `Soul.Composition3500.Controlled.human_nature` |
| controlled_turn | `Soul.Composition3500.ControlledTurn` |
| controlled_dwarves_orcs | `Soul.Composition3500.Controlled.dwarves_orcs` |
| controlled_orcs_dwarves | `Soul.Composition3500.Controlled.orcs_dwarves` |
| controlled_dwarves_vikings | `Soul.Composition3500.Controlled.dwarves_vikings` |
| controlled_vikings_dwarves | `Soul.Composition3500.Controlled.vikings_dwarves` |
| controlled_orcs_vikings | `Soul.Composition3500.Controlled.orcs_vikings` |
| controlled_vikings_orcs | `Soul.Composition3500.Controlled.vikings_orcs` |

Natural result receipts record effective campaign survivors and physical alive/reserve totals separately. Existing morale defeat can zero effective survivors even when bodies remain alive. This mission does not redesign that rule.
