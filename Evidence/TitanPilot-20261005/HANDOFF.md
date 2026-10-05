# Soul x Titan: exercised Clifftop pilot

2026-10-05: **PASS_CLIFFTOP_PACKAGED_TRAVERSAL_PILOT**. Clifftop loads and renders in a Soul-owned world, with exercised Soul spawn/input/navigation and RBSave restoration, in both editor-hosted and locally packaged Windows game. This supersedes the initial blocked preflight. It does not qualify Sulfur or the Dragon Graveyard campaign/battle roundtrip.

## Location and checkpoints

- Worktree: `D:\RefinedBadger\Worktrees\Soul-titan-pilot-20261005`
- Branch: `codex/soul-titan-pilot-20261005`; base `6d53714559d34a123b5394206866925a165373c4`.
- Pilot map: `/Game/Soul/Maps/Soul_TitanPilot`.
- Integrated donor: `/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid` only.
- Earlier checkpoints: `f471f48` preflight/tooling; `6fbc551` strict external-actor class gate and initial transfer; `db28f2f` initial pilot; `0c7e7c5` complete serialized instance material closure. Functional checkpoint: `cef33b8` (grounded terrace traversal, streaming spawn readiness, scoped cook/package runner). The following qualification evidence commit is identified by Git history and the final handoff message.
- No main/default merge, donor resave, original Soul worktree change, release or deployment was performed by this lane.

## Migration and fail-closed behavior

Final exact donor footprint: **346 packages/files, 152,385,152 bytes (145.33 MiB)**: 345 .uasset, one .umap, including all 233 external actors. `clifftop_supplement_transfer.json` is the complete final file/hash receipt. `clifftop_final_plan.json` revalidates installation against the current native report and prior receipt. All 346 source and destination hashes were rechecked after cooking/packaging; zero mismatches.

The incomplete AssetRegistry-query exception is limited to companion-covered external actors with one concrete `/Game/Environment/.../Name.Name_C` class. Its class package enters recursive closure and must be a native Engine Blueprint with complete dependency queries. Unknown/unresolved classes still fail. Every external actor also requires complete native serialized import/soft-reference reads. This found three instance material overrides omitted by the original registry-only closure; they were admitted through the same checks and copied as a 42,822-byte no-overwrite supplement. Class, instance, hard/soft and companion edges all receive forbidden/plugin checks. Regressions cover unknown classes, missing class packages, gameplay/plugin edges, incomplete serialized reads, source changes, companion omissions and destination collisions.

Only referenced support assets were copied. The footprint includes 35 landscape shader-support packages (130,746,712 bytes), not landscape actors/proxies. The largest normal textures are approximately 17–20 MB each. Two precisely classified passive foliage-support assets are admitted: native MaterialParameterCollection `MPC_Player` and native CanvasRenderTarget2D `RT_Player`. No foliage gameplay architecture was imported.

**Sulfur remains excluded.** The deeper native serialized closure contradicts the earlier zero-blocked report: `BP_Clifftop_Stall3_Bar` reaches `/Game/Characters/Main/Standard/oot_lostfang` and `/Game/Characters/SecondaryAnims/AS_Sitting_box`; it also reports missing `/Game/VFX/Environment/_Global/StylizedFire/Textures/T_CloudNoise`. Other unadmitted plugin dependencies remain visible. See `asset_registry_closure.json` and `MIGRATION_DESIGN.md`. No Sulfur Bandit map/actor tree was transferred.

Also excluded: TitanMain and its external/HLOD population, whole biomes, Temple/Armoury/Taedin Mines sets, Characters, Titan scripts/gameplay/framework, MapMode, quest/item/NPC/input/save/movement/fast-travel authority, donor renderer/collision settings, DDC/Intermediate/Saved, landscape proxies and wholesale shared foliage.

## Acceptance evidence

| Check | Result |
|---|---|
| Regression suite | 27/27 pass, tooling-tests.txt |
| Native SoulEditor build and Audit | Exit 0; final Clifftop closure has 0 blocked/0 missing |
| Native Windows Soul build | Exit 0; 99 actions, 742.96 seconds; two existing TCAT deprecation warnings |
| World creation/map check | Soul map saved; nav build tasks reach 0; **0 errors, 580 warnings** |
| Warnings | Donor Nanite meshes without VSM under deliberate DX11 settings; retained in CreatePilot.log |
| Streaming/load | LevelInstance loads; no unresolved package-load errors in final creation or either runtime |
| Soul spawn | Waits for streamed collision/nav before movement; grounded upper terrace near (-1000,0,2343) |
| Input | Packaged keyboard W moves 396.81 cm in one second through actual input/controller path |
| Navigation/collision | Two complete local routes, 28 reachable terrace samples, eight wall-blocking sweeps; pawn physically follows three-point route in 5.55 seconds, grounded at expected elevation |
| Persistence | Existing Soul.Campaign/RBSave saves, mutates hero XP, loads and compares exact captured state; both runtimes PASS |
| Authority | Soul GameMode/pawn/controller and initialized Soul subsystem; zero observed Titan/framework actors; existing RB Combat and RBSave retained |
| Targeted cook | Final exit 0, **0 errors/0 warnings**, 13.79 seconds on warm cache |
| Local IoStore Windows package | Exit 0; **605,626,331 bytes / 58 files**; no release/archive/deploy |
| Packaged runtime | Exit 0, all proof markers PASS; PackagedRuntime.log and screenshots |
| HLOD | Titan HLOD actors not copied; regeneration deferred for compact approximately 95 m × 94 m shell |
| Dragon Graveyard/reinforcement/return | Not exercised; existing campaign/battle architecture and reliability concerns unchanged |

