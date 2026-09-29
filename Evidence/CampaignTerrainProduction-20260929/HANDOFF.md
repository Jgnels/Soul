# Campaign Terrain V2 - production acceptance NOT achieved

Worker: `codex/soul-bannerlord-campaign-map-20260929` at `D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929`. Baseline: `c36e37196b69f065f734828e6c175aa9ba12e755`. Implementation/evidence commit: `1687e311b8eafbb20787cc0b6e861bc4ba8b5065` (155 files). This HANDOFF is an evidence-only follow-up; its commit is discoverable with `git log -1 -- Evidence/CampaignTerrainProduction-20260929/HANDOFF.md`. No push, merge, reset, clean, rebase, purchase or publication.

**This is an opt-in Landscape engineering checkpoint. Rendered evidence does not support the requested modern Bannerlord-style production category.** Keep the original procedural presentation as default. Do not call the terrain production objective complete.

## Exact changed-file inventory

[CHANGED_FILES.txt](CHANGED_FILES.txt) lists all 156 worker files, including this follow-up handoff. Runtime changes are limited to campaign presentation/camera/input-range integration, the Soul Landscape dependency/staged data, and opt-in benchmark/capture hooks. Owned TerrainV2 assets and source inputs are isolated. Tools add bake/import/build/archive/qualification support and extend cook preflight/source checks. The remaining files are immutable qualification evidence. No canonical campaign data, state/rules subsystem, battle module or donor asset is included.

## Architecture and external tools

`-SoulTerrainV2` streams `/Game/Soul/Campaign/TerrainV2/L_FounderTerrain` into the existing campaign and battle-return path. Canonical nine region IDs, ten founder connections, campaign state/rules, RB Save and battle authority remain unchanged. No second weather or optimization authority. Presentation data is in `Data/CampaignTerrainV2/presentation.json`, interpreted by SoulCampaignTerrain. Geography is scaled 20x; unit miniatures use one-quarter that scale to fit crossings.

Landscape: 1009 x 1009 samples, 64 components, 2 x 63-quad subsections, 5.6 km extent; roughly 1.3 km playable frontier. Height in cm = (RAW16-32768)*2.5. XY spacing 555.5555556 cm; location (-280000,-280000,0); Z scale 320. Final elevation range -15.05 to 305.525 m. Height SHA-256: `c8ce0cac1165738979233c5b1cfc5e717fbd3b377a5e70046ad2c9ff7fefb606`.

The deterministic owned bake supplies authored ridges, graded routes, river/floodplain, two tributaries, terraces, quarry benches, twelve fields and three road/river crossings. Bridge deck heights are shared by road and party presentation. Runtime height queries match Unreal Landscape triangle interpolation: 70 actual collision probes agree within 0.333 cm, including after battle return. Height/terrain never decides move legality.

Ground has macro/biome maps and licensed detail textures/normals. Roads use feathered dirt shoulders. Water remains a procedural surface with animated normals and needs more art work. Decorative trees, rocks, hedges and buildings use HISM in accordance with the inspected RB Optimization representation policy. Interactive regions remain separate. Road/bridge visibility, region selectability and visible/remembered/unknown information gates are preserved. No AI-generated player art.

No commercial-capable Gaea, World Creator or World Machine license was verified in the bounded local application/uninstall inspection. This does not establish ownership elsewhere. Native work proceeded; no community/evaluation output entered Content. See [external bakeoff/import contract](../../SourceArt/SoulCampaignTerrain/EXTERNAL_BAKEOFF.md), [tool inspection](terrain-tool-local-inspection.json), and [architecture](ARCHITECTURE.md).

Ignored, read-only junctions Content/Forest_village, Content/Kingdom_Capital and Content/Medieval_Megapack reference matching folders in `D:/Unreal Projects/AoEAssetRenderLab/Content`. Existing Dragon Graveyard, Knights, Dwarf and MagicSpells dependencies remain. Donor packages are excluded from commits; another machine needs the admitted library/junctions. [Final donor verification](donor-verification-final.json): unchanged size/last-write-time for 95 Dragon, 550 Forest/Kingdom and 993 Medieval files. This is metadata verification, not a cryptographic donor audit.

## Nwiro and reproduction

Nwiro verified the live worktree, UE 5.8.2-56702186, Entry and dirty-package state before mutation. [nwiro-capabilities.md](nwiro-capabilities.md) records discovery across Landscape/heightmaps, materials, splines, PCG/foliage, actors, screenshots and PIE. Used Nwiro execute_python, new/open/save level, create_landscape create/modify imports, asset/material inspection/editing, PIE start/stop and screenshot attempts. No AI material-generation tool was used.

