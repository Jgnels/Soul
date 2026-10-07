# Population shift progress — 2026-10-06

Shift started 15:13 UTC; roughly ten-hour budget, ending about 01:13 UTC October 7. Baseline HEAD a1cde1ea9f6e17bb0fc72764fa6b2690a10158cd; no push/merge. Existing dirty tracked files are recorded in preserved-tracked-hashes.json. Retained Landscape is unchanged.

## Human physical binding

The full owned Human city now retains every existing authored sublevel, with owned copies of SL_Houses and SL_Town_Props replacing only their source references. A single native tavern, LevelInstance_149 (LI_Building_05_Fix), plus its 23 independent nearby props/chimney smoke, is bound through three existing SoulSettlementBuildingActor groups to human.tavern. Root counts: 1, 10, 13. Original collision defaults saved intact. No donor writes; exact package hashes/classification in human-authored-groups-r1.json. This is authoring success, not runtime acceptance.

The source tavern was rendered in an owned inspection copy. Native component placements were exported (7,288 instances, 119 meshes, 492,288 lowest-render-LOD triangles). Its authored timber/plaster/stone/slate appearance is retained. Temporary inspection lights are not shipped.

The read-only Human survey now exports actual architectural mesh instances, native level-instance placement metadata and a coarse WorldStatic ground grid. First compile failed on a TObjectPtr deduction and was corrected; editor-survey-build-r2 passed. Guarded native survey r6 is running. No battlefield origin or miniature is accepted yet.

Existing proof observer has been adapted (not yet rebuilt/runtime-tested) for Human groups and recursively streamed children, including all actual input/save/service steps. The full city's army initialization now waits for requested sublevel collision before invoking the existing arena setup; no combat or result rule changes. This integration fix is pending build/runtime verification. Human fixture origin [0,0,0] is explicitly temporary and must be replaced by measured approach coordinates before running gameplay proof.

## Next

Review r6 geometry and collision; derive the matching city miniature with the proven bounded RenderLOD pipeline; bind existing Human DataAsset; build and run actual-input visit/construction/save/battle/fresh-load proof. Freeze settlement framework after this second proof, then use local approved DC/LF/FF/RH/AC/WZ/WC content for retained-map population. No third deep proof, terrain redraw, generative art or source recovery.

## 16:54 UTC checkpoint

Native r6 fully streamed 56,448 actors across 161 visible levels and exported 47,077 architectural placements. All 53,529 StaticMeshActor transforms/mesh paths exactly match r5. Same 61 prior external-actor warning paths; no new warning paths. Key donor maps and retained Landscape hashes match. See human-native-survey-r6-review.json and human-binding-preservation-r1.json.

25 deterministic native architectural parts compiled. Whole-city R1 simplification (329k triangles) is visually rejected: excessive surface loss. R2 preserves town surfaces; R3 also preserves native bridge/waterfront surfaces. Current DA base is SM_HumanCapital_Base_r3 (2,279,587 triangles), upgrade SM_HumanCapital_Upgrade_r1 (5,995 triangles). All native placements preserved; terrain, foliage and tiny dressing omitted only from the campaign derivative. This is heavier than desired and campaign fit/performance are unqualified. Unsaved inspection screenshots remain in Saved/NwiroScreenshots. No neutral diagnostic materials or inspection lights were saved.

Human DataAsset retains unchanged starting buildings and development definitions; only presentation fields bound. Editor build r2 passed. Game build running. Tools suite: 97/99 passed initially; two packaging omissions corrected, all seven packaging tests pass. Broader repeat is deferred until new proof changes settle. Next: actual Human construction/visit/F5/F9/battle qualification, then fresh process/performance and population.

## 17:10 UTC visual correction

Human functional run human-authored-input-r1 is loading the native city. Initial miniature screenshot is reviewed and NOT art-accepted: reduced castle roofs are missing, native lower-elevation structures sink into retained ground, and dry canal walls are geographically inappropriate. Current R3 stays isolated to the proof until corrected. Prepared 12 bounded intact native-part recipes for castle and selected tavern. Proposed campaign-only subset preserves complete castle silhouette plus nearby real town blocks; full authored city and donors remain untouched. Foundation fitting and new rendered comparison are required before acceptance. No terrain mutation.

## 17:15 UTC physical appearance proof

Human native city loaded. Starting and completed clean screenshots reviewed: the selected waterfront tavern is absent initially and physically appears after the existing two-day upgrade. The surrounding authored city stays intact. Runtime checks for three groups / 24 roots and nested native children pass. See human-physical-visual-review-r1.json. F9, return, battle and fresh-load/performance are still running or pending.

## 17:28 UTC bridge gate

