# Soul GPU stability — 2026-09-25

Status: **G0 PASS: final real-map framing and 76.83 seconds confirmed live without crash. One earlier D3D12 startup reset remains unexplained; G0–G6 PASS, including rendered campaign → Dragon battle → campaign → save/reload. One earlier G0 startup GPU reset remains unexplained but did not recur in the accepted ladder. G5/G6 used an explicit temporary filesystem-cache backend.**

This record separates earlier worktree evidence from current acceptance. No prior
filename containing "soak" or "PASS" establishes the duration or visual quality of
a new run. Default project RHI is D3D12; D3D11 evidence is diagnostic only.

## Machine and preflight

Read-only snapshot at 2026-09-26T03:06:17Z (2026-09-25 local):

- Authorized host: DESKTOP-Q1S3RPU; Intel i7-7700HQ; UE 5.8.2.
- NVIDIA GTX 1080: 8192 MiB total VRAM, 4646 MiB used, 50 C, 7% utilization.
- System RAM: 16338.9 MiB total, 7597.9 MiB free.
- No UnrealEditor/Soul process matched the preflight process inventory.
- No OS, driver, RHI, or product rendering settings changed by this audit.

## Verified historical failures

Paths below are under
D:\RefinedBadger\Worktrees\Soul-dragon-graveyard-proof-20260922 unless noted.

1. Evidence/DragonGraveyard/visual_acceptance_builtin_exposure_fix.log:
   D3D12 SM5, real donor map, 48 visual combatants, 1280x720 windowed.
   At 2026-09-24 04:02:51 UTC, frame 85, about four seconds after arena setup:
   DXGI_ERROR_DEVICE_REMOVED / DXGI_ERROR_DEVICE_HUNG.
   Breadcrumbs were active in CaptureConvolveSkyEnvMap / Capture Sky Raw and
   BasePass. A directional shadow atlas 8192x2048 was queued, not proven causal.
   Aftermath reports PageFault, AddressTranslationError, Graphics Read,
   Engine Reset true, Adapter Reset false. Local VRAM used 1092.20 MB versus
   7291 MB budget at frame 84: this record does not support VRAM exhaustion.
   Preserved binary: Saved/Logs/D3D12.0.2026.09.23-22.02.51.nv-gpudmp (331136 bytes).
   Classification: GPU page-fault/renderer/RHI failure; exact cause UNKNOWN.
2. Evidence/DragonGraveyard/web_soul_nohud_20260924.log:
   D3D12 SM5, 1280x720, 48 real combatants, magic proof enabled.
   Frame 48 at 2026-09-24 18:11:44 UTC: GPUSkinVertexFactory.cpp line 1343,
   assertion bPrevious. This is a separate engine skeletal-render assertion;
   it is not evidence of the same device-hung mechanism.
3. Evidence/DragonGraveyard/exposure_off_gamma3.log and
   Saved/Crashes/UECC-Windows-9B1F3F534BE03A616C08FBAC8732DB72_0000/:
   D3D12, 1280x720, external environment and real units, experimental exposure
   commands. Background Worker #1 crashed with Renderer.dll access violation
   at frame 3. Saved CrashContext.runtime-xml remains available.
4. The realtime qualification worktree's
   Saved/Crashes/UECC-Windows-FBB92769455AB3F8F94C919A2C84A482_0000/ records
   ZenStoreWriter.cpp line 514 during cooking. This is not a GPU crash.

The prompt's earlier DEVICE_RESET / 91 C / 32v32 clue was not independently
located in these audited files. Do not promote it to a verified root cause.

Historical runtime_visible_soak_30m_20260924.log ends during SDK startup;
packaged_allvisuals_runtime_soak_20m_20260924.log ends around twelve seconds
after arena setup. Neither has a verified complete soak termination in the
audited log, despite launch arguments requesting long runs. Both use D3D11.

## Environment identity and cost

- Actual map: /Game/Dragon_graveyard/Level/L_showcase_level.
- Existing Dragon worktree Content/Dragon_graveyard junction resolves to
  D:\Unreal Projects\AoEAssetRenderLab\Content\Dragon_graveyard.
- Knights_Pack and Dwarf_Pack junctions resolve to the corresponding directories
  under the same donor project. Treat all donor content as read-only.
- Evidence/DragonGraveyard/donor_audit.json records 420 actors: 361 StaticMeshActor,
  13 RectLight, 13 NiagaraActor, four BP_bones_C, and one Landscape.
  No LevelInstance class appears in that recorded actor inventory.
- lighting_inspect.json records movable lighting, captured-scene skylight,
  dense height fog, unbound postprocess, and MI_ground_floor landscape material.
- Largest donor texture packages are approximately 60–104 MB on disk. Package
  size is only a cost flag; actual resident GPU memory must be measured.
- D3D12 failure logs use SM5 and explicitly disable ray tracing. Generic startup
  Lumen/Nanite CVar lines alone do not prove those features rendered.

## Repeated rectangles — camera defect identified; remaining geometry unresolved

Existing screenshots were inspected as read-only in-memory JPEG thumbnails
because tools.view_image failed with the Windows sandbox-helper initialization
error. Original PNG files were unchanged; no desktop/UI capture was performed.

The two historical PNGs under the old Dragon worktree's Evidence/DragonGraveyard
show nearly identical close-up rectangular rock faces, despite different battle
origins:
- runtime_visible_bounded_proof_20260924.png / .log: origin (2000,-15000,0).
- runtime_visible_geom_safe_20260924.png / .log: origin (-22500,12500,0).

