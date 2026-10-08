# Six-faction configuration groundwork

This is a derived readiness check, not an activated campaign scenario. The existing `six_faction_sandbox_candidate` remains the only starting-position source. All 36 IDs, 51 edges, 12 owned starts and 24 neutral regions are preserved. No balance, diplomacy, ownership or runtime fixture was changed.

| Faction | Canonical capital | Secondary | Physical binding | Declared frontier mismatch |
|---|---|---|---|---|
| humans | `human_capital` | `crossroads` | Present | None |
| vikings | `viking_harbour` | `viking_forest_track` | Present | None |
| dwarves | `dwarf_hold` | `dwarf_forge_approach` | Present | None |
| orcs | `orc_camp` | `orc_war_camp` | Present | None |
| nature | `nature_treehold` | `nature_forest_clearing` | Present | None |
| dark | `dark_fortress` | `dark_castle_approach` | Present | None |

## Existing authority to extend

- `FSoulWorldState` / `FSoulWorldRules`: arbitrary faction ownership, legal adjacency, visibility and capture already exist. Keep the physical presentation separate.
- `FSoulCampaignBattleDescriptor::IsValid` currently admits exactly Humans / `human_knight` against Dwarves / `dwarf_warrior`. This is an intentional physical-runtime identity gate. Six-faction activation needs verified roster/presentation support through this existing bridge; deleting the whitelist alone would permit incorrect side-based assets.
- `FSoulFactionDefinition` / `FSoulFactionRules::Validate`: reuse faction roster/hero/settlement definitions. Do not infer a gameplay faction from terrain color.
- `FSoulStrategyAI::Choose`: existing deterministic candidate scoring accepts `ActingFactionId`, acting army/commander, visible enemy armies and strategic regions. The founder runtime still needs an explicit scheduler/state binding; a second strategic AI system is unnecessary.
- `FSoulCampaignRules`: preserve finite recruitment pools, action spending and day advancement.
- `USoulFounderPlaytestStateSubsystem`: current founder adapter intentionally remains a two-side fixture. Its owner conversion (`orcs` to the fixture enemy), one `EnemyUnitId`, static garrison counts, one-unit army restore shape and Human-specific recruitment must be generalized deliberately before activating this scenario.
- `USoulSettlementStateSubsystem` and RBSave remain the settlement and persistence authorities. The two authored-city proofs are frozen.

## Smallest next playable increment

After separate product approval, admit the six existing faction IDs and immutable roster bindings into the current campaign adapter; first qualify owner/roster-correct encounters and save rejection/roundtrip without enabling autonomous AI. Then bind one deterministic AI turn through `FSoulStrategyAI::Choose`. Keep pending starting-army quantities and diplomacy explicit. Do not activate the JSON by merely relaxing owner validation: that would disguise every opponent as the current single enemy unit type.

The existing authored environment role matrix remains authoritative. Human and Dwarf authored proof environments are qualified; Viking source availability and Nature/Orc capital assignments still need bounded content work in a later mission. No third city proof was started here.

See `six-faction-readiness.json` for exact canonical hashes, coordinates, frontier checks and blockers. See `save-compatibility.md` for isolated current slots and the no-silent-migration decision.

## Configuration admission boundary

The reusable roster contract requires seven families (four fighter, one ranged, one support/magic, one apex), exactly one quadruped body plan, and separate heroes. The current native faction test uses abstract fixture IDs, not six finished production rosters. Only the Human and Dwarf development data assets are present under `Content/Soul/Data/Settlements`; this is not evidence that all six recruitment/development definitions are ready. Historical city-design prose is not used to invent current unit IDs or override the later approved authored-environment assignments.

The next bounded configuration increment should supply explicit canonical faction-to-roster and roster-to-owned-presentation bindings, preserve the existing unsupported-matchup rejection, and qualify one newly admitted matchup before activating multi-faction AI. Starting-army numbers remain Balance Lab-owned and diplomacy remains unset.

## Directed context and cooked admission

`six-faction-admission.md` / `.json` now resolve all 102 directed canonical connections against existing recipe metadata, the 20 founder approach records, approved six-seat assignments and actual cooked map presence. The remaining 82 unset approach directions are explicit content debt; no values were invented and no new faction activated.
