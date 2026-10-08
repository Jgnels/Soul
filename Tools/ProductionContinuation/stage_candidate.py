"""Stage only a verified fresh cook and binary; no build, recook, archive or promotion."""
from pathlib import Path
import argparse,datetime,hashlib,json,os,re,shutil,subprocess,sys,tempfile,time
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
sys.path[:0]=[str(R/'Tools'),str(R/'Tools/WorldTerrain')]
from qualify_soul_vertical import conflicting_processes,gpu_sample,process_memory_sample
from host_commit import system_commit_sample
p=argparse.ArgumentParser();p.add_argument('--cook',type=Path,required=True);p.add_argument('--prior-profile',type=Path,help='Exact profile used for the verified cook, only for added loose fixture/save-slot compatibility.');p.add_argument('--run',required=True);p.add_argument('--minutes',type=int,default=45);p.add_argument('--loose-hardlink',action='store_true',help='Stage loose cooked content with same-volume hardlinks in a fresh temporary stage; not a distributable archive.');p.add_argument('--evidence-root',type=Path,default=E);a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to((R/'Evidence').resolve());E.mkdir(parents=True,exist_ok=True)
assert 1<=a.minutes<=60 and all(c.isalnum() or c in '-_' for c in a.run)
cook=a.cook.resolve();assert cook.is_relative_to((R/'Evidence').resolve())
cr=json.loads((cook/'Diagnostics/receipt.json').read_text());assert cr['pass'] and cr['scope']=='runtime'
profile=R/'Data/CampaignComposition/PackageProfile.json'
cook_compatibility={'asset_cook_inputs_unchanged':True,'added_loose_fixtures':[],'added_isolated_save_slots':{},'fresh_asset_cook':False}
if hashlib.sha256(profile.read_bytes()).hexdigest()!=cr['profile_sha256']:
 assert a.prior_profile and a.prior_profile.resolve().is_relative_to((R/'Evidence').resolve()),'Changed profile needs its preserved exact cook profile'
 assert hashlib.sha256(a.prior_profile.read_bytes()).hexdigest()==cr['profile_sha256'],'Prior profile does not belong to this cook'
 from cook_profile import verify_compatible_cook_profile
 cook_compatibility=verify_compatible_cook_profile(json.loads(a.prior_profile.read_text()),json.loads(profile.read_text()))
 cook_compatibility['prior_profile']=str(a.prior_profile.resolve())
 cook_compatibility['prior_profile_sha256']=cr['profile_sha256']
cook_compatibility['current_profile_sha256']=hashlib.sha256(profile.read_bytes()).hexdigest()
tr=json.loads((E/'target-receipt-verification.json').read_text());assert tr['pass']
assert hashlib.sha256((R/tr['target_receipt']).read_bytes()).hexdigest()==tr['receipt_sha256']
assert not conflicting_processes();out=E/'Local'/a.run;assert not out.exists();out.mkdir()
# A fresh OS-temp stage preserves prior cooks and builds. Loose hardlinks share only this run's owned cooked payload; donor packages are never linked or modified.
stage_root=Path(tempfile.mkdtemp(prefix='SoulCompositionStage_')).resolve();assert stage_root.is_relative_to(Path(tempfile.gettempdir()).resolve())
receipt_files={}
for item in json.loads((R/tr['target_receipt']).read_text())['BuildProducts']+json.loads((R/tr['target_receipt']).read_text())['RuntimeDependencies']:
 if item.get('Type') in {'SymbolFile','DebugNonUFS','BuildResource'}:continue
 source=Path(item['Path'].replace('$(ProjectDir)',str(R)).replace('$(EngineDir)','C:/Program Files/Epic Games/UE_5.8/Engine'))
 assert source.is_file(),source
 receipt_files[str(source.resolve())]=source.stat().st_size
