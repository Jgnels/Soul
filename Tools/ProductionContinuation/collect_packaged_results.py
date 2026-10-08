"""Consolidate existing cooked launch receipts and reviewed images; does not rerun gameplay."""
from pathlib import Path
import json,hashlib,datetime,re
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
rows=[]
for run in ['packaged-input-r1','packaged-input-r2','packaged-load-r1','packaged-human-r1','packaged-traversal-r1']:
 P=E/'Local'/run;s=json.loads((P/'runtime/summary.json').read_text());tele=[json.loads(x) for x in (P/'runtime/telemetry.jsonl').read_text().splitlines()];log=(P/'runtime/unreal.log').read_text(encoding='utf-8',errors='replace')
 passed=s.get('completion_marker_observed',False) and s.get('exit_code')==0 and s.get('clean_shutdown',False) and not s.get('crash_signatures')
 screenshots=[]
 for p in sorted((P/'User/Saved/Screenshots').glob('*.png')):screenshots.append({'path':p.relative_to(E).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
 r={'run':run,'pass':passed,'scope':'actual new cooked-content boundary, not a gameplay redesign','summary':(P/'runtime/summary.json').relative_to(E).as_posix(),'runtime_kind':s['runtime_kind'],'binary_sha256':s['executable_sha256'],'stop_reason':s['stop_reason'],'elapsed_seconds':s['elapsed_seconds'],'map_ready_seconds':(datetime.datetime.fromisoformat(s['ready_utc'])-datetime.datetime.fromisoformat(s['started_utc'])).total_seconds() if s.get('ready_utc') else None,'peak_gpu_c':max(g['temperature_c'] for x in tele for g in x['gpu']),'peak_device_vram_mib':max(g['memory_used_mib'] for x in tele for g in x['gpu']),'peak_process_working_set_mib':max(x.get('process_memory',{}).get('working_set_mib',0) for x in tele),'max_fps_cap':20,'performance_pass_inferred':False,'save_files':[p.name for p in (P/'User/Saved/RBSave/Domains').glob('*') if p.is_file()],'screenshots':screenshots}
 if run=='packaged-human-r1':
  raw=P/'User/Saved/packaged-human-r1_authored.json';r['authored_receipt']=json.loads(raw.read_text());r['reviewed_images']=['miniature_start','miniature_completed','city_start_clean','city_completed_clean','authored_battle_warm','campaign_after_battle'];r['visual_review']='PASS for state correspondence and actual authored battle/return; not final art acceptance';r['reinforcement_join_log_count']=log.count('SOUL_REINFORCEMENT_JOIN:');r['assertion_failure_count']=len(re.findall(r'SOUL_AUTHORED_CHECK[^\n]*pass=0',log));r['battle_result_lines']=[x for x in log.splitlines() if 'SOUL_CAMPAIGN_RESULT id=' in x];assert r['authored_receipt']['battle_won'] and r['assertion_failure_count']==0
 if run=='packaged-traversal-r1':
  legs=[{'from':a,'to':b,'remaining_ap':int(ap),'ferry':bool(int(f)),'hidden_frames':int(h)} for a,b,ap,f,h in re.findall(r'SOUL_COMPOSITION_TRAVEL_LEG from=(\w+) to=(\w+) ap=(\d+) ferry=(\d+) hidden_frames=(\d+)',log)]
  final=re.search(r'SOUL_COMPOSITION_TRAVEL_PASS legal_moves=(\d+) ferry_legs=(\d+) teleports=(\d+) ownership_overrides=(\d+)',log)
  assert final and len(legs)==40 and int(final[1])==40 and int(final[2])==6 and int(final[3])==int(final[4])==0
  r['journey']={'legal_moves':40,'ferry_legs':6,'unique_undirected_edges':len({tuple(sorted([x['from'],x['to']])) for x in legs}),'visited_region_ids':sorted({v for x in legs for v in [x['from'],x['to']]}),'teleports':0,'ownership_overrides':0,'legs':legs,'scope':'Representative 40-move journey across six macro-regions and existing crossing types; not exhaustive runtime traversal of all 51 edges.'}
  r['reviewed_images']=['traversal_01_river_ford','traversal_03_orc_broken_bridge','traversal_06_dark_fortress','traversal_15_ferry','traversal_17_viking_harbour','traversal_23_dwarf_forge_approach','traversal_32_human_capital','traversal_36_nature_treehold']
  r['visual_review']='PASS for actual cooked presentation/arrival and preserved Human representation; provisional regional art, hard shore masks, terrain repetition and angular roads remain visible. Several faction seats are still labels without final settlement art.'
 if run!='packaged-input-r1':assert passed,run
 rows.append(r)
out={'qualification':'PASS at corrected cooked boundary','initial_failure_preserved':True,'default_map_promoted':False,'runs':rows,'not_repeated':'Prior defeat/retry was not rerun because those systems are unchanged. This continuation revalidated all 51 routes analytically and against native collision, then cooked input/fresh-load/authored-city behavior and a representative 40-move cooked journey. Exhaustive runtime traversal of all 51 edges is not claimed.','known_render_limits':['Three CastleTown algae material fallbacks reported during cook, not repaired.','Strategic shores, sparse faction art and Dwarf exterior remain provisional.'],'performance_result':'Separate single uncapped 60-second attempt failed at 85 C; these capped functional runs are not performance qualification.'}
(E/'packaged-runtime-results.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')
b=json.loads((E/'packaged-boundary-repair.json').read_text());b['runtime_result']='PASS: packaged-input-r2, packaged-load-r1, packaged-human-r1, packaged-traversal-r1. Actual screenshots reviewed; see packaged-runtime-results.json.';(E/'packaged-boundary-repair.json').write_text(json.dumps(b,indent=2)+'\n',encoding='utf-8')
b=json.loads((E/'build-test-results.json').read_text());known={v['receipt'].replace('\\','/') for v in b['fresh_builds']}
for p in sorted((E/'Local').glob('build-composition-r*.log.result.json')):
 relative=p.relative_to(E).as_posix()
 if relative in known:continue
 d=json.loads(p.read_text());d['receipt']=relative;d['elapsed_wall_seconds']=(datetime.datetime.fromisoformat(d['finished_utc'].replace('Z','+00:00'))-datetime.datetime.fromisoformat(d['started_utc'].replace('Z','+00:00'))).total_seconds();b['fresh_builds'].append(d)
b['composition_descriptor_fix']='Target-wide EnablePlugins attempt rejected by shared installed environment (r2); exact candidate-only NonUFS descriptor retained (r3 PASS, metadata only, binary unchanged).';b['actual_stage_runtime_receipt']='packaged-runtime-results.json';(E/'build-test-results.json').write_text(json.dumps(b,indent=2)+'\n',encoding='utf-8')
print(json.dumps([{'run':r['run'],'pass':r['pass'],'peak_gpu_c':r['peak_gpu_c'],'seconds':r['elapsed_seconds']} for r in rows],indent=2))
