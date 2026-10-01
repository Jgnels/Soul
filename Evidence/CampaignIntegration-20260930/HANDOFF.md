# Packaged RC2 continuation — 2026-09-30

The packaged gap below is now qualified: see [RC2_HANDOFF.md](RC2_HANDOFF.md) and `rc2-package-receipt.json`. Fresh Win64 Development full cook/archive passed; the real packaged executable passed input/exact F5/F9, cold load, victory (22/0) and defeat (0/63) returns. Founder launcher: `Tools/Open_Soul_RC2.bat` (720p, D3D11, 30 FPS). A 1080p battle attempt hit the 85°C cutoff and is explicitly rejected. Shipping is outside the current Development-only script policy. Next gate: ordinary OS mouse/keyboard and physical controller feel on the package.

---

# Active waterfront integration — 2026-09-30

This section supersedes the earlier mesa result below. Mission authority is Jeff's active integration instruction. Host `DESKTOP-Q1S3RPU`; branch `codex/soul-bannerlord-campaign-map-20260929`; starting/current HEAD `01526778807656bca32d789e747a354664ef75fd`. Changes remain uncommitted.

## Current result

Soul now streams `/Game/SoulCampaignMountain/L_evil_waterfront` in its normal campaign through the existing adapter. Licensed packages and the heightfield remain on ignored external mounts. The nine canonical regions and ten routes are identical to the prior mesa profile; their dry-ground, grade, footprint and road-clearance checks pass on the new heightfield. The 18-site settlement proposal remains planning, not 18 implemented cities.

The army summary is clickable, and Home selects/focuses the company without spending an action. Gamepad bindings extend the existing player controller: left stick pan, right stick cursor, bottom face button selection, triggers zoom, shoulders orbit, Back company, Start next day, left/top face buttons town/battle, right face button close. Selection uses the existing HUD and region dispatcher. Existing Soul rules, RB Save, battle bridge, RB Combat/PBIL and reinforcement rules remain authoritative.

## Exercised acceptance

| Gate | Receipt | Result |
| --- | --- | --- |
| Final mouse/controller input, army selection, F5/F9 | `waterfront-input-03.json` | PASS; exact saved snapshot, normal cursor rays, route movement, pan/zoom/orbit/drag, distinct controller selection, company in viewport; no duplicate-hitbox warnings |
| Fresh-process F9 | `waterfront-load-01.json` | PASS; capital start restores River Ford and exact campaign snapshot |
| Physical victory and return | `waterfront-victory-01.json` | PASS; Dragon Graveyard, 46 vs 30, waves 6/4, survivors 20/0; watch captured, mana 72, day 2, AP 2; F9 and returned fortress mouse selection pass |
| Physical defeat and return | `waterfront-defeat-01.json` | PASS; Dragon Graveyard, bounded 12 vs 70 fixture, two enemy waves, survivors 0/61; River Ford return, enemy ownership retained, mana 80, day 2, AP 2; exact F9 restoration |

All four runs exited 0 with no crash signatures; peak GPU temperature was 81 C, below the existing 85 C cutoff. Runtime Landscape alignment passed 105/105 route samples and 93/93 dry samples distributed across the map, maximum errors 0.040 and 0.283 cm. The 28 wet grid locations are deliberately excluded from the dry-ground sweep.

Final input receipt records editor DLL SHA-256 `2847b3d402250dd49e0db000d7a1bd55af4a33aff4c1d529826c50a0f27804f0`. Cold-load and battle receipts identify the preceding DLL. The final rebuild only corrected the duplicate HUD hitbox name and strengthened the controller test driver (neutral axis samples and distinct selection checks); battle/save/terrain code is unchanged between those builds. Persisted victory/defeat snapshots are retained in `waterfront-*-snapshot.json`.

Win64 Development SoulEditor and Soul builds pass. Final native suite: 69 passed, no warnings/failures/skips. Python: 66 tool tests plus 5 campaign-view checks pass. Static packaging preflight passes with 48 cook roots; no cook or packaged runtime acceptance is claimed. See `waterfront-verification.json` and `waterfront-automation.json`.

## Evidence, preservation and remaining check

