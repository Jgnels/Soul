"""Explicit bounded candidate/runtime-profile cook; no default-map/config mutation or promotion."""
from pathlib import Path
import argparse,datetime,hashlib,json,os,shutil,subprocess,sys,time,tempfile
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'Tools'));sys.path.insert(0,str(R/'Tools/WorldTerrain'))
from qualify_soul_vertical import conflicting_processes,gpu_sample,process_memory_sample,close_owned_process
from host_commit import system_commit_sample
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--minutes',type=int,default=75);p.add_argument('--scope',choices=['candidate','runtime'],default='candidate');p.add_argument('--validate-only',action='store_true');p.add_argument('--os-temp-cook',action='store_true',help='Use a fresh OS temporary directory for cooked payload; receipts stay in evidence.');a=p.parse_args()
assert 1<=a.minutes<=90
profile=json.loads((R/'Data/CampaignComposition/PackageProfile.json').read_text());out=a.output.resolve();assert out.is_relative_to(Path('D:/RefinedBadger').resolve()) and not out.exists();assert not out.is_relative_to(R/'Content')
assert not conflicting_processes(),'one heavy process at a time'
assert all(not x.startswith('/Game/SoulCampaignWorld/') for x in profile['cook_roots'])
for name in profile['exact_additional_runtime_files']:assert (R/name).is_file(),name
out.mkdir(parents=True);logs=out/'Diagnostics';logs.mkdir();cooked=out/'Cooked'
if a.os_temp_cook:
 temporary=Path(tempfile.mkdtemp(prefix='SoulCompositionCookPayload_')).resolve();assert temporary.is_relative_to(Path(tempfile.gettempdir()).resolve());cooked=temporary/'Cooked'
gpu=shutil.which('nvidia-smi');assert gpu
exe=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
roots=profile['cook_roots'] if a.scope=='runtime' else profile['candidate_only_cook_roots']
cmd=[str(exe),str(R/'Soul.uproject'),'-run=Cook','-TargetPlatform=Windows','-Map='+('+'.join(roots)),'-OutputDir='+str(cooked),'-unattended','-nop4','-NullRHI','-nosound','-DDC=InstalledNoZenLocalFallback','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-abslog='+str(logs/'cook.log')]
config_hashes={str(p.relative_to(R)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [R/'Config/DefaultEngine.ini',R/'Config/DefaultGame.ini',R/'Soul.uproject']}
receipt={'command':cmd,'cooked_directory':str(cooked),'scope':a.scope,'requested_roots':roots,'packaged_runtime_qualified':False,'profile_sha256':hashlib.sha256((R/'Data/CampaignComposition/PackageProfile.json').read_bytes()).hexdigest(),'editor_sha256':hashlib.sha256((R/'Binaries/Win64/UnrealEditor-Soul.dll').read_bytes()).hexdigest(),'config_hashes':config_hashes,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'cutoff_gpu_c':85,'min_free_disk_gib':8,'min_commit_headroom_mib':4096,'max_private_commit_mib':20000,'minutes':a.minutes}
(logs/'invocation.json').write_text(json.dumps(receipt,indent=2));print('COOK_PREFLIGHT',str(logs),flush=True)
if a.validate_only:raise SystemExit(0)
assert shutil.disk_usage(cooked.parent).free>10*2**30,'Need >10 GiB free to begin bounded cook'
env=os.environ.copy();env['TMP']=env['TEMP']=tempfile.mkdtemp(prefix='SoulCompositionCook_');proc=None;start=time.monotonic()
try:
 with (logs/'console.log').open('w') as console,(logs/'telemetry.jsonl').open('w') as telemetry:
  proc=subprocess.Popen(cmd,cwd=R,env=env,stdout=console,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW);receipt['pid']=proc.pid
  while proc.poll() is None:
   sample={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'gpu':gpu_sample(gpu),'memory':process_memory_sample(proc.pid),'host':system_commit_sample(),'free_disk_gib':shutil.disk_usage(cooked.parent).free/2**30};telemetry.write(json.dumps(sample)+'\n');telemetry.flush()
   reason='thermal_cutoff_85c' if max(x['temperature_c'] for x in sample['gpu'])>=85 else 'disk_headroom' if sample['free_disk_gib']<8 else 'commit_headroom' if sample['host']['available_commit_mib']<4096 else 'process_commit_limit' if sample['memory'].get('private_commit_mib',0)>20000 else 'time_budget' if time.monotonic()-start>a.minutes*60 else None
   if reason:receipt['stop_reason']=reason;break
   time.sleep(5)
finally:
 if proc and proc.poll() is None:receipt['cleanup']=close_owned_process(proc)
 receipt['exit_code']=proc.poll() if proc else None;receipt['seconds']=time.monotonic()-start;receipt['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
 receipt['configuration_unchanged']=all(hashlib.sha256((R/n).read_bytes()).hexdigest()==h for n,h in config_hashes.items())
 files=[x for x in cooked.rglob('*') if x.is_file()] if cooked.exists() else []
 receipt['cooked_files']=len(files);receipt['cooked_bytes']=sum(x.stat().st_size for x in files);receipt['experimental_files']=[str(x.relative_to(cooked)) for x in files if any(v in str(x) for v in ['SoulCampaignWorld','SoulCampaignExpansion'])]
 receipt['candidate_map_written']=any(x.name=='L_Composition_3500_r2.umap' for x in files)
 paths=[x.as_posix().lower() for x in files]
 receipt['root_outputs']={root:any(path.endswith('/Content/'+root.split('/',2)[2]+ext) for path in [x.as_posix() for x in files] for ext in ['.uasset','.umap']) for root in roots}
 receipt['missing_root_outputs']=[root for root,exists in receipt['root_outputs'].items() if not exists]
 text=(logs/'cook.log').read_text(encoding='utf-8-sig',errors='replace') if (logs/'cook.log').exists() else ''
 receipt['summary_lines']=[l for l in text.splitlines() if any(s in l for s in ['Error Summary','Success -','Failure -','Cook complete','Packages Cooked','Error:'])][-70:]
 receipt['pass']=receipt['exit_code']==0 and not receipt.get('stop_reason') and receipt['candidate_map_written'] and not receipt['experimental_files'] and receipt['configuration_unchanged'] and not receipt['missing_root_outputs']
 (logs/'receipt.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt,indent=2),flush=True)
raise SystemExit(0 if receipt['pass'] else 1)
