# Campaign world architecture — implementation candidate

Recovery gate: local HEAD and origin/codex/soul-bannerlord-campaign-map-20260929 both resolve to 4842eeb403f6b9be6effdf9fe18fc5c24b499887. Initial status and SHA-256 receipts are adjacent. Existing modified Config/DefaultEngine.ini and Soul.uproject and local support files are preserved, not included in this change.

## Decision

Build one Soul-owned conventional Landscape, streamed as one level through the existing SoulCampaignTerrain adapter. Do not stitch regional maps. Retain the original regional maps as immutable source/reference material and a recovery path. Add an opt-in world profile until runtime and visual qualification support promotion.

Candidate configuration: 2033 × 2033 samples, 8 × 8 components, 2 × 2 subsections of 127 quads per component, 500 cm XY spacing, origin (-508000,-508000,0), Z scale 512 (4 cm per uint16 height unit). Physical buffered surface is 10.16 km square. Strategic geography keeps the existing map-to-world transform: X=(mapX−500)×1000 cm; Y=(475−mapY)×1000 cm. Structural polygon extent remains 9.35 × 8.55 km.

64 independently LOD-managed components and an approximately 8 MiB CPU height payload are a defensible first implementation on the qualification machine. The strategic camera needs the whole horizon; a new World Partition subsystem would add conversion/streaming failure modes without an established memory need. This is a measured candidate, not a claim of performance acceptance. Reconsider component layout/streaming only if runtime memory or frame receipts demonstrate a need.

Mountain05, Grassland_02 and Mesa_01 supply localized heightform detail, never whole rectangular biome tiles. Continuous authored ridges, valleys, coast, drainage, passes and route corridors establish the world. Heightfield and materials are local derivatives under SoulTerrainPreview/WorldTerrain and Content/SoulCampaignWorld; recipe/tooling and nonlicensed profile metadata belong in Git.

## Authority

All 36 structural sites and 51 route pairs constrain physical composition. Nine founder regions and ten existing founder edges remain the interactive SoulCore campaign. Other regions are physical context, not new ownership, action, save or movement authority. No battle rules or faction rules change. Terrain never decides legal adjacency.

The schematic river sketches do not pass through every named crossing. Physical drainage may be re-authored through those fixed strategic sites; legal graph edges crossing water receive presentation bridge/ford treatment. This changes no route pair. Grade, drainage, footprint and actual collision checks must pass before accepting the candidate.

## Acceptance

Required evidence: full-world and six-region Unreal renders, minimized-overlay overview, road/pass/water/capital close views, CPU height versus actual Landscape collision, route grades/clearances, 36-site/51-route preservation, source tests, Editor/game builds, nine-region input/save/battle roundtrips, and warmed 1080p campaign frame/temperature receipts. Builds alone cannot establish visual success. Preserve the 85 °C stop and run only one heavy Unreal/build process at a time.
