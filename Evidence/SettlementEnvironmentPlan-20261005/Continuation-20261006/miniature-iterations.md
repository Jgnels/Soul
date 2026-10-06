# Dwarf strategic representation iterations

The full visit/battle map remains the actual authored Citadel. These are deterministic owned-mesh campaign derivatives only.

| Candidate | Evidence and decision |
|---|---|
| r5 hall cutaway | Qualified visible start/completed state. Preserved fallback. Rectangular hall alone is provisional capital art. |
| r6 native-relative gate/cliffs | Rejected. The editor actor-list helper omitted loaded instance children, so most of the actual gate was absent. Black-stage screenshot retained in `Saved/NwiroScreenshots/Dwarf_gate_miniature_candidate_r6_warm.png`. |
| r7 complete gate/cliffs + hall | Corrected exporter uses `GameplayStatics.get_all_actors_of_class` and the already loaded gate level, including its 162 actors. No soft source-world reads. Native gate is now recognizable, but `Local/authored-fresh-r3/User/Saved/Screenshots/authored-fresh-r3_fresh_miniature.png` exposes cliff backsides and an oversized disconnected frontage in campaign. **Rejected for promotion.** Functional fresh restoration still passes; art acceptance does not follow from that. |
| r8 compact gate/hall | Preserved intermediate. Retains r5 native hall and state group; transforms the authored gate as one group (0.35 uniform scale, +90 degrees, compressed approach spacing). Full environment scale/layout remain unchanged. Backstage blocker pieces remain visible from the rear. |
| r9 compact gate/hall, blockers omitted | **Retained for the functional proof.** Omits six source backstage blocker meshes; base is 180,000 triangles plus the unchanged 14,840-triangle state piece. Native-1080p `authored-runtime-r9-native1080` start/completed images show the same hall's colonnade appearing behind the recognizable gate. Full input/visit/save/battle sequence and clean return pass. This is a lightweight state representation of the full authored environment, not its replacement. |

Do not interpret native level-instance `is_loaded=false` on packed LevelInstance-derived blueprints as missing visible geometry: packed mesh components can render without a loaded source ULevel. Never dereference their soft `world_asset` merely to inspect a map, because that can load an additional source world.

The original generic editor-actor survey and source export omit actual loaded-level children. Those receipts are preserved as historical observations. Current tools now iterate the loaded world, and r7 records explicit loaded-gate membership. No donor package was saved to repair the export.

The campaign art remains provisional: a roofless hall cutaway and gate at the existing temporary quarry anchor are too rectangular and insufficiently embedded to count as a finished Crownspine capital. The r9 change improves silhouette and state correspondence; it does not clear final settlement composition, hinterland or geographic-placement acceptance. The complete authored city remains the visit/battle environment at native scale. No terrain flattening or replacement art was introduced.

Final r9 images: `Local/authored-runtime-r9-native1080/User/Saved/Screenshots/authored-runtime-r9-native1080_miniature_start.png` and `_miniature_completed.png`. Keep r5/r8 and rejected r6/r7 assets and raw captures locally for comparison; do not overwrite them or promote failed iterations.