Both logs enable SoulRealtimeArenaProof, ExternalEnvironment and VisualUnits,
confirm the actual donor map, and initialize 48 real combatants. In that
worktree Source/SoulRealtimeBattle/Private/SoulRealtimeBattleArena.cpp:
- Constructor lines 183–187 sets DefaultPawnClass=nullptr and
  bStartPlayersAsSpectators=true.
- SetupArena lines 785 onward positions/possesses PlayerHero and calls
  SetViewTarget only inside the condition !bProof && PlayerHero.
- The donor audit records no PlayerStart and several grouped rock meshes near
  world origin.

Thus changing ArenaOrigin moved the battle but did not move the proof spectator
camera. The fixed, obstructed origin view explains these two misleading "battle"
captures. The same actual map/origin with proof mode disabled
(runtime_player_view_20260924.png / .log) shows real units on ground rather than
the repeated foreground blocks; the log confirms possession at
(550,-14740,190.15). web_soul_bones_20260924.png shows real units among large donor
rock geometry at another deliberately obstructed location.

This evidence supports a proof-camera authority defect, not duplicated levels.
The camera defect alone is not a complete explanation for all block-like imagery. The current corrected view still shows conspicuously repeated rectangular rocks and wave-like ground; root has not accepted the visual environment. A current G0 visual still must confirm the corrected observer actually shows
the intended battle area, landmarks, and no unwanted assembly. Root's new
SetupBattleCamera path is pending current graphical qualification.

A separate fallback defect also exists in the historical harness: without
ExternalEnvironment it creates a Cube floor; without VisualUnits it creates
cube combatants. Those flags were present in the two misleading PNG runs, so
fallback cubes do not explain those particular images.

The old generic SoulRealtimeArenaGameMode unconditionally creates 48 units.
Do not use it for G0. Use the new map-only mode and observer. Prior inspected
battle candidate is (2000,-15000,0); the earlier less obstructed proof pocket is
(-22500,12500,0).

## Current graphical gates

| Gate | Status | Map | Active units | Duration | RHI / resolution | Crash | Log / observations |
|---|---|---|---:|---|---|---|---|
| G0 map only, run 1 | FAIL (visual) | Actual donor above | 0 | 76.88 s after ready; 111.44 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G0_D3D12_map_only; clean exit; root review found white world |
| G0 map only, run 2 | FAIL (scenery framing) | Actual donor above | 0 | 76.97 s after ready; 98.98 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G0_D3D12_exposure_normalized; exposure visible, camera sees mostly ground |
| G0 map only, run 3 | FAIL (GPU startup) | Actual donor requested; LoadMap not reached | 0 | 0 s after ready; 19.39 s total | D3D12 SM5 / 1280x720, 30 FPS cap | Yes: DEVICE_RESET | VerticalRuns/G0_D3D12_graveyard_framed; frame 0 EndUpdateTexture3D, exit 3 |
| G0 Entry isolation | PASS (diagnostic only) | /Engine/Maps/Entry | 0 | 76.86 s after ready; 93.56 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G0_D3D12_Entry_diagnostic; exit 0; not Dragon acceptance |
| G0 real map after Entry | FAIL (visual geometry unresolved) | Actual donor above | 0 | 77.28 s after ready; 105.56 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G0_D3D12_after_Entry_isolation; bones visible, block-like rocks remain under investigation |
| G0 final accepted map only | PASS | Actual donor above | 0 | 76.92 s after ready; 76.83 s alive; 104.33 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G0_D3D12_final; root accepted ribcage/spine, terraces and battle foreground; exit 0 |
| G1 minimum combat, wide camera | Runtime/stability PASS; visual readability pending | Actual donor above | 2 | 66.61 s alive after ready; 97.41 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G1_D3D12_1v1; real result/PBIL and hold passed, fighters too small at wide framing |
| G1 minimum combat, closer camera | PASS | Actual donor above | 2 | 61.69 s alive after ready; 92.59 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G1_D3D12_combat_camera; readable Knight/Dwarf contact, correct result, exit 0 |
| G2 small squad | PASS | Actual donor above | 10 | 72.03 s alive after ready; 113.42 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G2_D3D12_5v5; real 5v5 resolved, hold/exit passed |
| G3 representative | PASS | Actual donor above | 30 (15v15) | 72.11 s alive after ready; 103.14 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G3_D3D12_15v15; dense real melee resolved, PBIL 4/4/4, exit 0 |
| G4 reinforcements | PASS | Actual donor above | 10 max active; pools 12/10 | 82.75 s alive after ready; 119.38 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G4_D3D12_reserves; six real waves, hostile reserve exhausted, victory 5/0 includes one friendly reserve |
| G5 RB Magic, first attempt | FAIL (startup timeout; no gameplay reached) | Actual donor requested | 0 initialized; 10 planned | 0 s ready; runner elapsed 339.80 s | D3D12 SM5 / 1280x720, 30 FPS cap | No GPU crash | VerticalRuns/G5_D3D12_magic; static-mesh readiness stall; owned child terminated at deadline |
| G5 RB Magic, cache diagnostic | PASS | Actual donor above | 10 max active; pools 12/10 | 125.47 s alive after ready; runner elapsed 277.38 s | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G5_D3D12_magic_filesystem_cache; Firebolt cast, mana 72, combat and six waves resolve; temporary DDC flag |
| G6 campaign round trip | PASS | Entry campaign → actual donor → Entry campaign | 10 max active; pools 12/10 | 71.72 s alive after disk restore; 139.23 s total | D3D12 SM5 / 1280x720, 30 FPS cap | No | VerticalRuns/G6_D3D12_campaign_roundtrip; real victory, correct target, rendered return, RBSave reload, exit 0 |

## G0 run 1 — stable process, white-world visual failure

Root executed this lane's first graphical test at 2026-09-26 03:27:59–03:29:51
UTC. Owned PID 17808; no processes were started/stopped by the audit worker.

