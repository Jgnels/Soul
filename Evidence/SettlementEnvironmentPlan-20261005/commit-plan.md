# Settlement-only commit isolation plan

Inspected HEAD `11cd71cd20ab936c2f32d28c4c2f4f18c8c290a7` and the current working tree on 2026-10-06 UTC. This is a read-only staging plan: no index, source, branch or commit was changed. The index was empty when inspected. The live Editor build validates the combined working tree, not a future isolated commit tree.

The settlement implementation can be isolated without discarding R10 or Crownstead work. Do not stage the whole current diff. Use explicit complete files where their entire HEAD diff is settlement work, and reconstruct mixed files against HEAD in a temporary index. Preserve the remaining working-tree changes byte-for-byte.

Root's chosen split is two commits after the relevant checks pass. The first is the canonical foundation, authored fixture and its exact cook integration. The second is visit/UI/controls qualification after its runtime pass. Neither requires a historical R10/pivot commit or a clean working tree.

First-commit source allowlist: ScenarioData h/new cpp; SettlementStateSubsystem h/cpp; SettlementBootstrapActor cpp; SettlementBuildingActor cpp; FounderPlaytestStateSubsystem h/cpp; the two new SettlementDevelopment/Presentation native test cpp files. Add the exact JSON recipe, authoring script and qualified generated DA. Its index-only DefaultGame addition is one root and its package script count is182; leave the current working tree at205. Include only foundation/asset/native/build evidence. These source files' complete HEAD diffs are settlement scope.

Second-commit source allowlist: CampaignActor h/cpp, PlayerController h, new VisitGameMode h/cpp, new controls qualifier; exact HUD/PlayerController/GameMode hunks below, plus the package-extractor directory-prefix guard needed by the Visit map-scope literal. Root approved replacing qualifier guards with direct FParse checks; the qualifier owner has applied that change in revision2, now read-only reviewed. Recompute this second patch against the actual first-commit HEAD. Its packaging count is already182 in that HEAD, so it needs no additional cookroot change.

## Evidence and limitations of the baselines

`development-source-before.json` and `visit-ui-source-before.json` record pre-edit hashes and sizes, not source backups. `construction-presentation-fix-receipt.json` additionally retains the original `ApplyIntegrity` body. `preserved-before.json` records the mission-start HEAD and unrelated working-tree hashes. `controls-qualifier-before.json` records the two pre-qualifier GameMode hashes. These prove provenance but cannot recreate arbitrary source bytes by themselves.

Comparing recorded hashes with HEAD bytes, allowing uniform LF/CRLF conversion, confirms the Scenario header, Settlement subsystem header/cpp, Bootstrap cpp, BuildingActor cpp, and CampaignActor cpp started at HEAD. Several header/Founder-state receipt hashes do not match HEAD even under that conversion; their complete current textual HEAD diffs were independently reviewed and contain only this settlement work. Do not interpret raw hash mismatch as proof of an unrelated semantic hunk, or claim these hashes are reconstructable backups.

The actual mixed changes are visible in HUD, PlayerController cpp, GameMode h/cpp, DefaultGame and the packaging tools. No settlement edit requires Terrain, WorldActor, camera, region actor, battle bridge, Soul.Build.cs, DefaultEngine, Soul.uproject or any R10 data file.

## Complete-file allowlist

The complete textual diff from current HEAD is within settlement scope for these existing files:

```
Source/Soul/Public/SoulSettlementScenarioData.h
Source/Soul/Public/SoulSettlementStateSubsystem.h
Source/Soul/Private/SoulSettlementStateSubsystem.cpp
Source/Soul/Private/SoulSettlementBootstrapActor.cpp
Source/Soul/Private/SoulSettlementBuildingActor.cpp
Source/Soul/Public/SoulFounderPlaytestStateSubsystem.h
Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp
Source/Soul/Public/SoulFounderPlaytestCampaignActor.h
Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp
Source/Soul/Public/SoulFounderPlaytestPlayerController.h
```

These new files are wholly settlement implementation/tests:

