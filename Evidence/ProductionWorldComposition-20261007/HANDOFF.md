# Production-world composition checkpoint — 2026-10-07

**Do not promote this candidate.** This run establishes a more useful geographic
composition and settlement scale, but it is not a completed production foundation.
The retained nine-anchor campaign remains the qualified playable reference.

Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Starting HEAD: `07a3095d5b82cc9bc07c9eb5130cd7af8d714b25`.
Final HEAD and local milestone commit: see `final-head.txt` beside this file,
written after committing to avoid a self-referential commit hash. No push or merge.

## Candidate and sources

Current map: `/Game/SoulCampaignComposition/L_Composition_3500_r2`.
3,500 × 3,500 m; 2041² height samples; 8 × 8 Landscape components, 255 quads,
one subsection. XY scale 171.568627 cm; Z scale 100; origin −175000/−175000/0 cm.
Traditional single Landscape in an isolated editor map; no World Partition or
runtime streaming-framework change. Full authored cities remain separate locations.

Native Mountain05 supplies the continuous skeleton, rotated one quarter turn and
uniformly scaled by 3500/8160. It is not an enlargement of the flattened 1.5 km
derivative. Ocean stays at the original zero datum.

FreshCan contributes a 0.677 km² western estuary/coast patch from its measured native
collision field, using uniform 0.62 XYZ scale and a soft boundary. The final Viking
site is on the native ocean-connected inlet shore, not the rejected isolated
FreshCan shelf. Mesa_01 contributes about 1.133 km² of eastern plateau/gully forms.
85.24% of the field remains unchanged outside those masks before narrow river work.
No new Grassland transplant, broad Human flattening or rectangular settlement pad.
Source transforms are recorded in `source-transform-proposal.json`; its initial
analytical status and Viking-role wording predate the final siting above.

Local height: `Data/CampaignCompositionLocal/Composition_3500_r2.png`.
SHA256 `bd84f1986c9fd71b51725647d8ae89644a0681c5d65f2d2aebcf6ab31383ba6b`.
Final material/control/map bindings: `final-presentation-binding.json`.

## Proven and measured

- Exact 36 canonical IDs and 51 legal unordered pairs preserved. They have physical
  candidate positions and fitted corridor data; they are **not** 36 playable runtime
  regions. Existing gameplay, save, construction and battle authority is unchanged.
- 36 native anchor traces initially passed, maximum source/collision difference
  0.506 cm. A further 835 native Landscape probes had zero misses and maximum
  difference 0.456 cm. Orc crossing anchor was subsequently moved roughly 3 m onto
  the exact bridge center; its collision was traced again when applied.
- Four downhill river centerlines, original-datum ocean, two native spill-level
  lakes and five provisional owned stone bridges render in the candidate.
- Human lake: approximately 158 × 137 m basin bounds, water 1.207 m, ~13,140 m².
  Heart River lower lake: water 0.704 m, ~4,427 m². Closed ponds are not treated as
  ocean outlets. Priority-flood filling is an analytical water surface, never a
  wholesale replacement terrain.
- Viking site is ~43.4 m from boundary-connected ocean. Harbor shelter and entry
  exist geographically; actual harbor composition/miniature is not installed.
- Human core remains ~69.7 × 51.4 m, Dwarf ~30.5 × 38.7 m. Human has substantial
  countryside and a nearby lake/tributary. This improves scale; it does not yet
  reproduce the authored city's island/quay relationship in the miniature.
- 18,524 existing owned pines in irregular woodland masses with clearings, road,
  water and settlement exclusions. Native placement had zero collision misses.
  This is provisional regional vegetation, not final six-biome foliage.
- Three initial bridge approach curves were accepted, two Orc curves rejected at
  ~33°. Subsequent fine-grid repair accepted 23/24 problem spans with no terrain
  edit. Current route presentation is `route-local-repair-study-r7.json`.
- Post-channel road/deck-plane check now has **50/51 routes within 22.1°**. The
  `crossroads → river_ford` approach still reaches 25.24° at two short samples.
  This is a known unqualified exception, not a waived grade pass. Widening the local
  search in r8 did not solve it and was not applied.

## Six-region status and unresolved art

| Region | Current physical composition | Remaining work |
|---|---|---|
| Human | Rolling western basin, lake, two tributaries, capital scale reference, woodlots | Actual capital gate/quay/island correspondence, farms, minor waterfront towns |
| Dwarf | Continuous northeastern ridges and high approaches, hold scale reference | Better natural footprint seating, engineered road/pass presentation, quarries |
| Viking | Ocean-connected inlet, coastal relief, boreal masses | Capital/harbor miniature and docks; final snow/coast treatment |
| Orc | Owned mesa/gully patch, Eastern Run, camp scale references | Source boundary still perceptible; broken bridge currently uses an intact stone bridge; proper faction population |
| Nature | Southwest river valley, large woods and clearings | Broader owned foliage mix, riverbanks, shrine/treehold representation |
| Dark | Southern relief and oppressive valley potential | Provisional tinted native surface, no fortress horizon; incomplete causeway presentation |

