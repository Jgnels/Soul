# Soul map polish — frozen geography

Transport-polish milestone, **not full visual acceptance or production promotion**.
The working 3.5 km composition remains the geographic foundation. Do not restart
terrain research, enlarge it, revive R10 or reshape regions to address these defects.

## Checkpoint and scope

- Branch: `codex/soul-bannerlord-campaign-map-20260929`.
- HEAD before this run and before its milestone commit:
  `98f78e7ae12797d966dedbd4784fdfce56269e72`.
- Final local HEAD and commit subject: adjacent `final-head.txt`, written after the
  commit to avoid a self-referential commit hash. No push or merge.
- Candidate: `/Game/SoulCampaignComposition/L_Composition_3500_r2`.
- Dimensions remain 3500 × 3500 m. All 36 anchor IDs/XY positions and 51 legal
  endpoint pairs are unchanged. No gameplay/save/battle/construction code changed.
- Only route presentation, road mask binding and Soul-owned crossing actors/assets
  changed. No Landscape height edits, donor saves or reference-slice changes.

## Proven / qualified

**River Ford:** `crossroads → river_ford` changed from the prior 25.2386° failure to
21.7686° on the imported quantized Landscape analytical sampler and 21.7626° in
native route collision probes. The bank approach curves around the steep section.
The crossing remains an unbridged ford with subtle owned stone bed/bank cues.
No terrain seating correction was needed.

**51/51 sampled route-grade checks pass.** `route-inventory.json` records every
legal connection, hierarchy, width, crossing and analytical maximum. Maximum over
the 51 routes is 22.0081° at ≤0.25 m spacing, including polyline vertices. Native
sampling covers 52,645 stations with zero misses; maximum route grade is 21.9613°.
Ground samples agree with the imported heightfield within 2.57 mm, including the
1 mm query offset. `native-route-final-summary.json` records the exact method and
raw diagnostics. Native grade uses horizontal travelled arc distance, not a chord
across bends. Duplicate merged station intervals under 20 cm are excluded from
native acceptance because millimetre ray errors dominate them; raw results remain
recorded. These are sampled centerline checks, **not a continuous derivative,
turning-radius, full-width clearance or gameplay movement guarantee**.

Native checks exposed actual defects beyond the original grade failure: roads
clipped Southern Crossing parapets and approached Broken Bridge at a bad angle.
Both now enter through deck ends along the bridge axis. Candidate-owned stone
collision matches rendered mesh. Timber uses a conventional deck collision hull
across plank gaps, with its top aligned to measured plank height; original meshes
remain unchanged. Render review also caught transverse timber rails, now corrected
to the mesh's actual longitudinal X axis.

## Paths removed, merged and retained

No legal road/edge was removed. Equivalent visible stretches were aligned to
shared corridors only when they rejoined at both ends, retained crossing usage and
did not abandon a distinct site approach. The two consolidation receipts contain
67 and 60 local edits across 22 and 18 routes respectively; these overlap and are
**not 127 distinct removed roads**. Inspect exact pairs and splice endpoints in
`corridor-consolidation.json` and `corridor-consolidation-r2.json`.

The strongest visible simplification is Nature Clearing → Treehold sharing the
River Woodland approach/common trunk. The existing separate southern crossing is
retained. The unused woodland bridge is hidden/inactive, with its original actor
preserved; a bounded attempt to connect it failed the east-bank grade constraint.
Do not restore it merely as decoration suggesting a false passage.

Retained proximity has specific roles: Viking coastal/forest/snow-pass branches
reach different sites; Dwarf quarry, snow basin and forge approaches remain distinct;
Orc ruined-field/badlands/war-camp branches retain their destinations; Human
crossroads, northern and coastal arrivals remain; both inlet ferries retain their
legal pairs. Shared presentation trunks do not create new canonical junctions.
Some close parallel stretches and angular junctions remain visible. **The stronger
claim “no unexplained visible duplicates anywhere” has not been fully established.**

