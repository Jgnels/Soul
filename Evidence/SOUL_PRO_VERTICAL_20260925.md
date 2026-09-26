# Soul vertical integration — 2026-09-25

**PASS — SOUL VERTICAL LOOP.** Implemented, exercised and committed on the isolated worker branch.
Rendered **G0–G6 PASS**, including the normal campaign → actual Dragon Graveyard
→ real battle → campaign consequence → disk save/mutate/reload path. An earlier
unexplained startup GPU reset did not recur through the accepted gates.

## Git

- Authorized host: DESKTOP-Q1S3RPU; worker:
  `D:/RefinedBadger/Worktrees/Soul-pro-vertical-20260925`.
- Worker branch: `astra/soul-pro-vertical-20260925`.
- Inspected all 19 registered Soul worktrees, branches, dirty state and Unreal
  processes before integration. Existing work was preserved.
- Integration base: `68fdf8a322870ffce942cac332fca581c19015ef`, latest available
  realtime qualification, retaining `da52545`/`34f4d66` reinforcement work.
- Dragon source: `13ebaf595812259e1ccd910c0532c44312461154`, newer than the
  supplied `596bc5d`; merged as `c3921fdb76da9e51192914c5fd7bc0691dbbd0db`.
- Campaign source: `d47e251584bc554ff756755ea6fce4c307df6b7e`; merged as
  `7a1896177aba160c28b3c1ef355154429c73590c`, the pre-implementation merge checkpoint.
- Canonical `d1d86593e03bb957f6049b2408f245b78bcef56b` on
  `astra/soul-primary-continuation-20260920` had untracked founder runtime;
  relevant files were copied selectively, leaving originals unchanged.
- Also inspected battlefield-environments `cb3c3e0`, overmap-integrated
  `3624bf6`, and remaining registered branches. Historical downstairs scale
  commits `19fbed3`/`a6ec602`/`52f9b08` were absent from available refs and transfer
  bundle (ends `f82f477`); their reconciliation is not claimed.
- Resulting implementation/runtime/test-evidence commit:
  **`5802f5311d15bf8171f5262d72f34cf81ea335aa`**.
  This receipt's containing commit records final acceptance of that implementation.
  No push, canonical merge, history rewrite or unrelated cleanup performed.
- Generated Android File Server credential settings are intentionally excluded
  from the index; the local `Config/DefaultEngine.ini` therefore remains dirty.
  No credential value is included in this receipt.
- Worker-only build adjustment: disabled the installed Cargo plugin because its
  missing `KitBash3dUsdTools` dependency prevents this checkout from building.
  This does not replace any Soul or RB gameplay authority.

## Systems

| Responsibility | Existing authority and integration |
|---|---|
| Campaign | `USoulFounderPlaytestStateSubsystem`, accepted overmap/start/handoff data, and founder campaign interaction |
| Encounter/result | `SoulCore`'s `USoulCampaignBattleBridge`; explicit encounter/source/target identities, factions, units, strategic pools, active cap and map; one notification per accepted result |
| Physical battle | `ASoulRealtimeArenaGameMode` through existing RB Combat; real contacts, deaths and survivor ledger |
| PBIL | Existing RBPBIL facade and TCAT CPU refresh policy; real occupancy drives approach queries and RB Combat group orders |
| Reinforcements | Existing `FSoulRealtimeBattleRules`; bounded active force, reserve debit after physical spawn, victory waits for hostile reserves |
| Magic | Existing RB Magic Firebolt asset/library/authority path; tactical cast originates from a living unit |
| Persistence | Existing RB Save atomic domain backend; explicit `Soul.Campaign` and `Soul.Settlements` checkpoint, unchanged format |

The supported representative matchup is Humans `human_knight` (Knight02) versus
Dwarves `dwarf_warrior` (Bedvar). Unknown or reversed faction/unit identities are
rejected rather than silently selecting these meshes. The scenario retains
legacy geography IDs `orc_watch` and `orc_camp`, explicitly changes their owner
to Dwarves and displays Dwarf Watch/Stronghold. Results target the committed
encounter, with deterministic coverage for both regions.

