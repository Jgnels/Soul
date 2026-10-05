# Soul campaign world authoring

This lane presents the canonical 36 sites and 51 routes on one Landscape. It does not add playable regions or change SoulCore adjacency. Runtime currently selects it with `-SoulWorldTerrain`; promotion requires the recorded runtime and visual gates.

Local licensed sources and derived payloads live under `D:/RefinedBadger/AssetLibraries/SoulTerrainPreview`. `Tools/Setup_Soul_World.ps1` mounts only the owned World content and height data. It verifies the baked profile/hash and refuses conflicting existing paths. Preserve the other local mounts and security configuration.

The current R6 height is `f032cbba35e73862953051dab5b14e3d6d8b08c817774c39e6c8cedb268eed29`. It is a bounded repair of the preserved `WorldTerrain/revisions/revision5` payload. Exact replay uses `bake_world.py --repair-lake`, then `bake_surface.py`. A fresh `bake_world.py` run reauthors the whole terrain from donor vocabulary; it is a new candidate and must be requalified. Do not overwrite a qualified payload without first preserving its external revision.

Use Python with numpy, Pillow and scipy. The baker also recognizes the task-local dependency directory under `WorldTerrain/dependencies`. Run `test_world.py` after baking. The physical buffer is 2033² samples at 5m spacing; the Landscape uses 8×8 components, 2×2 subsections of 127 quads, XY scale500 and Z scale512. Image row zero is UE Y minimum. Do not flip the import.

Only one editor, build or game process may run. `editor_session.py` starts a bounded offscreen authoring editor with the existing 85°C cutoff. In that session, run these operations sequentially:

1. `author_world.py import --reimport` to update the owned Landscape. Omit `--reimport` only for first creation.
2. `author_world.py textures` to import the macro/biome/region/detail/cultivation masks through the supported render-target conversion.
3. `author_world.py dress` to compile/save owned materials and the ocean surface.
4. Run relevant native automation, then `author_world.py close` and verify the session exited.

Texture source data remains linear. The ground material explicitly decodes the display-sRGB macro palette; biome/detail/cultivation channels are masks. All procedural field work is baked once rather than expanded per landscape pixel. No donor material or mesh is saved by these authoring steps.

`qualify_world.py` defaults to a command preview. `--execute --run <fresh-name>` owns one isolated run through the existing vertical qualification runner. Modes are `captures`, `input`, `load --load-from <passed-input-run>`, `victory`, `defeat`, and `performance --focus <region-or-overview>`. Performance is uncapped at 1920×1080 with 20s warmup and 60s samples. Logs, actual screenshots, temperature/memory telemetry and before/after hashes remain under the ignored evidence `Local` directory. A technical pass never grants visual acceptance.

See `Evidence/CampaignWorldTerrain-20261005/ARCHITECTURE.md` and revision receipts for the authority boundary, retained donor work, rejected renders and outstanding gates. Never reset/clean preserved local state or commit licensed binary payloads.