Generate inputs with `python Tools/bake_soul_campaign_terrain.py` (NumPy required). Import FounderHeight.png using Nwiro create_landscape: componentCount=8, sectionSize=63, numSubsections=2, editLayers=false, landscapeName=SoulFounderLandscapeV2, transform above, material /Game/Soul/Campaign/TerrainV2/M_FounderLandscape. Execute the owned `Tools/create_soul_terrain_v2_materials.py` and `Tools/finalize_soul_terrain_v2_import.py` in the live editor. Save only owned TerrainV2 packages.

Verified pitfalls: MaterialEditingLibrary uses texture input name UVs; connections now assert success. Nwiro direct material-pointer assignment did not refresh Landscape component material instances: finalization invokes reflected EditorSetLandscapeMaterial and restores dynamic material instances. Earlier checkerboard captures were rejected.

Nwiro PIE screenshots captured the editor viewport instead of the game and were rejected as evidence. Final captures use actual standalone D3D11 rendering through the owned-process runner with -RenderOffscreen -ForceRes. PNG dimensions are independently verified.

## Builds and regression gates

- SoulEditor Win64 Development final build: exit 0, [pass16 receipt](build-editor-pass16.log.result.json), 18:37:53-18:39:02 UTC.
- Soul Win64 Development final build: exit 0, [pass6 receipt](build-game-pass6.log.result.json), 18:39:39-18:41:59 UTC.
- Final V2 Soul. automation: 69 passed, zero failures/warnings, [report](automation-v2-qualified-result.json). No later runtime source/content changes; temporary target formatting was restored to exact pre-edit bytes.
- Procedural fallback Soul. automation: 69 passed, zero failures/warnings, [report](automation-fallback-result.json).
- Final source checks: 61 Tools tests and five campaign-view tests passed, [receipt](source-tests-qualified-final.json). Bake contracts include exact founder adjacency, payload/hash/range, margin, slopes, connected downhill drainage and bridge decks.
- [Run 09](runs/09-qualification/receipt.json): clean exit and 45 visual/input assertions. Actual controller selection/raycast, movement/action spending, finite recruitment, end day, hostile prompt, F5/F9 restore, exploration/remembered-state security, pan/zoom/orbit/MMB/Home and camera bounds. V2 reports vertices=0 for the old procedural terrain.

Normal PCH builds reproduced a baseline stall. Qualified build entry: `Tools/build_soul_terrain.ps1 -Target SoulEditor|Soul -Log <fresh path>`, using -NoPCH -NoUBA -MaxParallelActions=2. Editor-only opt-in SOUL_NO_PCH_COMPAT supplies a project-owned forced-include header for transitive declarations in installed modules; prior environment is restored. No vendor edits. Standard PCH and packaged/cooked launch are not claimed.

## Rendered evidence

Sixteen immutable final 1920x1080 PNGs with dimensions/SHA-256 in [runs/09-qualification](runs/09-qualification):

| Required view | Image (terrain-pass9_ prefix) |
| --- | --- |
| Overview candidate | initial.png |
| Capital/farmland/ground | capital_ground.png |
| Actual party travel | travel.png |
| River ford and bridges | frontier.png |
| Forest transition and close orbit | forest_pass.png, close_orbit.png |
| Quarry/shrine | quarry_shrine.png, shrine.png |
| North Pass | north_pass.png |
| Maximum bounds | wide_bounds.png |

These views were opened and visually inspected. The company is visibly on the capital-to-crossroads road. No checkerboard or shader-preparation overlay in reviewed final views; no exposed world edge in tested bounds. These are review candidates, not accepted production hero shots. Stronghold approach: [Campaign_Stronghold.png](runs/15-roundtrip-1080-qualified/Campaign_Stronghold.png), captured at 1920x1080 after legitimate conquest revealed it; visually reviewed with the returned campaign. All requested view categories now have actual 1080p captures.

[visual-iterations.md](visual-iterations.md) documents rejected attempts and corrections. Runs 01-08 retain immutable full images locally; compact receipts/hashes are committed. Run 09 and later qualification screenshots are committed. Full logs/telemetry remain under ignored Local/. Receipts include exact command, status, height/data/asset/module hashes and assertion lines.

## Real battle round trip

