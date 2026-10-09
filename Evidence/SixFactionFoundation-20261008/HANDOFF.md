# Six-faction foundation + Viking infantry — final handoff

## Authority and exact Git boundary

- Worktree: `D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929`
- Branch: `codex/soul-bannerlord-campaign-map-20260929`
- Starting HEAD: `41f604475e94905ff9afc2cb0e55148e0412618b`
- Qualified implementation commit: `14a840e089fa8c9a7a9f6fead4582a5c36db7963` — Add opt-in six-faction state and exact Viking infantry proof.
- Ending HEAD and complete local commit list: [final-head.txt](final-head.txt) and [final-closeout.json](final-closeout.json), written after the evidence commit to avoid a self-referential commit hash. These two receipts are intentionally local.
- Remote remains `4842eeb403f6b9be6effdf9fe18fc5c24b499887`. No push, merge, reset, clean, stash, donor save or map promotion.
- Exact 41 source/data/tool files: [commit-source-scope.json](commit-source-scope.json). Inherited dirty files are excluded and hashed in [preservation-after.json](preservation-after.json). Licensed assets remain local-only.

## Result

**SIX-FACTION CANONICAL STATE: PASS**
**ADVERSARIAL SAVE SAFETY: PASS**
**NEXT FACTION MATCHUP: PASS — Vikings**
**READY FOR CONTROLLED SIX-FACTION AI WORK: NO**

All requested implementation, save, roster, cooked-runtime, readiness and evidence phases are complete, including the optional opposite natural outcome. This closes under the mission's all-phases-complete exception; it does not claim eight hours of active work. Start was 2026-10-08 22:36:18 UTC; the post-commit closeout records actual wall-clock finish and elapsed time.

The 3.5 km composition remains **KEEP**, opt-in. The retained 1.5 km default/reference, 36 IDs, 51 legal pairs, heightfields, road/water geometry and authored cities were not edited. No broad art pass, third city framework or terrain work occurred.

## Six canonical factions through existing authority

`-SoulComposition -SoulSixFactionProof` reads the existing `six_faction_sandbox_candidate` overlay. The canonical start JSON is unchanged: 12 initially owned regions, 24 neutral, six factions, 36 regions and 51 legal pairs. Each faction owns its capital and secondary region.

| Faction | Capital | Secondary / controlled move | Exact infantry |
|---|---|---|---|
| Humans | human_capital | crossroads | human_knight |
| Dwarves | dwarf_hold | dwarf_forge_approach | dwarf_warrior |
| Orcs | orc_camp | orc_war_camp | orc_hammer_warrior |
| Vikings | viking_harbour | viking_forest_track | viking_axe_warrior |
| Nature | nature_treehold | nature_forest_clearing | None — unsupported |
| Dark | dark_fortress | dark_castle_approach | None — unsupported |

The existing founder subsystem remains the campaign adapter and `Soul.Campaign` save provider. SoulCore's existing world/graph/economy rules still own legality, capture, vision, AP and day changes. Human army/economy stays in its existing fields; five other canonical records live in the same subsystem. There is no duplicate Human ledger or second persistence authority. The old two-side enemy ledger is inactive in SixFactionProof.

Each army records exact army/faction/unit IDs, location, troops, AP/day, resources and faction knowledge. Human 45 / others 30, gold 3000 and 3 AP are qualification fixtures, not approved balance. Nature/Dark have real strategic identities but no invented combat substitutions.

I cycles read-only army inspection; Home returns to Humans. The runtime observer drove the actual controller I/F5/F9 path and performed six controlled legal moves through existing rules. Every army retained its identity and spent exactly one AP; neutral territory remained neutral. Non-Human hostile movement is rejected; autonomous strategy AI, diplomacy and recruitment economy remain OFF/unimplemented. Bridge/ford/ferry semantics and terrain adapters were unchanged.

## Save safety

