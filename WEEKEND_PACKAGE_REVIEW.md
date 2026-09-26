# Soul weekend packaging and S2 dependency closure — 2026-09-26

**Package acceptance: UNKNOWN. Runtime acceptance: UNKNOWN.** No UE, UBT, UAT,
cook, build, package, install, or game process was launched by this worker.
This is a source handoff, not a playable-build receipt.

Branch: `worker/Soul-weekend-package-20260926`.
Inspected base/HEAD: `fe3c418f22ca719bf36930dc2caad6008f7151f3`.
S2 packages the existing five-file packaging work plus the bounded source fix
below into one worker commit. Integrate that commit as a unit. No other-worktree
write, donor read/write/copy, UE process interaction, or machine-setting change
was made during S2. Historical donor audit findings below are from the prior
packaging review, not a new inspection of another worktree.

## Result and exact integration files

- `Config/DefaultGame.ini`: Win64 Development packaging defaults; 25 explicit
  package roots (two maps, 23 runtime asset loads); inherited credential filters
  extended to remove Android runtime/file-server/SDK sections and packaging
  settings from staged INIs. No broad content/data/evidence directory staging.
- `Source/Soul/Soul.Build.cs`: five exact NonUFS JSON runtime dependencies for
  the Win64 Game target. No gameplay or plugin source edits.
- `Tools/package_soul_weekend.ps1`: captain-only BuildCookRun wrapper, fixed UE
  5.8 installation, explicit project/output arguments, read-only preflight,
  optional `-ValidateOnly`, new external output directory, console/UAT logs,
  invocation receipt, and preserved nonzero UAT exit code.
- `Source/SoulRealtimeBattle/Private/SoulRealtimeBattleArena.cpp`: remove only
  the legacy key 2/3 cast bindings; retain all Firebolt paths and RB Magic logic.
- `Tools/test_package_soul_weekend.py`: six focused static checks, including
  Firebolt-only runtime/cook/preflight agreement and serialized profile names.
- `WEEKEND_PACKAGE_REVIEW.md`: this receipt.

`DefaultEngine.ini`, graphics settings, Android credentials, licensed assets,
serialized assets, plugins, and main were not modified. Existing JSON staging,
credential filters, wrapper safety checks, and UAT arguments were preserved.

## Captain command

After cherry-picking the S2 worker commit into the RC branch and wiring the existing
donor junctions there, run this exact PowerShell command **from the RC root**:

```powershell
powershell -NoProfile -NonInteractive -File .\Tools\package_soul_weekend.ps1 -Project (Join-Path (Get-Location).Path 'Soul.uproject') -Output 'D:\RefinedBadger\Packages\Soul-weekend-win64-development-20260926'
```

Append `-ValidateOnly` for preflight without UE/UAT/UBT. The output directory
must not already exist; use another new directory on subsequent attempts.
The script must be integrated alongside the project it builds and refuses
main/master and detached HEAD. It performs no checkout, commit, merge, reset,
clean, rebase, push, asset copy, donor repair, or process termination.

In the prior packaging review, this machine's Windows PowerShell execution policy rejected the attempted
`-File ... -ValidateOnly` invocation before the script ran. That invocation
returned 1; no policy was changed or bypassed. The captain needs an already
authorized PowerShell execution context for the command above. This worker
verified syntax through the PowerShell parser, which does not execute the script.

The wrapper invokes the installed
`C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat`
with these arguments (paths resolved from the parameters):

```text
BuildCookRun -nocompileuat -noturnkeyvariables -nop4 -unattended -utf8output
-project=<RC>\Soul.uproject -target=Soul -platform=Win64 -clientconfig=Development
-build -cook -stage -pak -iostore -package -archive -prereqs -nocleanstage
-stagingdirectory=<Output>\Stage -archivedirectory=<Output>\Archive
```

Prerequisites are staged, not installed. No game is launched. No cook/build
skip, map override, proof flag, NullRHI, or runtime command line is supplied.
The normal campaign remains `/Engine/Maps/Entry` with
`/Script/Soul.SoulFounderPlaytestGameMode`.