noncook_bytes=sum(receipt_files.values())
required_free=(8*2**30+noncook_bytes+512*2**20) if a.loose_hardlink else cr['cooked_bytes']+10*2**30
assert shutil.disk_usage(stage_root).free>required_free,'Insufficient stage headroom'
view=out/'CookView';view.mkdir();link=view/'Windows';target=Path(cr.get('cooked_directory',str(cook/'Cooked'))).resolve();assert target.is_dir()
assert target.is_relative_to(cook) or (target.is_relative_to(Path(tempfile.gettempdir()).resolve()) and target.parent.name.startswith('SoulCompositionCookPayload_'))
quote=lambda x:"'"+str(x).replace("'","''")+"'"
subprocess.run(['powershell','-NoProfile','-NonInteractive','-Command','New-Item -ItemType Junction -Path '+quote(link)+' -Target '+quote(target)+' | Out-Null'],check=True,creationflags=subprocess.CREATE_NO_WINDOW)
assert link.resolve()==target.resolve()
logs=out/'Diagnostics';logs.mkdir();stage=stage_root/'Stage';linked=[]
if a.loose_hardlink:
 assert target.drive.lower()==stage.drive.lower(),'Hardlinks require same volume'
 for source in target.rglob('*'):
  if not source.is_file():continue
  relative=source.relative_to(target)
  if 'Metadata' in relative.parts or source.suffix.lower() in {'.ini','.json','.txt','.csv'}:continue
  dest=stage/'Windows'/relative;dest.parent.mkdir(parents=True,exist_ok=True);assert not dest.exists()
  with source.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
  os.link(source,dest);assert os.path.samefile(source,dest)
  linked.append({'relative':relative.as_posix(),'bytes':source.stat().st_size,'sha256':digest})
 (logs/'prelinked-cooked-files.json').write_text(json.dumps(linked,indent=2),encoding='utf-8')
 print('PRELINKED_COOKED_FILES',len(linked),flush=True)
uat=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat')
args=['BuildCookRun','-nocompileuat','-noturnkeyvariables','-nop4','-unattended','-utf8output','-project='+str(R/'Soul.uproject'),'-target=SoulComposition','-platform=Win64','-clientconfig=Development','-skipbuild','-skipbuildeditor','-skipcook','-stage','-package','-nodebuginfo','-nocleanstage','-CookOutputDir='+str(link),'-stagingdirectory='+str(stage)]
if not a.loose_hardlink:args+=['-pak','-iostore']
ps='& '+quote(uat)+' '+ ' '.join(quote(x) for x in args)+'; exit $LASTEXITCODE'
cmd=['powershell','-NoProfile','-NonInteractive','-Command',ps]
env=os.environ.copy();scratch=stage_root/'Temp';scratch.mkdir();env['TMP']=env['TEMP']=str(scratch);env['uebp_LogFolder']=env['uebp_FinalLogFolder']=str(logs/'UAT')
config={n:hashlib.sha256((R/n).read_bytes()).hexdigest() for n in ['Config/DefaultEngine.ini','Config/DefaultGame.ini','Soul.uproject']}
receipt={'cook_profile_compatibility':cook_compatibility,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'command':cmd,'cook_receipt':str(cook/'Diagnostics/receipt.json'),'stage_root':str(stage_root),'stage':str(stage),'cook_view':str(link),'cook_view_target':str(target),'binary_sha256':hashlib.sha256((R/'Binaries/Win64/SoulComposition.exe').read_bytes()).hexdigest(),'config_before':config,'cutoff_gpu_c':85,'minimum_stage_free_gib':8,'minimum_source_free_gib':2 if a.loose_hardlink else 8,'stage_kind':'loose_cooked_hardlink_local_only' if a.loose_hardlink else 'pak_iostore','linked_cooked_files':len(linked),'noncook_receipt_bytes':noncook_bytes,'preflight_required_free_bytes':required_free,'promotion':False,'archive_created':False,'packaged_runtime_qualified':False}
(logs/'invocation.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8');print('STAGE_START',stage,flush=True)
start=time.monotonic();gpu=shutil.which('nvidia-smi');proc=None
try:
 with (logs/'console.log').open('w') as console,(logs/'telemetry.jsonl').open('w') as telemetry:
  proc=subprocess.Popen(cmd,cwd=R,env=env,stdout=console,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW);receipt['pid']=proc.pid
  while proc.poll() is None:
   sample={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'gpu':gpu_sample(gpu),'host':system_commit_sample(),'stage_volume_free_gib':shutil.disk_usage(stage_root).free/2**30,'source_volume_free_gib':shutil.disk_usage(R).free/2**30}
   telemetry.write(json.dumps(sample)+'\n');telemetry.flush()
   reason='thermal_cutoff_85c' if max(x['temperature_c'] for x in sample['gpu'])>=85 else 'disk_headroom' if sample['stage_volume_free_gib']<8 or sample['source_volume_free_gib']<(2 if a.loose_hardlink else 8) else 'commit_headroom' if sample['host']['available_commit_mib']<4096 else 'time_budget' if time.monotonic()-start>a.minutes*60 else None
   if reason:receipt['stop_reason']=reason;break
   time.sleep(5)
