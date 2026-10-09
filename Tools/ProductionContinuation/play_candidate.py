"""Manually play the isolated cooked candidate through the existing thermal guard.
No automated inputs, autobattle, terrain mutation, default-map promotion or save migration.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,subprocess,sys
from stage_manifest import verify_manifest_presence
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def prepare(stage_receipt:Path,human:bool,minutes:int,*,orc:bool=False,six:bool=False,viking:bool=False,evidence_root:Path=E):
 evidence_root=evidence_root.resolve()
 if not evidence_root.is_relative_to((R/"Evidence").resolve()):raise ValueError("Evidence must remain inside the Soul workspace.")
 if sum([human,orc,six,viking])>1:raise ValueError("Choose only one isolated proof fixture.")
 if not 1<=minutes<=30:raise ValueError('Review duration must be 1â€“30 minutes; no uncapped performance mode.')
 stage_receipt=stage_receipt.resolve()
 if not stage_receipt.is_relative_to((evidence_root/'Local').resolve()):raise ValueError('Require this milestone local stage receipt.')
 receipt=json.loads(stage_receipt.read_text(encoding='utf-8'))
 if not receipt.get('pass') or receipt.get('promotion') or receipt.get('archive_created'):raise ValueError('Require the verified local-only candidate stage.')
 stage=Path(receipt['stage'])/'Windows'
 manifest_files=verify_manifest_presence(stage_receipt,stage)
 exe=stage/'Soul/Binaries/Win64/SoulComposition.exe'
 if digest(exe)!=receipt['binary_sha256']:raise ValueError('The staged executable differs from its verified build.')
 isolated=json.loads((evidence_root/'staged-isolation-verification.json').read_text())
 if not isolated['pass'] or Path(isolated['stage']).resolve()!=stage.resolve():raise ValueError('Stage has no matching isolation verification.')
 fixture_path='Data/CampaignComposition/'+('SixFactionRuntimeProof.json' if six else 'VikingRuntimeProof.json' if viking else 'OrcRuntimeProof.json' if orc else 'HumanRuntimeProof.json' if human else 'RuntimeProof.json')
 if fixture_path not in {row['path'] for row in isolated['additional_data']}:raise ValueError('Selected fixture is not admitted by this verified stage.')
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
 profile='SixFactionProof' if six else 'VikingProof' if viking else 'OrcProof' if orc else 'HumanProof' if human else 'Founder';user=R/'Saved/CompositionPlaytest'/profile
 stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ');output=evidence_root/'Local'/('manual-play-'+profile.lower()+'-'+stamp)
 if output.exists():raise ValueError('Choose a fresh playtest timestamp.')
 flags=['-ForceRes','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-SoulComposition','-UserDir='+user.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
 if human:flags.append('-SoulHumanSettlementProof')
 if orc:flags.append('-SoulOrcMatchupProof')
 if six:flags.append('-SoulSixFactionProof')
 if viking:flags.append('-SoulVikingMatchupProof')
 cmd=[sys.executable,str(R/'Tools/qualify_soul_vertical.py'),'--ue-exe',str(exe),'--project',str(R/'Soul.uproject'),'--stage','G0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1920x1080','--max-fps','20','--diagnostic-rhi','d3d11','--duration',str(minutes*60),'--startup-timeout','600','--output',str(output)]+['--ue-arg='+f for f in flags]
 return {'command':cmd,'manifest_files_present':manifest_files,'stage_receipt':str(stage_receipt),'candidate_cooked_files_verified':checked,'persistent_isolated_user_directory':str(user),'save_slot':'Soul.Composition3500.'+profile,'duration_minutes':minutes,'review_fps_cap':20,'thermal_cutoff_c':85,'automatic_inputs':False,'performance_measurement':False,'promotion':False,'output':str(output)}
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--stage-receipt',type=Path,required=True,help='Explicit verified stage; never silently launch an older binary.')
 p.add_argument('--evidence-root',type=Path,default=E)
 fixture=p.add_mutually_exclusive_group()
 fixture.add_argument('--human-proof',action='store_true',help='Play the Human construction/visit/battle fixture.')
 fixture.add_argument('--orc-proof',action='store_true',help='Play the exact Orc hammer-infantry qualification fixture.')
 fixture.add_argument('--six-proof',action='store_true',help='Play the canonical no-AI six-faction state sandbox.')
 fixture.add_argument('--viking-proof',action='store_true',help='Play the exact Viking Snow Pass encounter.')
 p.add_argument('--minutes',type=int,default=20);p.add_argument('--dry-run',action='store_true');a=p.parse_args()
 plan=prepare(a.stage_receipt,a.human_proof,a.minutes,orc=a.orc_proof,six=a.six_proof,viking=a.viking_proof,evidence_root=a.evidence_root)
 if a.dry_run:print(json.dumps(plan,indent=2));return 0
 # Existing process/temperature checks remain inside the one authorized runner.
 user=Path(plan['persistent_isolated_user_directory']);user.mkdir(parents=True,exist_ok=True)
 receipt=a.evidence_root/'Local'/('manual-launch-'+Path(plan['output']).name+'.json');receipt.write_text(json.dumps(plan,indent=2)+'\n',encoding='utf-8')
 return subprocess.run(plan['command'],cwd=R).returncode
if __name__=='__main__':raise SystemExit(main())
