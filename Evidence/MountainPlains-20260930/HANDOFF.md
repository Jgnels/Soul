# Mountain05 rolling plains handoff — 2026-09-30

## FACT
Worker: `D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929`, branch `codex/soul-bannerlord-campaign-map-20260929`. Session base `9b3b3e9128dfe93352170c08c1d33b86d326e337`; tools commit `1db984a`. The older consolidated authority remained6d53714; it was not edited. No push or merge.

Editable map: `D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/Content/SoulCampaignMountain/L_mountain_campaign.umap`, package `/Game/SoulCampaignMountain/L_mountain_campaign`. Open in SoulTerrainPreview using `Open-Mountain-Plains.ps1`. This launcher preserves an already-running preview editor; it has been syntax inspected, not interactively exercised after shutdown.

Comparison: http://127.0.0.1:8766/mountain-plains.html . HTML at the preview root; actual1920x1080 screenshots in `TerrainWork/Captures`. Matched before/after names: `L_before__final_human_plains.png` / `L_mountain_campaign_human_plains.png`, and corresponding overhead files. Final campaign, dwarf_foothills and northern_lake views also visually inspected. HTTP200 verified. Browser automation failed to initialize due local sandbox helper error, so interactive slider UI was not browser-qualified.

Terrain: one2041x2041 Landscape,64components,1.5x1.5km extent. Central edit area0.314km²; inner rolling-plains core0.127km²; core slope95th percentile5.163°, max11.501°. Retains20% smoothed native relief in its center;120m inward feather; maximum cut49.718m at campaign scale. Outside the marked polygon and ground at/below1.5m are numerically unchanged. Lake remains. Eastern/southern mountain geography remains. No desert added; no separate biome-map patches.

Three observed iterations: v1 overly uniform floor/rim and repeating textures; v2 retained source rolling relief and broader transition, removed water stripes; final kept v2 geometry with restrained ground palette and shoreline material treatment. These are terrain-study materials, not finished environment art. Old donor foliage and decorative actors were omitted entirely from the derived map, not merely hidden. No artificial settlement/faction authority added.

Verification:61 existing Tools tests PASS;5 campaign-view tests PASS;4 terrain path checks PASS at22° maximum local slope, including northern-lake land connection. Source bake reproduces imported PNG SHA256 `9c0b4170cbf34bde647b4cd02526af93cc27d4bfb332cef54373f5fa8cb98fc2`. Ten retained Python scripts parse. Terrain shoreline/outside-mask/16-bit quantization assertions PASS.

Unreal: imported, rendered, saved, unloaded to Entry, reopened.121/121 collision sweeps (radius0.25cm) hit Landscape; maximum height error0.088867cm. **Zero-width rays at40 exact outer-component sample vertices miss**, while nearby sub-centimeter rays hit. Rebuilding collision did not remove this after reopen. Keep the caveat; do not claim the zero-width-ray test passed. Final map camera saved to human plains. Capture editor shut down normally at14:30:12UTC, D3D11 cleanup and LogExit confirmed.

Preservation:790 donor files unchanged by size/nanosecond timestamp. Mountain05 umap SHA256 unchanged `3C65ED7EE42B3632AAAD11D635432F070735B0FAD66F7EE6B132D0D508D987B2`. Original donor maps/materials were never saved. Licensed terrain/heightmaps/textures and generated captures remain local outside Git.

## INFERENCE
The new central terrain is suitable for settlement placement and route authoring, with natural mountain enclosure and coast access. Terrain sampling supports that conclusion; it is not campaign gameplay proof. Remaining rim detail is intentional foothill transition but can be locally refined when actual roads and settlement pads are placed.

## UNKNOWN / remaining gates
The new map is **not integrated into Soul campaign gameplay**. Existing SoulTerrainV2 assumes1009² data,5.6km bounds,20x scale and fixed dressing placement. It needs a bounded map profile plus placements/routes before this asset can safely replace that map. No C++ or strategic authority changed. No affected C++ target required compilation; no new game build, save/load/recruitment automation, real Dragon Graveyard entry or campaign→battle→campaign test was run for this terrain. Original full vertical-slice acceptance remains incomplete.

Production art gaps: no newly scaled forests, settlements, playable roads, biome identity, atmospheric horizon or campaign HUD on this study map. Water is a simple presentation plane. Full gameplay performance and click behavior at terrain precision seams are unqualified. Existing northern-lake route passes the sampled terrain test; army pathfinding has not been attached to it.

Measured blockers resolved/contained: full8161² native export exhausted memory (71.5MiB allocation failed under commit pressure);2041² export succeeded. Setting render-target gamma explicitly caused blank export; default-gamma RG export decoded correct16-bit heights and matched source bounds. Offscreen screenshot tasks required viewport draws. No pagefile/driver/system changes were made. No assertion of eight uninterrupted productive hours is made; the session included an execution gap.

## Exact retained source files
`Tools/MountainTerrain/`: README.md, bake_plains.py, build_plains_gallery.py, capture_plains.py, dress_plains.py, import_plains.py, nwiro_client.py, set_final_view.py, start_capture.py, validate_plains.py, verify_collision_surface.py.
`Evidence/MountainPlains-20260930/`: HANDOFF.md, campaign-view-qualification.txt, local-artifact-sha256.json, mountain-collision-surface.json, mountain05-before-terraform-hash.json, preserved-tools-verification.json, source-qualification.txt, terrain-file-qualification.json, terrain-plan-v2.json, validation-v2.json.

## Intentionally uncommitted / local
Pre-existing worker dirty state preserved: Soul.uproject, .mcp.json, Evidence/CampaignTerrainProduction-20260929/ROBUST_CODEX_PROMPT.md, Soul.uproject.nwiro-terrain-backup-20260929-102724. Preview project, its Nwiro enablement backup, derived .umap/.uasset files, raw/export/baked PNG/NPY data, all captures, large logs and intermediate failed exports stay outside Git. No licensed files staged.

Next meaningful implementation: adapt the existing terrain presentation adapter to this extent and height encoding, place the canonical nine locations on legal dry terrain, author terrain-conforming routes and crossings, then run campaign mechanics and real battle roundtrip qualification. Do not treat this terrain deliverable as completion of those gates.
