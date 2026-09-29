# Soul campaign-world handoff — 2026-09-29

## FACT

Worker: `D:\RefinedBadger\Worktrees\Soul-bannerlord-campaign-map-20260929`
Branch: `codex/soul-bannerlord-campaign-map-20260929`
Base: `6d53714559d34a123b5394206866925a165373c4` from `astra/soul-finalproject-consolidated-20260927`.
Qualified source HEAD: `0111b28f8b40a282e662277367f78bece67f7d2d`. The following evidence-only commit contains this handoff; use `git log -6 --oneline` to identify it.

Local commits, oldest first:

- `867d616` — explicit random-stream includes needed by SoulCore without a PCH.
- `1dc495a` — continuous campaign terrain, routes, location silhouettes, party, exploration treatment, perspective navigation and compact HUD.
- `9830306` — framing, terrain/river correction, recovery feedback and runtime qualification.
- `0d0b9ac` — explicit World/GameInstance dependencies in the existing foundation adapter; no behavior change.
- `0111b28` — final silhouettes, ambient readability, labels, input coverage, remembered-territory information guard and B-key battle-entry qualification.

No push or merge was performed. The live consolidated checkout remains at the verified base with its original dirty/untracked paths. None were copied into the worker. After the evidence commit the worker should have no tracked or untracked changes outside ignored local runtime artifacts.

### What changed

The nine-region strategic topology now appears on one elevated terrain surface with valleys, ridges, a continuous river, ten curved terrain-following routes, forest/crop instances and faceted rocks. Capital, road junction, bridge/ford, quarry, shrine, forest settlement, watch, pass and stronghold have different silhouettes. Restrained standards show current ownership; small groups represent the company and visible defenders. Movement changes canonical strategic state immediately, then animates the company along the same physical road. No second movement simulation was introduced.

The perspective camera has oblique depth, smooth WASD/drag pan, wheel zoom, limited Q/E orbit, Home focus, bounds and terrain clearance. The campaign HUD uses a compact resource strip, selected-place context, town/recruitment controls, skill choices and a battle result card. Unknown places remain unselectable and concealed in a spatial terrain veil; remembered places remain visible but do not disclose current ownership/forces. Roads to unknown places are hidden. Current authoritative scenario names are **humans versus dwarves**, while existing `orc_*` region IDs remain intact.

`USoulFounderPlaytestStateSubsystem`, `FSoulWorldRules`, campaign simulation, battle bridge and realtime battle implementation remain authoritative. Their simulation behavior was not replaced. Hidden cube collision proxies support existing clicks. No visible sphere regions or permanent campaign debug-line routes remain.

### Exact verification

| Check | Result |
|---|---|
| Baseline campaign-view / Tools tests | 2/2 and 54/54 passed |
| Baseline Unreal `Soul.` suite | 67 passed, 0 failed, 0 not run |
| Final campaign-view source checks | 5/5 passed |
| Final Tools unittest discovery | 55/55 passed |
| Final Unreal `Soul.` suite | **69 passed, 0 warnings, 0 failed, 0 not run**; 4.3812s test execution |
| Soul Win64 Development | Built successfully; final incremental 53.19s |
| SoulEditor Win64 Development | Soul + SoulCore + SoulRealtimeBattle + RBFoundationAdapters built successfully; final incremental 17.51s |
| Whitespace/diff validation | `git diff --check` passed |
| Rendered campaign, 1280x720 | All input/visibility assertions passed; normal exit 0; 113.25s |
| Rendered campaign, 1920x1080 | All input/visibility assertions passed; actual PNG dimensions verified; normal exit 0; 112.98s |
| Real Dragon Graveyard stress round trip | Victory, correct strategic return, RBSave restoration, 70s return observation; normal exit 0; 164.75s |
| Real defeat/recruit/retry | Two resolved encounters, finite recruitment, both returns and RBSave restorations; normal exit 0; 122.84s |

The complete 69-test inventory/results are in [automation-final-result.json](automation-final-result.json). Coverage includes vertical campaign victory/defeat, survivor and ownership persistence, encounter identity/stale-result rejection, multiple encounters, save/load/recovery, finite recruitment, skill progression, battle bridge and existing realtime behavior. Two added Unreal tests check terrain/routes/camera/selection invariants. The five source checks replace obsolete orthographic expectations.

Rendered input qualification used simulated events through the real player controller: location cursor raycasts/clicks, a town HUD button, number-key recruitment, wheel/WASD, Q orbit, MMB mouse-axis drag, Home, Space, Escape, F5 and F9. Additional campaign action-path checks exercised legal exploration and rejected distant friendly/hostile selections identically without consuming actions. The final stress run commits via the actual B-key binding. This is rendered Unreal execution, not a static screenshot mockup or an OS-level manual playtest.

### Battle result and integration evidence

Stress arguments were exactly `-SoulActivePerSide=35 -SoulPlayerPool=100 -SoulEnemyPool=100 -SoulRealtimeVisualUnits -SoulRealtimeMagicProof`.