New slot: `Soul.Composition3500.SixFactionProof`. Existing retained/default, Founder, HumanProof and OrcProof slots remain unchanged. Viking battle uses isolated `Soul.Composition3500.VikingProof`. Each runtime run uses a separate evidence UserDir.

Existing `Soul.Campaign` / `Soul.Settlements` domain schemas remain unchanged. SixFactionProof adds a mandatory profile/version, unique canonical region/owner rows and five faction rows to the existing payload. Validation completes in temporary values before assignment. No 9→36 migration exists.

[adversarial-save-cases.json](adversarial-save-cases.json) records 21 corrupt-snapshot cases plus actual retained-save and wrong-profile rejection. Unknown/duplicate/missing IDs, bad ownership, roster substitution, invalid counts/AP/resources and malformed rows reject without changing the valid snapshot. Actual RBSave tests corrupt each domain separately and verify both domains roll back. Distinct faction balances restore exactly. Earlier unbound Viking saves keep `None` and remain battle-ineligible rather than silently gaining the new unit.

Cooked F5→day mutation→F9 restores all six armies/owners/resources exactly. Separate-process restoration passes, and the final binary restores the same six-faction save byte-for-byte. See [state-proof-cooked.json](state-proof-cooked.json), [state-proof-final-binary.json](state-proof-final-binary.json), [save-compatibility.md](save-compatibility.md).

## Viking roster and natural outcomes

Selection took approximately 5.7 minutes using local owned assets, primarily `D:/Unreal Projects/AoEAssetRenderLab/Content`. Ulf had a complete owned humanoid body, axe and compatible full-pose warrior family already available. Nature/Dark required more unqualified animation/weapon work. No download, purchase, donor edit or generative art was used.

- Mesh: `/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SK_Ulf_Full`
- Weapon: `/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SM_Viking_Axe`
- Animations: `/Game/Fantasy_Pack/Animations/1With_Weapon/` — idle, forward/back/left/right run, seated idle, four attacks, hit and death.
- Native tests require own-skeleton compatibility, non-additive full poses, positive duration, exact body/weapon and `hand_r` attachment. Body scale 1, relative facing -90 degrees. Weapon is cosmetic/no-collision; RBCombat still owns contact and damage.
- Initial and reserve Vikings are exact Line-role axe infantry; they cannot fall through to another species or role mesh.
- Field: `/Game/Dragon_graveyard/Level/L_showcase_level`, existing `dragon_pass` recipe at (-4000, -16000, 0) cm. Canonical Mountain Shrine→Snow Pass is a real legal connection. Rocky constrained pass geography fits the encounter; snow/boreal art and a Viking capital are NOT qualified.

| Cooked natural proof | Starting forces | Result / return | Save / fresh load |
|---|---|---|---|
| Ordinary victory | 45 Human / 30 Viking, 15 active per side | 34 / 0 survivors; Snow Pass captured; return to Snow Pass | PASS, exact snapshot and identical save bytes |
| Opposite variant | 3 Human / 30 Viking | 0 / 30 survivors; return to Mountain Shrine | PASS, exact snapshot and identical save bytes |

Neither battle forced a winner or used automatic spell assistance: both record zero magic casts. Victory spawned all 30 exact Viking bodies/axes and four enemy reinforcement waves; 496 accepted RB contact log entries. Defeat used 15 initial exact Viking bodies/axes, no enemy casualties or required reserve wave; 55 accepted contacts. See [viking-victory-result.json](viking-victory-result.json) and [viking-defeat-result.json](viking-defeat-result.json).

Eight close cooked poses were inspected: idle, run, four attacks, hit and death. Correct grip/deformation/facing/proportions observed; tactical texture softness remains. This paused diagnostic is separate from ordinary combat. See [viking-pose-result.json](viking-pose-result.json). The earlier unusable motion-blurred pose run is rejected and retained under Local; it is not acceptance evidence.

## Builds, tests and stage