```
Source/Soul/Private/SoulSettlementScenarioData.cpp
Source/Soul/Public/SoulSettlementVisitGameMode.h
Source/Soul/Private/SoulSettlementVisitGameMode.cpp
Source/Soul/Private/Tests/SoulSettlementDevelopmentTests.cpp
Source/Soul/Private/Tests/SoulSettlementPresentationTests.cpp
```

The nonvisual recipe and reproducible owned data-asset authoring script are also in scope after root's final review:

```
Data/SettlementEnvironments/CrownsteadDevelopmentProof.json
Tools/SettlementEnvironments/author_development_scenario.py
```

The recipe is an authoring input, not a new runtime JSON dependency or saved-state authority. Include the generated `Content/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof.uasset` only if ordinary repository asset policy permits and its authoring/read-back receipt is complete. Otherwise explicitly document its external distribution path plus exact hash; do not call a source-only commit a complete cooked package. Do not force-add ignored donor or proxy folders to carry this one data asset.

## Mixed files: exact included and excluded changes

| File | Include in settlement index version | Keep only in working tree / other commit |
|---|---|---|
| `SoulFounderPlaytestHUD.cpp` | `Engine/World.h`, the three `SoulSettlement*` includes, Visit state selection, Development lambda/visit panel, proof-only campaign panel sizing/gating/build/visit controls, Visit hitbox dispatch | `SoulCampaignTerrain.h`, `SoulPlaytestRegionActor.h`, `StaticMeshComponent.h` additions and the whole World projected-bounds army-label change. Keep the HEAD army-label condition in the settlement index version. |
| `SoulFounderPlaytestPlayerController.cpp` | Visit/World includes needed by the new visit dispatch; U/V bindings; visit routing for day/hire/build/F5/F9/Escape; campaign visit action | World-specific 4,000,000 cm trace-distance ternary. Keep HEAD's `100000.f * SoulCampaignTerrain::Scale()` line in the settlement index version. |
| `SoulFounderPlaytestGameMode.cpp` | Only qualifier flag parsing and the two-line early Tick dispatch, if the qualifier is included and independently usable | Retained-capture guards/dispatch, World camera focus, World fill light, benchmark duration/overview changes. |
| `SoulFounderPlaytestGameMode.h` | Only `TickSettlementDevelopmentQualification` and its development-prefixed fields/bool | `TickWorldCapture`, its view/shot fields and `BenchmarkSampleSeconds`. Existing generic VisualStep/bStarted/ExpectedSnapshot fields are already in HEAD. |
| `Config/DefaultGame.ini` | Exactly `+MapsToCook=(FilePath="/Game/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof")` appended to HEAD config | All prior 23 R10/proxy/verge additions. The settlement-only index has **182** roots (HEAD181 + proof1); the preserved full working tree stays **205**. |
| `Tools/package_soul_weekend.ps1` | For an independent commit directly on HEAD, change the two reviewed cook-count literals **181→182** in its index version | Current205 count, World height assertion and runtime JSON count8→10. Do not stage204→205 literally onto HEAD and claim it is self-contained. |
| `Tools/test_package_soul_weekend.py` | Only the two-line exclusion of directory string literals ending `/` before loadable-package extraction | World JSON additions, count8→10 and World height assertion. Its cookroot-count assertion is already derived from config; no new fixed count is needed. |

The two include additions in PlayerController are `SoulSettlementVisitGameMode.h` and `Engine/World.h`; “World” there means the UE header, not the opt-in terrain profile.

Editor-r1 compiler feedback has now been repaired in the first-commit files: the opt-in load local is `DevelopmentScenario` (avoids shadowing the earlier scenario variable) and SettlementStateSubsystem explicitly includes `Engine/GameInstance.h`. Source18/18 and scoped diff checks pass after these two changes; the rebuilt Editor/native result remains pending. See `development-compile-repair-r1.json` for exact before/after hashes.

## Qualifier dependency repaired before inclusion

