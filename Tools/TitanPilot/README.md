# Titan pilot verification tools

Scope: isolated `codex/soul-titan-pilot-20261005` branch only. Titan is read-only.
No tool here copies donor assets or changes gameplay, renderer, input, save, or combat authority.

1. `audit_packages.py` reads the two named donor maps, both companion trees and recursively discovered environment packages. It hashes every scanned file and preserves chains to prohibited references. This string census includes import metadata and historical names: **it is not a hard-reference or missing-runtime-asset verdict**.
2. `Run-TitanPilot.ps1 -Stage BuildEditor` participates in the existing Studio Control machine mutex, refuses pre-existing UE/dotnet workloads and builds only this worktree with two compiler actions. It writes its receipt here, not into shared orchestration state. Direct UBT invocation follows the implementation used by existing `RB_BUILD.ps1`, avoiding global status writes and the known Build.bat lock problem. `-Stage Audit` invokes the new editor-only `SoulTitanAudit` commandlet.
3. The commandlet mounts the donor's Content directory under `/Game/` **in its own process**, scans package metadata with UE AssetRegistry, and never loads/saves donor UObjects or starts Titan modules. Every map's external actors and external objects enter the closure, including nested maps. Hard and soft edges are separate. Blocked paths stay visible and do not expand gameplay branches. Missing/query failures cannot unlock migration.
4. `prepare_migration.py` accepts only that native report. It refuses unresolved contamination, missing packages, failed registry queries, unknown plugin/script dependencies, an incomplete companion tree, changed source size, destination collisions, closure over 1 GiB, and free space below 15 GiB after migration. A successful result is a hashed plan for review, **not asset copying or pilot acceptance**.

The initial native commandlet remains UNCOMPILED until the existing editor lane is freed. Do not call it qualified based on Python tests.

Commands from repository root:

```powershell
python Tools/TitanPilot/audit_packages.py --donor 'D:\Unreal Projects\ProjectTitan' --output Evidence/TitanPilot-20261005/binary_preflight.json
python -m unittest discover -s Tools/TitanPilot -p 'test_*.py' -v
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage BuildEditor
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage Audit
python Tools/TitanPilot/prepare_migration.py --audit Evidence/TitanPilot-20261005/asset_registry_closure.json --donor 'D:\Unreal Projects\ProjectTitan' --seed ClifftopMine --output Evidence/TitanPilot-20261005/clifftop_migration_plan.json
```

`RemoteSigned` is invocation-scoped for this locally authored script; no machine/user execution policy is changed. Build and audit receipts distinguish lane rejection from an actual compiler/editor exit. No automatic retries or termination of another worker's process.

Before copying, review the two `Blueprint/FoliageInteraction` data assets by native class and dependency edges. They are deliberately still rejected; changing a prefix allowlist alone is not qualification. For Sulfur, determine whether `BP_Clifftop_Stall3_Bar` character/animation references are actual dependencies or historical/editor metadata. If actual contamination is confirmed, stop that donor under the mission's explicit rule. Do not import Characters or TitanMain to satisfy it.

After a clean native plan: preserve `/Game/...` names with a hash-checked, no-overwrite package transfer including all sidecars and companion trees; validate package loads in Soul. Create `/Game/Soul/Maps/Soul_TitanPilot` only from qualified content. Navigation and optional HLOD belong to this new Soul map. No Titan renderer/configuration files, DDC, Intermediate or Saved trees are migration inputs.

Rollback of this checkpoint is the ordinary Git revert of its commits. No imported binary files, project map defaults, or donor package mutations need reversal.