Hostile travel cannot precede battle. Victory applies actual player survivors,
depletes the defeated garrison and captures that target once. Defeat returns to
the source, preserves surviving enemies and does not grant capture or victory XP.

RB Save originally failed because an unused registered RBItemEconomy provider
required an initialized server economy. The reusable scoped API preserves strict
all-domain behavior; it does not unregister authorities or fabricate payloads.
Explicit selection rejects missing/duplicate/unknown domains and fails if a
selected provider fails. See `Evidence/SOUL_RBSAVE_SCOPE_20260925.md`.

## Tests

| Check | Verified result | Evidence |
|---|---|---|
| Existing non-UE overmap baseline | 41/41 PASS | `Evidence/SOUL_OVERMAP_BASELINE_20260925.log` |
| Same overmap suite after integration | 41/41 PASS | `Evidence/SOUL_OVERMAP_FINAL_20260925.log` |
| Soul automation, exact final source / NullRHI | 52/52 Success; zero warnings, failures or not-run; 4.357649 s | `Evidence/automation_soul_qualified_final.log`; `Evidence/AutomationSoulQualifiedFinal/index.json` |
| RB.PBIL automation | 4/4 PASS; one benign world-cleanup warning | `Evidence/automation_pbil.log`; `Evidence/AutomationPBIL` |
| Knight/Dwarf animation compatibility | 1/1 Success | `Evidence/automation_animation.log`; `Evidence/AutomationAnimation` |
| Bounded GPU runner safety | 9/9 mocked tests PASS | `Evidence/SOUL_GPU_RUNNER_FINAL_20260925.log` |
| Scoped-save / qualification editor builds | Succeeded, 266.24 s / 99.20 s | `Evidence/vertical_build_save.log`; `Evidence/vertical_build_qualification.log` |
| Latest qualified editor binary | Succeeded, 36.57 s | `Evidence/vertical_build_combat_camera.log` |

The 41-step suite is `python Tools/run_soul_overmap_nonue_acceptance.py`: a
non-graphical data/contract regression, not proof of the runtime round trip.
The final 52-test run covers the strengthened scoped-selection validation and
all final source changes. Its 11 focused vertical tests include:
`EncounterContract`, `VictoryCorrectTarget`,
`DefeatConsequence`, `BridgeRejectsPrematureVictory`, `RBSaveDomainRoundTrip`,
`SupportedMatchupContract`, `RBSaveScopedDomains`,
`CasualtiesCannotConsumeUnspawnedReserve`, `LastActiveDeathRetainsHostileReserves`,
`SequentialWavesPreserveLedgerAndCaps`, and `KnightUE4DwarfAnimationCompatibility`.
These exercise target identity,
hostile ordering, inverse outcome, survivor/XP/ownership persistence, invalid
restore rejection, explicit save scope, reserve conservation and active caps.

## Battle and graphical gates

Real map: `/Game/Dragon_graveyard/Level/L_showcase_level`; measured battle origin
`(2000,-15000,0)`. Normal campaign scenario has real strategic pools **12/10** and
active cap **5 per side**. Qualification changes counts explicitly.

All completed graphical gates below used default **D3D12 SM5**, **1280×720
windowed**, and a reversible development **30 FPS** cap. G1–G5 held for 60 seconds
after the real battle result; G6 held the returned
campaign for 70.03 seconds after verified persistence. Duration below is confirmed
live time after map readiness, except G6 measured after the save/reload VERIFIED
marker. These gates require sustained observation, not merely a successful load.

