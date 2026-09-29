# Terrain V2 production lane

Baseline: c36e37196b69f065f734828e6c175aa9ba12e755. Acceptance requires rendered quality, unchanged campaign behavior, both Development builds, Soul automation, real battle return and uncapped or explicitly controlled 1080p performance. A better material alone is not acceptance.

Inspection: Nwiro confirms Soul at the admitted worktree, UE 5.8.2-56702186, Entry world, no dirty map packages. Existing Soul.uproject Nwiro enablement, .mcp.json, backup and ROBUST_CODEX_PROMPT.md predate this work. Preserve them.

V2 is opt-in with -SoulTerrainV2 until qualified. Original Entry/procedural presentation remains available without it. A project-owned Landscape sublevel streams into the existing campaign, including after battle return. Baked height data supplies presentation height queries only; no terrain query decides legal moves, ownership or exploration. One presentation scale multiplies the existing nine-place mapping by 20 (centimeters), producing a 5.6 km terrain with a roughly 1.3 km playable frontier. Canonical data remains unchanged.

Only project-owned generated inputs, materials, Landscape and code are committed. Existing licensed packs are referenced through local ignored junctions and never saved or modified. Hyper tree availability was checked first; the installed resource subset contains a cut log rather than usable complete trees. The local Forest Village/Kingdom Capital packs have complete vegetation and settlement modules. RB Optimization representation policy was inspected: static decorative HISM is appropriate; no new actor promotion/culling authority is needed.

Installed-tool inspection found no Gaea, World Creator or World Machine installation in uninstall records or the standard application directories checked. This is not proof of license ownership elsewhere. No commercial-capable terrain-tool license verified; use the native bake/import route and retain a reproducible external bakeoff contract.

Height range after clipping rejection uses 2.5 cm per uint16 increment (Z scale 320). The river is a authored control-point curve with a continuous downstream grade; tributary outlets share its XY position and surface elevation. Main/tributary surfaces extend into their carved banks for terrain occlusion of edges. Bake tests cover payload/hash/range, exact adjacency/endpoints, scale/margins and connected downhill main drainage.

The mapping also carries three road/river crossings and twelve field parcels. Bridge decks and travelling party heights share RoadSurface; bridge children inherit the authoritative road exploration visibility. Field bounds exclude incidental trees/rocks. Licensed Medieval Megapack house/roof and wall modules supply the human streets and civic enclosure; frontier masonry differs from the capital. All decoration uses HISM. Unit miniatures remain abstract presentation, at one-quarter the geography scale so they fit crossings.

Landscape finalization must invoke the reflected EditorSetLandscapeMaterial setter, which refreshes component material instances. The Nwiro pointer assignment alone left a checkerboard. Material graph links are asserted; the library's texture input name is UVs (Coordinates is shortened by Unreal's material-graph API). Reimport only owned textures/materials and save only the owned Landscape. No vendor package save is necessary.

The machine reached the runner's pre-existing 85 C cutoff in uncapped and later 60 FPS runs. Keep rejected receipts; never raise that cutoff. A 45 FPS controlled run is explicit in launch commands and benchmark receipts. Benchmark samples use wall-clock frame intervals after 20 seconds of warmup, for 60 seconds; screenshots happen after measurement. No 30 FPS limit is used for campaign qualification.