Fresh SoulEditor, Soul and SoulComposition builds PASS. [final-build-results.json](final-build-results.json) binds them to exact source hashes. 38 native tests PASS: five SixFaction, 19 Vertical campaign/save, ten combat/presentation and two each default/Composition CampaignWorld. The final pose-only C++ addition received another ten-test combat regression and fresh builds of all targets. Relevant tool suites: 9 + 6 + 21 tests PASS. See [final-native-results.json](final-native-results.json), [source-tool-tests-closeout.json](source-tool-tests-closeout.json).

The only compiled change after the ordinary victory/full six-state proof was the explicitly requested pose observer. The final binary was then exercised through cooked poses, natural defeat and both cold loads; unchanged victory/state evidence was retained rather than repeating long proofs. [binary-requalification-scope.json](binary-requalification-scope.json) records the exact delta.

A fresh Viking axe dependency cook PASS supplements the verified base world cook. Final stage receipt: `Local/stage-six-viking-r2/Diagnostics/receipt.json`. Final executable SHA256: `f42eab96854c71936e954e2bfe077bf91e789c8fbd063faf7a6625581c3c8986`.

Stage: `C:/Users/Jeff/AppData/Local/Temp/SoulCompositionStage_e2wn5rqe/Stage/Windows`. This is an isolated local loose stage, not a distribution archive or fresh full-world cook. 10,754 base cooked files remain byte-identical; 16 copied sidecars add six axe packages. Existing base AssetRegistry/shader archives remain unchanged; the axe uses explicit LoadObject dependencies. No R10/Expansion payload or machine token was admitted. Manifest before/after: 12,673 entries present, base hashes and added hashes checked, executable matches fresh build. The external temp-file remover was not diagnosed.

## Limits / preserved state

- No full six-faction military AI: only Human→Dwarf/Orc/Viking is admitted, **3 of 30 ordered cross-faction pairs**. The other 27 reject. Nature/Dark and seven-family rosters remain incomplete.
- Directed approach records remain **20 explicit / 82 unset**. The exercised pass uses the existing fallback; no guessed directional recipe was added.
- The Viking battle-only fixture intentionally leaves Human Capital unowned and logs the existing settlement-development UNREADY warning. It does not qualify city construction or strategic day/recruitment progression. The six-faction sandbox binds Human development normally. No settlement framework was redesigned.
- Existing unresolved modeling-function warning remains in cooked battle. CastleTown/WebBrowser/shader debt was not revisited or claimed repaired. [runtime-warning-audit.json](runtime-warning-audit.json) and [qualification-limits.md](qualification-limits.md) distinguish observed warnings from prior untested ones.
- All rendered work was capped functional testing with the 85 C guard. Peak observed across this mission's functional runs: 67 C. No uncapped performance qualification was repeated; the earlier 85 C failure remains open.
- Existing Dwarf exterior and six-region art debt remain provisional. No map art acceptance is implied.
- Final preservation receipt checks 314 protected files, 54 existing qualification saves, 17 inherited dirty files, 50 Viking donor packages and their 50 Soul copies. Generated cache compression was lossless and hash-verified; nothing was deleted. External cache junctions were skipped.

## Review and next action

[visual-review.html](visual-review.html) contains reviewed six-faction state, battle/return/cold-load and close-pose evidence. Full PNGs/logs/stages remain intentionally local under `Local/`; compact receipts and source are committed. [six-faction-readiness.md](six-faction-readiness.md) / [JSON](six-faction-readiness.json) cover every faction, settlement status and all ordered pairs.

Manual guarded review uses `Tools/ProductionContinuation/play_candidate.py` with this final stage receipt, `--evidence-root Evidence/SixFactionFoundation-20261008`, and exactly one of `--six-proof` or `--viking-proof`. It uses isolated persistent review slots and no automatic inputs. Check stage integrity first if temporary files have aged or disappeared.

Next meaningful action: define the bounded controlled-AI action/admission policy, including explicit behavior for unsupported encounters, and qualify Nature/Dark exact infantry or further directed battle pairs before military AI activation. Approve non-Human economy/recruitment and starting-force balance separately. Do not reopen terrain or authored-city work to do this.