| Gate | Status | Active units | Confirmed live | Result / evidence under `Evidence/VerticalRuns` |
|---|---|---:|---:|---|
| G0 real map only | PASS | 0 | 76.83 s | Actual bones/terraces/clearing visible; `G0_D3D12_final` |
| G1 real minimum combat | PASS | 1v1 | 61.69 s | Victory 1/0; 15 contacts; PBIL 2/2/2; readable contact; `G1_D3D12_combat_camera` |
| G2 small squad | PASS | 5v5 | 72.03 s | Victory 1/0; 76 contacts; PBIL 2/2/2; coherent real-unit approach/melee; `G2_D3D12_5v5` |
| G3 representative | PASS | 15v15 | 72.11 s | Victory 5/0; 216 contacts; PBIL 4/4/4; `G3_D3D12_15v15` |
| G4 reinforcements | PASS | Cap 5/side; pools 12/10 | 82.75 s | Three waves/side; victory 5/0; `G4_D3D12_reserves` |
| G5 RB Magic | PASS | Cap 5/side; pools 12/10 | 125.47 s | Real Firebolt, mana 80→72, one cast; `G5_D3D12_magic_filesystem_cache` |
| G6 campaign round trip | PASS | Cap 5/side; pools 12/10 | 71.72 s after VERIFIED | Campaign consequence + disk reload + return hold; `G6_D3D12_campaign_roundtrip` |

G4 delivered human 2+2+2 and enemy 2+2+1 reinforcements. Victory after 24.20 s
retained four living human actors plus one undeployed strategic reserve; hostile
active/reserve counts were both zero. It recorded 142 contacts and PBIL 16/16/9.

G5 cast RB Magic Firebolt through the reusable authority: mana changed 80→72,
cast count one, and battle continued to victory 5/0 after 24.22 s, with three
waves per side, 137 contacts and PBIL 16/16/8. Its first attempt timed out before
readiness (339.80 s, no GPU crash); the preceding logs contained Zen failure.
Jeff confirmed concurrent machine maintenance. One controlled retry changed
only the engine cache launch argument to `-DDC=InstalledNoZenLocalFallback`;
RHI/rendering/map/units stayed unchanged. That retry exited 0 after 125.47 s of
confirmed live observation (277.38 s total). It establishes the G5 gameplay
result, not a proven root cause for the earlier cache/startup stall. The DDC
override is a temporary process launch argument, not a committed product default.

All committed accepted G0–G6 summaries record exit 0, no crash and at least
60 seconds of confirmed live observation. PBIL counts are
queries/successful queries/orders. Reinforcement result fields count **waves**;
physical reserve admissions are separately logged as unit counts.

### Environment diagnosis and installed assets

The misleading historical repeated-block view was partly a view-authority bug:
proof mode never assigned the battle observer and showed obstructed world
origin despite different arena locations. Observer placement is now explicit.
A second defect interpreted donor exposure bounds −0.5…0 EV100 as legacy
luminance; map-local runtime conversion to 0.707107…1 restored visible scenery.

Read-only audits found 420 authored actors, 439 mesh components and 25 unique
meshes, with no level instances, Cube substitution or 12-triangle fallback.
The donor itself repeats rock meshes (93/81/56 instances of its three main
rocks), including large perimeter scales. That authored repetition remains;
it was not removed or disguised as a new environment. Final G0 visual review
accepted the actual ribcage/spine, rock terraces and battle foreground.

`Evidence/DragonGroundContract.json` records 1,892 real collision traces, zero
errors. All nine current spawn-area samples hit flat Landscape at Z=100; an
XY-overlapping lava plane lies 38.53 cm below it. `DragonRenderContract.json`
and the GPU receipt preserve the mesh/LOD audit and its query limitations.

**Local dependency:** `Content/Dragon_graveyard`, `Content/Knights_Pack` and
`Content/Dwarf_Pack` are read-only junctions to corresponding directories under
`D:/Unreal Projects/AoEAssetRenderLab/Content`. The Git branch does not contain
these donor assets and is not a self-contained transferable package.
`Content/MagicSpells` is also an ignored read-only junction, targeting
`D:/RefinedBadger/Worktrees/Soul-dragon-graveyard-proof-20260922/Content/MagicSpells`.
That installed spell asset directory is required to reproduce RB Magic execution.

## GPU

One new GPU crash occurred in `G0_D3D12_graveyard_framed`: frame-0
`DXGI_ERROR_DEVICE_RESET`, active `EndUpdateTexture3D`, Aftermath Timeout / Adapter
Reset, before map load or battle initialization; zero units. Cause **UNKNOWN**.
The crash report/dump and checksum are preserved under that run. It does not
establish a camera, map-geometry, combat-load or VRAM-exhaustion cause.