Actual F5/F9 restored both existing domains exactly, including mid-construction physical rollback. Completed state survives campaign return and repeat visit. Actual H companion service purchase charges the existing price. B enters the full authored Human environment through the existing realtime battle mode; pending native collision load before army initialization. Battle/reinforcement/result and fresh process/performance remain pending. Current miniature art correction prepared but not yet authored while this single runtime owns the heavy lane.

## 17:31 UTC thermal stop and bounded correction

Human functional r1 stopped by the unchanged 85 C guard during the third city load (battle), before armies initialized. No full functional/battle pass. Earlier construction, physical state, exact save restoration, service purchase and visit/return assertions passed. Code inspection found the deferred battle startup still rendering its temporary default view while native levels stream. Added loading-only viewport world-render suppression, with prior state restored before real battle initialization and on EndPlay. Existing HUD/status stays active, and no frame caps, battle rules or quality settings change. This narrow correction requires editor/game rebuild and real retry.

## 18:30 UTC Human gate and native LOD finding

The complete Human runtime r1 passed actual construction, physical tavern visibility, service purchase, exact two-domain F5/F9 including mid-construction rollback, campaign return and repeat visit through step 21. The subsequent authored battle load hit the unchanged 85 C cutoff before army initialization. It is not a battle pass. A loading-only viewport suppression correction and bounded restored-checkpoint battle observer are built in both SoulEditor and Soul. Runtime retry remains pending.

R3 and R5 miniatures remain unaccepted. A native SM_ConeRoof_01 at forced source LOD5 has actual holes even with neutral unlit material; screenshots in Saved/NwiroScreenshots/human-native-roof-lod5-neutral-unlit.png and source-LOD0 comparison isolate this from donor material warnings. The derivative now tests native RenderLOD0, 0.01 cm coincident-edge welding and per-mesh attribute-aware simplification. All 72 owned castle source derivatives completed, preserving original source/material paths and receipts; seven native prefab assemblies are being rebuilt. No source packages or full city placements changed. Current scenario still points to R3 until the new render is reviewed.

The retained terrain remains the existing 1.5 km corridor with nine physical anchors. Pending population will use approved local assets where those actual region roles and measured ground fit permit; this does not establish six full macro-regions or authorize moving their capitals into unrelated corridor nodes.

## 19:20 UTC second-proof functional result

Human restored battle r2 passed fresh-process exact campaign/settlement restoration, matching starting/upgraded R6 miniatures, actual native city battle (30 initial bodies), real reserves [16,15], natural victory (26 survivors, zero enemies), campaign return and post-result F5/F9. No forced result. Loading-only render suppression avoided the previous thermal failure. Native city battle load was approximately 17 minutes; whole run 1422 seconds. This is functional acceptance only: the default battlefield camera is hidden by native forest at origin [-9000,21000,400], so battle placement is visually rejected. One bounded approach/camera correction remains, plus uncapped performance. Full city, selected native tavern and donors remain intact.

R6 miniature now uses repaired native RenderLOD0 castle source, 0.01cm seam weld and attribute-aware per-mesh reduction. Base 1,977,644 triangles, tavern 58,888. Roof surfaces restored. R6 actual fresh start/restored images visibly differ by the tavern; campaign layout/hinterland remains medium-detail, not final art. Diagnostic derivatives are not accepted shipping content.

Seven approved Forest_village / Medieval_Warzone prefabs were inspected as real native assemblies and their source transforms/materials exported. Compilation is next. A source-only change reconnects existing measured Crownstead farms/verges to its authored miniature, which previously bypassed that dressing; pending build/render. No terrain changed.

## 19:50 UTC population implementation checkpoint

Eight owned native derivatives are saved and hash-verified: two Forest_village dwellings, a native wooden platform, two Medieval_Warzone defense clusters, tent/supply group, trebuchet/supplies, and Ravenhold gatehouse tower. Ravenhold SM5 failed at the 16-sampler limit; one bounded owned-copy correction uses shared wrap samplers in the original graph/function and six native material-instance copies. Original texture parameters, UVs and geometry remain. Actual unlit before/after screenshot reviewed: checkerboard removed. Source donor hashes unchanged for all copied dependencies; full 1,597-package Ravenhold baseline completed. No third deep authored-city proof.

Eighteen measured native assembly placements across seven existing corridor nodes are implemented, pending runtime rendering. Native footprints use 169 ground samples, road clearance, uniform proportions and <=65cm relief; short decorative approach paths connect to actual existing road segments only when <=22 degrees. Forest generic scatter and AlienPlanet Orc camp/watch markers are replaced by reviewed native forest/camp/outworks assemblies. Orc full-capital environment routing remains pending; these are provisional occupied outworks, not a completed major city. Existing capital farms/verges now also dress the stateful Human miniature. Retained terrain bytes remain unchanged.

