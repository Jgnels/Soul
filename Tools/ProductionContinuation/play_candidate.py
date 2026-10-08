"""Manually play the isolated cooked candidate through the existing thermal guard.
No automated inputs, autobattle, terrain mutation, default-map promotion or save migration.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def prepare(stage_receipt:Path,human:bool,minutes:int):
 if not 1<=minutes<=30:raise ValueError('Review duration must be 1–30 minutes; no uncapped performance mode.')
 stage_receipt=stage_receipt.resolve()
 if not stage_receipt.is_relative_to((E/'Local').resolve()):raise ValueError('Require this milestone local stage receipt.')
 receipt=json.loads(stage_receipt.read_text(encoding='utf-8'))
 if not receipt.get('pass') or receipt.get('promotion') or receipt.get('archive_created'):raise ValueError('Require the verified local-only candidate stage.')
 stage=Path(receipt['stage'])/'Windows';exe=stage/'Soul/Binaries/Win64/SoulComposition.exe'
 if digest(exe)!=receipt['binary_sha256']:raise ValueError('The staged executable differs from its verified build.')
 isolated=json.loads((E/'staged-isolation-verification.json').read_text())
 if not isolated['pass'] or Path(isolated['stage']).resolve()!=stage.resolve():raise ValueError('Stage has no matching isolation verification.')
 for row in isolated['additional_data']:
  if digest(stage/'Soul'/row['path'])!=row['sha256']:raise ValueError('Staged presentation payload changed: '+row['path'])
 hdri=stage/'Engine/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin'
 if not hdri.is_file():raise ValueError('Required cooked content mount descriptor is missing.')
 # Verify the cooked candidate and its regional derivatives against the post-UAT receipt.
 linked=json.loads((stage_receipt.parent/'prelinked-cooked-files.json').read_text())
 checked=0
 for row in linked:
  if '/SoulCampaignComposition/' in '/'+row['relative']:
   if digest(stage/row['relative'])!=row['sha256']:raise ValueError('Candidate cooked asset changed: '+row['relative'])
   checked+=1
 if not checked:raise ValueError('Candidate map/asset hashes were not found in the stage receipt.')
 profile='HumanProof' if human else 'Founder';user=R/'Saved/CompositionPlaytest'/profile
 stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ');output=E/'Local'/('manual-play-'+profile.lower()+'-'+stamp)
 if output.exists():raise ValueError('Choose a fresh playtest timestamp.')
 flags=['-ForceRes','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-SoulComposition','-UserDir='+user.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
 if human:flags.append('-SoulHumanSettlementProof')
 cmd=[sys.executable,str(R/'Tools/qualify_soul_vertical.py'),'--ue-exe',str(exe),'--project',str(R/'Soul.uproject'),'--stage','G0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1920x1080','--max-fps','20','--diagnostic-rhi','d3d11','--duration',str(minutes*60),'--startup-timeout','600','--output',str(output)]+['--ue-arg='+f for f in flags]
 return {'command':cmd,'stage_receipt':str(stage_receipt),'candidate_cooked_files_verified':checked,'persistent_isolated_user_directory':str(user),'save_slot':'Soul.Composition3500.'+profile,'duration_minutes':minutes,'review_fps_cap':20,'thermal_cutoff_c':85,'automatic_inputs':False,'performance_measurement':False,'promotion':False,'output':str(output)}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage-receipt',type=Path,default=E/'Local/stage-loose-r2/Diagnostics/receipt.json');p.add_argument('--human-proof',action='store_true',help='Play the already-qualified Human construction/visit/battle fixture.');p.add_argument('--minutes',type=int,default=20);p.add_argument('--dry-run',action='store_true');a=p.parse_args()
 plan=prepare(a.stage_receipt,a.human_proof,a.minutes)
 if a.dry_run:print(json.dumps(plan,indent=2));return 0
 # Existing process/temperature checks remain inside the one authorized runner.
 user=Path(plan['persistent_isolated_user_directory']);user.mkdir(parents=True,exist_ok=True)
 receipt=E/'Local'/('manual-launch-'+Path(plan['output']).name+'.json');receipt.write_text(json.dumps(plan,indent=2)+'\n',encoding='utf-8')
 return subprocess.run(plan['command'],cwd=R).returncode
if __name__=='__main__':raise SystemExit(main())
