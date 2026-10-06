# Selected Human capital — chronological integration evidence

**Final status:** fully streamed runtime r5 and owned waterfront material repair verified. Remaining castle materials and Human gameplay/state/miniature integration are unfinished. Earlier failures and proposed next steps below are historical; the final section and HANDOFF.md are authoritative.

Jeff supplied `D:/RefinedBadger/AssetLibraries/MedievalKingdom-42d4a792`. No recovery search or download was repeated. `human-complete-donor-before.json` inventories 8,083 files, 8.57 GB, with 6,328 map/external-package hashes. Previous incomplete junctions are preserved under `Local/PreservedHumanMounts/`; the complete source is mounted at its original package paths.

The actual `/Game/CastleTown/Levels/Persistant/PL_CastleTown` loads. The first cold load took approximately eight minutes of local mesh/texture cache work. The initial survey found 3,137 actors, including 145 Level Instances. The main castle, town houses and wall instances had not loaded in the first overview. That image is rejected as a complete city render; it is not evidence of missing restored source files.

A single Soul-owned persistent wrapper was saved as `/Game/Soul/Maps/Settlements/L_HumanCapital_Authored`, 11,022,239 bytes. It retains the original sublevel, mesh and material references. Original source map SHA256 remains `d8cd81d8d536b2ed6f285fc4a0663899f52e97441ffe66931396db0a06c3541c`. The initial wrapper receipt is `owned-variant-L_HumanCapital_Authored.json`.

Native instance load requests and an unsaved explicit-streaming castle probe eventually triggered the original house/castle instance loads. Their cold mesh builds exhausted system commit memory in `editor-r13`: private commit last sampled at 28,062.97 MiB, working-set peak 12,647.57 MiB; GPU 36 C and 3,687 MiB. Unreal reports a paging-file/commit allocation failure. The guard also failed to launch telemetry under that memory pressure and terminated its owned child. This is a resource failure, not a thermal cutoff or an accepted city render.

The temporary streaming probe was not saved. Do not duplicate it on a fresh attempt. No donor packages were intentionally saved. Recheck donor hashes after further work. The next bounded attempt must use a fresh editor (no retained Python actor/world references from earlier Dwarf/source/owned-world loads) and the warm cache. First establish complete native instance loading and rendered city composition before selecting development actors or building a miniature.