Human battle presentation correction is limited to runtime tree-instance removal inside a 100x86m elliptical approach clearing, with its initial camera facing toward town. Native architecture, Landscape and low vegetation remain; no donor/map writes or visit-world changes. Current rebuild follows one corrected UE TObjectPtr deduction error. Actual battle visual acceptance and fresh uncapped performance still pending.

Paired population captures reuse the existing retained capture sequence at 320m ordinary strategic distance, with an explicit SoulPopulationReview visibility override. It affects art-review presentation only; normal fog/collision/save behavior is unchanged. Eighteen current campaign-view source checks pass.

## 20:15 UTC Human real battle accepted; framework frozen

Corrected r3: real natural victory, 27 allied survivors / zero enemies; 16/15 real reserve bodies arrived. Exact fresh restoration, native group state, campaign result return and post-result F5/F9 pass. Both battle screenshots reviewed: troops are visible on native ground with recognizable authored town/waterfront backdrop. The bounded runtime-only clearing omits 202 tree instances; architecture, Landscape, low vegetation, donor and visit world remain intact. Clean exit0; no crashes. See human-authored-battle-r3-review.json for telemetry and baseline-warning comparison.

The existing Dwarf architecture generalizes to the native Human tavern. Authored-settlement framework development is frozen. No third deep proof. Fresh Human visit/performance qualification remains, then use the proven pipeline for retained population.

## 20:30 UTC current builds and fresh visit

SoulEditor r8 passed (64.12 s); Soul game population r1 passed (248.11 s). Latest ordinary-view source checks remain 18/18. Human fresh-performance r1 has restored both domains exactly in a new process and entered the full city through actual V input. Native uncapped city/campaign measurement and clean return remain pending. Current evidence gallery now shows reviewed physical state, R6 before/after restoration, corrected real battle and campaign return; rejected R3 remains explicitly marked.

## 20:47 UTC Human fresh measurements; thermal limitation

Native fresh checkpoint restore, completed physical city state and visit/return checks passed and screenshots are reviewed. City measured 23.185 ms mean (43.13 FPS), P95 28.132, P99 32.027, peak 84 C. Three-node campaign measured 11.362 ms mean (88.01 FPS), but reached the unchanged 85 C cutoff near the window end. The guard terminated the owned process: combined fresh-performance run is FAILED, notwithstanding the native completion marker. Full raw profiles and thermal receipt preserved. Do not weaken the cutoff or call this a clean performance pass. Human second-proof functional/battle result stands; framework remains frozen. Moving to paired retained population renders.

## 21:55 UTC retained population iteration

Human qualification committed at e30b5df88c4c9db9528b61cf69b04dbb55731284. Human full-city functional/battle proof stands; combined fresh uncapped run remains thermally failed. No third deep city.

Before/after r1 capture pair completed cleanly with eighteen identical actual campaign cameras; normal nine-node input r1 passed movement, recruitment, knowledge/selection, camera and exact F5/F9. Native forest huts replace small generic scatter; native occupied Orc outworks/camps replace AlienPlanet markers. R1 remains medium-detail: Human miniature too small, sparse disconnected districts, thin crops, dark forest wood, isolated camp towers. Six macro-regions are not physically established by this retained nine-node corridor. No invented capital relocation.

R7 Human campaign-only derivative doubles uniform miniature scale to 0.24, refits foundations and omits the detached island front gate; full authored environment remains unchanged. Base 1,874,909 triangles; same 58,888-triangle native tavern upgrade. R2 actual capture completed cleanly (287.47s), twenty FPS functional cap. Larger silhouette reads better, but new Human street ribbons have reversed winding and render black: rejected pending correction. Connecting native Ravenhold wall improves the camp but adds native SM5 floor-material sampler warning; bounded owned-copy repair pending. Source winding is corrected, not yet rebuilt/rendered. No terrain/map bytes changed.

Owned Forest material-instance and crop mip-coverage copies remain unbound candidates. Immediate staging screenshots can precede viewport updates; settled actual runtime capture is visual authority. Native materials retained until a demonstrably better correction is reviewed.

Eight qualified Medieval modular buildings are fitted for Crossroads (six) and Old Quarry support (two), with <=65cm footprint relief, unchanged-road clearance and native frontage. These are appropriate minor-site uses of 60k LOD1 derivatives, not a third capital proof. Build/render pending. Existing native campaign benchmark will provide final uncapped measurements; the redundant uncommitted observer branch was removed exactly against its pre-edit snapshot.

## 23:30 UTC final native population qualification

R6 actual render passes all 22 Lit 1080p views, unchanged campaign snapshot and clean exit (280.66 s). Thirty-six native assemblies across eight retained sites plus Human R7; no rejected footprints or material-usage fallbacks. North Pass tower moved 11.18 m to a measured existing parcel: one visible approach now passes at 17.75 degrees. Field defenses remain unconnected. Native terrain/map/profile hashes are unchanged. Final ordinary and close views remain provisional art, with sparse wider composition, weak crops, mismatched shrine core and uneven miniature scale.

