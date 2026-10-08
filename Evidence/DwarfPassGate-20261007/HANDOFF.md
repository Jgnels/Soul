# Soul Dwarf pass / final visual gate — 2026-10-07

**VISUAL GATE: FAIL. FUTURE PRODUCTION FOUNDATION: NO at this checkpoint.**
The terrain remains the frozen working composition. Do not reopen terrain research, replace the qualified 1.5 km reference, or enable candidate runtime merely because its route checks pass. The requested visual outcome was not achieved.

## Git / scope

Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Starting HEAD / HEAD before scoped closeout commit: `bf106ae4d0b25c107ef2531bfe823956ee6a0b33`.
Ending HEAD and this run's exact commit are in adjacent `final-head.txt` and `commit-receipt.json`, written after the single scoped local commit. These local receipts are outside that commit to avoid a self-referential hash.
No push, merge, reset, clean, stash or donor save. Inherited untracked/licensed work remains local. Only this pass's Tools and compact evidence are committed; candidate assets, screenshots, raw traces and before snapshot remain local-only.

Candidate: `/Game/SoulCampaignComposition/L_Composition_3500_r2`, 3.5 x 3.5 km. Macro geography, heightfield, 36 canonical anchor positions/IDs, and 51 legal endpoint pairs unchanged.

## Dwarf constriction: rejected with evidence

The candidate road dip is at domain XY **2730.915, 675.142 m**, approximately 61 m from the inherited Hold. Its roughly 10.8 m transverse rise at +/-30 m is real, but does not establish a defensible canyon. Rendered flanks roll into accessible grass slopes.

Read-only hypothetical walls across the road were tested without building them or changing gameplay topology:

- 60 m wall: dry bypass **156.853 m** around a 100 m start/end approach; fine 0.25 m grade maximum **21.3184 degrees**.
- 100 m wall: dry bypass **181.338 m**; fine maximum **21.0034 degrees**.
- Independent native Landscape probes: 67 and 78 stations, zero misses, approximately **20.853 degrees** maximum between those coarser stations. The analytical fine check is the denser grade evidence.

This is a terrain-centerline counterexample, not an army-width/navmesh/siege qualification. It is sufficient to reject the claimed natural flank closure; the rendered ground also shows obvious bypass space. No wall, gate or terrain barrier was forced into this dip.

**Nearby constriction used: NO. Dwarf moved: NO.** Before and after actor origin in Unreal cm: `(100197.475953, -116064.598530, 4051.680763)`, scale `(0.5, 0.5, 0.5)`. The inherited gate + exposed Caravan Hall cutaway and previous retaining/seating experiment remain explicitly rejected/provisional. No new Dwarf decoration, city framework, construction logic or authored-environment change.

Receipts: `constriction-bypass-analysis.json`, `constriction-native-bypass.json`, `actor-final-receipt.json`. Review `Local/captures-constriction-before/constriction_context.png`, `constriction_flanks.png`, and final Dwarf views. The initial `captures-constriction-before/constriction_road_axis.png` camera was underground and is rejected; use `captures-final/dwarf_rejected_gate_approach.png`.

## Roads: passing geometry retained; art goal incomplete

Two bounded optimization passes across three conspicuous Crownspine windows produced 12 rejected solver trials. Clamped cubic curves preserved endpoint tangents and were checked independently against the triangle-exact frozen heightfield at 0.1 m locally and 0.25 m across the route. Solver success was never accepted in place of grade evidence. Every candidate violated the 22.1-degree acceptance gate; none was bound to the map.

**Paths removed/merged/moved this pass: zero.** Final route JSON is byte-identical to prior `MapFinalPolish/Local/routes-presentation-final5.json`, copied to this pass's `Local/routes-final.json`. Existing shared trunks remain. Distinct legal destinations retained.

The latest ordinary/Human/mountain/Nature/Orc views were reviewed. Remaining defects include angular Crownspine switchbacks, awkward joins, and some near-parallel/split-rejoin presentation. In particular Snow Pass / Mountain Shrine proximity remains an unresolved art item; a failed grade trial does not justify it artistically. No claim that all duplicate road art is resolved. No terrain alteration to force a passing curve.

## Broken Bridge: bounded local improvement, provisional

Two owned-derived ruined stone visual meshes now have unequal angled break faces and four asymmetric parapet gaps. Original licensed materials retained; no generative art. New packages:
`/Game/SoulCampaignComposition/PassGatePolish/SM_BrokenBridge_West_r1`
and `SM_BrokenBridge_East_r1`.

The original stone actors are hidden visually, with their collision unchanged; new visual actors have no collision. The timber repair, twelve inherited rubble groups, approach and legal passage are unchanged. This preserves the qualified travel surface. Caveat: old parapet collision persists outside the travel lane where visual notches were cut; this is not a fully matched damaged-bridge collision asset.

Reviewed wide/close renders show asymmetric broken parapets, but stone decks and the repair still look too clean. This is a modest presentation improvement, **not finished ruin art**. No other crossing, Human capital, ford, ferry, foliage, terrain, material family or settlement changed.

