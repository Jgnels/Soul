# Clean Foundation Admission + next faction proof

**CLEAN COMPOSITION SOURCE: PASS**
**NEW FACTION MATCHUP: PASS**
**WORLD FOUNDATION: KEEP — opt-in, not promoted**
**READY TO BEGIN BROADER SIX-FACTION IMPLEMENTATION: YES — bounded implementation, not full runtime activation**

## Exact boundary

Worktree: `D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929`
Branch: `codex/soul-bannerlord-campaign-map-20260929`
Starting HEAD: `10aa400d75c6a4bf5db2d7f403122a6e3e17b323`
Clean-source admission commit: `94773d34c535971088548a7ec74c12057e18f1c8`
Ending HEAD and complete local commit list: **`final-head.txt` / `final-closeout.json`**, written after commits to avoid a self-referential commit hash.
Remote remains `4842eeb403f6b9be6effdf9fe18fc5c24b499887`. Nothing pushed or merged.

Candidate `/Game/SoulCampaignComposition/L_Composition_3500_r2` remains 3.5 × 3.5 km and opt-in. Default/reference remains `/Game/SoulCampaignMountain/L_evil_waterfront`. Frozen heightfield, 36 IDs/anchors, 51 legal endpoint pairs and route geometry remain unchanged. No terrain, roads, Dwarf exterior, authored-city framework, balance or six-faction AI work was reopened.

## Clean admitted source — proven

The previously qualified Composition loader, native height sampling, dense route use, ferry presentation, camera bounds, selection/movement, settlement transforms and existing save/battle hooks are now reviewed source rather than an unexplained dirty binary. The mixed terrain diff was reduced from 828 changed lines to 122. Default retained behavior uses the tracked implementation.

`admitted-source-scope.json` lists the first 16 source/test files and nine supporting tools. `orc-source-scope.json` lists the exact second increment. `clean-git-replay.json` proves all 157 first-phase compiled source inputs match the admission commit (only Git LF versus working CRLF normalization). The Orc source manifest and final Git replay establish the same boundary for the latest binary. No hidden source patch is required.

Three excluded inherited units remain byte-preserved outside compilation: `SoulCampaignExpansion.cpp`, `SoulCampaignExpansion.h`, `SoulCampaignWorldCapture.cpp`, under `Local/Excluded/Source/...` and their original `Local/Before` backups. Mixed R10 source branches/data dependencies were not admitted. One old retained-map broadleaf mesh path already present at starting HEAD remains for default compatibility; it is not an experimental world loader.

Existing licensed/local assets, installed UE/RB toolchain and preserved local mount/config support remain required. This is source reproducibility with those dependencies, not a license-free distribution or a claim of bit-identical PE timestamps.

## Fresh builds and regressions

Fresh **SoulEditor, Soul and SoulComposition: PASS**, both after separation and after the exact Orc increment. Latest binary hashes and invocation receipts: `fresh-builds-orc-r1.json`. All 157 compiled source inputs remain unchanged. `link-boundary-orc-r1.json` excludes the inactive experimental objects. `default-target-dependency-isolation.json` confirms regular Soul received no candidate-only/experimental loose payloads.

Native default CampaignWorld **2/2**, Composition CampaignWorld **2/2**, exact Orc assets **1/1**, Vertical combat/save suite **19/19 PASS**. The Vertical runner initially expected 15, although all 19 source-declared tests succeeded; `native-vertical-count-review.json` records that count mismatch without rewriting the raw receipt or rerunning passing tests.

Source-view **8/8**, admission **6/6**, continuation **6/6**, input contracts **3/3**, clean guarded runner **12/12**, cook-profile guards **6/6**, manual launch boundary **7/7**, staged-copy preservation **1/1 PASS**. Syntax and scoped diff checks pass. Relevant original logs remain in Local.

The clean first-phase cooked binary passed mouse/keyboard/controller code-path input, camera, selection, recruitment, F5/F9, separate-process exact restoration, 40 legal moves across 31 distinct edges/28 regions with six ferry legs, bridge and River Ford presentation. Controller hardware was not manually exercised. This is not exhaustive runtime traversal of all 51 edges; their engineering gate and data are preserved.

Human authored proof passed native tavern starting/completed state, matching miniature, construction/service, two-domain restoration, visit/revisit, real city battle, reinforcements, natural victory (25/0), return and F5/F9. It was not expensively rerun after the Orc-only branch because the existing fixture behavior is unchanged and native default/Composition/Vertical regressions passed.

## New Orc matchup — proven

Selection used the primary Soul library first and took about five minutes. Exact identity: **`orcs` / `orc_hammer_warrior`**, against the existing Human player pair. All 21 source-family packages remain byte-identical between AoEAssetRenderLab and Soul; no acquisition or donor changes.

Owned mesh `/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SK_Orc_Hummer`; hammer `SM_Hummer`; native idle/run, three attack clips, hit and death from the same family. Native tests verify skeleton compatibility, full-pose clips, CAT palm attachment and existing RB weapon profile. The new pair bypasses the old mixed Evil presentation branch; every initial/reserve enemy is the correct hammer infantry. Existing fixtures are unchanged. Dedicated side/back clips are absent, so the owned run clip remains a provisional stance fallback.

