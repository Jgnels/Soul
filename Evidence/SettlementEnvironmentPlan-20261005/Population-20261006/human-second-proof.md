# Human Capital — second and final deep authored proof

Current result: the existing Dwarf settlement architecture generalizes to the intact Hivemind city. Framework development is frozen. No third deep city proof.

The selected original is `/Game/CastleTown/Levels/Persistant/PL_CastleTown`, supplied complete at `D:/RefinedBadger/AssetLibraries/MedievalKingdom-42d4a792`. Soul's wrapper is `/Game/Soul/Maps/Settlements/L_HumanCapital_Authored`. The full city retains its native scale and composition; 161 visible levels and all 53,529 native static-mesh placements match the preceding survey. Donors are read-only; generated maps, source derivatives and evidence remain local under the established source-commit policy.

The buildable native waterfront tavern is `LI_Building_05_Fix`, `SL_HumanCapital_Houses:LevelInstance_149`. Three existing state adapters control 24 persistent roots and 825 nested children. Starting state omits this building and its associated dressing; the remainder of the authored city starts present. Actual U/Space construction spends 200 gold and completes after two days. The building appears, companion service unlocks, and H uses the existing 1,200-gold hire. F5/F9 restores both existing domains exactly, including mid-construction physical rollback. Return/revisit preserves the same state. No gameplay or save authority was added.

R6's campaign base preserves the native central keep plus thirty nearby real town blocks; the upgrade is the same native tavern. Native low-LOD roof holes were repaired in deterministic owned RenderLOD0 derivatives. Fresh F9 visibly changes the campaign representation. This is provisional campaign art: detached outer gate and town-to-road composition still need population review. The full visit/battle city is never reduced to these miniatures.

`Local/human-authored-battle-r3` cleanly passes a real battle through the existing bridge: 30 initial bodies, actual reserves 16/15, natural victory with 27 allied survivors and zero enemies, campaign return and exact post-result F5/F9. Reviewed screenshots show visible forces on original ground with the native town/waterfront behind. The only battle geography adjustment is runtime omission of 202 tree instances inside the bounded 100 x 86 m approach ellipse; donor packages and the separately loaded visit world are unchanged. No siege system or combat-rule change.

Fresh performance run `Local/human-authored-fresh-performance-r1` restores the independently supplied checkpoint, loads the same completed native city and returns to the same campaign state. Fresh-city and returned-campaign screenshots are reviewed. **The combined guarded run fails at 85 C**, despite native state checks and its completion marker: the runner terminates the owned process. Do not label it a clean fresh-performance pass.

Measured native 1920 x 1080, 100% scale, VSync off, uncapped, 20 s warmup / 30 s sample:

| Fixed view | Mean / average FPS | P95 | P99 | Below 30 / 40 / 60 | GPU peak |
|---|---:|---:|---:|---:|---:|
| Native tavern visit | 23.185 ms / 43.13 | 28.132 ms | 32.027 ms | 8 / 303 / 1293 of 1294 | 84 C |
| Three-node proof campaign | 11.362 ms / 88.01 | 12.694 ms | 13.379 ms | 0 / 0 / 0 of 2641 | **85 C stop** |

City visual readiness took about 13.6 minutes from visit input; the root LoadMap figure of 164.5 seconds excludes nested preparation. Return LoadMap took 143.7 seconds. Peak sampled process private commit 13,529 MiB, working set 7,097 MiB; device-wide VRAM 7,508 MiB. This is editor-game source content, one fixed tavern view with the actual visit HUD, not full-city worst-case or cooked performance. No 30-FPS cap hides either measured window.

Builds: SoulEditor population r8 succeeds in 64.12 s; Soul game population r1 in 248.11 s. Tools 99/99, campaign-view 18/18, existing native settlement suite 7/7 passed in this shift. Per-run source/DLL hashes are preserved. Final retained population checks remain separate.

Receipts: `human-authored-groups-r1.json`, `human-native-survey-r6-review.json`, `human-physical-visual-review-r1.json`, `human-authored-battle-r3-review.json`, `human-fresh-performance-r1-summary.json`, `human-miniature-r6-campaign-review.json`, `visual-review.html`.

Next: finish the first retained-map population render/input/performance pass. Do not restart Human source recovery, Dwarf polish, another city proof, or R10 terrain work.
