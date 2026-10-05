# Reproduction and exact invocations

Run from `D:\RefinedBadger\Worktrees\Soul-titan-pilot-20261005` on `codex/soul-titan-pilot-20261005`. Native stage receipts (`*-attempt.json`) record the exact executable, argument array, times, exits and free space. The runner participates in the existing Studio Unreal mutex and refuses competing workloads or less than 15 GiB free.

```powershell
python -m unittest discover -s Tools/TitanPilot -p 'test_*.py' -v
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage BuildEditor
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage Audit
```

The original no-overwrite transfer is recorded in `clifftop_transfer.json`. The subsequently identified instance materials used these commands; both original and supplemental receipts remain preserved:

```powershell
python Tools/TitanPilot/prepare_migration.py --audit Evidence/TitanPilot-20261005/asset_registry_closure.json --donor "D:/Unreal Projects/ProjectTitan" --seed ClifftopMine --prior-receipt Evidence/TitanPilot-20261005/clifftop_transfer.json --output Evidence/TitanPilot-20261005/clifftop_supplement_plan.json
python Tools/TitanPilot/transfer_clifftop.py --audit Evidence/TitanPilot-20261005/asset_registry_closure.json --plan Evidence/TitanPilot-20261005/clifftop_supplement_plan.json --prior-receipt Evidence/TitanPilot-20261005/clifftop_transfer.json --receipt Evidence/TitanPilot-20261005/clifftop_supplement_transfer.json
```

Those historical transfer commands intentionally refuse a second copy. To re-plan the already completed installation, use `clifftop_supplement_transfer.json` as the prior receipt. To audit a genuinely empty destination, omit the prior receipt. Never delete existing files merely to bypass collision checks.

```powershell
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage CreatePilot
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage Runtime
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage BuildGame
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage CookWindows
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage PackageWindows
powershell -NoProfile -ExecutionPolicy RemoteSigned -File Tools/TitanPilot/Run-TitanPilot.ps1 -Stage PackagedRuntime
python Tools/TitanPilot/qualify_evidence.py
```

`CreatePilot` refuses an existing map. Any rebuild in this pass first preserved the prior generated Soul map under the evidence directory after verifying both paths remained inside the isolated worktree. Donor packages were never resaved. Interactive editor-hosted traversal uses `Tools/TitanPilot/Play-TitanPilot.bat`; WASD/mouse/Space move/look/jump and F5/F9 use the existing Soul campaign RBSave handlers. Automated proof flags alone enable automatic exit.

`CookWindows` intentionally overrides the campaign release cook roots with the one pilot map. `PackageWindows` stages only the existing targeted cook into a new timestamped local directory, with no archive/publish/deploy. The separate weekend release packaging script is not invoked. Packaged qualification uses `-NotInstalled` and a worktree-local `-UserDir` to isolate its saves.

The opt-in `-CustomConfig=TitanPilot` profile excludes `/Landmass/Landscape/BlueprintBrushes` from this cook only. These unused editor startup materials caused eight NeverCook dependency errors in the initial attempt; their obstruction log is retained. The verified donor graph contains no Landmass edge. Missing-dependency validation remains enabled. Epic documents this scoped config mechanism in [FConfigCacheIni::GetCustomConfigString](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/FConfigCacheIni/GetCustomConfigString?lang=en-US); the installed UE 5.8 `ConfigCacheIni.h` also defines its directory and development command-line behavior. Earlier plugin-disable command-line experiments did not prevent the engine mounting those plugins and are not used by the final recipe.

GPU observations used `nvidia-smi --query-gpu=name,memory.total,memory.used,utilization.gpu --format=csv,noheader`; values are whole-GPU observations, not a per-process profiler. Process memory and D: free space were sampled alongside them in `runtime_memory_samples.json`.