R7 actual construction/service controls confirm exact both-domain F5/F9, clean exit; independent fresh F9 visibly restores the native tavern. Ten existing native tests pass (settlement 7, campaign geometry/selection 2, settlement save 1). Default-Mesa native suite scope is distinct from actual retained nine-node input/render/height qualification. Editor approach build passes 41.58 s; game approach build passes 89.31 s. Final normal input after the single placement correction is running.

Uncapped nine-node campaign run FAILED at 85 C. Its native 20 s warmup / 45 s fixed Crownstead window produced mean 12.1859 ms (82.06 FPS), P95 13.505, P99 14.3069; below 30/40/60 counts 1/1/3 of 3693. The native window completed during guarded close handling: diagnostic data, not sustained thermal acceptance. Correct evidence is under that run's isolated User/Saved. Older shared Saved copies were explicitly quarantined and excluded. Human full-city separate combined run also remains thermally failed. No cap or cutoff was changed.

Full SHA256 reread of 3,017 donor packages across five families passes unchanged. Retained-only source/index selection is being prepared to commit the population code without sweeping in the prior rejected R10 implementation or machine/security state. All live working files are preserved.

## Final bounded closeout — 2026-10-07 00:10 UTC

Jeff narrowed the stop condition to the capital-road correction, minimum regression and preservation. Content and framework are frozen. Final 52.334 m route tail clears native Human footprints by >=340.69 cm centerline (>=80.69 cm road edge), grade <=12.95 degrees, preserving both anchors/legal connection and all terrain/city bytes. Ordinary/west/south screenshots visibly verify the correction. No new content or other polish.

Final R7 render: 22 Lit 1080p captures, unchanged campaign snapshot, 36 placed/zero rejected, no new material fallback, clean exit 265.03 s, peak 66 C under functional 20 FPS cap. North Pass remains two placements/one valid lane at 17.75 degrees. Normal input r4 passes all movement, recruitment, selection, camera/controller and F5/F9 assertions, clean exit 196.31 s; fresh r2 restores exact campaign state and the native tavern visibly, clean exit 113.11 s. No expensive Human full-city rerun.

Editor build 47.07 s and game build 94.40 s pass. Tools 99/99 and view tests 18/18 pass. Final native geometry/selection 2/2 pass; the preceding settlement/save set remains qualified. The final test editor required its owned-child guard termination after requested close (exit 1); native test success is not presented as a clean runtime exit. All functional game sessions above exited cleanly. No new thermal test; both prior 85 C failures stand.

Twenty-two untouched inherited tracked files remain byte-identical to shift start. Mixed inherited source/config files keep old R10 edits in their live working copies; only retained-only index blobs enter the milestone commit. No licensed mesh/map/material payload, credentials or machine-specific config is staged. Exact final HEAD and complete remaining dirty-file list are in the authoritative handoff. Stop after closeout.

## Final bounded closeout - 2026-10-07 00:10 UTC

Jeff narrowed the stop condition to the capital-road correction, minimum regression and preservation. Content and framework are frozen. Final 52.334 m route tail clears native Human footprints by at least 340.69 cm centerline (80.69 cm road edge), grade <=12.95 degrees, preserving both anchors/legal connection and all terrain/city bytes. Ordinary/west/south screenshots visibly verify the correction. No new content or other polish.

Final R7 render: 22 Lit 1080p captures, unchanged campaign snapshot, 36 placed/zero rejected, no new material fallback, clean exit 265.03 s, peak 66 C under functional 20 FPS cap. North Pass remains two placements/one valid lane at 17.75 degrees. Normal input r4 passes movement, recruitment, selection, camera/controller and F5/F9 assertions, clean exit 196.31 s; fresh r2 restores exact campaign state and the native tavern visibly, clean exit 113.11 s. No expensive Human full-city rerun.

Editor build 47.07 s and game build 94.40 s pass. Tools 99/99 and view tests 18/18 pass. Final native geometry/selection 2/2 pass; preceding settlement/save tests remain qualified. Final test editor required owned-child guard termination after requested close (exit 1); native test success is not a clean runtime exit. All functional game sessions above exited cleanly. No new thermal test; both prior 85 C failures stand.

Twenty-two untouched inherited tracked files remain byte-identical to shift start. Mixed inherited source/config files keep old R10 edits in live working copies; only retained-only index blobs enter the milestone commit. No licensed mesh/map/material payload, credentials or machine-specific config is staged. Exact final HEAD and complete remaining dirty-file list are in the authoritative handoff. Stop after closeout.