Epic documents that [Level Instances outside a World Partition main world do not automatically receive a streaming strategy](https://dev.epicgames.com/documentation/en-us/unreal-engine/level-instancing-in-unreal-engine). This is relevant context, not proof of the cause here. Do not claim a specific engine bug or convert the environment architecture until the fresh native load is measured.

## Fresh native load r14

The fresh process loads the saved owned wrapper without the unsaved probe or retained earlier-world Python references. Its original castle sections, curtain walls, gate and town-house instances start loading normally. This is evidence against treating the earlier absent buildings as an architecture defect. The read-only readiness request remained queued behind native loading and timed out; it must not be resubmitted blindly.

At 10:09:47 UTC the private-memory sample reached 20,080.73 MiB. The new guard stopped the owned editor at 10:10:01, before another system allocation failure. Peak sampled working set was about 11 GB. No complete overview, development group or Human visit/battle acceptance is claimed. No extra instance or architecture conversion was saved.

Next bounded attempt: use the warmed cache and inspect native compiler concurrency controls before loading. A Windows system-commit sampler now preserves at least 4,096 MiB host headroom independently of the per-process guard. This permits a measured larger process allowance when host headroom is actually available without changing the paging file or removing the thermal cutoff. Do not spend the remainder of the shift repeating uncontrolled loads.

## r16 complete instance load, inspection side effect, r17 rejection

r16 verifies and sets native static-mesh and texture compilation concurrency to one worker each. It successfully reports **145 levels loaded in 435.036 seconds** after the initial persistent-map load. This is the first complete top-level native instance expansion in the continuation. The queued readiness inspector then reads `world_asset`, a soft UWorld property that Python may resolve/load; the log immediately creates another original `LI_CurtainWall_Corner_02` world. Host commit headroom falls below the preserved 4,096 MiB threshold, and the guard closes the process. This timing implicates an inspection side effect, but it does not prove that all excess memory came from the probe. No complete render was captured.

The inspector now uses only `get_loaded_level()` paths and never dereferences source-world soft pointers. r17 tests synchronous mesh/texture compilation as a separate bounded resource diagnostic. It crashes early in `ShowFlags.cpp:1136`, `IsInGameThread()`, on a foreground worker, with ample memory. That mode is rejected; do not repeat it or modify the engine to pursue it.

r18 returns to the verified one-worker asynchronous controls, keeps both memory guards, and will avoid source-world dereferencing entirely. First obtain a rendered view after native instance completion and release transient objects before a scalar-only survey. This is the final repeated whole-city editor-load attempt before moving to other useful integration work if the complete scene remains resource-bound. The donor and saved wrapper have not been edited by these diagnostic loads.
# Final bounded whole-city editor attempt (r18, 11:11 UTC)

All 145 native level instances finished loading in 343.142 seconds. Explicit GC completed. During subsequent native asset preparation the editor reached 24,030 MiB private commit with only 4,176 MiB host commit headroom; the guarded process then closed. Its cleanup exceeded both bounded ten-second waits, so the old launcher did not write `session.json`. `launch.json`, telemetry and `unreal.log` remain the evidence; no successful render or physical Human proof is claimed. No Unreal process remained afterward. The launcher now preserves its guard decision before cleanup and records cleanup exceptions.

The corrected inspector did not produce a receipt in this attempt. Reading soft source-world properties was removed from both inspectors, but this attempt still exceeded the available memory budget without those reads. Therefore the source-world-read issue alone does **not** explain the load limitation. Native loading also reports one missing `/Bridge/MSPresets/MSTextures/WhitePlaceholder` plugin reference in MI_BedSheet; this is a plugin mount dependency, not grounds to resume source recovery.

Further repeated full-city editor loads are stopped for this shift. Keep the complete source and owned wrapper. Next Human work should prepare native assets/sublevels in bounded batches or qualify a cooked/runtime streaming path, preserving the full authored city. No substitute capital, terrain redraw, paging-file change, or donor edit is authorized by this result.

## Read-only native game-world path, 12:58 UTC

A technically different `-game` survey now opens the intact owned wrapper with a neutral native game mode, isolated UserDir, native authored cameras, memory guards and the 85 C cutoff. It does not initialize settlement gameplay, save donor packages or bind an unqualified Human environment to Crownstead.

Direct cold-registry r1 stalled without logged progress for seven minutes at the `SL_Landscape` sub-world-partition initialization. It was closed after about twelve minutes, at peak 7,870 MiB private commit. The current registry references were then copied into a new isolated UserDir using the same bounded cache-seeding method as the working Dwarf runner. r2 moved past that point and loaded/rendered real castle, market and waterfront geometry within roughly 12.8 GiB private commit. This establishes useful progress, not a proven engine-root-cause diagnosis.

**r2 images are rejected as complete-city evidence.** Its first observer counted 21 visible levels out of 156 and captured while additional native instances were still pending. The castle/market appear, but houses and some finished materials are absent. The process later remained in async-loading shutdown and was closed by the owner guard. A completion marker and three images are not acceptance.

The observer now waits for every requested native streaming level, global async loading and asset compilation before inventory or captures. It also records the actual loaded runtime actor/mesh inventory for subsequent state-group selection. Inspection-only preparation uses 5 FPS to conserve the thermal budget; this mode makes no performance claim. Dwarf performance qualification remains uncapped.

All 59 unique external-actor load-error packages from r2 exist locally. Their serialized script-class strings are exclusively `/Script/UnrealEd.GroupActor`, consistent with editor-only grouping metadata rather than missing city meshes. See `human-r2-external-actor-failures.json`. This byte-level observation is not a license to suppress other dependency errors or skip rendered inspection. No recovery search/download is warranted.

Epic's [UE-356455](https://issues.unrealengine.com/issue/UE-356455) and [UE-227634](https://issues.unrealengine.com/issue/UE-227634) describe distinct editor/standalone partition issues. Neither establishes the cause of this load, so no engine patch, landscape conversion or donor change is justified from those reports.

## Fully-streamed gate r3 and bounded extension, 13:22 UTC

r3 made steady progress from 146 pending native levels to 32 out of 161 before the observer's ten-minute streaming deadline. It deliberately failed without capturing or declaring the city complete. The owned child exited; `session.json` records `native_survey_completed: false` despite an exit-zero shutdown. Memory/thermal guards were not the stopping cause.

One final runtime inspection r4 extends only the Human streaming deadline to twenty minutes. Its parent has a 26-minute wall-clock limit; the native observer retains its overall 25-minute bound, 20 GiB process-private limit, 4 GiB host commit headroom and 85 C cutoff. This extension follows measured loading progress, not a missing-source theory. No further whole-city editor retry or donor mutation is involved. Final status must follow the actual readiness gate and rendered images.

## Complete native runtime inspection r4, 13:40 UTC

The extended bounded run completed cleanly with **56,443 actors, 17 original cameras and 161 visible levels**, after waiting for every requested level and native async/asset preparation. All three 1920x1080 original-camera captures were inspected. The authored castle, houses, market, docks, forest and surrounding mountains are physically present. This resolves the complete-runtime-load question; it does not establish visit/state/battle or performance acceptance. Inspection cap5 remains non-performance evidence. Peak sampled private commit13,549.75 MiB, working set9,584.3 MiB, GPU78 C and device-wide VRAM3,864 MiB. Whole process elapsed1,067.46s including preparation/captures/shutdown.

The captures expose a specific visual defect: three waterfront materials (`MI_Stab_Algae`, `MI_Wood_Algae`, `MI_StoneWall_Algae1`) compile to the default material under SM5. The log reports a null texture at the shared `ML_BasicTextured_01` normal parameter, spelled `Normnal` in the donor. Native slab/wood/moss normal overrides are present; source recovery is not relevant. Gray waterfront surfaces prevent full visual acceptance. `human-runtime-r4-review.json` records this distinction.

A blank-editor inspection (no full-city reload) produced nine Soul-owned material/function copies under `/Game/Soul/Materials/Settlements/HumanCapital/`. They retain the native layer/blend settings and textures and supply a conventional donor slab-normal default to the null node. The initial preview was provisionally rejected because its left/right order was misread. The +Y-facing camera places the owned +X wall on screen-left; it is textured, while the donor/default-material wall on screen-right is gray. The later settled comparison and explicit actor/material positions correct that interpretation. A cached-Base defect was a hypothesis, not a verified cause. Native parent setters now explicitly keep the owned dependency chain consistent. All nine donor material/function hashes remain unchanged.

The first native parent-cache repair attempt crashed at an unnecessary `SetParent(nullptr)` call before any package save. That call was removed. Candidate backups are preserved in `Local/HumanMaterialsBeforeNativeReparent`; the native setter retry must be rendered and checked before admission. This editor-only repair is not a gameplay/state-system change.

## Owned waterfront repair saved, 14:20 UTC

The settled two-wall comparison verifies the owned copy renders the original textured stone/algae while the donor material falls back to gray. The null normal node references missing `/Bridge/MSPresets/M_MS_Glass_Material/Textures/FlatNormal`; this is a plugin-content dependency, not missing CastleTown source. The owned copy supplies a valid native donor normal as the default while preserving actual native layer overrides. Runtime-used textures include the original stone/wood/moss normals and colors. No generated texture or graph redesign is introduced.

`SL_HumanCapital_Waterfront` is a single Soul-owned copy of the native `SL_BridgesAndWalls`. It retains all geometry/transforms and changes 1,016 matching material slots to the three owned materials. The city wrapper changes only that one native streaming reference through the guarded native API, without loading the entire city into the editor. The original root wrapper and pre-repair owned materials are preserved locally. Full-city runtime r5 is the acceptance gate for visible gray-surface removal; save success alone is not acceptance. All 8,083 donor file records and 6,328 package hashes, plus nine additional material/function hashes, match their baselines.

## Fully streamed repair qualification r5, 14:44 UTC

The owned waterfront repair passes direct rendered comparison. Four original-resolution captures were reviewed, including a transient whole-city overview. The authored multi-island town, bridges, houses, windmills, castle and surrounding landscape remain intact. All 53,529 StaticMeshActor transforms and mesh paths match r4 with duplicate multiplicity; zero added/removed. This comparison excludes foliage instances, component-level blueprint geometry and collision.

Some castle parapets remain gray outside the repaired waterfront. Three original donor MI compilation warnings still occur. The overview retains heavy native haze and is content evidence, not final campaign art. Full runtime load is verified; Human physical development, miniature, visit/battle routing and uncapped performance remain unqualified. See `human-runtime-r5-review.json` and `human-owned-repair-final-manifest.json`. No donor or retained campaign terrain was changed.