`acceptance.json` checks hashes, receipts, errors and required markers. `CreatePilot.log` has full map-check output. One exact editor HLOD-template import warning resolves when its editor module loads; it is separately recorded, not treated as a donor missing reference. Packaged runtime needs no such exception.

The donor is raised walls, terraces and block foundations, without Titan's surrounding terrain. The overview screenshot shows this limitation. The proof deliberately spawns on a connected upper terrace: an earlier ground-plane-only route was rejected as insufficient. It does not establish connectivity across every block or collision across an entire future campaign region.

## Performance and storage

GTX 1080, DX11, 1280×720, RT off, GI/reflections/VSM off, low shadows/effects/post, 2 GiB texture pool, 60 FPS cap. Packaged sample: **3,873 frames, 16.78 ms average, 153.73 ms maximum**, after initial settling during the 75-second proof. These are game tick deltas, not an uncapped GPU benchmark or percentile capture. Editor-hosted sample averaged 16.80 ms; its maximum hit the engine's 400 ms delta clamp.

Packaged invocation to grounded spawn was approximately **20.2 seconds**; persistent-map LoadMap alone took 0.070 seconds and excludes startup/streaming. Editor-hosted sampled peak working set: **2,226,102,272 bytes (2.07 GiB)**. Whole-GPU observation reached 4,519 MiB of 8,192 MiB, including other desktop processes, not a Soul-only VRAM measurement. See receipts/log timestamps and `runtime_memory_samples.json`.

Initial D: free was approximately 31.71 GiB. Final recorded free: **21,439,180,800 bytes (19.97 GiB)**, above the 15 GiB reserve. Cooked Windows output: **259,029,388 bytes / 1,263 files**. Staged package: approximately 577.57 MiB. Compiler outputs/shared caches account for substantially more than donor content; global disk changes cannot all be attributed to this worktree. `storage_final.json` records sizes and source hash verification.

The first cook exposed unused Landmass editor startup materials referencing NeverCook engine editor resources. The final opt-in `Config/Custom/TitanPilot/DefaultGame.ini` excludes only `/Landmass/Landscape/BlueprintBrushes`; the donor closure has no Landmass edge. Missing-dependency checks stay active. Original failures and unsuccessful plugin-option diagnostics are preserved. Normal Soul campaign packaging is unchanged.

## Reproduce, rollback and next step

[COMMANDS.md](COMMANDS.md) provides exact invocations and all eight stage recipes. Interactive traversal: `Tools/TitanPilot/Play-TitanPilot.bat` (WASD/mouse/Space, F5 save/F9 load). Local packaged executable: `Saved/StagedBuilds/TitanPilot-20261005-183633/Windows/Soul/Binaries/Win64/Soul.exe`; use packaged receipt map/render arguments and worktree-local -UserDir. Omit -SoulTitanPilotProof for interactive play; that flag alone enables automatic test exit.

Rollback uses checkpoint reverts, respecting dependent map/source/assets. Original receipts and prior generated Soul maps remain retained. Never reset/clean unrelated state. Local generated `Config/DefaultEngine.ini` changes were preserved and excluded from this lane's final commits. A concurrent checkpoint had included generated Android File Server configuration; the current committed tip removes that block, but this pass does not rewrite published history.

The smallest next step is to decide the existing campaign return-map/GameMode contract, then exercise one Soul-owned encounter bridge through the existing Dragon Graveyard path. [NEXT_HANDOFF.md](NEXT_HANDOFF.md) identifies functions, scenario data and the return-mode mismatch, and why travel-failure recovery needs coverage before calling that roundtrip reliable. No parallel save/combat/campaign system is needed.

**Widening to Castle/Grassland/Arctic is not justified as an import expansion yet.** This validates the selective migration method and one architectural shell. Finish bounded campaign-return/traversal integration and independently audit the next donor. Sulfur needs a clean-copy stripping/dependency resolution pass before admission.