"""Fresh local cooked stage: verified unchanged payload plus a freshly built monolithic executable.
Never overwrites the previous stage. Immutable payload uses same-volume hardlinks;
configuration, metadata and the executable are private copies. No new asset cook is claimed.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,os,shutil,subprocess,sys,tempfile
R=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'Tools/ProductionContinuation'))
from stage_manifest import verify_manifest_presence
sys.path.insert(0,str(R/'Tools'))
from qualify_soul_vertical import conflicting_processes

def digest(path):
 with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

p=argparse.ArgumentParser();p.add_argument('--prior',type=Path,required=True);p.add_argument('--evidence-root',type=Path,required=True);p.add_argument('--run',required=True)
p.add_argument('--compress-executable',action='store_true',help='Lossless NTFS compression of only the new private executable; retain the 8 GiB reserve.')
p.add_argument('--heartland-data',action='store_true',help='Explicitly admit the two reviewed Heartland JSON definitions; no cooked assets or config changes.')
a=p.parse_args()
E=a.evidence_root.resolve();prior=a.prior.resolve()
assert E.is_relative_to(R/'Evidence') and prior.is_relative_to(R/'Evidence')
assert all(c.isalnum() or c in '-_' for c in a.run) and not conflicting_processes()
old=json.loads(prior.read_text());assert old['pass'] and not old['promotion']
oldroot=Path(old['stage'])/'Windows';manifest_count=verify_manifest_presence(prior,oldroot)
exe_relative=Path('Soul/Binaries/Win64/SoulComposition.exe');fresh=R/'Binaries/Win64/SoulComposition.exe'
assert digest(oldroot/exe_relative)==old['binary_sha256']
binary=digest(fresh);assert binary!=old['binary_sha256'],'Require a genuinely fresh gameplay executable'
verification=json.loads((E/'target-receipt-verification.json').read_text());assert verification['pass']
target_path=R/verification['target_receipt'];assert digest(target_path)==verification['receipt_sha256']
target=json.loads(target_path.read_text(encoding='utf-8-sig'))
profile=R/'Data/CampaignComposition/PackageProfile.json'
assert digest(profile)==old['cook_profile_compatibility']['current_profile_sha256'],'Asset/data boundary changed; use normal staging/cook admission'
assert all(digest(R/name)==value for name,value in old['config_before'].items()),'Source configuration differs from the verified sanitized stage'
admitted_data={}
heartland_paths={'Data/SettlementEnvironments/EnvironmentRegistry.json','Data/SettlementEnvironments/HeartlandDevelopment.json'} if a.heartland_data else set()
checked=0
for row in target['BuildProducts']+target['RuntimeDependencies']:
 if row.get('Type') not in {'DynamicLibrary','NonUFS'}:continue
 path=row['Path'].replace('\\','/')
 if path.startswith('$(ProjectDir)/'):
  relative=Path('Soul')/path[len('$(ProjectDir)/'):];source=R/path[len('$(ProjectDir)/'):]
 elif path.startswith('$(EngineDir)/'):
  tail=path[len('$(EngineDir)/'):];relative=Path('Engine')/tail;source=Path('C:/Program Files/Epic Games/UE_5.8/Engine')/tail
 else:raise ValueError('Unrecognized runtime dependency root')
 if path.startswith('$(ProjectDir)/') and path[len('$(ProjectDir)/'):] in heartland_paths:
  assert source.is_file();content=json.loads(source.read_text());assert content['schema']==1
  admitted_data[relative.as_posix()]={'relative':relative.as_posix(),'source':str(source.relative_to(R)),'sha256':digest(source)}
  continue
 assert source.is_file() and (oldroot/relative).is_file(),str(relative)
 assert digest(source)==digest(oldroot/relative),'Runtime dependency changed: '+str(relative)
 checked+=1
assert len(admitted_data)==len(heartland_paths),'All requested data must be in the fresh target receipt'
base=json.loads((prior.parent/'prelinked-cooked-files.json').read_text())
for row in base:assert digest(oldroot/row['relative'])==row['sha256'],row['relative']
out=E/'Local'/a.run;assert not out.exists();out.mkdir(parents=True)
diagnostics=out/'Diagnostics';diagnostics.mkdir();(diagnostics/'UAT').mkdir()
stage_parent=Path(os.environ['LOCALAPPDATA'])/'Soul/CampaignAlphas';stage_parent.mkdir(parents=True,exist_ok=True)
stage_root=Path(tempfile.mkdtemp(prefix='FourFactionAlpha-',dir=stage_parent)).resolve();stage=stage_root/'Stage';dest=stage/'Windows'
assert oldroot.drive.lower()==dest.drive.lower()
private_suffix={'.ini','.json','.txt','.csv','.target','.modules','.log'}
files=[f for f in oldroot.rglob('*') if f.is_file()]
# Exact private byte count plus 64 MiB for this ~13k-entry NTFS tree.
# The 8 GiB free-space floor is still checked before, during and after staging.
private_bytes=sum(f.stat().st_size for f in files if f.suffix.lower() in private_suffix)
required=fresh.stat().st_size+private_bytes+64*2**20
compression=None;linked=copied=0
if a.compress_executable:
 # Allocate only one private file first. Eight MiB covers its four new directories;
 # all remaining tree allocation is separately reserved after compression.
 assert shutil.disk_usage(stage_root).free>=8*2**30+fresh.stat().st_size+8*2**20
 own_exe=dest/exe_relative;own_exe.parent.mkdir(parents=True,exist_ok=True)
 assert own_exe.resolve().is_relative_to(stage_root) and not own_exe.exists()
 shutil.copy2(fresh,own_exe);copied=1
 assert shutil.disk_usage(stage_root).free>=8*2**30 and digest(own_exe)==binary
 result=subprocess.run(['compact.exe','/C','/EXE:LZX',str(own_exe)],capture_output=True,text=True)
 assert result.returncode==0 and digest(own_exe)==binary,'New executable compression failed; partial stage preserved'
 compression={'format':'NTFS_LZX','relative':str(exe_relative),'sha256_before':binary,'sha256_after':digest(own_exe),'output':result.stdout.strip()}
 assert shutil.disk_usage(stage_root).free>=8*2**30+private_bytes+64*2**20
else:
 assert shutil.disk_usage(stage_root).free>=8*2**30+required,'Preserve the existing 8 GiB staging reserve'
for source in files:
 rel=source.relative_to(oldroot);to=dest/rel;to.parent.mkdir(parents=True,exist_ok=True)
 if rel==exe_relative:
  if not a.compress_executable:shutil.copy2(fresh,to);copied+=1
 elif source.suffix.lower() in private_suffix:shutil.copy2(source,to);copied+=1
 else:os.link(source,to);linked+=1
 if linked%1000==0:assert shutil.disk_usage(stage_root).free>=8*2**30
for kind in ['UFS','NonUFS']:
 name='Manifest_'+kind+'Files_Win64.txt';shutil.copy2(prior.parent/'UAT'/name,diagnostics/'UAT'/name)
shutil.copy2(prior.parent/'prelinked-cooked-files.json',diagnostics/'prelinked-cooked-files.json')
for row in admitted_data.values():
 to=dest/row['relative'];to.parent.mkdir(parents=True,exist_ok=True)
 assert to.resolve().is_relative_to(dest.resolve()) and (not to.exists() or to.suffix=='.json')
 shutil.copy2(R/row['source'],to);assert digest(to)==row['sha256']
manifest=diagnostics/'UAT/Manifest_NonUFSFiles_Win64.txt'
if admitted_data:
 lines=manifest.read_text(encoding='utf-8-sig').splitlines()
 names={line.split('\t')[0].replace('\\','/') for line in lines}
 for row in admitted_data.values():
  if row['relative'] not in names:lines.append(row['relative']+'\t'+datetime.datetime.now().strftime('%Y-%m-%dT%H:%M:%S'))
 manifest.write_text('\n'.join(lines)+'\n',encoding='utf-8')

receipt=dict(old)
receipt.update(stage=str(stage),stage_root=str(stage_root),stage_kind='loose_verified_payload_fresh_binary_local_only',
 prior_stage_receipt=str(prior),binary_sha256=binary,started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
 fresh_monolithic_build_receipt_sha256=digest(target_path),unchanged_runtime_dependencies_checked=checked,
 unchanged_base_cooked_hashes_checked=len(base),linked_immutable_files=linked,private_copied_files=copied,
 fresh_asset_cook=False,source_cooked_bytes_preserved=True,configuration_unchanged=True,
 promotion=False,archive_created=False,packaged_runtime_qualified=False,
 prior_executable_preserved=digest(oldroot/exe_relative)==old['binary_sha256'],
 fresh_executable_matches=digest(dest/exe_relative)==binary,external_remover_diagnosed=False)
receipt['admitted_project_data']=list(admitted_data.values())
receipt['durable_stage_outside_temp']=True
receipt['private_executable_compression']=compression
receipt['pass']=False
path=diagnostics/'receipt.json';path.write_text(json.dumps(receipt,indent=2)+'\n')
verify_manifest_presence(path,dest)
assert shutil.disk_usage(stage_root).free>=8*2**30
receipt['pass']=receipt['prior_executable_preserved'] and receipt['fresh_executable_matches']
path.write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'pass':receipt['pass'],'stage':str(stage),'receipt':str(path),'fresh_asset_cook':False,'linked_files':linked,'private_copies':copied,'base_cook_hashes':len(base),'runtime_dependencies':checked},indent=2))