- Evidence/VerticalRuns/G0_D3D12_map_only/{launch.json,unreal.log,telemetry.jsonl,summary.json}.
- Actual donor LoadMap completed at 03:28:25.453 UTC in 5.241 seconds.
  SOUL_G0_READY logs simulation=0, units=0.
- Runner observed 76.88 seconds after readiness (111.44 seconds total).
  D3D12 SM5, windowed 1280x720, development t.MaxFPS 30; unchanged product RHI.
- Clean WM_CLOSE shutdown, exit 0; no crash signatures, new crash reports, or
  GPU dumps. This establishes only this bounded no-combat runtime interval.
- 22 telemetry samples. GPU temperature rose from 45 C to maximum 64 C.
  Adapter-wide VRAM maximum 5767 MiB / 8192 MiB (includes other applications).
- After 20 seconds of post-ready warmup (12 samples): UE working set
  2767.16–2781.54 MiB; first 2767.16, last 2777.88 MiB. Private commit
  3589.35–3595.13 MiB; first 3595.13, last 3589.37 MiB. No obvious runaway
  growth in this short window; this is not a long-duration leak test.
- Runtime explicitly disables ray tracing. It compiles donor seagull/snow
  Niagara systems. Streaming texture pool reports 1000 MB after startup.
  A driver-dependent TSR 16-bit VALU warning appears, without a crash.
- Root's inspection of Saved/Screenshots/Vertical_G0_A.png found a solid white
  world beneath the HUD. Therefore G0 visual acceptance FAILS; do not escalate
  to G1 yet. Camera/landmark qualification remains pending a visible image.

## White-world exposure diagnosis — confirmed by G0 run 2

The donor volume inspection records AutoExposureMinBrightness=-0.5,
AutoExposureMaxBrightness=0, AutoExposureBias=1, all bounds overridden.
Those bounds make sense as EV100, not as nonpositive luminance.

Read-only local engine evidence:
- Engine/Source/Runtime/Engine/Private/SceneView.cpp:201 registers
  r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange with literal default
  0, even though its help text calls 1 the UE5 default.
- Engine/Source/Runtime/Engine/Classes/Engine/Scene.h:1991 documents the
  unit switch between luminance and EV100.
- Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessEyeAdaptation.cpp
  converts EV100 bounds only when the runtime CVar is 1; otherwise it uses the
  stored min/max directly.
- Donor project DefaultEngine.ini, Dragon worktree config/dirty config, current
  Soul config, and current Saved config have no exposure-unit override.
  The donor/Dragon config differences inspected concern Android file server,
  not exposure. No credential values were retained here.

The supported hypothesis is therefore EV100-style donor bounds being interpreted
as legacy luminance, the reverse of the unverified earlier clue. G0 run 2 now logs the runtime CVar as 0 and confirms this unit mismatch.
Recommended next single change is map/camera-local exposure-unit normalization:
when legacy range is active, convert the intended EV100 bounds to positive
luminance (default attenuation gives about 0.7071–1.0), retaining exposure and
lighting quality. A camera-local equal positive min/max=1 is an alternative
bounded diagnostic. Do not globally disable eye adaptation, change gamma, edit
the shared donor map, or change RHI to obscure the failure.

## Bounded runner and response rule

Tools/qualify_soul_vertical.py is development-only. It launches one owned UE
process, refuses existing Soul/Unreal processes, uses windowed 1280x720 and
t.MaxFPS 30, samples NVIDIA telemetry and the owned process's working set, peak
working set, private commit, and pagefile usage about every five seconds, and stops its
own child at >=85 C, a crash signature, unavailable telemetry, startup timeout,
or the bounded observation deadline. It first requests WM_CLOSE only for its
own PID, then terminates only that child if needed. It never retries.

The runner preserves new crash folders and GPU dumps, command line, console and
UE logs, telemetry, RHI lines, and a JSON summary under Evidence/VerticalRuns.
Its default-mode successful exit requires the requested map-ready duration and clean WM_CLOSE
shutdown, with no crash signatures. Early exit, forced termination, and a crash
race before cleanup return nonzero. This still means observation completed, not
graphical PASS. The runner rejects packaged bootstrap executables; pass the
actual Binaries/Win64/Soul executable so it owns the process being measured.
Camera responsiveness, actual scene identity, gameplay, and sustained memory
behavior still require inspection. No graphical run has been executed by this
audit worker.

Smallest diagnostic: G0, actual donor, zero combatants, unchanged project D3D12,
1280x720, 30 FPS, >=60 seconds after map-ready. If it fails, retain the evidence,
isolate one renderer/environment variable supported by the new failure, and
retest once. Do not add gameplay until G0 is qualified. Do not repeat an unchanged
crashing scenario more than twice.

## Explicit self-exit completion mode

The runner now supports --completion-marker <exact-log-marker> with a bounded
--completion-timeout (default 300 seconds after readiness, maximum 900).
This opt-in mode waits for the owned child to self-exit with code 0. It requires
the marker, no crash/thermal/telemetry failure, and at least --duration seconds
(minimum 60) of confirmed live observation after map readiness. Time after the
last live-process sample does not count. Marker absence, early/nonzero exit,
crash, thermal cutoff, and completion timeout all return nonzero.

For a campaign path that otherwise exits immediately after save/reload, hold the
returned campaign for 70–75 seconds before exiting. Do not manufacture an earlier
ready marker. The extra margin covers the runner's five-second sampling cadence.
Summary reports alive_observed_after_ready_seconds separately from wall time and
retains automatic_acceptance=false. No visual PASS is generated by this mode.

