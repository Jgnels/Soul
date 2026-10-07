"""Index actual captures and qualification receipts; no generated game content."""
from pathlib import Path
import json,html,hashlib,ast,re
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionWorldComposition-20261007';L=E/'Local'
captions={
'whole_labels_minimized':'3.5 km world without labels. Ridge belts, inlet and woodland masses are present. Materials and road presentation remain provisional; square outer bounds are exposed.',
'ordinary_heartland':'Human countryside has room around the unchanged capital core. Some parallel paths and unfinished approaches remain.',
'human_capital_scale':'Unchanged owned Human miniature beside lake/tributary. Full authored city untouched. Strategic island/quay correspondence and gate entry are incomplete.',
'dwarf_mountains':'Continuous Crownspine relief and small hold reference. Natural bench seating and engineered approaches still need work.',
'northern_coast':'Ocean-connected inlet and boreal terrain. Actual Viking harbor miniature/dock composition is not yet placed.',
'orc_eastern_terrain':'Owned Mesa forms provide distinct gullies and plateaus. Saturated orange iteration rejected. Source transition and surface treatment remain visible.',
'nature_terrain':'Coherent woodland masses, clearings and a downhill river valley. Foliage family diversity and bank art remain provisional.',
'dark_southern_terrain':'Southern mountain forms retained. Repeated rock texture rejected in favor of tinted native surface. No fortress horizon yet.',
'pass_study':'Relief constrains the route. Road mask has angular bends; this is not finished pass construction.',
'ford_candidate':'River Ford is a real channel crossing. The crossroads approach still has a short 25.24 degree grade exception and is not accepted.',
'minor_settlement_scale':'Minor buildings remain small relative to terrain. Some bases intersect the slope; scale proof, not finished village placement.',
'close_ground_road':'Close material and minor-building inspection. Surface repetition, road edges and seating remain provisional.',
'major_river':'Human tributary and lake context. All four centerline profiles descend; bank detail and source transition are unfinished.',
'bridge_bank_inspection':'Road bends into the shortest practical measured bank span. Native owned stone bridge seated on banks; lightweight wooden options remain pending source inspection.'}
cards=[];images=[]
for name,caption in captions.items():
 p=L/'candidate-r7-review-captures'/(name+'.png');assert p.exists(),p
 rel=p.relative_to(E).as_posix();images.append(dict(name=name,path=rel,sha256=hashlib.sha256(p.read_bytes()).hexdigest(),review=caption))
 cards.append(f'<section id="{name}"><h2>{html.escape(name.replace("_"," "))}</h2><a href="{rel}"><img loading="lazy" src="{rel}" alt="{html.escape(caption)}"></a><p>{html.escape(caption)}</p></section>')
page='''<!doctype html><html><head><meta charset="utf-8"><title>Soul composition candidate — not promoted</title><style>body{background:#14191c;color:#e2e5df;font:17px/1.5 system-ui;margin:auto;max-width:1500px;padding:30px}h1{font-size:30px}h2{font-size:21px}a{color:#a6d1da}img{width:100%;height:auto}section{margin:36px 0;border-top:1px solid #586068;padding-top:12px}.status{border-left:5px solid #cb994e;padding:12px 20px;background:#242b2f}nav{display:flex;gap:16px;flex-wrap:wrap}</style></head><body><h1>Soul — 3.5 km composition candidate</h1><div class="status"><strong>REVIEW CANDIDATE · NOT PROMOTED</strong><p>36 canonical sites and 51 legal connections. 50/51 post-channel road/deck-plane grade checks pass; River Ford approach remains 25.24°. The retained playable campaign and donors are preserved. No runtime integration or sustained performance pass is claimed.</p></div><p><a href="HANDOFF.md">Authoritative handoff</a> · <a href="qualification-summary.json">Qualification receipt</a> · <a href="validation-r2.json">Grade and preservation validation</a></p><nav>'''+''.join(f'<a href="#{n}">{html.escape(n.replace("_"," "))}</a>' for n in captions)+'</nav>'+''.join(cards)+'''<h2>Rejected iterations retained</h2><p><a href="Local/candidate-r2-geography-captures/whole_labels_minimized.png">Oversaturated source palette</a> · <a href="Local/candidate-r2-geography-captures/bridge_bank_inspection.png">Road missing bridge ends</a> · <a href="Local/candidate-r2-geography-captures/human_capital_scale.png">Stepped lake edge</a> · <a href="Local/candidate-r4-final-captures/dark_southern_terrain.png">Repeated southern rock pattern</a></p></body></html>'''
(E/'visual-review.html').write_text(page,encoding='utf-8')
samples=[json.loads(x) for x in (L/'editor-r1/telemetry.jsonl').read_text().splitlines() if x.strip()]
native_log=(L/'editor-r1/unreal.log').read_text(encoding='utf-8-sig',errors='replace');tests=[line for line in native_log.splitlines() if 'Test Completed. Result=' in line and 'Soul.Integration.CampaignWorld' in line]
assert len(tests)==2 and all('Result={Success}' in t for t in tests)
(E/'native-tests.txt').write_text('\n'.join(tests)+'\nExisting editor binary; candidate layout has no runtime binding.\n')
for p in (R/'Tools/CampaignComposition').glob('*.py'):ast.parse(p.read_text(encoding='utf-8-sig'))
summary=dict(status='isolated composition evidence; production acceptance not met',source_tests_passed=21,native_existing_binary_tests_passed=2,editor_build='not run; no new C++ changes',game_build='not run; inherited source drafts preserved',fresh_map_reload=True,native_height_probes=json.loads((E/'native-height-probes-r2.json').read_text()),post_channel_routes_with_grade_exception=1,performance=dict(benchmark_run=False,reason='visual gates remain open',editor_fps_cap=12,screenshot_dimensions=[1920,1080],peak_gpu_c=max(g['temperature_c'] for s in samples for g in s['gpu']),peak_total_gpu_used_mib=max(g['memory_used_mib'] for s in samples for g in s['gpu']),peak_editor_private_mib=max(s['process_memory']['private_commit_mib'] for s in samples),minimum_available_physical_mib=min(s['system_memory']['available_physical_mib'] for s in samples),thermal_cutoff_c=85,note='shared desktop VRAM and capped editor telemetry; not mean/P95/P99 or FPS qualification'),images=images,all_253_preservation_hashes_match=True)
(E/'qualification-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps({k:v for k,v in summary.items() if k not in ['images','native_height_probes']}))