Immutable PNGs and receipts: `D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/SoulIntegration/Qualification/waterfront-*`. Inspected final controller/company view, route/camera views, cold F9, physical battle, victory/fortress selection and defeat return. Logs and initial dirty/save backups are under ignored `Local/`.

The final candidate was also launched visibly at 1280×720 for OS input. Windows Computer Use found its window but screenshot capture failed with `FrameArrived timed out: timed out waiting on channel`, then `window capture timed out: timed out waiting on channel` after refreshing the window and retrying. No blind OS clicks were issued. The owned run was interrupted; it is not an acceptance pass. No Soul/Unreal editor process remained afterward. See `waterfront-os-input-01.json`.

Remaining founder/hardware check: ordinary OS mouse/keyboard and attached-controller feel. Automated controller events do not establish physical-device behavior. Launch with `Tools/Open_Soul_Campaign_Mesa.bat` (the retained launcher name now opens waterfront). No additional terrain comparison or polish is required before this check.

`waterfront-preservation.json` records preservation of unrelated dirty files, 3,667 licensed-file size/mtime checks, the waterfront map hash, and byte-for-byte restoration of the original player checkpoint and backup. Test checkpoints are archived locally; existing save history remains, including appended test versions. `git diff --check` passes. No reset, clean, stash, branch change, merge, push, publishing, purchase, credential/security change or licensed source save was performed.

---

# Earlier mesa integration receipt — 2026-09-30

Host: `DESKTOP-Q1S3RPU`. Worktree: `D:\RefinedBadger\Worktrees\Soul-bannerlord-campaign-map-20260929`.
Base commit: `5471391`, branch `codex/soul-bannerlord-campaign-map-20260929`.

## Result

The normal Soul campaign now streams approved `/Game/SoulCampaignMountain/L_mesa_coast_v2` through the existing game-owned campaign adapter. Nine existing regions and ten canonical connections have terrain-fitted anchors and dry, traversable route polylines. Canonical rules, RB Save, exploration, encounter creation and physical battle return remain the existing authorities. There is no new terrain renderer or strategic rules subsystem.

The campaign uses the actual Landscape and water; the old procedural surface is not built. Study lights/cameras are removed only from the transient streamed instance. Licensed packages and the extracted binary heightfield stay in external asset libraries through ignored junctions. No source map or material was saved. `Data/CampaignMesa/presentation.json` contains the game-owned placement profile. `Data/CampaignMesa/README.md` describes setup and scope.

## Gates exercised

Current authoritative runtime receipts are `input-bounded.json`, `load-bounded.json`, and `roundtrip-01.json`. Earlier `input-final`/`load-final` names are superseded by the bounded runs. Each current run exited 0 without crash signatures at 1920×1080, D3D11, a 30 FPS cap. These are Unreal editor `-game` runs, not packaged acceptance.

- **Navigation and selection:** actual player-controller simulated keyboard, wheel, mouse-axis and mouse-button input; viewport ray verifies the selected region. Recruitment, capital → crossroads → ford movement, action spending, hostile selection without premature movement, minimum/maximum zoom, orbit, keyboard pan, MMB drag, and Home passed. Later exploratory coverage also uses the existing campaign click handler directly; not every edge was traversed with OS mouse input.
- **Terrain/anchors:** 105/105 runtime Landscape probes passed with maximum height error 0.040 cm. All ten route centerlines stay dry; maximum measured longitudinal grade is 19.831°. Route shoulders and settlement footprints pass dry-ground checks. Rendered road triangles have zero sampled Landscape penetration. See `route-validation.json` and the five Mesa integration tests.
- **F5/F9:** save through the ordinary RB Save input path, change region by mouse, then load through F9. Exact campaign snapshot equality covers actions, army, economy, exploration and encounter state. Selection, panels, camera and physical company synchronize to the saved region. A separate cold process started at the capital and restored the prior ford checkpoint using F9, with exact snapshot and presentation checks.
- **Battle roundtrip:** mouse-selected Dwarf Watch from the ford, pressed the normal B input, created `encounter.2.1.river_ford.orc_watch`, and opened `/Game/Dragon_graveyard/Level/L_showcase_level`. Forces were 46 versus 30 with 15 active units per side plus reserves. Physical combat and RBMagic Firebolt resolved to victory: 24 allied / 0 hostile survivors, mana 72. Return restored the approved terrain, human ownership of the watch, army 24, XP 500, gold 4160, day 2 and 2 remaining actions. Deliberately disturbed live return state was recovered through F9 with exact snapshot equality. A subsequent real viewport mouse click selected Dwarf Stronghold and opened its battle prompt. The returned campaign remained alive for 70 seconds and exited cleanly.