Validation: python Tools/test_qualify_soul_vertical.py — 9/9 fully mocked tests
PASS. They cover sufficient observation, early exit, dead polling interval,
missing marker, nonzero exit, crash, thermal cutoff, bounded timeout, and original
duration-mode behavior. No Unreal process was launched by these tests.
## G0 run 2 — exposure recovered, camera framing pending

Evidence/VerticalRuns/G0_D3D12_exposure_normalized contains the launch, UE log,
20 telemetry samples, and summary. Owned PID 18612 ran 2026-09-26
03:37:59–03:39:38 UTC: 76.97 seconds after observed readiness, 76.88 seconds of
confirmed live observation, 98.98 seconds total. D3D12 SM5, 1280x720 windowed,
30 FPS cap, zero simulation/units, clean WM_CLOSE exit 0. No crash signatures,
new crash reports, or GPU dumps. Maximum 63 C and 5697 MiB adapter-wide VRAM.
After 20 seconds of post-ready warmup, UE working set 2753.89–2782.15 MiB,
private commit 3594.65–3603.96 MiB; no obvious runaway growth in this short run.

The log confirms extended=0, authoredMin=-0.5, authoredMax=0, bias=1, followed
by SOUL_DRAGON_EXPOSURE_NORMALIZED min=0.707107 max=1. Root visual inspection
confirmed that terrain is now visible. The remaining visual failure is scenery
framing: the camera points steeply down and excludes nearby graveyard landmarks.

Prior battle_zone_visual_analysis.json supports retaining origin (2000,-15000):
it is the best recorded visual clearing, 6.91% upper-bound obstruction, with
12 dragon landmarks within 12000 cm. BP_bones4 bounds-center is
(3444.9,-11134.0,1053.1), 4127 cm from the arena; the next dragon skeleton is
(4895.2,-8993.9,320.7). The former camera offset (-1900,-2100,+2200), aiming at
battle center with 65-degree horizontal FOV, points down about 38 degrees.
BP_bones4 is only about 11 degrees below its horizon, placing it above the
roughly 39-degree vertical field. This explains a mostly ground-only image.

Root is testing a camera-only change: offset (-1800,-3200,+1700), aiming at
Center+(500,1000,350), FOV 65, with an alternate lateral view. Projection of the
recorded landmark positions fits both battle ground and bones. Current runtime
visual confirmation is still required. The older origin (-22500,+12500) really
is distant fringe (nearest recorded landmark 24052.6 cm) and is not recommended
for recognizable graveyard qualification.
## G0 run 3 — D3D12 initialization GPU reset, before map load

Evidence/VerticalRuns/G0_D3D12_graveyard_framed contains the exact command,
unreal.log, three telemetry samples, summary, and preserved crash directory.
Owned PID 14544 started 2026-09-26 03:44:34.565 UTC and exited at 03:44:53.979;
19.39 seconds total, zero seconds after readiness, exit code 3. Default D3D12
SM5, 1280x720 windowed, development 30 FPS cap, -SoulMapOnly, -unattended,
-nosound. The real donor URL was requested, but no LoadMap or SOUL_G0_READY
line was reached. No units or battle simulation initialized.

At 03:44:47.097 the log says Initializing Engine. At 03:44:47.631, frame 0,
the device reports DXGI_ERROR_DEVICE_RESET. Active breadcrumb:
EndUpdateTexture3D; EndDrawingViewport was finished. No active compute queue
nodes, shader diagnostic, fault address, or Dragon sky-capture marker were
reported. Aftermath status is Timeout, Adapter Reset true, Engine Reset false.
A following CreateCommittedResource failure for a tiny 2x2 resource is after
the reset; it is not evidence that a large allocation exhausted VRAM.
Post-reset budget values are invalid/unavailable and are not used here.

The last sample, at 10.25 seconds, was 48 C, 4899 MiB adapter-wide VRAM,
1168.85 MiB UE working set and 1341.36 MiB private commit. Three samples do not
establish long-term memory behavior; they show no measured overheating before
this failure. Driver version in the crash context is 582.53. No driver/OS
settings were changed.

The camera framing change was present in the binary, but its BeginPlay path
was not reached. This crash does not establish camera, donor geometry,
exposure, Niagara, unit count, or battle AI as the cause. The historical
SkyCapture crash instead reported DEVICE_HUNG / PageFault / Engine Reset true;
its signature is materially different. Current classification: D3D12 GPU
initialization timeout/reset; underlying cause UNKNOWN. Determinism and
map-dependence are unestablished. There has been no unchanged retry.

Smallest current failing scenario: this project's D3D12 game startup with the
real donor URL and map-only flags, before map load, zero combatants, frame 0.
The next single-variable diagnostic is the same bounded command with only the
map URL changed to /Engine/Maps/Entry and the same map-only game mode. Keep
D3D12, resolution, frame cap, and observation duration unchanged. This can
separate startup failure from loading the donor. A second same-signature
D3D12 startup failure ends retries on that path. Any later D3D11 run must be
explicitly diagnostic, not product acceptance. This audit worker launched none.

Preserved binary (already copied inside the crash directory by the runner):
Evidence/VerticalRuns/G0_D3D12_graveyard_framed/crashes/
UECC-Windows-F393231B49105D55F641D7AAE6602889_0000/
D3D12.0.2026.09.25-21.44.50.nv-gpudmp — 45964 bytes.
SHA256 a3f64c3797a8a41d3d9efc571dc5ffc342e9d5c53fb286d167e4b18207f1e68f.
The retained file hash matches Saved/Crashes exactly; SHA256SUMS.txt is beside
it. CrashContext.runtime-xml, UEMinidump.dmp and crash log are retained too.

## Entry-only isolation — completed without GPU crash