- Actual map: `/Game/Dragon_graveyard/Level/L_showcase_level`.
- 35 active + 65 reserves per side at entry; **1,562 accepted RB Combat contacts and 34 reinforcement waves** recorded.
- PBIL CPU grid initialized at 66x46; RB Magic Firebolt cast, mana 80 -> 72.
- Encounter `encounter.1.1.river_ford.orc_watch` resolved as victory, **6 allied / 0 hostile survivors**.
- Player region `orc_watch`, owner `humans`, matching survivor counts/mana, resolved encounter recorded and pending encounter cleared.
- Return save loaded after deliberate changes to player region, army count and XP; complete captured save-domain snapshot matched.
- Earlier stress run also passed with 3/0 survivors. Physical battle results vary; this is not a deterministic casualty claim.

Recovery began with 1 versus 100. After defeat, the player returned to `river_ford`, ownership remained `dwarves`, survivors were 0/100. Normal travel back to capital and three recruits spent three finite pool entries and the correct gold. Retry used fresh ID `encounter.3.2.river_ford.orc_watch`, then a second real defeat returned correctly and restored its save. The inspected final image shows an empty fighting company and recruitment guidance. Receipts preserve both identity and consequence assertions.

### Visual evidence and acceptance

All final images below were opened and inspected. Multiple observed failures were corrected across eight numbered visual passes, followed by final qualification at both resolutions; see [visual-iterations.md](visual-iterations.md).

- Before/after, same 1280x720: [before](before-1280.png) / [after](after-1280.png).
- [Full-HD initial campaign](after-1920.png), [frontier/ford](frontier-1280.png), [hostile battle prompt](battle-prompt-1920.png), [quarry/shrine](shrine-1920.png).
- [Close zoom/orbit](close-orbit-1280.png), [maximum zoom/pan](wide-bounds-1280.png).
- [Real Dragon Graveyard](dragon-battle-final-1280.png), [conquest return](roundtrip-return-final-1280.png), [hostile stronghold](stronghold-final-1280.png), [defeat/recovery return](recovery-return-final-1280.png).

CORE gates exercised:

- [x] Isolated worker branch/worktree; unrelated live state preserved.
- [x] Existing strategic authority preserved; no parallel campaign truth.
- [x] Continuous 3D campaign world replaces the visible node-link graph, spheres and debug roads.
- [x] Perspective camera, pan, drag, zoom, bounded orbit and terrain clearance work.
- [x] Terrain depth, major location identities and legal physical roads are visible.
- [x] Human/hostile ownership, company, defenders, selection and exploration states are readable.
- [x] Compact campaign HUD; selection, movement, finite recruitment and save/load exercised.
- [x] Battle commit enters real Dragon Graveyard with consolidated code.
- [x] Victory and defeat return to campaign; strategic consequences and saved state verified.
- [x] Relevant automated tests and both affected target builds pass.
- [x] Rendered evidence exists at both requested resolutions; before/after and final screenshots inspected.
- [x] Licensed donor content excluded from Git and not edited/reserialized.

### Performance and isolation

Campaign construction: 123,201 terrain vertices, ten routes, 455.41ms at 720p / 400.11ms at 1080p. Decorative trees/buildings use instanced components; terrain knowledge updates only when its signature changes, and party Tick disables when travel finishes. No dynamic materials are constructed per frame.

At the intentional 30 FPS cap, campaign samples averaged **29.25 FPS (720p)** and **28.50 FPS (1080p)**. Samples include screenshot capture and its reported 400ms maximum frame; they are not steady-state uncapped benchmarks. Final stress peaked at 83 C GPU temperature, 6,919 MiB total GPU memory and 3,110.82 MiB process working set. Initial total GPU allocation was already 6,097 MiB. Recovery peaked at 7,062 MiB total GPU memory. No crash signatures or abnormal exits occurred in the four final rendered runs.

Licensed content is available only through worker-local junctions for `Dragon_graveyard`, `Knights_Pack`, `Dwarf_Pack` and `MagicSpells`, targeting the existing proof worktree. No donor save operations were used. Dragon Graveyard's 95-file length/UTC-modification-time manifest is unchanged; [donor-verification.json](donor-verification.json). No files from these four directories are tracked. Only the two new `/Game/Soul/Campaign/` materials are source-owned and committed.

### Measured blockers resolved / local artifacts

- Incremental MSVC PCH compilation stalled on this machine. Builds succeeded with `-NoPCH -NoUBA -MaxParallelActions=2`; project PCH settings were not changed. This exposed pre-existing transitive include dependencies; the two header-only fix commits make them explicit.
- Early missing material shaders, incorrect color conversion, road winding, river occlusion, label scale, camera edges and HUD-obscured test clicks were found from rendered evidence and corrected.
- UE clamped an early requested 1080p viewport to 888x500. That run was rejected; `-ForceRes` and verified PNG dimensions are used in final qualification.
- Sandbox helper setup failed for ordinary command/image tools. Approved elevated shell execution was used. One automatic review rejected an ambiguous cleanup; narrower worker-only cleanup was subsequently approved. No unresolved approval block remains.
- UE's AndroidFileServer startup generated worker config noise on an early run. It was disabled for all subsequent runs; worker config was restored only after provenance review. The live dirty config was untouched.

