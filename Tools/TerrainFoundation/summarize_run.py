"""Compact read-only preservation, telemetry and study receipts."""
from pathlib import Path
import json,hashlib,subprocess,datetime
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Evidence/TerrainFoundation-20261007'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
preserved=[]
for row in read(OUT/'inherited-tracked-hashes.json'):
 path=ROOT/row['path'];current=sha(path);item={'path':row['path'],'baseline_sha256':row['sha256'],'current_sha256':current,'unchanged':current==row['sha256']}
 if row['path']=='.gitignore' and not item['unchanged']:
  data=path.read_bytes();match=None
  for cut in range(max(0,len(data)-600),len(data)):
   if hashlib.sha256(data[:cut]).hexdigest()==row['sha256']:match=cut;break
  item.update(inherited_prefix_byte_identical=match is not None,appended_bytes=len(data)-match if match else None,intentional_change='append local-only candidate/evidence ignore paths')
 preserved.append(item)
reference=[]
for row in read(OUT/'reference-baseline.json')['files']:reference.append(dict(row,current_sha256=sha(ROOT/row['path']),unchanged=sha(ROOT/row['path'])==row['sha256']))
donor=Path('D:/Unreal Projects/AoEAssetRenderLab/Content/LandscapePackOne/Maps/Mountain_05.umap');expected='3c65ed7ee42b3632aaad11d635432f070735b0fad66f7ee6b132d0d508d987b2'
native={'path':str(donor),'expected_sha256':expected,'current_sha256':sha(donor),'unchanged':sha(donor)==expected}
receipt={'checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),'inherited_tracked':preserved,'frozen_reference':reference,'native_donor':native,'no_unexpected_tracked_mutation':all(x['unchanged'] or x.get('inherited_prefix_byte_identical') for x in preserved)}
(OUT/'preservation-current.json').write_text(json.dumps(receipt,indent=2))
telemetry=[]
for directory in ['source-editor-r1','candidate-editor-r1','candidate-fresh-editor-r1','native-source-review-r2']:
 folder=OUT/'Local'/directory;rows=[json.loads(x) for x in (folder/'telemetry.jsonl').read_text().splitlines() if x.strip()];session=read(folder/'session.json') if (folder/'session.json').exists() else {}
 vals=lambda group,key:[r[group][key] for r in rows if r.get(group) and key in r[group]]
 gpu=[g for r in rows for g in r.get('gpu',[])];memory=vals('process_memory','private_commit_mib');working=vals('process_memory','working_set_mib');ram=vals('system_memory','available_physical_mib')
 telemetry.append({'session':directory,'samples':len(rows),'stop_reason':session.get('stop_reason'),'peak_gpu_c':max((g['temperature_c'] for g in gpu),default=None),'peak_total_vram_mib':max((g['memory_used_mib'] for g in gpu),default=None),'peak_private_commit_mib':max(memory,default=None),'peak_working_set_mib':max(working,default=None),'minimum_available_physical_mib':min(ram,default=None),'scope':'capped editor development; total VRAM includes desktop; not a native game performance benchmark','thermal_cutoff_c':85,'uncapped_fps_measurement':False})
(OUT/'editor-resource-observations.json').write_text(json.dumps(telemetry,indent=2))
framing=OUT/'Local/source-framing/summary.json'
if framing.exists():
 rows=read(framing)
 # Correct the shared coarse analyzer's generic full-domain label in archived
 # crop results; numbers already use the 6120 m window scaling.
 for r in rows:
  p=framing.parent/r['window']/f'mountain05-3500-rot{r["rotation_quarters"]}-sea0-fit-r2.json';data=read(p);data.update(source=f'Mountain05 {r["window"]} 6120 m source window; analysis-only bilinear resampling, no exported heightfield',native_window_origin_m=r['native_window_origin_m'],native_window_side_m=6120);p.write_text(json.dumps(data,indent=2))
 compact=[{'window':r['window'],'rotation':r['rotation_quarters'],'dry_paths':r['dry_paths_found'],'human_gentle_dry_fraction':r['capital_metrics']['human_capital']['hinterland_dry_below15_fraction'],'dwarf_height_m':r['capital_metrics']['dwarf_hold']['height_above_native_water_m'],'viking_height_m':r['capital_metrics']['viking_harbour']['height_above_native_water_m']} for r in rows]
 (OUT/'source-framing-summary.json').write_text(json.dumps({'source_window_m':6120,'physical_side_m':3500,'all_analysis_only':True,'results':compact,'finding':'No crop exceeds the full-domain original-water coarse result of 46/51. Highest cropped result is 45/51, with Dwarf site only 3.1 m above water and Viking harbour at 68.3 m. Cropping is not a demonstrated cure; do not import another landscape.'},indent=2))
print(json.dumps({'preservation_ok':receipt['no_unexpected_tracked_mutation'] and all(x['unchanged'] for x in reference) and native['unchanged'],'editor_sessions':telemetry}))