Evidence/VerticalRuns/G0_D3D12_Entry_diagnostic changed only the requested map
to /Engine/Maps/Entry, retaining the map-only game mode and default D3D12 SM5,
1280x720 windowed, 30 FPS cap, -unattended and -nosound. PID 13752 ran
2026-09-26 03:47:06–03:48:39 UTC, 93.56 seconds total. The log confirms Entry
loaded in 0.0622 seconds and SOUL_G0_READY simulation=0 units=0.
The runner observed 76.86 seconds after readiness, 76.75 confirmed live;
clean WM_CLOSE exit 0, no crash signatures or new preserved crash files.
Nineteen samples measured maximum 56 C, 5177 MiB adapter VRAM and 2077.99 MiB
UE working set including startup. No RHI, driver, or product settings changed.

This passes only the Entry initialization isolation check. It shows that a
D3D12 startup can complete after the reset, but does not prove the prior fault
was deterministic, map-dependent, or solved. The real Dragon map, corrected
camera, and gameplay are still unqualified. Root may perform one controlled
return to actual-map G0 after logic verification. Do not describe Entry as G0
Dragon acceptance or advance to combat on this evidence alone.

## Actual-map return after Entry — stable process, geometry bar not met

Evidence/VerticalRuns/G0_D3D12_after_Entry_isolation: PID 14552,
2026-09-26 03:54:31–03:56:16 UTC. Actual donor URL, unchanged D3D12 SM5,
1280x720, 30 FPS cap, no units/simulation. 105.56 seconds total,
77.28 after readiness, 77.19 confirmed alive. Clean exit 0 and no new crash.
Twenty-one telemetry samples measured maximum 63 C and 5471 MiB adapter VRAM.
Twelve samples after 20 seconds of warmup show working set 2775.63–2786.38 MiB
and private commit 3573.55–3587.96 MiB. The startup reset did not reproduce
in this bounded return; its cause remains unknown.

Root's screenshot review sees bones, but also repeated large rectangular rocks
and wave-like ground. This does NOT meet the user's real-environment visual bar.
The prior spectator-camera defect was real but is not accepted as a complete
explanation for the remaining scene appearance. No G1 escalation on this result.

Read-only donor inventory contains only L_showcase_level and L_assets_showcase.
Binary asset-registry metadata in SM_rock_01, SM_rock_02 and SM_rock_03 reports
NaniteEnabled=False and NaniteFallbackPercent=100.0, LODGroup=smallprop, with
stored triangle counts 18484 / 37026 / 18484. SM_ground_rock_01 likewise reports
Nanite disabled and 2011 triangles. These actual asset records oppose the
hypothesis that the visible rocks are coarse Nanite fallback geometry under
SM5. They do not by themselves identify the runtime-selected LOD or explain
surface shape. The current log confirms PCD3D_SM5; generic Nanite builder/CVar
startup messages do not prove the assets use Nanite.

Tools/inspect_soul_dragon_render_contract.py is a narrow read-only editor audit
for Root to run under NullRHI. It loads the real map, queries unique meshes'
Nanite settings and LOD triangle/vertex counts, actor assignments/scales/LOD
constraints, and landscape material. It writes Evidence/DragonRenderContract.json;
no asset setter, geometry build, asset save, or gameplay path is invoked.
Python syntax validation passed. Runtime audit result is pending.

## Read-only render audit — authored rock repetition and lava-plane overlap

The first NullRHI audit wrote Evidence/DragonRenderContract.json and the UE
commandlet completed without log errors. Its JSON nevertheless records 270
query errors: StaticMeshEditorSubsystem is unavailable in this commandlet,
plus two property-name mismatches. Core direct mesh/actor queries succeeded;
missing Nanite settings must not be interpreted as false. The script now uses
direct reflected Nanite properties and native mesh vertex queries for Root's
follow-up. Screen-size/reduction queries are explicitly unavailable when that
optional subsystem is absent.

The successful records confirm 420 actors, 439 mesh components, 25 unique
meshes, no LevelInstance actor, no Cube mesh, and no forced-LOD/min-LOD override.
SM_rock_02 is repeated 93 times; SM_rock_01 81 times; SM_rock_03 56 times.
Their ordinary LOD triangle counts are:
- SM_rock_01 / 03: 18484, 9242, 4620, 2310.
- SM_rock_02: 37026, 18512, 9256, 4628.
Thus the inspected render data contains no cube-like 12-triangle fallback.
Near the arena, SM_rock_147 and 148 use the real SM_rock_02 / MI_rock_02 at
uniform scales 1.7769 and 2.5489. Perimeter instances 178 and 179 use scale
47.6579; these repeated large rock forms are stored in the donor map itself.
This does not establish that the final runtime LOD or visual composition is
acceptable, and the user's rectangular-chunk bar remains in force.

A concrete placement concern emerged: donor Plane4 uses MI_lava, location
(5222.689,-9776.346,61.473), extent (6000,6000,approximately 0), scale 120.
Its XY bounds contain the current arena center (2000,-15000). Plane3 is a
second MI_lava surface elsewhere. The broad wave-like surface may be this
authored lava plane. Bounds alone do not establish which surface spawn traces
hit or which surface is visibly on top. The Landscape uses MI_ground_floor.

Tools/inspect_soul_dragon_ground.py therefore performs read-only editor-world
Visibility traces matching the runtime spawn direction and Z range, records
actual blocking hit actor/component/material/Z/normal, samples current spawns,
and searches a bounded grid near graveyard bones for dry, flat, connected-scale
candidate areas. It never saves or mutates donor data. Collision and graphical
acceptance remain pending; zero/failed traces cannot produce a safe candidate.

## Ground collision result — retain the tested clearing