Run 10 at 1080p/45 FPS resolved an actual defeat, applied casualties and mana (80 to 72), returned to V2 and verified RB Save restoration. It hit the unchanged 85 C guard during the return hold: **incomplete**, not a pass. Run 12 at 720p/45 FPS also hit the guard. Integration-only run 13 at 720p/30 FPS completed with exit 0 and the full 70.01-second return hold. Existing qualification inputs were 140 allied / 100 hostile pool, active cap 35 per side; this is an integration fixture, not a balance test. Actual victory returned 54 allied / 0 hostile survivors, changed orc_watch ownership to humans, left mana 72, and survived a deliberately perturbed live state followed by exact RB Save restoration. Peak GPU temperature 78 C. See [qualified roundtrip receipt](runs/13-roundtrip-qualified/receipt.json). Run 15 then completed the same full integration path at 1920x1080/30 FPS: actual victory 48 allied / 0 hostile, orc_watch owned by humans, mana 72, exact RB Save restoration, 70.03-second return hold, clean exit 0, peak GPU 79.0 C. [Final 1080p roundtrip receipt](runs/15-roundtrip-1080-qualified/receipt.json). The battle and campaign return retain the explicit 30 FPS integration cap; campaign performance is measured separately at 45 FPS below. Battle runs are not campaign performance evidence. No battle code/content was changed.

## Controlled campaign benchmark

DESKTOP-Q1S3RPU; GTX 1080 8 GB; i7-7700HQ; 16 GB RAM; UE 5.8.2 D3D11. Actual 1920x1080; VSync off; explicit 45 FPS cap. Twenty-second warmup, sixty-second wall-clock sample; screenshot after measurement. No editor/build/automation runs during measurement. Other desktop applications remain open; initial total GPU allocation approximately 6.4 GB. GPU memory is total desktop usage.

| View | Frames | Mean ms / FPS | P95 ms | P99 ms | Frames below 30 / 40 / 60 FPS | Longest continuous below 30 | Peak GPU |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Overview | 2692 | 22.286 / 44.87 | 22.423 | 23.614 | 5 / 14 / 2691 | 1 frame / 42.31 ms | 72 C |
| Close forest | 2696 | 22.258 / 44.93 | 22.415 | 23.129 | 0 / 6 / 2696 | 0 frames / 0 ms | 64 C |

[Overview receipt](runs/11-benchmark-overview/receipt.json) and [close forest receipt](runs/14-benchmark-forest/receipt.json), raw CSVs and post-measurement PNGs are retained. Both screenshots were opened. The close forest benchmark preserves initial fog state: unknown town objects remain hidden, so it is not an explored settlement workload. Table percentiles use independent NumPy interpolation; in-engine nearest-index percentiles differ slightly. Most frames being below 60 FPS is expected with the explicit 45 cap. No sustained sub-30 interval was measured in either view. This qualifies controlled performance only. Uncapped headroom and prolonged thermal stability remain unqualified. Earlier uncapped/60 FPS runs reached the 85 C cutoff; it was never raised or bypassed.

## Remaining production defects / next action

**Visual production gate fails.** Regular cottages/huts, oversized repeated civic silhouettes, sparse settlement fabric, broad bland ground, patterned ridge shading/rock scatter, jagged tributary intersections, simple water, weak quarry/pass/stronghold identity, and subdued atmospheric depth still read as a prototype. Labels/selection brackets remain prominent. Numeric slope success does not establish visual acceptance.

Next meaningful work: author the capital-to-ford frontier as one coherent composition with varied streets/gates and believable building scale, irregular field/woodland edges, sculpted banks/confluences, erosion/deposition-led ridge materials and subordinate strategic overlays. Retain this Landscape/height-query integration and canonical rules. Validate one close/overview pair early; reject it if these defects still dominate. A verified commercial terrain license could improve geological input through the existing import contract; absence did not stop the native pass.

Still unqualified: production visual category, packaged/cooked runtime, uncapped performance, extended thermal soak and every possible camera pose. Repeated 85 C cutoffs constrain sustained battle qualification on this machine. These limits must stay visible in promotion decisions.

Launch `Tools/Open_Soul_Campaign_TerrainV2.bat` for opt-in V2 (explicit 45 FPS). Omit -SoulTerrainV2 for the original presentation. Pre-existing Soul.uproject Nwiro enablement, .mcp.json, project backup and ROBUST_CODEX_PROMPT.md are preserved and excluded from worker commits.
