from pathlib import Path
import json,html,hashlib,subprocess
R=Path.cwd();E=R/'Evidence/DwarfPassGate-20261007';B=R/'Evidence/MapFinalPolish-20261007'
p=json.loads((E/'preservation-final.json').read_text());nr=json.loads((E/'native-route-final-summary.json').read_text());bp=json.loads((E/'constriction-bypass-analysis.json').read_text())
telem=[json.loads(l) for l in (E/'Local/editor-r1/telemetry.jsonl').read_text().splitlines()]
resource=dict(capped_editor_only=True,cap_fps=12,campaign_performance_not_run=True,peak_temperature_c=max(g['temperature_c'] for t in telem for g in t['gpu']),peak_vram_mib=max(g['memory_used_mib'] for t in telem for g in t['gpu']),peak_editor_working_set_mib=max(t['process_memory'].get('peak_working_set_mib',0) for t in telem),peak_editor_private_commit_mib=max(t['process_memory'].get('private_commit_mib',0) for t in telem),thermal_cutoff_c=85,stop_reason='requested_authoring_close',cleanup='Guard wait timed out after termination; subsequent Win32_Process query confirmed PID 30296 absent.')
(E/'review-resource-observations.json').write_text(json.dumps(resource,indent=2))
views=['whole_labels_minimized','ordinary_campaign','human_roads','dwarf_context','dwarf_close','dwarf_rejected_gate_approach','mountain_pass','broken_bridge_wide','broken_bridge_close','nature_paths','orc_routes']
notes={
 'whole_labels_minimized':'Frozen 3.5 km geography unchanged. This candidate remains opt-in; no production promotion.',
 'ordinary_campaign':'Roads unchanged. Distinct destinations remain; shallow near-parallel loops and geometric junctions still need art review.',
 'human_roads':'Current accepted arrivals retained. No Human capital, ford, lake or terrain change.',
 'dwarf_context':'FAIL: rejected cutaway remains at its inherited location. Nearby constriction has bypassable grassy flanks.',
 'dwarf_close':'FAIL: exposed Caravan Hall interior remains a proof miniature, not a convincing exterior pass fort.',
 'dwarf_rejected_gate_approach':'Evidence of failure, not a road-through-gate acceptance view. The proposed constriction was not built upon.',
 'mountain_pass':'Angular bends remain. Both bounded spline trials failed independent fine grade checks and were rejected.',
 'broken_bridge_wide':'Owned stone ends now have asymmetric parapet gaps; original timber passage retained. Small improvement, still provisional.',
 'broken_bridge_close':'Angled cut faces and four unequal parapet breaks. Original collision retained on hidden stone actors; new visible derivatives have no collision. Off-lane missing-parapet collision is provisional.',
 'nature_paths':'No route changes accepted this pass. Tight turns and joins remain visible.',
 'orc_routes':'No macro/material transition edits. Broken Bridge is the only changed content.'}
oldmap={'dwarf_context':'dwarf_approach','broken_bridge_wide':'broken_bridge'}
cards=[]
for name in views:
 old=B/'Local/captures-final6'/(oldmap.get(name,name)+'.png')
 if not old.exists():
  hits=list((B/'Local').glob('captures-*/'+name+'.png'));old=hits[-1] if hits else None
 pair=''
 if old and old.exists():pair='<figure><figcaption>Prior MapFinalPolish</figcaption><img loading="lazy" src="../MapFinalPolish-20261007/'+old.relative_to(B).as_posix()+'"></figure>'
 pair+='<figure><figcaption>Current DwarfPassGate</figcaption><img loading="lazy" src="Local/captures-final/'+name+'.png"></figure>'
 cards.append('<section><h2>'+name.replace('_',' ')+'</h2><p>'+notes[name]+'</p><div class="pair">'+pair+'</div></section>')