Root's completed Evidence/DragonGroundContract.json records 1892 actual editor
Visibility traces, zero errors, and five provisional conservative candidates.
The initial Python NativeBreak binding failure is retained separately in
DragonGroundContract_first_binding_probe.json. The corrected script uses
HitResult.to_tuple(), checks the UE5.8 18-field schema and vector positions,
and successfully records real blocking surfaces.

All nine current samples (center, both +/-700 spawn sides, +/-600 lateral
positions, and +/-1800 centerline extents) hit the actual Landscape at Z=100,
normal (0,0,1), with no lava material. Plane4 lies at Z=61.472638, 38.53 cm below
those sampled surfaces. Therefore its XY overlap is not evidence that the
current battlefield is on lava; the earlier audit's XY exclusion is deliberately
conservative and can reject valid terrain. The wave-like material appearance
must not be asserted to be a visible lava overlay on this evidence.

Retain the current origin (2000,-15000), whose spawn surfaces are now measured.
The alternate (4000,-18000) has 25/25 flat dry samples but moves the nearest
bones to 6888 cm rather than 4127 cm, without fixing a demonstrated current
collision defect. The minimum remaining visual change is camera framing that
shows the bones together with the battlefield: same observer offset, aim at
Center+(500,1800,600). This changes the actual view authority, not donor geometry.
Current visual confirmation and subsequent bounded combat are still required.

The authored donor repeats its rock meshes; that repetition remains. There is
no evidence of duplicated level instances, fallback Cube substitution, or
12-triangle LOD collapse. This audit does not claim that authored rock repetition
was removed or that changing camera framing alone guarantees visual acceptance.

## Final G0 — PASS, real environment and sustained bounded observation

Evidence/VerticalRuns/G0_D3D12_final: PID 11244,
2026-09-26 04:16:33–04:18:17 UTC. Actual Dragon Graveyard map, zero units and
simulation, unchanged default D3D12 SM5, 1280x720 windowed, development 30 FPS
cap. 104.33 seconds total; 76.92 seconds after readiness, 76.83 confirmed alive.
Clean WM_CLOSE exit 0, no crash signatures, no newly preserved crash files.
Twenty-one samples: maximum 65 C, 5556 MiB adapter-wide VRAM. Twelve samples
after 20 seconds of warmup: UE working set 2787.20–2802.89 MiB and private
commit 3621.81–3628.18 MiB. No obvious runaway growth in this bounded window.

Root inspected the final engine screenshot and accepted environment identity
and framing: real dragon ribcage/spine and authored rock terraces are clearly
visible behind the actual battle foreground. Observer focus is
Center+(500,1800,600); the world-origin-only obstruction is gone. The donor's
authored repeated rock meshes remain, explicitly unchanged and not claimed
removed or polished. No shared donor asset, RHI, OS, driver, or product quality
setting was altered to obtain this view.

This completes G0 only. The earlier frame-0 initialization reset is still
preserved and unexplained, although it did not recur during Entry isolation
or the two subsequent real-map runs. It is not erased by this PASS. Combat,
reinforcements, magic, and campaign round trip need their own staged runs.

## G1 first 1v1 — runtime/stability PASS, closer-view check pending

Evidence/VerticalRuns/G1_D3D12_1v1: PID 9552,
2026-09-26 04:18:37–04:20:14 UTC. Actual donor, human Knight02 versus Dwarf
Bedvar, two real actors/two groups, no reserves or magic. Default D3D12 SM5,
1280x720, development 30 FPS cap. 97.41 seconds total; 71.76 seconds wall time
after readiness and 66.61 seconds confirmed live. The expected arena PASS
marker was observed after the result plus a 60-second hold. Intentional exit 0,
no crash signatures/new crash files. Nineteen samples: maximum 60 C and
5556 MiB adapter-wide VRAM.

The real battle resolved in 8.28 seconds: won=1, survivors 1/0, 15 accepted
contacts, PBIL queries/success/orders 2/2/2. Dead enemy no longer participates;
PBIL CPU height map was initialized at 66x46. This exercises runtime spawn,
approach, combat, damage, death, result and sustained post-result observation.

Root judged the wide screenshot insufficient for requested battle readability:
the fighters were too small and low-contrast. Therefore runtime and stability
pass, but the complete G1 visual gate is pending. The only planned change for
one closer-view repeat is the normal battle camera: offset (-850,-1800,1000),
aim Center+(250,1400,300), FOV 75. The accepted map-only G0 camera is unchanged.
No unit count, rendering quality, RHI or environment change accompanies this.

Nine post-warmup samples before the final PASS/teardown marker show UE working set 3046.47–3058.22 MiB and private commit 3882.04–3884.22 MiB. One final sample catches normal process-memory release during shutdown and is excluded from this steady-state range.

## G1 closer-camera repeat — PASS

Evidence/VerticalRuns/G1_D3D12_combat_camera: PID 4252,
2026-09-26 04:23:21–04:24:54 UTC. Actual donor, two real opposing units,
unchanged D3D12 SM5, 1280x720 and 30 FPS cap. 92.59 seconds total,
66.89 seconds wall time after readiness and 61.69 seconds confirmed alive.
PASS marker after the 60-second post-result hold, intentional clean exit 0,
no crash signatures or new preserved crashes. Eighteen telemetry samples:
maximum 63 C and 5643 MiB adapter-wide VRAM. Nine warm samples before exit:
working set 3042.91–3050.65 MiB; private commit 3871.48–3874.58 MiB.

