"""Assemble honest local before/after evidence; never generate game art."""
from pathlib import Path
import json,html,hashlib,ast,datetime
R=Path(__file__).resolve().parents[2];E=R/'Evidence/MapPolish-20261007'
notes={
'whole_labels_minimized':'Macro relief, footprint and regional masks unchanged. Roads are less dominant; this remains provisional campaign art.',
'ordinary_campaign':'Shared alignments and narrower roads reduce clutter. Some parallel stretches and angular junctions still warrant targeted review.',
'human_roads':'Arrivals reach the city and widths are reduced. Distinct branches remain; this is not a claim that every visible near-parallel path is art-approved.',
'human_capital_approach':'Main/coastal arrivals avoid housing and join the open street/forecourt. City scale is unchanged. Exact authored gate/quay and island correspondence remain unresolved.',
'river_ford':'Unbridged shallow crossing, bank curvature and subtle owned bed stones. Native maximum on the crossroads route is 21.76 degrees. Water edges remain provisional ribbons.',
'major_stone_bridge':'Existing major stone span retained with native collision and continuous route approach. Material/abutment art remains provisional.',
'minor_bridge':'The old unused woodland bridge is hidden, with no legal edge removed. A bounded attempt to use it failed the east-bank grade gate.',
'broken_bridge':'Intact stone span replaced by two cut owned stone ends and a narrower timber repair. The crossing reads as repaired ruins; clean cut faces remain provisional.',
'ferry_landing':'The central inlet keeps a ferry passage. Road approaches terminate at fitted shore landings, not inland endpoints or a giant sea bridge.',
'dwarf_approach':'KNOWN DEFECT: the footprint spans about 7.22 m of ground relief. A bounded 16 m bench search did not find a better seat. No terrain pad or hold rescale was made.',
'mountain_pass':'Authored relief retained. Grade-qualified curves still have tight, pathfinder-like turns; turning-radius/roadbed construction is not fully qualified.',
'orc_routes':'Badlands military roads narrowed and local approaches refined. Mesa identity retained. Regional donor/material transition was not changed.',
'nature_paths':'Clearing/Treehold roads share the River Woodland approach. Unused crossing removed. Forest diversity and remaining tight bends are provisional.',
'close_road':'Softer, narrower material footprint. Minor-site base/slope intersections remain, and the road is still a surface mask rather than a constructed roadbed.',
'minor_timber_bridge':'13 m minor tributary span uses owned Forest_village timber modules. Rails now run parallel to travel; native deck collision is continuous. Support/abutment detail is provisional.',
'ferry_landing_close':'Owned timber jetty and piles meet the shore approach. Ferry boat/service presentation is not implemented on this candidate.',
'southern_crossing':'Routes now enter along the bridge deck axis instead of clipping stone parapets. No terrain changes.'}
receipt=json.loads((E/'Local/captures-final/receipt.json').read_text());sections=[];images=[]
for view in receipt['views']:
    name=view['name'];parts=[]
    for stage,label in [('before','Before'),('final','After')]:
        p=E/'Local'/('captures-'+stage)/(name+'.png')
        if not p.exists():continue
        url=p.relative_to(E).as_posix();parts.append(f'<figure><figcaption>{label}</figcaption><a href="{url}"><img loading="lazy" src="{url}" alt="{html.escape(name)} {label}"></a></figure>')
        images.append(dict(path=url,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
    sections.append(f'<section id="{name}"><h2>{html.escape(name.replace("_"," "))}</h2><p>{html.escape(notes[name])}</p><div class="pair">'+''.join(parts)+'</div></section>')
nav=' '.join(f'<a href="#{v["name"]}">{v["name"].replace("_"," ")}</a>' for v in receipt['views'])
(E/'visual-review.html').write_text('''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Soul map polish — frozen geography</title><style>body{background:#172127;color:#e7e9e2;font:16px/1.5 system-ui;margin:0;padding:32px}main{max-width:1800px;margin:auto}h1{font-size:32px}h2{text-transform:capitalize}a{color:#9fcee1}nav{display:flex;gap:12px;flex-wrap:wrap}section{margin:38px 0}figure{margin:0;min-width:0}img{width:100%;display:block}figcaption{padding:8px;background:#26343c}.pair{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.notice{background:#403827;padding:18px;border-left:4px solid #d4b46c}@media(max-width:900px){.pair{grid-template-columns:1fr}}</style><main><h1>Soul map polish · frozen 3.5 km geography</h1><p class="notice"><strong>Transport milestone, not production acceptance.</strong> 51/51 sampled analytical/native route-grade checks pass. Dwarf seating, some road turns, capital waterfront correspondence and water/material presentation remain provisional. No runtime integration or uncapped performance claim.</p><p>Unreal 1920×1080 captures, fixed before/after cameras, 12 FPS guarded editor review. Click an image for full size. Three additional crossing detail views have no matching before camera. <a href="route-atlas.html">51-route analytical atlas</a> · <a href="HANDOFF.md">Handoff</a></p><nav>'''+nav+'</nav>'+''.join(sections)+'</main></html>',encoding='utf-8')
(E/'visual-review.json').write_text(json.dumps(dict(editor_only=True,final_views=len(receipt['views']),paired_before_views=14,resolution=[1920,1080],fps_cap=12,visual_acceptance=False,notes=notes,images=images),indent=2))
# Native tests use the existing retained presentation, not the candidate adapter.
log=(E/'Local/editor-r1/unreal.log').read_text(errors='replace')
lines=[l for l in log.splitlines() if 'CampaignWorld' in l or 'Queue Empty 2 tests performed' in l]
(E/'native-tests.txt').write_text('\n'.join(lines)+'\n')
passed=[l for l in lines if 'Test Completed. Result={Success}' in l]
(E/'native-tests.json').write_text(json.dumps(dict(command='Automation RunTests Soul.Integration.CampaignWorld',passed=len(passed),expected=2,tests_qualify='Existing campaign geometry/camera/selection implementation in already-built editor; not candidate runtime movement/save/battle',fresh_build=False),indent=2))
assert len(passed)==2
count=0
for p in (R/'Tools/MapPolish').glob('*.py'):ast.parse(p.read_text(encoding='utf-8-sig'),filename=str(p));count+=1
(E/'tool-syntax.json').write_text(json.dumps(dict(python_files=count,ast_parse='pass',does_not_claim_unreal_execution_of_every_script=True),indent=2))
# The final camera save changes only the owned map. Refresh its checksum after capture.
p=E/'preservation-final.json';pres=json.loads(p.read_text());mp=R/'Content/SoulCampaignComposition/L_Composition_3500_r2.umap';sha=hashlib.sha256(mp.read_bytes()).hexdigest()
for row in pres['files']+pres['changed']:
    if Path(row['path'])==mp:row['after']=sha;row['unchanged']=sha==row['before']
for row in pres['candidate_assets']:
    if row['path']=='Content/SoulCampaignComposition/L_Composition_3500_r2.umap':row['sha256']=sha;row['bytes']=mp.stat().st_size
pres['owned_map_hash_refreshed_after_final_camera_save_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();p.write_text(json.dumps(pres,indent=2))
# Capped editor observations only. Never manufacture frame-time statistics.
telemetry=[json.loads(l) for l in (E/'Local/editor-r1/telemetry.jsonl').read_text().splitlines() if l.strip()]
gpu=[g for t in telemetry for g in t.get('gpu',[])];mem=[t['process_memory'] for t in telemetry if t.get('process_memory')]
perf=dict(kind='capped editor review observations, NOT a gameplay benchmark',native_capture_resolution=[1920,1080],fps_cap=12,thermal_cutoff_c=85,peak_gpu_c=max(g['temperature_c'] for g in gpu),peak_gpu_memory_used_mib=max(g['memory_used_mib'] for g in gpu),peak_process_working_set_mib=max(m['working_set_mib'] for m in mem),peak_private_commit_mib=max(m['private_commit_mib'] for m in mem),mean_frame_ms=None,p95_ms=None,p99_ms=None,frames_below30=None,frames_below40=None,frames_below60=None,uncapped_profile_skipped_reason='Visual acceptance remains incomplete; respect the visual-first gate.')
(E/'review-resource-observations.json').write_text(json.dumps(perf,indent=2))
print(json.dumps(dict(views=len(receipt['views']),native_tests=len(passed),syntax_files=count,map_sha256=sha,review=perf),indent=2))