The output contains `Stage`, `Archive`, and `Diagnostics` (invocation JSON,
console log, UAT logs, exit code). UAT's log-directory cleanup is redirected
to a new directory inside this output. Existing outputs are never reused.
UAT still writes its ordinary RC `Binaries`, `Intermediate`, and `Saved`
build/cook outputs. The captain owns those processes and their engine/toolchain
side effects. Archive portability and cooked contents remain unverified.

## Runtime dependency audit

`SoulFounderPlaytestStateSubsystem.cpp` reads exactly these files using
`FPaths::ProjectDir()/Data`, now staged loose at the same project-relative paths:

```text
Data/soul_vertical_scenario_20260925.json
Data/soul_world_overmap_v1_20260922.json
Data/soul_campaign_start_states_v1_20260922.json
Data/soul_overmap_battle_handoff_v1_20260922.json
```

`RBFoundationManifest.cpp` reads `Plugins/RBFoundation/StackManifest.json`
through its plugin base directory; it is the fifth staged JSON file. Other
Data, Evidence, save files, and development captures are not staging inputs.

The scenario selects `/Game/Dragon_graveyard/Level/L_showcase_level` and returns
to `/Engine/Maps/Entry`. `SoulFounderPlaytestCampaignActor::StartBattle` selects
the real-time game mode; `SoulRealtimeBattleArena` resolves the campaign result
and opens the return map. The packaging lane does not change these behaviors.

The exact cook list covers the source's eleven Knight/Dwarf mesh alternatives,
six movement/attack/death animations, the Firebolt definition and its
presentation profile, engine Cube/Sphere, and two RBWeather constructor-loaded
assets, plus both maps. Formation alternatives already loaded by the existing
runtime are included; no new units or spells were added. Profile hard/soft
references and map references are left for Unreal's dependency traversal.

The installed UE 5.8 `CookOnTheFlyServer.cpp` documents package names in
`MapsToCook`; its `CollectFilesToCook` passes each entry to `AddFileToCook`
without restricting it to a world. This deliberately includes exact non-map
packages to avoid whole donor directory cooks. Do not add UAT `-map` or cooker
`-package` arguments: those supersede this list. Do not add `-allmaps`,
`-cookall`, soft-reference skipping, or broad cook directories.

The prior packaging review's read-only checks against `Soul-pro-vertical-20260925`
found all original 29 cook roots (the current 25 are a strict subset).
The three donor junctions resolve under
`D:\Unreal Projects\AoEAssetRenderLab\Content`:
`Dragon_graveyard`, `Knights_Pack`, `Dwarf_Pack`.
MagicSpells resolves to
`D:\RefinedBadger\Worktrees\Soul-dragon-graveyard-proof-20260922\Content\MagicSpells`.
These junctions are absent from this packaging worker; captain wires RC.

**S2 source/config closure:** read-only inspection of the three serialized Soul
presentation profiles confirms these exact references. Presence findings in the
second column are inherited from the prior packaging review:

| Soft reference | Present through current MagicSpells junction |
| --- | --- |
| `/Game/MagicSpells/Fire/FX/NS_Fireball` | Yes |
| `/Game/MagicSpells/Electric/FX/NS_ChainLightning` | No |
| `/Game/MagicSpells/Ice/FX/NS_Ice_Hailstorm` | No |

The legacy `PlayerTick` contained key 2/3 calls to `CastPlayerSpell` with the
ChainLightning/Blizzard definition and profile paths. `CastPlayerSpell` loads
both assets, and `SpawnSpellPresentation` synchronously loads the profile's
Niagara soft pointer. Explicitly cooking those profiles therefore pulled in
their missing VFX dependencies; preflight also required both effects outright.
`URBMagicPresentationProfile` owns those pointers, so no new magic system or
asset-authoring workaround is needed.