Environment: `/Game/Dragon_graveyard/Level/L_showcase_level`, existing `dragon_watch` hostile mesa-field recipe at Orc Watch. This is a suitable existing field proof, not a universal faction battlefield or new authored capital.

**Ordinary no-spell run:** 45 Human / 30 Orc strategic bodies, 15 active per side; all 30 Orc bodies verified; 662 accepted RBCombat damage events; six allied and four hostile reinforcement waves. Natural defeat: **0 allied / 7 hostile**, return to **River Ford**. Automatic post-result RBSave, controller F9 and exact separate-process F9 restoration PASS. F5 input itself was qualified in the clean Composition input run.

**Separate rendered vertical-observer run:** existing `SoulVerticalQualification` also supplies five ordinary spell actions. Natural victory: **20 allied / 0 hostile**, return to **Orc Watch**, mana 18. Exact in-process and separate-process restoration PASS. No forced winner, damage override or new battle authority. These two runs are distinct; the rendered observer is not presented as ordinary no-spell play.

`orc-matchup-result.json` and `orc-visual-evidence.json` contain exact receipts/capture hashes. This qualifies **one Orc infantry unit**, not seven families or a balanced faction army.

## Cook, staging and review

Fresh code was staged against the existing verified unchanged isolated 216-root cook. No new asset cook is claimed. Nine direct Orc roots were already cooked. The profile admits only one new loose Orc fixture and one isolated save slot; guards reject any changed cook roots/default policy. Default target/map remains unchanged.

Stage: `C:/Users/Jeff/AppData/Local/Temp/SoulCompositionStage_odmtlcab/Stage/Windows`. Receipt: `Local/stage-orc-r1/Diagnostics/receipt.json`. This is a local loose hard-linked qualification stage, not a store-ready archive.

Post-run review caught missing old-timestamp copied resources after initial stage verification. Exact UAT-manifest copies were restored; no existing donor or linked cooked bytes changed. New staging code refreshes only standalone copy timestamps and skips hardlinks. The missing-file pattern is consistent with temp aging, but the remover was not identified. **All 12,655 UAT manifest files remained present after the repaired-stage victory cold load.** `stage-post-runtime-integrity.json` and repair receipts retain the discrepancy and bounded fix. Treat this as verified current-stage integrity, not a claim that external cleanup was diagnosed completely.

The existing manual launcher now requires an explicit stage, validates all recorded UAT resources plus its binary/fixture/94 candidate cooked assets, and isolates Founder/HumanProof/OrcProof user directories. `Local/manual-orc-launch-plan.json` is a validated dry-run; no manual play session was silently launched.

Known nonblocking warnings remain: three CastleTown algae shader fallbacks, invalid shader maps, unresolved modeling function mount, missing WebBrowser materials. The modeling function is cooked, but its experimental plugin includes runtime modules; blindly enabling it was rejected as an unproven fix. See `cooked-warning-triage.json`.

## Saves and broader factions

Unchanged slots: `Soul.VerticalCampaign`, `Soul.Composition3500.Founder`, `Soul.Composition3500.HumanProof`. New isolated qualification slot: `Soul.Composition3500.OrcProof`. Existing RBSave schemas/domains remain; no nine-to-36 migration. Region selection reframes after restore; arbitrary camera-pose persistence is not claimed.

Six-faction readiness now records both admitted ordered matchups, actual Orc support and the unsupported Viking/Nature/Dark bindings. All six starting faction IDs/possessions and approved environment assignments are preserved. Of 102 directed connections, 20 have existing approaches and **82 remain null**. No forces, diplomacy or approaches were invented; no six-faction AI activated. See `six-faction-groundwork.md`, readiness/admission JSON and `save-compatibility.md`.

## Preservation and remaining limits

`preservation-final.json`: **314 protected files unchanged**, **21 Orc donor/local packages unchanged and identical**, **17 excluded inherited tracked files unchanged**, compiled inputs unchanged. Exact hashes include the frozen 3.5 km heightfield/candidate, retained map/height/presentation, Mountain05/FreshCan/Mesa and Human/Dwarf authored environments. Licensed payloads, machine configuration, original dirty drafts, saves and large Local evidence remain intentionally local-only. No reset, clean, stash, donor save, push or merge.

Performance remains **NOT QUALIFIED / previous 85 C failure open**. No uncapped test was repeated. Capped functional peaks: clean Human 84 C; ordinary Orc battle 73 C; rendered Orc battle 71 C; Orc cold loads 64 C and 54 C. These do not establish 40+ FPS or sustained thermal acceptance; the 85 C guard is unchanged.

Art remains provisional: Dwarf exterior, Crownspine road surfaces, Viking settlement presentation, Orc Mesa transition, Nature banks/scatter, Dark fortress/causeway and ground tiling. This mission does not claim art completion or full six-faction gameplay.

## Review and exact next action

Open **`visual-review.html`** for the new Orc proof, and **`clean-source-review.html`** for the fresh clean-source campaign/Human proof. Raw logs, screenshots, stage receipts, backups and saves are under Local; compact receipts are alongside this handoff.

Next: a separately scoped canonical multi-faction ownership/army/save-adapter increment using existing SoulCore rules, initially without autonomous six-faction AI. Keep exact roster rejection, admit actual owned unit definitions, and add adversarial cross-faction restoration tests before activation. Do not reopen terrain, Dwarf presentation or authored-city framework work as a prerequisite.