Road widths now distinguish main 3.8 m, secondary 3.0 m, mountain 2.8 m, woodland
2.4 m and Orc military 3.2 m, replacing the roughly 5.15 m uniform footprint.
Supersampled softer edges and bounded corner rounding reduce the pathfinder look.
Only the control texture's road channel changed; other regional channels are intact.
The roads remain material footprints, with tight local turns still requiring art
review. Do not interpret the grade passes as final road construction acceptance.

## Crossing hierarchy

| Location | Current presentation | Status |
|---|---|---|
| Human west | 26 m owned stone span | Major crossing retained; native collision works; provisional abutment/material art |
| Southern Crossing | 16 m owned stone span | Six route approaches aligned through deck ends; provisional art |
| Human north tributary | 13 × 3.45 m timber, eight owned modules | Lighter minor crossing; rails parallel; support detail provisional |
| Orc Broken Bridge | Two cut stone ends, ~4.88 m ruined stone gap, 8.2 × 3.1 m timber repair | Five modules; legal passage retained; clean cut faces need ruin art |
| River Ford | Shallow unbridged crossing | Grade solved; 38 low owned rock instances; water-edge art still weak |
| Woodland bridge | Hidden/inactive original actor | No fitted route used it; failed bounded connection preserved as rejected evidence |
| Central inlet ferry | ~153.94 m water passage, two timber landings | Land approaches reach shoreline; no sea bridge |
| Southern inlet ferry | ~202.28 m water passage, two timber landings | Land approaches reach shoreline; no sea bridge |

Four ferry jetties range ~7.96–22.25 m; these are landing decks, not bridges across
the inlet. No ferry boat/animation/runtime service was added. Timber comes from the
already-owned `Forest_village/Meshes/Wood_modules/SM_bridge_module`; stone comes
from `Kingdom_Capital/Meshes/Bridge/SM_arch_bridge_01`. The Six Sides pack was not
found in the bounded local inspection, so no time was spent acquiring it.

## Settlement and six-region status

Human arrivals now avoid native housing clusters, meet the central open street and
have a short forecourt spur. The capital was not moved, resized or redesigned.
Lake geometry is retained. Exact authored gate/quay/island correspondence is **not
solved**; do not describe the new spur as a verified gate connection.

Dwarf seating remains a **known defect**. The measured axis-aligned footprint spans
~7.22 m of relief. A bounded search within 16 m did not find a better bench under
its displacement criterion. This screening does not prove all seating options
impossible. No hold movement or broad flat pad was introduced. A localized owned
foundation/terrace solution needs design review before terrain work.

| Macro-region | This pass | Still provisional / not done |
|---|---|---|
| Human | Narrower shared roads, city arrivals, ford, minor timber crossing | Quay/lake correspondence, farmland, roadbed art |
| Dwarf | Shared corridor cleanup, mountain road width/grades | Hold seating, pass engineering/quarry dressing |
| Viking | Route cleanup preserves coast/forest/pass choice | Harbor context and boreal identity unchanged |
| Orc | Military road hierarchy, Broken Bridge, local grade/curve fixes | Mesa material transition and ruin finish |
| Nature | Shared woodland trunk, unused bridge removed, narrower paths | Foliage diversity, riverbank finish, tight turns |
| Dark | Grade/curve cleanup and southern ferry landings | Fortress horizon/causeway identity unchanged |

Minor settlement base/slope intersections remain. No full population pass, new
settlement, city framework, regional terrain or decorative content phase was begun.

## Tests, runtime and performance

- 21 source tests pass: campaign-view/input suites; `source-tests.txt/json`.
- Two existing native tests pass: `Soul.Integration.CampaignWorld.CameraAndSelection`
  and `TerrainAndRoutes`; `native-tests.txt/json`.
- 28 polish Python files parse; `tool-syntax.json`.
- Native candidate collision and per-route grade checks described above pass.
- No new C++/project configuration authored. No fresh SoulEditor/game build or
  package claimed. Existing inherited source drafts were preserved, not promoted.
