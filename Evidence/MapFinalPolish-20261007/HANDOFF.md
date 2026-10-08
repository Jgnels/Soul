# Final map polish checkpoint — visual gate remains open

Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Starting HEAD: `e714535172049f7ebfddb7729ea583d8a53ee03b`.
Ending HEAD / local commit: recorded after the scoped checkpoint commit in `final-head.txt` (kept as a local receipt to avoid a self-referential commit hash).
Candidate: `/Game/SoulCampaignComposition/L_Composition_3500_r2`.
No push, merge, reset, clean, stash, donor save or production promotion.

**Future production foundation recommendation: NO at this checkpoint.**
Keep the frozen 3.5 km geography as the working composition; this does not reopen source research. The visual gate is not passed, so candidate runtime integration and performance qualification were not started. The requested complete mission is not achieved.

## Current decisive blocker: Dwarf representation

Jeff reviewed the supported Hold screenshot and correctly rejected its defensive logic: a gate fort only makes sense here if the road passes through a major canyon and the walls close the passage. The current structure stands on an open slope and can be bypassed.

The inherited r9 miniature is a native-derived **gate plus cutaway Caravan Hall**, not a finished exterior capital. The prior `Continuation-20261006/dwarven-proof-review.md` explicitly says to accept functional shared state/source correspondence but not finished capital art. Foundation engineering did not cure that representation mismatch.

This run raised the miniature rigidly 3.25 m without changing XY/scale and added owned DwarvenCitadel retaining walls, rectangular paving and a 10.86-degree ramp. A 13.85 m, 20.19-degree foot spur connects the snow-basin approach. These remain **rejected presentation experiments**, not a qualified Hold solution. No terrain pad was made. The rear approach still meets the closed hall side. Do not promote these experiments merely because floor contact improved.

Read-only terrain sections at the gate show one flank about 0.16 m higher and the other about 1.96 m lower than the center at 15 m lateral offsets; this is not a canyon. A regional road-constriction screen is in `dwarf-existing-constriction-screen.json`. Its best nearby transverse dip is 61 m away with roughly 10.8 m rise on both sides at 30 m offsets; it has NOT been visually qualified as an impassable canyon or approved for relocation. No anchor or gate was moved to it.

A direction question was presented to Jeff: keep the Hold location and replace the cutaway with a suitable native exterior, fit the gate into a genuine existing pass with a bounded placement adjustment, or leave Dwarf placement provisional. No answer is assumed. No new city proof/framework is needed for any eventual presentation correction.

## What changed and what remains

- Four local route uses gradually share existing Human/forest trunks: Crossroads–Ford with Ford–Orc Watch; Forest Edge–Orc Watch and Forest Edge–North Pass with the existing southern-crossing corridor; Crossroads–Quarry with Crossroads–Forest Edge. An Orc Camp–Badlands stretch also shares the War Camp road where their geography already coincides.
- No legal edge was removed. Distinct settlement/pass/ferry destinations remain. A Snow Pass–Mountain Shrine sharing attempt failed the frozen-ground grade gate and was rejected. Its near-parallel stretch remains a known art-review item, not automatically justified by that numerical failure.
- Bounded curve trials: initial local rounding, then 13 tangent-curve changes and 16 short-backtracking removals. Some mountain switchbacks, Y joins and fitted-graph character remain. Do not claim every visible duplicate or turn is resolved.
- Human keep: a 24.50 m forecourt spur (6.90-degree maximum) and owned native CastleTown cobbles reach the represented keep doorway. Capital XY/scale unchanged. The miniature omits the authored outer gate/curtain walls; exact island/quay correspondence is **not** proven. No authored city edit.
- Stone bridges: eight owned masonry wing-wall details outside travel lanes. Decks/collision unchanged.
- Minor northern timber bridge: six supports beneath the existing 13 m deck; longitudinal rail orientation retained.
- Broken Bridge: ruined stone ends plus timber repair retained; twelve small owned masonry-rubble groups added outside passage. Cut faces remain too clean.
- Both inlet ferries retained, with eight mooring posts on the four existing jetties. No road or giant bridge across the sea; no new ferry gameplay system.
- River Ford remains unbridged, with partly buried owned bank rocks. A copy of the licensed Forest_village water material now feathers the final 1 m at each bank only near the ford: full effect inside 18 m, zero beyond 38 m. Water geometry/level, collision and terrain unchanged. Matching renders show a softer local edge; the wider river ribbon remains provisional.
- No further faction population, Mesa terrain change, Human flattening, Crownspine reshape, new river, or settlement framework work.

## Verified engineering

`route-inventory.json` / `route-atlas.html` verify the same 36 IDs, anchor coordinates and 51 canonical endpoint pairs. All 51 analytical checks pass at up to 0.25 m sampling.