finally:
 if proc and proc.poll() is None:
  # Stop only this newly launched UAT process tree; never global process-name kills.
  receipt['cleanup']=subprocess.run(['taskkill','/PID',str(proc.pid),'/T','/F'],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout;proc.wait(timeout=20)
 receipt['exit_code']=proc.poll() if proc else None;receipt['seconds']=time.monotonic()-start
 receipt['configuration_unchanged']=all(hashlib.sha256((R/n).read_bytes()).hexdigest()==h for n,h in config.items())
 receipt['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
 (logs/'receipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
exe=stage/'Windows/Soul/Binaries/Win64/SoulComposition.exe'
receipt['executable_present']=exe.is_file()
# UAT normally filters sensitive ini keys. Verify the known local editor token
# is absent/empty; sanitize only a newly staged copy if it survived filtering.
receipt['staged_config_sanitization']=[]
for ini in stage.rglob('*.ini') if stage.exists() else []:
 text=ini.read_text(encoding='utf-8-sig',errors='strict');section='';lines=[];changed=False
 for line in text.splitlines(keepends=True):
  if line.strip().startswith('['):section=line.strip()
  if section=='[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]' and re.match(r'^\s*SecurityToken\s*=',line) and line.split('=',1)[1].strip().strip(chr(34)):
   line='SecurityToken=\n';changed=True
  lines.append(line)
 if changed:
  assert ini.resolve().is_relative_to(stage.resolve()) and not ini.is_symlink()
  before=hashlib.sha256(ini.read_bytes()).hexdigest();ini.write_text(''.join(lines),encoding='utf-8')
  receipt['staged_config_sanitization'].append({'file':ini.relative_to(stage).as_posix(),'field':'AndroidFileServer.SecurityToken','before_sha256':before,'after_sha256':hashlib.sha256(ini.read_bytes()).hexdigest(),'action':'blanked in staged copy only'})
if receipt['exit_code']==0 and exe.exists() and not receipt.get('stop_reason'):
 resource_cmd=['powershell','-NoProfile','-NonInteractive','-File',str(R/'Tools/Stage-SoulTcatResources.ps1'),'-ProjectRoot',str(R),'-WindowsPackageRoots',str(stage/'Windows'),'-ExecutableName','SoulComposition.exe']
 resources=subprocess.run(resource_cmd,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW);receipt['resources_exit']=resources.returncode;(logs/'supplemental-resources.txt').write_text(resources.stdout+resources.stderr,encoding='utf-8')
if receipt['exit_code']==0 and receipt.get('resources_exit')==0:
 from staged_copy_timestamps import refresh_stage_copies
 receipt['staged_copy_freshness']=refresh_stage_copies(stage/'Windows')
if a.loose_hardlink:
 changed=[]
 for row in linked:
  source=target/row['relative'];dest=stage/'Windows'/row['relative']
  with source.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
  if digest!=row['sha256'] or not dest.is_file() or not os.path.samefile(source,dest):changed.append(row['relative'])
 receipt['changed_or_unlinked_cooked_files']=changed
 receipt['source_cooked_bytes_preserved']=not changed
receipt['pass']=not receipt.get('changed_or_unlinked_cooked_files') and receipt['exit_code']==0 and receipt['executable_present'] and receipt.get('resources_exit')==0 and receipt['configuration_unchanged'] and not receipt.get('stop_reason')
(logs/'receipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8');print(json.dumps(receipt,indent=2),flush=True);raise SystemExit(0 if receipt['pass'] else 1)
