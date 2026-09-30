# Grassland transplant — 2026-09-30
FACT: Grassland_02 elevation forms transplanted into Mountain_05 human peninsula, with north/west shoreline extension and a 105m mountain transition. Separate derived map /Game/SoulCampaignMountain/L_grass_coast_v2 in SoulTerrainPreview.
FACT: Original water footprint, underwater heights and outside-mask heights preserved. Four sampled routes pass 22-degree threshold. Saved Unreal level reloaded; 121/121 collision sweeps pass, maximum height error 0.07361cm.
FACT: Five 1920x1080 Unreal views inspected: human plains, overhead, campaign, dwarf foothills, northern lake. Comparison: http://127.0.0.1:8766/grassland-transplant.html; files under preview TerrainWork/Captures/L_grass_coast_v2_*.png.
FACT: 790 donor files retain original size and modification time. Licensed assets and derivatives remain outside Git. Existing unrelated dirty files untouched.
INFERENCE: Donor hills and opened coast improve the enclosed-dish appearance. Eastern/southern mountains remain a clear boundary; shoreline and material polish remain.
UNKNOWN: Actual campaign navigation/integration and battle round trip on this derived terrain are unqualified. Route checks are terrain-only.
Reproduce using preview TerrainWork/before.npy and grassland02-2041-rg.png, bake_grass_transplant_v2.py, import_plains.py grass_coast_v2, dress_plains.py, start_capture.py, validate_grass.py and collision scripts. Export source read-only; never save donor maps.