Battle resolved in 8.19 seconds: victory, survivors 1/0, accepted contacts 15,
PBIL queries/success/orders 2/2/2, no reserve waves or magic. Root visually
verified two real contrasting humanoids (taller gold Knight, shorter gray
Dwarf), readable active contact poses, correct result counter, and no exploded
or squashed mesh. Real graveyard bones remain in view. This meets the minimum
G1 readability bar. Authored fog and repeated rock terraces remain; no polish
claim is made. Camera was the only material change from the first 1v1 run.

## G2 5v5 — PASS

Evidence/VerticalRuns/G2_D3D12_5v5: PID 17100,
2026-09-26 04:25:27–04:27:21 UTC. Actual donor, ten real combatants,
D3D12 SM5, 1280x720, 30 FPS development cap. 113.42 seconds total;
77.22 seconds wall time after readiness, 72.03 confirmed live. PASS marker
after the post-result 60-second hold; intentional clean exit 0, no crash/new
crash files. Twenty-two samples: maximum 64 C, 5624 MiB adapter-wide VRAM.
Result in 13.32 seconds: victory, survivors 1/0, 76 accepted contacts,
PBIL queries/success/orders 2/2/2, no reserves or magic. Root inspected the native battle capture and accepted ten distinguishable real Knight/Dwarf units approaching and clashing coherently, with the correct resolved result. G2 PASS.


## G3 representative 15v15 — PASS

Evidence/VerticalRuns/G3_D3D12_15v15: PID 15752,
2026-09-26 04:31:30–04:33:13 UTC. Thirty real active combatants, actual donor,
D3D12 SM5, 1280x720, development 30 FPS cap. 103.14 seconds total;
77.47 seconds wall time after readiness, 72.11 confirmed live. Intentional
clean exit 0 and expected PASS marker after the 60-second post-result hold;
no crash signatures/new crash files. Twenty telemetry samples: maximum 61 C
and 2392 MiB adapter-wide VRAM. Initial adapter usage was 1250 MiB, lower than
previous runs; adapter totals include unrelated application allocations and
must not be treated as per-game comparative scaling measurements. Warm
pre-teardown UE working set 3071.16–3075.05 MiB; private commit
3902.71–3911.50 MiB.

Result in 15.40 seconds: victory, survivors 5/0, 216 accepted contacts,
PBIL queries/success/orders 4/4/4, no reserve waves or magic. Root accepted
the native battle capture at the minimum prototype bar: distinguishable real
factions approach and engage in dense melee, graveyard bones remain visible,
and no exploded/squashed mesh is seen. Captures are preserved in this run's
evidence. No larger active force was needed to qualify the representative gate.

## G4 bounded reinforcements — PASS

Evidence/VerticalRuns/G4_D3D12_reserves: PID 11688,
2026-09-26 04:33:33–04:35:33 UTC. Strategic pools 12/10, active cap five per
side, actual donor, D3D12 SM5, 1280x720, development 30 FPS cap. 119.38 seconds
total; 87.88 seconds wall time after readiness, 82.75 confirmed alive.
Expected PASS marker after the 60-second post-result hold, intentional clean
exit 0, no crash/new crash files. Twenty-three telemetry samples: maximum
62 C, 5995 MiB adapter-wide VRAM.

Six actual reinforcement deliveries are logged: human 2+2+2, enemy 2+2+1.
One human strategic reserve remains undeployed; the enemy pool is exhausted.
The battle resolves after 24.20 seconds with victory 5/0, meaning four living
human actors plus one reserve, 142 accepted contacts, and PBIL
queries/success/orders 16/16/9. The surviving friendly reserve is retained
rather than fabricated as an active actor. Root inspected the result capture's
active/reserve counters, stable faction/death poses, and correct resolution;
both battle and result captures are preserved. G4 PASS.

## G5 first attempt — startup timeout, no GPU crash or magic execution

Evidence/VerticalRuns/G5_D3D12_magic: owned PID 14304. The launch requested the
same actual donor, strategic pools 12/10, cap five per side, and tactical RB
Magic. D3D12 SM5, 1280x720 and the development 30 FPS cap were retained.
The log stopped while waiting for donor static meshes before playing; no
arena/PBIL initialization, readiness marker, spell cast or battle result was
reached. This therefore does not test tactical magic behavior.

The runner reports startup_timeout, elapsed 339.80 seconds from its execution
clock, zero observed/confirmed-live time after readiness, exit code 1,
clean_shutdown=false, cleanup=terminate_owned_child. Summary UTC timestamps
are start 2026-09-26 04:35:50.813 and stop 04:42:11.709; broader launch/host delay
is visible and should not be confused with ready-world observation. Crash
signatures and preserved crash lists are empty. This is a bounded failed
startup, not a GPU crash or PASS. Root observed wider command/file contention;
its cause remains unknown.

Root started one controlled retry under
Evidence/VerticalRuns/G5_D3D12_magic_filesystem_cache. The only diagnostic
launch change is -DDC=InstalledNoZenLocalFallback, an existing engine cache
backend defined in Engine/Config/BaseEngine.ini. The RHI, assets, rendering
settings, map, force counts and magic configuration remain unchanged. No
product configuration, global OS setting or donor asset is modified by this
launch argument. Retry outcome and graphical verdict are pending. The GPU
audit worker did not query, launch, terminate or otherwise control UE processes.
## Confirmed concurrent maintenance during G5 stalls

Jeff confirmed that maintenance is running on this machine while the G5
startup/cache stalls and broader command/file contention are being observed.
This is a confirmed concurrent workload, not proof of the sole stall cause.
The earlier G0–G4 accepted runs completed before these stalls were noticed.
Neither a GPU failure nor a tactical magic failure is established by G5's
pre-gameplay timeout. Root is holding all new Unreal launches pending Jeff's
confirmation that maintenance is paused; the already-running cache diagnostic
remains under its existing bounded runner. This audit neither stops maintenance
nor changes OS settings, and does not launch or control Unreal.
## G5 filesystem-cache diagnostic — PASS with recorded contention delay