S2 removes those two bindings, their four definition/profile cook roots, and
the two corresponding preflight effect requirements. Firebolt remains on key 1
in both input paths, and the qualification cast remains unchanged. The normal
formation HUD and campaign hero's known spell already specify Firebolt only.
Firebolt's definition/profile serialized names contain no ChainLightning,
Blizzard, or SoulProof references. Preflight still requires the exact Fireball
asset; the static suite checks that its effect list agrees with the selected
serialized spell/profile assets. Tags, unshipped assets, authoring scripts, and
editor proof tests remain available but do not become explicit weekend roots.

Campaign startup, battle handoff, result resolution, return-map travel, and save
code are unchanged. This closes the identified source/config requirement for
unshipped spell effects; it does not prove the cooked transitive closure of
external donor maps/assets. No broader roots, cook-all switches, soft-reference
skipping, copied assets, or fallback VFX were introduced. The dirty donor remains
an external input; byte-identical builds and frozen transitive closure are not
claimed.

Campaign persistence uses RB Save's selected domains (`Soul.Campaign`,
`Soul.Settlements`) and slot `Soul.VerticalCampaign`. The actual path is
`FPaths::ProjectSavedDir()/RBSave/Domains/Soul.VerticalCampaign.domain.rbsave`.
It is created at runtime, never staged. The packaged Saved-directory resolution
and write access require runtime verification; no absolute user-profile location
is assumed. Optional RBSave world/coordinator paths and RBOptimization audit
outputs also derive from ProjectSavedDir and require no packaged input.
TCAT shader/resource paths were inspected; standard engine/plugin shader
staging is retained, with no plugin edits or speculative build-linkage changes.

## Static evidence actually obtained

- S2 `python -B Tools/test_package_soul_weekend.py`: **6/6 PASS**. Compared non-test
  C++/header literal asset paths and scenario maps with the exact cook list;
  compared actual JSON readers with staging rules and parsed all five JSONs;
  inspected the three binary profile reference names and selected Firebolt
  spell/profile names read-only; checked Firebolt-only input/cook/preflight
  agreement and the retained campaign known spell; checked
  campaign startup, bounded staging, and credential filters; parsed PowerShell
  and inspected its UAT argument AST without executing the packaging script.
- `git diff --check`: PASS (Git reports the existing CRLF conversion policy for
  `Soul.Build.cs`; no whitespace error).
- Prior review's read-only reference-tree existence audit: 29/29 cook roots present;
  1/3 MagicSpells profile targets present, two missing as listed above.
- Prior review's optional script preflight invocation: BLOCKED by Windows script execution
  policy before script execution, exit 1. No successful script execution,
  simulated UAT invocation, or native-exit propagation runtime test is claimed.
- Prior review inspected installed UE 5.8 cooker, ProjectParams, DeploymentContext,
  staging/command-line code, TargetReceipt and RunUAT source read-only to check
  package roots, NonUFS receipt staging, INI deny lists, command arguments,
  logging, and exit handling. No UE help process was run.

- S2 did not rerun full script preflight: this worker has no donor junctions.
  No C++ compile or UE automation test ran. Static name inspection is not an
  Unreal asset-registry/cooker traversal.

Next acceptance belongs to the captain: cherry-pick the single worker commit
(including the pre-existing packaging work), wire RC donors, run the six static
checks and `-ValidateOnly` in an authorized PowerShell context, then perform
the Win64 Development build/cook/package. Inspect staged INIs/manifests for
credential exclusion and bounded content. Launch archived `Soul.exe` with no
arguments, verify normal campaign -> Dragon Graveyard -> reinforcement waves
-> campaign, key 1 Firebolt with visible Fire VFX and committed spell effects,
inert keys 2/3, then RB Save persistence/reload. Inspect cooked dependency/log
output for unexpected ChainLightning/Blizzard or missing Electric/Ice packages;
if a donor map introduces them transitively, stop and report that separate
dependency rather than broadening this contract. A successful UAT exit alone does
not establish those runtime outcomes. **Runtime/package acceptance UNKNOWN.**