The F9 fix adds a transient successful-load revision observed by the existing campaign actor. It cancels stale presentation travel and resynchronizes the UI/company/camera; it does not change the save schema. Terrain integration also corrected company/region scale mismatch, route endpoint shortcuts, cottages intersecting routes, road ribbon clipping, and maximum-zoom camera framing beyond the map edge. These corrections came from runtime/screenshots, not a further donor study.

## Evidence and reproduction

Screenshots and immutable receipts live outside Git:
`D:\RefinedBadger\AssetLibraries\SoulTerrainPreview\SoulIntegration\Qualification\`.
Each receipt records exact terrain/profile/editor DLL hashes, assertions, screenshot hashes/dimensions, exit state and GPU telemetry. Inspect:

- `input-bounded\input-bounded_wide_bounds.png` for maximum zoom and bounded framing;
- `input-bounded\input-bounded_close_orbit.png` for route/company clearance;
- `load-bounded\Campaign_Cold_Load.png` for cold F9 restoration;
- `roundtrip-01\Vertical_Battle.png` for physical battle;
- `roundtrip-01\Vertical_Campaign_Return.png` and `Campaign_Stronghold.png` for return and fortress selection.

The initial battle screenshot includes fallback/checkerboard environment materials during first-use shader preparation. It verifies physical gameplay, not finished battle art. Campaign route/anchor screenshots were inspected after warmup. No new visual polish was added.

Launch ordinary play with `Tools/Open_Soul_Campaign_Mesa.bat`. Controls: WASD pan, wheel zoom, MMB drag, Q/E orbit, Home company focus, left-click region, T town, B battle, Space next day, F5 save, F9 load, Esc close. Use `Tools/Setup_Soul_Mesa.ps1` to verify mounts. Junctions are not an ACL write barrier: never save donor/derived source assets through them.

Reproduce automated gates with `Tools/qualify_soul_mesa.ps1 -Mode Input -Run <fresh-name>`, then `-Mode Load`, then `-Mode Roundtrip`. Input/Load must run in that order because Load compares the preceding F5 snapshot. Qualification uses the normal scoped RB Save slot; preserve a player checkpoint first.

## Verification scope and next gate

Both Win64 Development builds passed: SoulEditor in 66.11 seconds and Soul in 176.98 seconds (`build-editor-result.json`, `build-game-result.json`). The final native `Soul.` suite passed all 69 tests with zero warnings/failures/skips and process exit 0 (`automation-final-receipt.json`, `automation-final-result.json`). Python checks: 66 tool tests plus 5 campaign-view checks passed (`source-checks.json`). `git diff --check` returned 0. Static package preflight passed with 48 exact cook roots and the Mesa profile/height staging entries. Static preflight does not establish cook or packaged runtime success.

`preservation-final.json` verifies all five pre-existing dirty files by SHA-256, unchanged size/mtime for all 3,652 monitored licensed files, and the approved map hash. The original campaign save and backup were restored byte-for-byte after qualification; the final test save was copied into ignored local evidence first. Existing save history was retained; test runs appended history entries. Peak GPU temperature across the three final runtime gates was 70 C. Changes remain uncommitted in this worktree; `CHANGED_FILES.txt` identifies this lane's files, excluding the pre-existing dirty state.

The user-facing RC2 gaps now have controller-level runtime evidence on the integrated terrain. OS-level manual input, gamepad behavior, packaged RC2 acceptance and production art approval are not claimed. Existing scenario geography/recipe tags and Dragon Graveyard selection remain unchanged; this integration does not invent western-mesa regions, new lava rules, or an artificial river for the retained `river_ford` ID.

The next founder gate is ordinary play of this integrated campaign: camera/readability, region selection, F9 and battle/return feel. This can use the launcher above. No further terrain donor comparison is needed. No push, publish, merge, reset/clean, credential edit or licensed source edit was performed.