`native-route-final-summary.json`: **51/51 PASS, 52,954 stations, zero misses**. Highest accepted sampled route grade 22.07781099 degrees. Crossroads–River Ford **21.76053862 degrees**. Maximum sampled Landscape-vs-heightfield difference 0.00257593 m. Native sampling uses 1 m travel stations and 0.25 m crossing stations; intervals under 0.20 m from overlapping station sets are retained in raw diagnostics but excluded from acceptance to avoid millimetre query noise. This is not a continuous-grade, turning-radius, road-width-clearance or runtime movement guarantee.

- Source regression: 21/21 PASS (`source-tests.txt/json`).
- Existing built native `Soul.Integration.CampaignWorld.CameraAndSelection` and `.TerrainAndRoutes`: 2/2 PASS (`native-tests.txt/json`). They do not qualify this candidate's runtime adapter.
- SoulEditor fresh build: NOT RUN; no C++/runtime source changed this pass.
- Soul game fresh build: NOT RUN, same reason.
- Candidate gameplay map load/camera/input/movement/bridges/ford/ferry: NOT QUALIFIED. Editor map load and review rendering work.
- Candidate F5/F9, construction visibility, visit/return, natural victory or defeat/retry: NOT RUN; visual-first gate remains closed. Prior authored-city proof is preserved, not re-claimed as candidate integration evidence.

## Runtime source audit boundary

Read-only audit found inherited World/R10 and Expansion drafts in terrain/camera/world-actor/state/player-controller/HUD/build/config paths. They remain exactly as inherited and uncommitted. Do not enable `SoulWorldTerrain` or `SoulCampaignExpansion` to shortcut candidate integration. A future opt-in presentation binding must use existing authority, preserve baked road art, distinguish ferry travel height, and reuse existing settlement miniature/state adapters. No competing system was created.

## Performance

No uncapped gameplay performance run: visual and runtime gates did not pass. Average FPS/frame time, P95/P99 and frames below 30/40/60 are deliberately unreported.

Capped 12 FPS editor review telemetry is in `review-resource-observations.json`; it is not a campaign benchmark. The 85 C hard guard remained intact. Observed peak GPU temperature was 64 C. Exact final resource maxima are in the receipt. No thermal-loop benchmark.

## Preservation / exact local asset state

`preservation-baseline.json` and `preservation-final.json` cover 356 baseline files. **355 unchanged; only the candidate umap changed intentionally.** All 33 inherited tracked modifications are byte-identical to their start hashes, including machine-specific/configuration and inactive drafts. New derived presentation assets are listed in the candidate asset manifest; they remain local-only.

- Candidate before: `fe7a6ab40c270fc18d1beb5ee6e782fa4cf3d1fae3091e19c263d78f1766ba6e`.
- Candidate after: `4a233e1056a5e1190e2afe413cbe4321808b2ae5a15baddf4016a89fb88a896e`.
- Frozen 3.5 km height: `bd84f1986c9fd71b51725647d8ae89644a0681c5d65f2d2aebcf6ab31383ba6b`.
- Retained 1.5 km map: `d18c21c2aeadf50036ef3a605737bac971e2e91e2a43aaa988d92f5987a8094f`.
- Retained height: `908f9de444112694afe6a242db86a45c64adef8bcaa392c464e304a1f1c3990b`.
- Retained presentation: `6d93437e19aaf9efb3683aabace2431ca3b05234dcef935b3c10e0d362e95eaf`.
- Mountain05, FreshCan, Mesa, Human authored and Dwarf authored baseline files unchanged; exact per-file hashes in preservation receipts.

Before snapshot: `Local/Before/Content/SoulCampaignComposition/`.
Current roads: `Local/routes-presentation-final5.json`; current mask material texture `/Game/SoulCampaignComposition/FinalPolish/T_RoadControl_final5`.
Local bank material: `/Game/SoulCampaignComposition/FinalPolish/MI_FordBankWater_r2`.
Source recipes and rejected-trial warnings: `Tools/MapFinalPolish/README.md`.

## Evidence and next action

`visual-review.html` has matching prior/current views, plus Dwarf close/context, broken-bridge close and ford close. Primary final set: `Local/captures-final6/`; latest ford: `Local/captures-ford-edge-r2/`. Other Local captures preserve rejected/intermediate iterations. `Local/route-atlas/` has all 51 analytical plates. No generated game art.

Next meaningful action: resolve the Dwarf exterior/pass-fort placement direction using the actual native environment and existing terrain. Do not keep decorating this unsupported cutaway. Then finish the remaining conspicuous road joins, clear the visual gate, and only then bind/qualify the existing campaign presentation adapter. Do not replace the retained map automatically.

Owned capped editor process was saved and stopped through its existing guard stop file. Guard receipt: `requested_authoring_close`, `terminate_owned_child`, exit code 1; this is intentional teardown, not a gameplay acceptance result.