## Fresh validation

- Canonical structure: 36 IDs / fixed physical anchors / 51 legal pairs preserved.
- Analytical routes: **51/51 PASS**, independent 0.25 m checks.
- Native collision: **51/51 PASS, 52,954 stations, 0 misses**.
- Highest accepted native sampled grade: **22.07781099 degrees**.
- Crossroads to River Ford: **21.76053862 degrees**, unbridged and unchanged.
- Maximum sampled Landscape/heightfield difference: **0.00257593 m**.
- Native grade uses horizontal travel arc, 1 m stations and 0.25 m crossing stations; overlapping intervals below 0.20 m are retained as raw diagnostics but excluded from grade acceptance for ray-query numerical noise. This is not continuous-grade, turning-radius, road-width or runtime movement proof.
- Existing source input/view tests: **21/21 PASS** (`source-tests.txt`).
- Existing built native `Soul.Integration.CampaignWorld.CameraAndSelection` and `.TerrainAndRoutes`: **2/2 PASS** (`native-tests.txt`). This binary was not rebuilt and these tests do not qualify the candidate adapter.
- Fresh SoulEditor / game builds: **NOT RUN**, no C++/runtime source changed.
- Candidate runtime adapter, gameplay camera/input/movement, ferry semantics, F5/F9, construction, visit/return, battle/natural victory: **NOT RUN** because visual gate failed. Editor rendering is not runtime acceptance.
- No `SoulWorldTerrain` / `SoulCampaignExpansion` shortcut enabled. No gameplay/save/battle authority changes.

## Performance / guard

No uncapped native-1080p campaign benchmark: visual/runtime gates not met. No mean/P95/P99/FPS-floor claim. Capped 12 FPS editor review only; measured peak GPU **66 C**, peak VRAM **6925 MiB**, editor peak working set **5275 MiB**, peak private commit **7277 MiB**. The absolute 85 C guard was unchanged. See `review-resource-observations.json`.
Owned editor saved and stopped via guard stop file. Guard termination wait timed out; a subsequent Win32_Process query confirmed owned PID 30296 absent. This is teardown evidence, not gameplay acceptance.

## Preservation

`preservation-final.json`: **368/369 baseline files unchanged**, only the candidate map changed intentionally. **All 33 inherited tracked modifications byte-identical**. Mountain05, FreshCan, Mesa, Human authored city, Dwarf authored environment, retained reference/presentation, frozen 3.5 km height and old route/material assets preserved. Per-file coverage is exactly the baseline manifest, not a claim to hash every unrelated vendor file on disk.

Candidate before: `4a233e1056a5e1190e2afe413cbe4321808b2ae5a15baddf4016a89fb88a896e`.
Candidate after: `c468893303763b1dae1b536884079ceaf40ff2e9e45675869e38e7ed23b4feb4`.
Frozen 3.5 km height: `bd84f1986c9fd71b51725647d8ae89644a0681c5d65f2d2aebcf6ab31383ba6b`.
Retained 1.5 km map: `d18c21c2aeadf50036ef3a605737bac971e2e91e2a43aaa988d92f5987a8094f`.
Retained height: `908f9de444112694afe6a242db86a45c64adef8bcaa392c464e304a1f1c3990b`.
Retained presentation: `6d93437e19aaf9efb3683aabace2431ca3b05234dcef935b3c10e0d362e95eaf`.

Snapshot: `Local/Before/Content/SoulCampaignComposition/`. Exact current local packages: `candidate-asset-manifest.json`. These licensed derivative packages are intentionally not committed. Source recipes are in `Tools/DwarfPassGate/`.

## Evidence / next action

`visual-review.html` contains all eleven final views and matching prior renders where available. Native final images: `Local/captures-final/`. All 51 analytical plates: `route-atlas.html` / `Local/route-atlas/`. Rejected road trials: `mountain-curves-r1.json`, `mountain-curves-r2.json`.

**PROVEN:** preservation, canonical topology, sampled route grades/collision, existing regression tests.
**PROVISIONAL ART:** bridge damage, road presentation, current strategic settlement representations.
**KNOWN DEFECT:** Dwarf cutaway on bypassable terrain; nearby dip cannot secure a pass fort; angular/parallel road artifacts; overly clean bridge surfaces and off-lane visual/collision mismatch.
**NOT YET IMPLEMENTED/QUALIFIED:** convincing Dwarf pass-fort replacement, accepted mountain-road smoothing, candidate runtime adapter and gameplay/performance acceptance.

Next meaningful action needs a bounded presentation/location decision: identify a genuinely defensible existing pass within explicitly authorized placement scope, or choose a suitable enclosed Dwarf mountain-entrance exterior whose role does not falsely claim to seal this open slope. Do not decorate or move the rejected cutaway into the failed 61 m dip. Keep this pass stopped and the geography frozen; do not quietly broaden the search or reshape terrain.
