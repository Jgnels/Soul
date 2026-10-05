# Soul x Titan pilot — blocked preflight checkpoint

Recorded 2026-10-05. **Pilot acceptance NOT achieved. No donor packages were copied.**

## Authority and isolation

- Worktree: `D:\RefinedBadger\Worktrees\Soul-titan-pilot-20261005`
- Branch: `codex/soul-titan-pilot-20261005`
- Verified clean starting HEAD: `6d53714559d34a123b5394206866925a165373c4`
- Titan donor: `D:\Unreal Projects\ProjectTitan`, read-only throughout this work.
- No original Soul worktree mutations, no main/default merge, no release/publish/deploy.
- Checkpoint changes: editor-only audit commandlet, read-only Python census, guarded native build/audit runner, migration-plan validator, tests and evidence. Runtime/gameplay files and configuration remain unchanged.

## Why the lane stopped before Unreal qualification

Three pre-existing Titan processes were already running before this mission:

| PID | Process | Start, local | Command intent |
|---|---|---|---|
| 11592 | UnrealEditor-Cmd.exe | 06:43:01 | ProjectTitan, NullRHI, `-ExecCmds=Quit` |
| 25676 | UnrealEditor.exe | 06:43:11 | ProjectTitan, NullRHI, `-ExecCmds=Quit` |
| 21008 | UnrealEditor.exe | 06:44:01 | ProjectTitan, NullRHI, `-ExecCmds=AssetRegistry.DumpState,Quit` |

They remained running at the guarded build attempt at 08:45. Initial observed working sets totaled about 5.3 GiB on the 16 GiB machine; later working sets fell, but the processes remained alive. Existing Studio Control tooling rejects competing UE sessions. This product runner uses the same machine mutex and likewise fails closed without launching another heavy job or writing a competing shared queue.

An asynchronous approval request asked whether to close **only those three existing audit processes without saving assets**. They predate this mission and are outside its isolated ownership; no answer had been received at this checkpoint. No process was terminated. This is a resource/ownership blocker, not a compiler error or automatic approval-review rejection.

`BuildEditor-attempt.json` records the blocked native build admission. The compiler did not run. The new C++ commandlet is therefore **uncompiled and unqualified**.

## Phase 1 findings

All eight requested donor audit artifacts were read. Their fingerprints are in `source_verification.json`.

The prior `pilot_dependency_hard_only.json` has mode `hard_package_references_only` and only two `/Game` packages for the Clifftop map: the map and a single external actor anchor. It is **not** an actor-complete migration manifest. Its later `.py` file was changed to a map-plus-companions implementation, but the corresponding newer output was not present. The earlier broad closure report gives sizes but no complete package list.

The new read-only binary census includes every external actor/object for both selected maps, recursively follows environment package strings, and includes companions of nested maps. It stops expansion at forbidden paths. It also sees historical import metadata; missing strings and forbidden strings are **not automatically proof of a live hard dependency**. Native UE classification remains required.

| Donor | Root external actors | Scanned existing packages | Scanned bytes |
|---|---:|---:|---:|
| `/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid` | 233 | 344 | 152,378,712 |
| `/Game/Environment/Sulfur/Level_Instances/LI_Sulfur_BanditRestOutpost` | 368 | 1,167 | 644,438,341 |

Unique scanned union: **1,464 package files / 664,406,963 bytes**. This is a bounded **candidate census**, not the required/approved migration footprint. Every scanned file has a SHA-256 in `binary_preflight.json`. Rechecking all 1,464 files found **zero changes since the scan**. Exact relative filenames, dependency strings and obstruction chains are preserved there.

### References requiring classification before copying

Both donors:

`M_BaseMaterial` / material instances reference:
- `/Game/Blueprint/FoliageInteraction/MPC_Player`
- `/Game/Blueprint/FoliageInteraction/RT_Player`

These names suggest passive shader support (material parameter collection/render target), not an executable gameplay Blueprint. They remain rejected by the conservative migration gate until native class/edge verification. Do not import the foliage interaction gameplay system to service them.

Sulfur:

`LI_Sulfur_BanditRestOutpost`
→ external actor `E/II/HPGVE0OLZE6XA593NWVVAX`
→ `/Game/Environment/Clifftop/BlueprintActors/BP_Clifftop_Stall3_Bar`
→ `/Game/Characters/Main/Standard/oot_lostfang`
and `/Game/Characters/SecondaryAnims/AS_Sitting_box`.

The prior broad registry closure independently reports about 0.025 GiB of Characters for Sulfur. The new string census identifies a concrete route to investigate; **no claim yet that the edge is runtime-hard**. Under the mission's fail-closed rule, stop this donor if native classification confirms unexpected contamination. Do not widen to Characters.

