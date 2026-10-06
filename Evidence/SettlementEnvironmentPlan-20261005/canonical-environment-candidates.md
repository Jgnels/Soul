# Existing settlement candidate bindings

**2026-10-06 continuation:** the selected Human source is complete at `D:/RefinedBadger/AssetLibraries/MedievalKingdom-42d4a792`. Its owned wrapper is `/Game/Soul/Maps/Settlements/L_HumanCapital_Authored`; full rendered/state qualification remains pending because of editor memory pressure. Dwarf Hold now has a functional authored wrapper at `/Game/Soul/Maps/Settlements/L_DwarfHold_Authored`, verified construction/visit/save and a corrected-approach natural victory with real reserves on both sides. Its temporary cutaway miniature and geography are not final capital art. See `Continuation-20261006/dwarven-proof-review.md` and `human-wrapper-progress.md`. No strategic assignment is changed.

Six major city IDs and eight existing noncanonical minor slots are preserved. These are proposed environment candidates except the explicitly opt-in Dwarf proof. Human physical groups remain unbound; the Dwarf Caravan Hall has 56 qualified persistent roots. Existing logical building IDs are copied separately into the JSON.

| Region ID | Existing settlement ID | Candidate source | Actual candidate map | Status |
|---|---|---|---|---|
| `dark_fortress` | `city.dark_fortress` | AC | `/Game/AlienPlanet/Levels/L_Showcase` | Jeff-selected capital; local complete package paths available; actual fortress/approach qualification pending |
| `dwarf_hold` | `city.dwarf_hold` | DC, LF | `/Game/DwarvenCitadel/Maps/DwarvenCitadel` | Full Citadel wrapper qualified for development, visit, save and real battle; temporary proof geography and miniature art remain provisional. LF keeps its separate forge role. |
| `human_capital` | `city.human_capital` | MK | `/Game/CastleTown/Levels/Persistant/PL_CastleTown` | Jeff-selected complete source supplied; owned wrapper created; full editor preparation memory-bound. No recovery/download task. |
| `nature_treehold` | `city.nature_treehold` | FF | `/Game/Forest_village/Level/L_showcase_level` | Town source available; promotion to full Nature capital is a candidate, not explicit Jeff capital assignment. Great-tree requirement unresolved. |
| `orc_camp` | `city.orc_ruinhold` | RH, CD | `/Game/Ravenhold/Scenes/HM-FortCastle_Kit_Demo` | Candidate occupied fortress/ruin; preserve authored layout and existing city.orc_ruinhold identity; style/layout not yet accepted |
| `viking_harbour` | `city.viking_harbour` | VK | Unverified / not selected | Jeff-selected capital payload missing; historical MainVillage path is not currently a live map |
| `coastal_ruins` | None (existing candidate only) | Retained local base | Unverified / not selected | Retain existing approved Coastal Ruins setting; no new listing substituted |
| `crossroads` | None (existing candidate only) | FV | `/Game/Medieval_Fantasy_Village/Levels/L_Medieval_Town` | Existing noncanonical market-town slot; use a coherent authored subset only after inspection |
| `dark_castle_approach` | None (existing candidate only) | AC | `/Game/AlienPlanet/BluePrints/LI_Entrance` | Existing ward-bastion candidate; entrance scene needs actual bounds and route inspection |
| `dwarf_forge_approach` | None (existing candidate only) | LF | `/Game/Legendary_Forge/Maps/L_showcase_forge` | Existing forge-outpost candidate; bounded authored outer-works subset rather than whole duplicate city |
| `nature_forest_clearing` | None (existing candidate only) | FF | `/Game/Forest_village/Level/L_showcase_level` | Existing grove-village candidate; retain coherent authored clearing/subset |
| `northwest_march` | None (existing candidate only) | FV, VT | Unverified / not selected | Existing neutral trade-post candidate; neither harbor nor new city ID is implied |
| `orc_war_camp` | None (existing candidate only) | WC, WZ | `/Game/WarCamp_Collection/Maps/Overview/Camp_Garrison_War_Collection` | Existing camp candidate; actual native tent/prop group to be selected |
| `viking_forest_track` | None (existing candidate only) | VT, VK | Unverified / not selected | Existing village candidate; exact selected Viking local payloads missing |

No new harbor/city slot is created to accommodate a pack. TS, DO, LV, AV and GT remain available faction-family coverage until an existing suitable node/slot is explicitly chosen. This preserves the current strategic scope.

[canonical-environment-candidates.json](canonical-environment-candidates.json) also records all 36 existing biome/landform/feature/battle-recipe contexts, exact source hashes and the 51-edge source authority. It does not rewrite any recipe. The old recipe donor strings are kept as historical inputs; a capital battle scene for the new MK proof must be explicitly qualified against the existing battle bridge.

Continuation closeout (2026-10-06): Complete supplied source; r5 fully streamed 56,443 actors / 161 levels with four reviewed captures. Owned waterfront repair verified; some castle parapets remain gray. Physical development, miniature and visit/battle routing remain unbound. No further source recovery/download. See [final Human review](Continuation-20261006/human-runtime-r5-review.json), [visual comparison](Continuation-20261006/visual-evidence.html), and [handoff](HANDOFF.md). Earlier source/runtime blockers above are historical.
