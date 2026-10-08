"""Assemble scoped comparison evidence with failed visual gates explicit."""
from pathlib import Path
import json,html,hashlib,ast
R=Path(__file__).resolve().parents[2];E=R/'Evidence/MapFinalPolish-20261007';B=R/'Evidence/MapPolish-20261007'
notes={
'whole_labels_minimized':'Frozen 3.5 km heightfield, regional masks and 36 anchors unchanged. Whole-world art remains provisional.',
'ordinary_campaign':'Several equivalent local corridors now share a trunk. Remaining sharp junctions and near-parallel northern approaches are not all art-qualified.',
'human_roads':'Shared Ford, Quarry, Forest and inlet-approach stretches reduce duplicate road marks. Separate legal destinations and the two ferry passages remain.',
'human_capital_approach':'Owned CastleTown cobbles and a 24.50 m forecourt link now reach the represented keep doorway. The r7 miniature omits the authored outer gate/curtain walls: exact island/quay correspondence is NOT qualified.',
'river_ford':'Still unbridged. Owned partly buried bank rocks and a local water-edge feather improve shallow-crossing cues. Frozen terrain and crossing level preserved. Crossroads route: 21.7605 degrees native.',
'major_stone_bridge':'Owned masonry wing walls seat the approach outside the travel lane. Original deck collision and route retained; art remains provisional.',
'minor_timber_bridge':'Actual 13 m Human northern timber crossing, with longitudinal rails and six piers beneath the deck. This is not the old hidden unused woodland bridge.',
'broken_bridge':'Ruined stone ends plus timber repair retained. Twelve small native masonry-rubble clusters soften the broken ends; some cut faces remain too clean.',
'ferry_landing':'Both inlet ferry routes retained. Four existing jetties receive mooring posts. No boat/service simulation or candidate runtime ferry behavior has been qualified.',
'dwarf_approach':'REJECTED PRESENTATION. Retaining masonry and paving fix floor contact but the gate/cutaway hall stands on an open slope. Jeff requires a pass fort to close a real canyon around the road. It does not do so here.',
'mountain_pass':'Bounded curves and short-kink removal preserve the 22.1-degree sampled gate, but conspicuous switchbacks and angular joins remain. The visual gate has NOT passed.',
'orc_routes':'Equivalent Camp/Badlands/War Camp geography shares a trunk. Existing distinct destinations retained. No Mesa heightform or region change.',
'nature_paths':'Local curve and kink corrections improve individual turns. Separate Shrine, Clearing, Woodland and Treehold connections remain; some fitted-graph character is still visible.',
'close_road':'Widths retained: main 3.8 m, secondary 3.0 m, mountain 2.8 m, woodland 2.4 m, Orc military 3.2 m. Surface-mask roadbed remains provisional.',
'dwarf_foundation_close':'Rejected as finished art despite better contact. This is the previously qualified FUNCTIONAL gate/hall cutaway, not a qualified exterior capital.',
'dwarf_context':'Wider geography exposes the defect: the walls do not span a canyon and can be bypassed on the surrounding slope.',
'broken_bridge_close':'Owned rubble details are outside the legal timber passage. The cut stone faces still need a stronger ruin treatment.',
'ford_bank_close':'Local material-only bank feather: full effect within 18 m of the ford, zero beyond 38 m. No generated texture, river mesh edit, water-level change or terrain edit.'}
receipt=json.loads((E/'Local/captures-final6/receipt.json').read_text());views=[(v['name'],E/'Local/captures-final6'/(v['name']+'.png')) for v in receipt['views']]
ford=E/'Local/captures-ford-edge-r2/river_ford.png'
if ford.exists():views=[(n,ford if n=='river_ford' else p) for n,p in views]
views += [('dwarf_foundation_close',E/'Local/captures-curves-r5/dwarf_foundation_close.png'),('dwarf_context',E/'Local/captures-dwarf-context/dwarf_context.png'),('broken_bridge_close',E/'Local/captures-curves-r5/broken_bridge_close.png'),('ford_bank_close',E/'Local/captures-ford-edge-r2/ford_bank_close.png')]
sections=[];images=[]
for name,p in views:
 if not p.exists():continue
 before=B/'Local/captures-final'/(name+'.png');parts=[]
 for q,label in [(before,'Previous MapPolish authority'),(p,'Current trial — not production acceptance')]:
  if not q.exists():continue
  url=__import__('os').path.relpath(q,E).replace('\\','/');parts.append(f'<figure><figcaption>{label}</figcaption><a href="{url}"><img loading="lazy" src="{url}" alt="{name}"></a></figure>');images.append(dict(path=url,sha256=hashlib.sha256(q.read_bytes()).hexdigest()))
 sections.append(f'<section><h2>{html.escape(name.replace("_"," "))}</h2><p>{html.escape(notes[name])}</p><div class="pair">'+''.join(parts)+'</div></section>')
