# Soul RC source convergence - 2026-09-26

Acceptance for this worker: integrate the proven package commits and the inspected dirty RC source changes in isolation; preserve the normal campaign -> physical Dragon Graveyard -> reserves -> result -> continuation -> RB Save flow; reject provably stale editor binaries before cook; execute source/static checks only. Runtime/package acceptance remains UNKNOWN.

## Integration

Base: `fe3c418f22ca719bf36930dc2caad6008f7151f3`.
Branch: `codex/soul-rc-convergence-20260926`.
Worktree: `D:/RefinedBadger/Worktrees/Soul-rc-convergence-20260926`.

1. `7a2d94d` cherry-picks proven `de204aa299c57fd8fa12cf494c8e7a32009ba677`.
2. `08535b0` cherry-picks proven `a5b0b5f1bf0072fdf1a54dbc285540008bb8bb7c`.
3. `1616cdd` preserves dirty RC DX11, F5/F9 debug-binding clearance, campaign camera and readable labels, 45/30 strategic pools with 15 active per side, manual Firebolt, reinforcement HOLD inheritance, battle camera focus, and continuation/defeat-recovery regression cases.
4. The following packaging commit retains RC runtime shader settings, two-action/no-UBA build and DDC fallback arguments, and adds the editor freshness gate.

The worker commits apply cleanly in this order. The RC arena diff applies without textual conflicts on top: the worker removes unshipped keys 2/3, while RC changes BeginPlay, SpawnFormation and SetupBattleCamera. Dirty files cannot safely be replaced with either worker wholesale: DefaultGame.ini and the packaging wrapper carry additional runtime shader and machine-budget fixes. Their tests require a semantic merge: retain the worker's exact Firebolt-only roots and exact UAT-token assertions, then add the RC shader assertions and two bounded-execution arguments.

The generated Android file-server section in dirty DefaultEngine.ini is excluded from this isolated branch. DX11 is preserved. The live file remains untouched. No content/junctions/binaries were copied, created or rewired.

## Editor preflight

`Tools/Test-SoulEditorFreshness.ps1` is a read-only helper. `package_soul_weekend.ps1` calls it before content validation, ValidateOnly success, output directory creation or UAT. There is no bypass flag. The wrapper retains `-build -skipbuildeditor -cook`: building the game cannot establish editor freshness.

Checks cover SoulEditor Win64 Development receipt identity, installed engine version and BuildId, declared project/enabled-local-plugin modules, receipt-listed transitive local plugins, module manifests, standard DLL names, missing/zero-size DLLs, DLLs newer than their receipt, native source/rules/descriptors newer than the receipt, each module's native inputs newer than its DLL, and transitive local dependency headers/rules newer than consuming DLLs. Dependency names are conservatively gathered from Build.cs string literals. Successful packaging diagnostics record the receipt SHA-256, timestamp, BuildId and checked module count.

This is conservative static rejection, not compiler provenance. Timestamp-preserving edits, stale binaries copied with misleading timestamps and dynamic/nonliteral dependency declarations cannot be certified. Git checkout timestamps and Build.cs changes can conservatively demand rebuilding even when an incremental build does not recompile a DLL. Resolve that with a real rebuild of the affected editor modules, never by touching timestamps or bypassing the gate. Keep source frozen between final preflight and cook. No existing binary hash baseline is claimed.

Source-only invocation (run from the intended checkout under its owning account):

```powershell
. ./Tools/Test-SoulEditorFreshness.ps1
Assert-SoulEditorFreshness -ProjectRoot (Get-Location).Path -EngineRoot 'C:/Program Files/Epic Games/UE_5.8'
# The wrapper's -ValidateOnly performs the same check plus content/config checks.
```

## Evidence and remaining blockers

- 6 package source/AST tests PASS after combining the RC and workers.
- 15 synthetic editor-preflight fixtures PASS; they execute PowerShell against fake files only.
- 2 campaign camera/label geometry tests PASS.
- 9 qualification-runner safety tests PASS with mocked process/GPU functions.
- UE automation cases in SoulVerticalCampaignTests.cpp are preserved and extended from the RC, but were NOT run or compiled here. Existing reinforcement tests cover the 15-per-side cap; those also were NOT run here.
- Live RC read-only freshness check rejects SoulRealtimeBattleArena.cpp (2026-09-27T00:07:34.6125895Z) newer than SoulEditor.target (2026-09-26T23:45:58.1968862Z); its editor DLL is older still (2026-09-26T23:45:54.3982221Z).
- Isolated wrapper `-ValidateOnly` exits 1 on absent SoulEditor.target, before UE/UAT/UBT or output creation, as intended for a source-only worktree.
- Local UE source `Engine/Source/Runtime/RenderCore/Private/ShaderLibrary/ShaderCodeLibrary.cpp:2260` reads bShareMaterialShaderCode from GGameIni; this supports retaining the RC setting and excluding the whole packaging section from the denylist.

Next authorized build owner must rebuild SoulEditor from the final integrated source, rerun static preflight, then perform the separately authorized build/cook/package and real input-driven packaged loop acceptance at 30 active. Verify manual key 1, HOLD during reserve arrivals, actual result identity and survivor counts, second encounter after ending day, F5/F9, and fresh-process save/reload. Prior small-force or editor receipts do not certify this final package. No UE, UBT, UAT, cook, package or runtime was launched by this worker.

This addendum supersedes WEEKEND_PACKAGE_REVIEW.md wherever that historical worker receipt recommends denying the whole packaging config section or omits the RC machine-budget arguments.