A single-variable Entry-map diagnostic and subsequent real-map G0–G6 runs
completed without recurrence. Accepted G0–G4 gates reached at most 65°C; measured
post-warmup process memory was bounded in these short runs. This is not a long
soak or proof that the startup fault is fixed. Historical GPU/renderer failures
remain separately recorded. No RHI, OS/driver setting or product quality was
changed. Exact telemetry, durations, crash paths and earlier failed visual runs:
`Evidence/SOUL_GPU_STABILITY_20260925.md`.

## End-to-end

**PASS, rendered on default D3D12 SM5:** normal Entry campaign → encounter →
actual Dragon Graveyard → real battle → real result → campaign → persistence.
`Evidence/VerticalRuns/G6_D3D12_campaign_roundtrip` preserves the launch, complete
log, native captures, telemetry and runner summary. G6 ran after maintenance
was paused, with the same engine-provided filesystem DDC launch override as G5.

The normal campaign action path committed encounter
`encounter.1.1.river_ford.orc_watch`, source River Ford, target Dwarf Watch
(`orc_watch`). Real pools **12/10**, active cap **5 per side**, three reinforcement
waves per side and one tactical **RB Magic Firebolt** resolved to victory with
**5/0 survivors**, **145 accepted contacts**, and PBIL **17/17/8** in **24.05
simulation seconds**. The correct target became human-owned. The returned
campaign HUD was visually confirmed at Dwarf Watch with five knights and
**500 XP** (140 campaign + 360 battle).

RB Save wrote the actual disk checkpoint. Qualification deliberately changed
live location, army and XP, loaded the disk file, and verified full campaign
snapshot equality. It then held the returned campaign for **70.03 simulation
seconds**, emitted `SOUL_CAMPAIGN_ROUNDTRIP_PASS` and exited 0. The runner measured
**139.23 s total**, **71.72 s confirmed live after VERIFIED**, with no GPU crash.
The inverse defeat path has deterministic result/save tests; a rendered defeat
is not claimed.

- Runtime file: `Saved/RBSave/Domains/Soul.VerticalCampaign.domain.rbsave`.
- Rendered checkpoint: `Evidence/Persistence/rendered_victory.domain.rbsave`,
  1,504 bytes; SHA-256:
  `2981D715C93BB9E25A5DBA42FE99B1777791005DEEE1F11B88A611D0E2D612AF`.
- Earlier real NullRHI round trip also passed: `Evidence/headless_campaign_saved_roundtrip.log`,
  survivors 5/0, waves 3/3, one cast, 146 contacts, PBIL 16/16/7, battle 24.28 s,
  actual disk reload and 70.00 s return hold. Preserved
  `Evidence/Persistence/headless_victory.domain.rbsave` is 1,504 bytes, SHA-256
  `D230DD0C38A3551F8DE2B90D4AC52233A76CDEB23B7B31FC61E6812629D2A931`.

### Minimal manual route

Launch the isolated worker's normal default campaign in PowerShell:

    & 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\RefinedBadger\Worktrees\Soul-pro-vertical-20260925\Soul.uproject' /Engine/Maps/Entry -game -windowed -ResX=1280 -ResY=720 -ExecCmds='t.MaxFPS 30'

Click Crossroads, River Ford, then Dwarf Watch; press **B**. The campaign mode
opens Dragon Graveyard and returns with the physical battle result. **F5** saves;
**F9** loads the scoped campaign checkpoint. No proof-map or developer-only route
is required. Resolution and frame cap above are development qualification aids.

## Remaining blockers

No functional or graphical blocker remains for this qualified representative loop.
Implementation and runtime/test evidence are committed on the isolated branch.

Residual risk: the earlier unexplained startup GPU reset remains unresolved,
although accepted G0–G6 completed without recurrence. This bounded qualification
does not establish a permanent GPU fix or long-term soak. Installed donor
junctions remain required to reproduce the qualified local build.