(E/'visual-review.html').write_text('''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Soul final map polish — visual gate open</title><style>body{background:#182027;color:#eee;font:16px/1.5 system-ui;margin:30px}main{max-width:1800px;margin:auto}a{color:#a4d4ef}.notice{border-left:4px solid #da9b62;padding:16px;background:#382f2b}section{margin:35px 0}h2{text-transform:capitalize}img{width:100%;display:block}figure{margin:0;min-width:0}figcaption{background:#27343e;padding:8px}.pair{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}@media(max-width:900px){.pair{grid-template-columns:1fr}}</style><main><h1>Soul · final local map polish</h1><p class="notice"><strong>Visual gate NOT passed. Future production foundation recommendation: NO at this checkpoint.</strong> All 51 sampled route-grade checks pass, but Dwarf placement is rejected and angular road presentation remains. No candidate runtime adapter, fresh gameplay, save/battle roundtrip or performance qualification is claimed. Frozen macro geography remains the working foundation; this is not a new terrain-source decision.</p><p><a href="route-atlas.html">All 51 analytical route plates</a> · <a href="native-route-final-summary.json">Native collision receipt</a> · <a href="HANDOFF.md">Handoff</a></p>'''+''.join(sections)+'</main></html>',encoding='utf-8')
(E/'visual-review-receipt.json').write_text(json.dumps(dict(editor_only=True,resolution=[1920,1080],fps_cap=12,visual_acceptance=False,foundation_recommendation='NO',notes=notes,images=images),indent=2))
log=(E/'Local/editor-r1/unreal.log').read_text(errors='replace');lines=[l for l in log.splitlines() if 'CampaignWorld' in l or 'Queue Empty 2 tests performed' in l];(E/'native-tests.txt').write_text('\n'.join(lines)+'\n');passed=[l for l in lines if 'Test Completed. Result={Success}' in l];assert len(passed)==2
(E/'native-tests.json').write_text(json.dumps(dict(command='Automation RunTests Soul.Integration.CampaignWorld',passed=2,expected=2,fresh_build=False,scope='Existing built campaign tests, NOT candidate gameplay/input/save/battle'),indent=2))
for p in (R/'Tools/MapFinalPolish').glob('*.py'):ast.parse(p.read_text(encoding='utf-8-sig'),filename=str(p))
t=[json.loads(l) for l in (E/'Local/editor-r1/telemetry.jsonl').read_text().splitlines() if l.strip()];gpu=[g for v in t for g in v.get('gpu',[])];mem=[v['process_memory'] for v in t if v.get('process_memory')]
(E/'review-resource-observations.json').write_text(json.dumps(dict(kind='Capped editor review, NOT gameplay performance',fps_cap=12,thermal_cutoff_c=85,peak_gpu_c=max(g['temperature_c'] for g in gpu),peak_device_vram_mib=max(g['memory_used_mib'] for g in gpu),peak_process_working_set_mib=max(m['working_set_mib'] for m in mem),peak_private_commit_mib=max(m['private_commit_mib'] for m in mem),average_frame_ms=None,average_fps=None,p95_ms=None,p99_ms=None,frames_below30=None,frames_below40=None,frames_below60=None,profile_not_run_reason='Required visual gate has not passed; runtime gate not reached.'),indent=2))
print('REVIEW',len(views),'NATIVE_TESTS',len(passed))