(E/'visual-review.html').write_text('''<!doctype html><meta charset="utf-8"><title>Soul final visual gate: FAIL</title><style>body{background:#172127;color:#e7e7df;font:17px system-ui;max-width:1700px;margin:auto;padding:28px}a{color:#a8d8e9}h1{color:#ffd4a1}.pair{display:flex;gap:12px}figure{margin:0;flex:1;min-width:0}img{width:100%}section{margin:38px 0}figcaption{padding:8px 0}</style><h1>VISUAL GATE: FAIL · FUTURE PRODUCTION FOUNDATION: NO</h1><p>The 3.5 km geography remains the working foundation. This pass disproved the nearby Dwarf pass-fort placement and preserved it as provisional. No runtime or performance work was undertaken.</p><p><a href="HANDOFF.md">Handoff</a> · <a href="route-atlas.html">51-route analytical atlas</a> · <a href="native-route-final-summary.json">Fresh native 51/51 qualification</a></p><p>Native screenshots below, not generated artwork. Paired views are matching where available. Dwarf failure views deliberately show the unchanged rejected representation.</p>'''+''.join(cards),encoding='utf-8')
maxgrade=max(r['max_grade_deg'] for r in nr['routes']);candidate=next(r for r in p['files'] if r['path'].endswith('L_Composition_3500_r2.umap'))
trials=sum(len(json.loads((E/f'mountain-curves-r{i}.json').read_text())['trials']) for i in [1,2])
starting=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
(E/'HANDOFF.md').write_text(f'''# Soul Dwarf pass / final visual gate — 2026-10-07

**VISUAL GATE: FAIL. FUTURE PRODUCTION FOUNDATION: NO at this checkpoint.**
The terrain remains the frozen working composition. Do not reopen terrain research, replace the qualified 1.5 km reference, or enable candidate runtime merely because its route checks pass. The requested visual outcome was not achieved.

## Git / scope

Branch: `codex/soul-bannerlord-campaign-map-20260929`.
Starting HEAD / HEAD before scoped closeout commit: `{starting}`.
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

Two bounded optimization passes across three conspicuous Crownspine windows produced {trials} rejected solver trials. Clamped cubic curves preserved endpoint tangents and were checked independently against the triangle-exact frozen heightfield at 0.1 m locally and 0.25 m across the route. Solver success was never accepted in place of grade evidence. Every candidate violated the 22.1-degree acceptance gate; none was bound to the map.

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
- Native collision: **51/51 PASS, {nr['points']:,} stations, {nr['misses']} misses**.
- Highest accepted native sampled grade: **{maxgrade:.8f} degrees**.
- Crossroads to River Ford: **21.76053862 degrees**, unbridged and unchanged.
- Maximum sampled Landscape/heightfield difference: **{nr['max_landscape_height_delta_m']:.8f} m**.
- Native grade uses horizontal travel arc, 1 m stations and 0.25 m crossing stations; overlapping intervals below 0.20 m are retained as raw diagnostics but excluded from grade acceptance for ray-query numerical noise. This is not continuous-grade, turning-radius, road-width or runtime movement proof.
- Existing source input/view tests: **21/21 PASS** (`source-tests.txt`).
- Existing built native `Soul.Integration.CampaignWorld.CameraAndSelection` and `.TerrainAndRoutes`: **2/2 PASS** (`native-tests.txt`). This binary was not rebuilt and these tests do not qualify the candidate adapter.
- Fresh SoulEditor / game builds: **NOT RUN**, no C++/runtime source changed.
- Candidate runtime adapter, gameplay camera/input/movement, ferry semantics, F5/F9, construction, visit/return, battle/natural victory: **NOT RUN** because visual gate failed. Editor rendering is not runtime acceptance.
- No `SoulWorldTerrain` / `SoulCampaignExpansion` shortcut enabled. No gameplay/save/battle authority changes.

## Performance / guard

No uncapped native-1080p campaign benchmark: visual/runtime gates not met. No mean/P95/P99/FPS-floor claim. Capped 12 FPS editor review only; measured peak GPU **{resource['peak_temperature_c']:.0f} C**, peak VRAM **{resource['peak_vram_mib']:.0f} MiB**, editor peak working set **{resource['peak_editor_working_set_mib']:.0f} MiB**, peak private commit **{resource['peak_editor_private_commit_mib']:.0f} MiB**. The absolute 85 C guard was unchanged. See `review-resource-observations.json`.
Owned editor saved and stopped via guard stop file. Guard termination wait timed out; a subsequent Win32_Process query confirmed owned PID 30296 absent. This is teardown evidence, not gameplay acceptance.

## Preservation

`preservation-final.json`: **368/369 baseline files unchanged**, only the candidate map changed intentionally. **All 33 inherited tracked modifications byte-identical**. Mountain05, FreshCan, Mesa, Human authored city, Dwarf authored environment, retained reference/presentation, frozen 3.5 km height and old route/material assets preserved. Per-file coverage is exactly the baseline manifest, not a claim to hash every unrelated vendor file on disk.

Candidate before: `{candidate['before_sha256']}`.
Candidate after: `{candidate['sha256']}`.
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
''',encoding='utf-8')
print('DOCUMENTED',resource)