- Candidate runtime adapter, selection/movement, F5/F9 and battle roundtrip were
  **not attempted** because the requested visual gate remains incomplete. Existing
  reference-map proofs remain prior evidence, not acceptance of this candidate.
- No uncapped profile: visual acceptance must precede it. The single guarded editor
  session used a 12 FPS cap and 1920×1080 captures. Observed peak GPU 60°C, GPU memory
  6825 MiB, process working set 6839 MiB, private commit 8905 MiB. These are capped
  review observations, **not 40+ FPS evidence**. Mean/P95/P99 and threshold counts
  are unmeasured. The 85°C guard was unchanged. Editor was saved and the owned guard
  closed it with `requested_authoring_close`; exit code alone is not acceptance.

## Preservation, files and visual evidence

`preservation-final.json`: 345/346 baseline files are byte-identical. The sole
intentional changed baseline asset is the candidate map. All 33 inherited tracked
dirty files retain their initial hashes except the appended polish-only `.gitignore`
entry. Existing R10/source/config/security/licensed local state remains untouched.

- Candidate before SHA256: `df962594b425048fc5a56c09d0b58acdb0caa84d444ae45e14eccd1da733a01c`.
- Candidate after SHA256: `fe7a6ab40c270fc18d1beb5ee6e782fa4cf3d1fae3091e19c263d78f1766ba6e`.
- Frozen composition height PNG, unchanged: `bd84f1986c9fd71b51725647d8ae89644a0681c5d65f2d2aebcf6ab31383ba6b`.
- Retained 1.5 km map, unchanged: `d18c21c2aeadf50036ef3a605737bac971e2e91e2a43aaa988d92f5987a8094f`.
- Retained height, unchanged: `908f9de444112694afe6a242db86a45c64adef8bcaa392c464e304a1f1c3990b`.
- Retained presentation, unchanged: `6d93437e19aaf9efb3683aabace2431ca3b05234dcef935b3c10e0d362e95eaf`.

Complete Mountain05/FreshCan/Mesa/reference/settlement/source paths and hashes are
in the preservation receipts. Original candidate materials/controls are preserved;
new bindings live under `/Game/SoulCampaignComposition/Polish/`. Licensed packages,
derivatives, screenshots, native hit arrays, route coordinates and analysis
dependencies remain local-only. Commit scope is Soul-owned scripts/light evidence
plus the exact polish ignore hunk. No credentials or machine configuration staged.

Open `visual-review.html`: 14 fixed before/after camera pairs plus three extra final
crossing views; 17 final Unreal screenshots under `Local/captures-final/`.
`route-atlas.html` has all 51 analytical plates, explicitly not rendered game art.
The full before candidate snapshot is `Local/Before/Content/SoulCampaignComposition/`.
Final route data is `Local/routes-presentation-r4.json`, final road mask is
`Local/Composition_Polish_Control_r4.png`. Earlier iterations remain preserved.

Rejected attempts include using the spare woodland bridge, early quarter-metre-only
acceptance before native deck checks, parapet-clipping bridge approaches, timber
collision gaps, and transverse rail orientation. These defects are not hidden by
the final sampled pass count. Earlier receipts are historical; final native summary,
final capture receipt and `timber-orientation-r2.json` supersede them.

## Exact next action

Review the paired Human/Nature/mountain-pass road views and Dwarf seat with Jeff.
Keep this macro geography frozen. Next bounded work should resolve the Dwarf
foundation/arrival and remaining tight/shared road junctions, then the capital's
authored waterfront correspondence and crossing/water-edge finish. No new terrain
foundation or broad reshaping is justified. After visual acceptance, bind this
presentation to the existing campaign adapter and qualify input/save/battle, then
run one guarded native-1080p profile. This candidate is promising and cleaner, but
**not yet strong enough to promote as a production-ready replacement**.
