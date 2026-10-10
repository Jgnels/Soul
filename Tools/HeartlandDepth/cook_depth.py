"""Dependency-closed additive local cook. Requires the verified existing base stage.
This is intentionally NOT a standalone or distribution cook. Base shader archives
and registry remain immutable; new materials carry inline shader code.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,os,shutil,subprocess,sys,time
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'Tools'));sys.path.insert(0,str(R/'Tools/WorldTerrain'));sys.path.insert(0,str(R/'Tools/ProductionContinuation'))
from qualify_soul_vertical import conflicting_processes,gpu_sample,process_memory_sample,close_owned_process
from host_commit import system_commit_sample
from stage_manifest import verify_manifest_presence
p=argparse.ArgumentParser();p.add_argument('--run',required=True);p.add_argument('--minutes',type=int,default=45);a=p.parse_args()
assert 1<=a.minutes<=60 and all(c.isalnum() or c in '-_' for c in a.run)
E=R/'Evidence/HumanHeartlandDepth-20261010';boundary=E/'Local/depth-cook-boundary.json';b=json.loads(boundary.read_text());assert b['pass'] and not b['standalone_cook']
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
prior=Path(b['base_receipt']);assert digest(prior)==b['base_receipt_sha256'];base=json.loads(prior.read_text());stage=Path(base['stage'])/'Windows';verify_manifest_presence(prior,stage)
for row in b['base_dependencies']:assert digest(stage/row['relative'])==row['sha256']
for row in b['cook_packages']:assert digest(R/row['source'])==row['source_sha256']
assert not conflicting_processes()
out=E/'Local'/a.run;assert not out.exists();out.mkdir();cooked=out/'Cooked';logs=out/'Diagnostics';logs.mkdir()
assert min(shutil.disk_usage(out).free,shutil.disk_usage('C:/').free)>10*2**30,'Retain the existing bounded cook reserve'
roots=[x['package'] for x in b['cook_packages']]
cmd=['C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',str(R/'Soul.uproject'),'-run=Cook','-TargetPlatform=Windows','-Map='+('+'.join(roots)),'-SkipHardReferences','-SkipSoftReferences','-OutputDir='+str(cooked),'-unattended','-nop4','-NullRHI','-nosound','-DDC=InstalledNoZenLocalFallback','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-ini:Game:[/Script/UnrealEd.ProjectPackagingSettings]:bShareMaterialShaderCode=False','-abslog='+str(logs/'cook.log')]
assert len(subprocess.list2cmdline(cmd))<30000,'Use bounded package batches; never silently truncate roots'
config={str(p.relative_to(R)):digest(p) for p in [R/'Config/DefaultEngine.ini',R/'Config/DefaultGame.ini',R/'Soul.uproject']}
receipt={'command':cmd,'boundary_sha256':digest(boundary),'cooked_directory':str(cooked),'requested_roots':roots,'standalone_cook':False,'requires_base_receipt':str(prior),'inline_material_shaders':True,'cutoff_c':85,'min_free_gib':8,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'config_hashes':config,'pass':False}
(logs/'invocation.json').write_text(json.dumps(receipt,indent=2));gpu=shutil.which('nvidia-smi');assert gpu
start=time.monotonic();proc=None
try:
 with (logs/'console.log').open('w') as console,(logs/'telemetry.jsonl').open('w') as telemetry:
  proc=subprocess.Popen(cmd,cwd=R,stdout=console,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
  while proc.poll() is None:
   sample={'elapsed':time.monotonic()-start,'gpu':gpu_sample(gpu),'memory':process_memory_sample(proc.pid),'commit':system_commit_sample(),'free_gib':min(shutil.disk_usage(out).free,shutil.disk_usage('C:/').free)/2**30}
   telemetry.write(json.dumps(sample)+'\n');telemetry.flush()
   reason='thermal_cutoff' if max(x['temperature_c'] for x in sample['gpu'])>=85 else 'disk_reserve' if sample['free_gib']<8 else 'commit_headroom' if sample['commit']['available_commit_mib']<4096 else 'time_limit' if sample['elapsed']>a.minutes*60 else None
   if reason:receipt['stop_reason']=reason;break
   time.sleep(4)
finally:
 if proc and proc.poll() is None:receipt['cleanup']=close_owned_process(proc)
 receipt['exit_code']=proc.poll() if proc else None;receipt['seconds']=time.monotonic()-start
 files=[];missing=[]
 for row in b['cook_packages']:
  prefix=Path('Soul/Content')/row['package'][6:];found=False
  for ext in ('.uasset','.umap','.uexp','.ubulk','.uptnl'):
   rel=prefix.with_suffix(ext);source=cooked/'Windows'/rel
   if not source.is_file():source=cooked/rel
   if source.is_file():files.append({'relative':rel.as_posix(),'source':str(source),'sha256':digest(source),'bytes':source.stat().st_size,'package':row['package']});found|=ext in ('.uasset','.umap')
  if not found:missing.append(row['package'])
 receipt['files']=files;receipt['missing_packages']=missing;receipt['config_unchanged']=all(digest(R/n)==h for n,h in config.items())
 receipt['source_unchanged']=all(digest(R/x['source'])==x['source_sha256'] for x in b['cook_packages'])
 receipt['pass']=receipt['exit_code']==0 and not receipt.get('stop_reason') and not missing and receipt['config_unchanged'] and receipt['source_unchanged']
 (logs/'receipt.json').write_text(json.dumps(receipt,indent=2));print('DEPTH_COOK',receipt['pass'],'files',len(files),'missing',missing,'stop',receipt.get('stop_reason'))
raise SystemExit(0 if receipt['pass'] else 1)