Ignored and intentionally uncommitted: `Evidence/CampaignWorld/Local/` full logs, telemetry, automation HTML/report assets and temporary evidence collectors; `Saved/`, `Intermediate/`, `Binaries/`, runtime saves/screenshots/caches; the four licensed-content junctions. No generated builds, donor assets or giant logs are included in the commits.

### Reproduce locally

For normal play, launch the built editor with the worker project and `/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode`, `-game -windowed -ResX=1280 -ResY=720 -ForceRes -d3d11 -DisablePlugins=AndroidFileServer -DDC=InstalledNoZenLocalFallback`. Omit qualification flags. WASD/MMB pan, wheel zoom, Q/E orbit, Home company focus; click nearby places, T town, 1 recruit, Space next day, B battle, F5 save, F9 load.

Exact automated runtime command arrays are retained in the four `*-final-result.json` / `qualified-*-result.json` receipts. They use `Tools/qualify_soul_vertical.py`; full source and automation commands/results are recorded above and in the local logs.

## INFERENCE

The inspected result meets the directional category change: a stylized 3D fantasy strategy world with physical geography and places. It is a playable vertical-slice presentation, not production-level Bannerlord art. The final core gates passed before eight hours; this handoff uses the user's explicit early-finish exception instead of extending the lane with unrelated work.

Remaining production-quality work is primarily art and UX: authored terrain/biomes and riverbanks, textured environment assets, richer settlement silhouettes and animation, better road crossings, more atmospheric sky/water, a dedicated widget/style system, and replacing the existing battle debug HUD. The current nine-place topology and aggregate miniature forces remain deliberately small. These are visual/production gaps, not claims of additional completed systems.

## UNKNOWN

A cooked/packaged build was not produced or played in this lane. `Soul.exe` compiles, but runtime acceptance is **UnrealEditor -game under D3D11**, rendered offscreen. Multi-hour campaign soak, uncapped performance, other GPUs/RHIs, controller navigation, audio and arbitrary aspect ratios were not qualified. Donor verification uses file size/mtime, not a new full cryptographic asset audit. No claim of shipping or AAA visual quality is made.

Next meaningful action: review the before/after and conquest/recovery images, then play this isolated branch on the same machine. A later packaging/art-production lane can start from this qualified source commit; merging remains Jeff's decision.

## Exact source-owned files changed from base

- `Config/DefaultGame.ini`
- `Content/Soul/Campaign/M_CampaignSurface.uasset`
- `Content/Soul/Campaign/M_CampaignTerrain.uasset`
- `Plugins/RBFoundationLegacyAdapters/Source/RBFoundationAdapters/Private/RBFoundationRBAdapterSubsystem.cpp`
- `Soul.uproject`
- `Source/Soul/Private/SoulCampaignCamera.cpp`
- `Source/Soul/Private/SoulCampaignVisualQualification.cpp`
- `Source/Soul/Private/SoulCampaignWorldActor.cpp`
- `Source/Soul/Private/SoulFounderPlaytestCampaignActor.cpp`
- `Source/Soul/Private/SoulFounderPlaytestGameMode.cpp`
- `Source/Soul/Private/SoulFounderPlaytestHUD.cpp`
- `Source/Soul/Private/SoulFounderPlaytestPlayerController.cpp`
- `Source/Soul/Private/SoulPlaytestRegionActor.cpp`
- `Source/Soul/Private/Tests/SoulCampaignWorldTests.cpp`
- `Source/Soul/Private/Tests/test_weekend_campaign_view.py`
- `Source/Soul/Public/SoulCampaignCamera.h`
- `Source/Soul/Public/SoulCampaignWorldActor.h`
- `Source/Soul/Public/SoulFounderPlaytestCampaignActor.h`
- `Source/Soul/Public/SoulFounderPlaytestGameMode.h`
- `Source/Soul/Public/SoulFounderPlaytestHUD.h`
- `Source/Soul/Public/SoulFounderPlaytestPlayerController.h`
- `Source/Soul/Public/SoulPlaytestRegionActor.h`
- `Source/Soul/Soul.Build.cs`
- `Source/SoulCore/Public/SoulCombat.h`
- `Source/SoulCore/Public/SoulRetreat.h`
- `Tools/create_soul_campaign_materials.py`
- `Tools/package_soul_weekend.ps1`
- `Tools/qualify_soul_vertical.py`
- `Tools/test_package_soul_weekend.py`
- `Tools/test_qualify_soul_vertical.py`
- `Tools/test_soul_campaign_input.py`

The accompanying `Evidence/CampaignWorld/` files are review receipts/screenshots only; its exact tracked inventory is available through `git ls-files Evidence/CampaignWorld`.