Additional Sulfur strings include `/Game/Maps/LV_ChunkDownloadLobby`, `/Game/Maps/TitanMain` through a nested crate LevelInstance, and Water material functions. These may include historical metadata, but they cannot be silently treated as approved dependencies. Native hard/soft edges must settle them.

Excluded throughout: TitanMain and its external/HLOD population, Temple, Armoury, Taedin Mines, Character packages, Titan runtime/editor modules and custom gameplay plugins, Blueprint/Framework, MapMode, quests/items/NPC/input/save/movement/fast-travel authority, whole biome/shared foliage/landscape folders, donor config, DDC, Intermediate and Saved trees. Exclusion here means **not copied**; candidate string scans were read-only.

## Acceptance matrix

| Requirement | Result |
|---|---|
| Isolated branch and base verified | PASS |
| Donor read-only during this work | PASS; 1,464 scanned package hashes rechecked unchanged |
| Reusable fail-closed tooling | 8 Python tests PASS; native commandlet uncompiled |
| Exact approved donor closure | NOT ESTABLISHED; candidate census only |
| Exact migrated footprint | **0 packages / 0 bytes** |
| Project builds after integration | NOT RUN; lane admission blocked, no integration |
| Pilot map | **NOT CREATED**; intended `/Game/Soul/Maps/Soul_TitanPilot` |
| Donors integrated | **NONE** |
| Load/render / missing references / map check | NOT ATTEMPTED; no pilot map |
| Soul spawn/input / navigation / collision | NOT ATTEMPTED |
| HLOD / streaming | NOT ATTEMPTED; no Titan HLOD copied |
| Cook/package | NOT ATTEMPTED; earlier gates blocked |
| Performance | No pilot measurements; no fabricated FPS/load/memory result |
| Soul gameplay authority | Static preservation only: no runtime code/config changes, no donor gameplay copied; live initialization untested |
| Dragon Graveyard roundtrip | NOT ATTEMPTED; no relocation and no reliability rewrite |
| Disk space | Initial 34,051,424,256 bytes (31.71 GiB); evidence recheck 34,047,512,576 bytes (31.71 GiB), above 15 GiB reserve |

Other pre-existing processes can write caches/logs on D:, so global free-space change cannot be attributed solely to this worktree.

## Smallest next action and continuation

1. Resolve ownership of the three pre-existing audit processes. Close only with approval or let their owner end them. Recheck the existing machine lease.
2. Run the checked-in guarded `BuildEditor` stage, fix any compiler errors locally, then `Audit`. See `Tools/TitanPilot/README.md` for exact commands. Native output must include all root and nested companion trees; zero-dependency/query failures cannot be silently accepted.
3. Verify the passive material-support classes/edges for Clifftop and explicitly review their narrow treatment. Run the plan validator only after the report has no unresolved forbidden/missing dependencies. Preserve `/Game` identities; do not rename binary files to relocate packages.
4. Stop Sulfur if its character/map references are genuine contamination. Record native hard/soft edge evidence instead of importing excluded branches. A separate bounded stripping decision would be a later pass.
5. Only after native closure admission, transfer exact hashed packages with no overwrites and verify in Soul. Then create the Soul-owned pilot map, compact layout, Soul spawn/controller and navigation. Existing founder GameMode intentionally has no pawn and spawns the graph campaign/camera, so it cannot be assumed to provide terrain traversal. A small pilot-specific traversal mode can reuse Soul state/RBSave without spawning that graph over donor geometry; this has **not** been implemented or exercised.
6. Qualify loading/rendering, map check, representative navigation/collision, DX11 low-cost rendering, storage, cook and runtime. Keep RB Combat and current campaign/battle bridge untouched. Dragon Graveyard content is not present in this clean worktree by default (`.gitignore` excludes that donor); do not claim a roundtrip until its existing dependencies are available and exercised.

**Widening to Castle/Grassland/Arctic is NOT justified.** Resolve the two first-proof donors and exercise a Soul-owned map first.

## Evidence and rollback

- `binary_preflight.json`: per-package hashes/bytes/references, complete candidate lists, blocked chains.
- `binary_preflight_summary.txt`: concise per-donor census output (PowerShell UTF-16).
- `source_verification.json`: requested audit fingerprints, package recheck, storage and zero-copy receipt.
- `BuildEditor-attempt.json`: blocked admission, exact competing PIDs.
- `tooling-tests.txt`: 8 passing guardrail tests.
- `Tools/TitanPilot/README.md`: exact reproducible commands and policy limits.

Revert the checkpoint commits to remove this tooling. No imported donor binaries or maps need cleanup; no existing Soul assets were overwritten. Do not reset/clean unrelated state.