Roads are material footprints following fitted geography, not final road art.
Some parallel branches, angular bends and abrupt capital trims remain. Two explicit
ferry segments cross the native inlet within existing legal edges; no ferry action
or new topology is implemented. Their landing approaches include dry portions and
need final shore alignment. Do not draw them as bridges or dry roads across sea.

Close views also expose imperfect miniature seating: some minor building bases
intersect sloping ground, and the Dwarf footprint needs a better natural bench.
These are scale-test objects, not production-quality settlement placement. The
Human core is beside water but still lacks a believable final gate/quay approach.

River banks and transitions remain visually simple. Lake stair-stepping and
translucent river/lake overlap were corrected with clipped source triangles and
overlap removal. Ford sill is a bounded ~67.7 m² edit, max raise ~0.981 m, intended
0.25 m depth; its approach is still unqualified. No further terrain edit was made
to force the road-grade check green.

The new owned Six Sides bridge pack is recorded in
`settlement-waterfront-direction.md`. No local source was found in the initial
bounded checks. It was not downloaded or substituted with generated art. Inspect
its actual wooden meshes for minor crossings when available; publisher style is
not itself proof that every mesh fits Soul. Current five bridges use owned
Kingdom_Capital stone geometry as provisional crossing/scale evidence.

## Tests, builds, performance

21 source tests passed (18 campaign-view tests and 3 input tests); the 18-test
campaign-view set also passed directly. Existing native
`Soul.Integration.CampaignWorld.CameraAndSelection` and `TerrainAndRoutes` passed
in the already-built editor. Native source/collision probes and fresh map reload
passed. These do not qualify gameplay movement on the new layout.

No new Soul C++ or project setting was authored. **No fresh SoulEditor/game build,
package, new-map input/save or battle roundtrip is claimed.** The worker contains
inherited unqualified source drafts; they were preserved, not compiled or promoted
as part of this content study. Prior retained-map and authored-city proofs remain
prior evidence, not rerun acceptance for this candidate.

No uncapped performance benchmark: the visual gates are not yet satisfied. Review
used 1920×1080 screenshots and a 12 FPS editor cap under the existing 85°C guard.
`qualification-summary.json` records observed memory/temperature only. Do not
interpret capped editor readings as a 40+ FPS pass or repeat thermal tests now.

## Preservation and evidence

All 253 baseline files matched SHA256, including the retained map, heightfield,
presentation/canonical data, Human/Dwarf authored/proxy packages, Mountain05 source
and all 84 FreshCan payload files. See `preservation-baseline.json` and
`preservation-final.json`. All inherited tracked dirty files matched their initial
hashes except the intentionally appended four composition-ignore lines in
`.gitignore`. No security configuration or licensed payload is committed.

Retained map SHA256:
`d18c21c2aeadf50036ef3a605737bac971e2e91e2a43aaa988d92f5987a8094f`.
Retained height SHA256:
`908f9de444112694afe6a242db86a45c64adef8bcaa392c464e304a1f1c3990b`.
Retained presentation SHA256:
`6d93437e19aaf9efb3683aabace2431ca3b05234dcef935b3c10e0d362e95eaf`.

Open `visual-review.html`. Final 14 views are under
`Local/candidate-r7-review-captures/`; earlier iterations are retained separately.
Rejected work includes closed-pond-as-ocean drainage, cached checkerboard material,
oversaturated orange badlands, repeated southern rock material, stepped lake shore,
river/lake translucency overlap, pre-channel-only route acceptance, and r8's
unsuccessful expanded ford search. No rejected candidate was promoted.

Intentional changes: new `Tools/CampaignComposition` authoring/analysis scripts,
new composition evidence, local candidate maps/materials/water meshes/control data,
and the composition-only ignore hunk. Historical material bootstrap script remains
local and uncommitted; see the tools README. Existing dirty source/configuration,
rejected R10, expansion studies, licensed mounts and machine files remain inherited.

## Exact next action

Review the r7 geographic composition with Jeff before calling it the future
foundation. Preserve the current source and 3.5 km envelope. Next implementation
should finish the single River Ford approach, improve shared road/crossing and
capital waterfront correspondence, and replace provisional surface treatment with
the strongest owned materials. Only after those visual gates should this isolated
layout enter the existing campaign presentation adapter for movement/input/save
qualification and one guarded performance run. Do not restart the terrain bakeoff,
increase the footprint, flatten Human land or resume settlement-framework work.

This is preferable as a **composition direction** to cramming the founder slice,
but the present rendered candidate is not proven superior as finished art to the
retained map and is not Bannerlord/Warhammer quality. No production promotion.