The first `SoulSettlementDevelopmentQualification.cpp` revision called `SoulCampaignTerrain::World()`, `ReviewCapture()` and `RetainedReviewCapture()`. These functions are absent from HEAD's Terrain header; `EvilCorridor()` already exists. Staging that original qualifier would have created an unintended dependency on uncommitted R10/retained infrastructure.

The owner applied the approved smallest repair in revision2: direct `FParse::Param` checks preserve the World flag, both World capture aliases, and retained capture semantics; the actual EvilCorridor() check remains. Read-only review confirms those three missing-HEAD function references are gone. Do not include Terrain changes as a qualifier dependency. The qualifier still belongs in the second commit, after its own controls runtime pass.

Read-only qualifier review also found one concrete panel-sequencing issue: EndDay closes the town; the original case11 T opened it immediately before case12 expected closed. Revision2 now asserts the automatic close, reopens on a later step, and waits for the rendered Hire hitbox. Mid/completed screenshots now occur after reopening/rendering; completed screenshot existence is checked before a later-step F9. Duplicate build/hire tests assert the handler's rejection message in addition to unchanged snapshots, so a no-op click cannot pass. The owner refreshed its source receipt; the actual controls run remains pending.

## Proposed safe execution, only after final qualification

1. Recheck HEAD is still the recorded commit, inspect `git diff --cached --name-only`, and preserve current source/config hashes. If the index is not empty, stop and reconcile ownership; do not clear it. Inspect final native/build/runtime receipts, separating controls-only PASS from missing authored environment, miniature and battle-environment proof.
2. Create a temporary index under this task's ignored Local evidence directory, initialize it from the current HEAD, and stage only the explicit complete-file allowlist into that temporary index. This does not change the real index or working tree. Do not use repository-wide add or directory globs.
3. Materialize HEAD versions of each mixed file in Local, apply only the semantic changes listed above, and review their complete text diff against HEAD. Apply the resulting patch with `git apply --cached --check` and then `git apply --cached` against the temporary index. The index config/count are182 while the working tree remains205. No checkout/reset of the working tree is necessary.
4. Review temporary staged filenames, complete diff and `git diff --cached --check`. Search the staged diff for excluded R10/World terrain/capture/camera/cook/data additions. Validate that native tests and new Visit/Scenario source resolve entirely against the staged tree; the qualifier caveat above must be resolved or omitted. A build of the combined live tree is not independent commit-tree build evidence. Run the necessary checks on a materialized candidate tree later under root's single-heavy-process rule.
5. Export the reviewed temporary staged patch and apply it to the real index only after a fresh empty-index/HEAD check. Review the real staged diff again before root commits. Immediately verify protected machine/support hashes and that the uncommitted R10/pivot remainder remains present. Do not overwrite the working tree with the staged182-root version after commit; the remaining182→205 working-tree diff is intentional.

If root chooses a prior separate infrastructure-preservation commit, recalculate all counts and hunk boundaries against that new HEAD. Do not reuse a patch based on `11cd71cd` after HEAD changes. The original one-line Package.h fix is already committed and must not be reconstructed or recommitted.

## Evidence allowlist and exclusions

Good settlement evidence to stage individually after current build/runtime completion: `development-architecture-audit.md`, `first-proof-plan.md`, `development-source-before.json`, `visit-ui-source-before.json`, `construction-presentation-fix-receipt.json`, `development-cook-receipt.json`, `development-source-receipt.json`, `canonical-development-source-review.md/json`, this `commit-plan.md`, final scenario-authoring receipt, qualifier before/final receipts, and concise new native/build/controls result receipts. Keep claims scoped to what passed. Shared environment direction/catalog documents can be a separate architecture evidence commit after their owner's review.

Exclude `.mcp.json`, DefaultEngine, Soul.uproject, plugin/machine configuration, large Local evidence, binaries/DLLs, ignored vendor/proxy payload, all R10 World terrain assets/data, previous pivot evidence, unrelated support files and whole Evidence-directory staging. No secrets were read to prepare this plan. The preserved R10 work remains an explicit separate research/infrastructure change, with its visual rejection retained.