Evidence/VerticalRuns/G5_D3D12_magic_filesystem_cache: PID 21136,
2026-09-26 04:42:50–04:47:28 UTC. Same actual donor, pools 12/10, cap five per
side and tactical RB Magic. The single launch change from the failed attempt
is -DDC=InstalledNoZenLocalFallback. Default D3D12 SM5, 1280x720 and the
30 FPS development cap remain. No global rendering/configuration change.
The cache argument is a temporary qualification measure, not a product default.

Runner elapsed 277.38 seconds; 130.69 seconds wall time after readiness,
125.47 confirmed live. PASS marker plus clean intentional exit 0, no GPU crash
signature/new crash files. Fifty-three samples: maximum 56 C and 5831 MiB
adapter-wide VRAM. Twenty warm samples excluding the final ten seconds show
UE working set 3130.96–3259.01 MiB and private commit 3908.41–4249.48 MiB.
There was substantial startup/asset delay and an observed 33-second frame
stall; confirmed concurrent maintenance remains a possible contributor, not
an established sole cause. Do not present this run as a smooth-performance
qualification or silently omit the temporary cache backend.

At 04:45:20.799 the retained log records RB_MAGIC_CAST:
spell=Magic.Spell.Fire.Firebolt, mana=72.0, activeAreas=0. The real battle
continues and resolves: won=1, survivors 5/0, waves 3/3, magic=1, contacts 137,
PBIL queries/success/orders 16/16/8, simulation duration 24.22 seconds.
Root inspected the native battle capture showing the real units/map and HUD
mana 72 / casts 1, with no broken mesh, and preserved battle/result captures.
The spell source/path, cast log and continued combat are verified; the
screenshot is NOT claimed to capture the transient Firebolt visual effect.

G5 runtime, gameplay-path and stability acceptance is PASS with these explicit
limits. Root continues to hold all new UE launches, including G6, until Jeff
confirms maintenance is paused. This successful bounded retry does not prove
that maintenance alone caused the first timeout or erase the earlier G0 reset.
## G6 campaign round trip — PASS after maintenance-paused confirmation

Jeff explicitly confirmed maintenance was paused before Root launched G6.
Evidence/VerticalRuns/G6_D3D12_campaign_roundtrip: PID 15696,
2026-09-26 04:52:21–04:54:40 UTC. Normal Entry campaign mode initializes the
encounter, travels to the actual Dragon Graveyard, resolves real combat,
returns to campaign, saves with RBSave, reloads from disk and holds the restored
campaign for 70 simulation seconds. Launch retains the successful G5 temporary
-DDC=InstalledNoZenLocalFallback backend, default D3D12 SM5, windowed 1280x720,
30 FPS development cap, and no global rendering/driver/OS changes.

Runner elapsed 139.23 seconds; readiness is explicitly the verified disk
restore, not the initial campaign load. It records 76.92 seconds wall time
and 71.72 seconds confirmed live after restore, completion marker
SOUL_CAMPAIGN_ROUNDTRIP_PASS, intentional clean exit 0, no GPU crash/new crash
files. Twenty-seven samples: maximum 56 C, 5989 MiB adapter-wide VRAM. Ten warm
restored-campaign samples excluding final teardown: UE working set
3070.47–3080.11 MiB, private commit 3816.66–3826.34 MiB.

The retained log records encounter.1.1.river_ford.orc_watch, source river_ford,
target orc_watch, map /Game/Dragon_graveyard/Level/L_showcase_level, real
human_knight/dwarf_warrior pools 12/10, cap five per side. Battle victory
survivors 5/0, waves 3/3, magic=1, accepted contacts 145, PBIL
queries/success/orders 17/17/8, simulation battle duration 24.05 seconds.
Result applies to orc_watch, then RBSave writes and reloads campaign state:
region=orc_watch, knights=5, xp=500.

Root accepted the rendered return capture: campaign graph/HUD shows Region
Dwarf Watch, Knights 5, Hero L2 XP500 and "Campaign reloaded with RB Save."
Both native battle and campaign-return captures are preserved in the run.
The post-victory checkpoint is retained at
Evidence/Persistence/rendered_victory.domain.rbsave, 1504 bytes,
SHA256 2981d715c93bb9e25a5dba42fe99b1777791005deee1f11b88a611d0e2d612af.
This is exercised campaign → encounter → Dragon Graveyard → real battle →
correct result → rendered campaign return → persisted/reloaded consequence.

## Final GPU qualification assessment

All seven staged graphical gates completed with retained logs, telemetry,
minimum post-ready observation, clean exits and Root's applicable visual
verdicts. Maximum representative force tested was 30 active real actors;
reinforcement/magic/campaign runs bounded active forces at ten while preserving
larger real strategic pools. No D3D11 or alternative RHI was used for this lane's
accepted runs. Resolution/frame cap/cache flags are explicit development aids,
not silently committed product rendering defaults.

One GPU crash occurred earlier: frame-0 D3D12 EndUpdateTexture3D DEVICE_RESET /
Aftermath Timeout, before map load or battle setup. The report/dump/hash are
preserved. Its underlying cause is UNKNOWN; no claim is made that it was fixed.
The Entry isolation and later real-map accepted ladder did not reproduce it.
No crashing scenario was brute-force repeated unchanged. One separate G5
startup timeout occurred during confirmed concurrent maintenance, with no GPU
crash and no spell execution; the one cache-backend change produced a completed
G5, and G6 completed after maintenance was confirmed paused. These observations
do not establish maintenance as the sole cause or prove a long-duration soak.
