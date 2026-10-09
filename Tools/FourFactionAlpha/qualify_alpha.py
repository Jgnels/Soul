"""Capped, thermally guarded playable-alpha qualification through existing runtime runner."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys
R=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(R/'Tools/ProductionContinuation'))
from stage_manifest import verify_manifest_presence
p=argparse.ArgumentParser()
p.add_argument('mode',choices=['defense','defense-load','alpha','cold'])
p.add_argument('--run',required=True);p.add_argument('--evidence-root',type=Path,default=R/'Evidence/FourFactionAlpha-20261009')
p.add_argument('--source',type=Path);p.add_argument('--stage-receipt',type=Path)
p.add_argument('--seed',type=int,default=1701);p.add_argument('--target-day',type=int,default=21)
proof=p.add_mutually_exclusive_group()
proof.add_argument('--alpha-defense',action='store_true')
proof.add_argument('--alpha-attack',action='store_true')
p.add_argument('--fps',type=int,default=10)
p.add_argument('--checkpoint-day',type=int,choices=range(5,11),default=10)
a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to(R/'Evidence')
assert 10<=a.fps<=30 and 0<=a.seed<=1000000 and 2<=a.target_day<=60
assert all(c.isalnum() or c in '-_' for c in a.run)
O=E/'Local'/a.run;assert not O.exists();U=O/'User';U.mkdir(parents=True)
exe=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe')
if a.stage_receipt:
 d=json.loads(a.stage_receipt.read_text());assert d['pass'];exe=Path(d['stage'])/'Windows/Soul/Binaries/Win64/SoulComposition.exe'
 assert hashlib.sha256(exe.read_bytes()).hexdigest()==d['binary_sha256'];verify_manifest_presence(a.stage_receipt,exe.parents[3])
else:
 cache=R/'Intermediate/CachedAssetRegistry';dest=U/'Intermediate/CachedAssetRegistry'
 for ref in cache.glob('*.ref'):
  name=ref.read_text(encoding='utf-8-sig').strip();assert Path(name).name==name
  if (cache/name).is_file():
   dest.mkdir(parents=True,exist_ok=True)
   for f in [ref,cache/name]:shutil.copy2(f,dest/f.name)
if a.source:
 assert a.mode in ['defense-load','cold'] and a.source.resolve().is_relative_to(R/'Evidence')
 slot='Soul.Composition3500.Controlled.orcs' if a.mode=='defense-load' else 'Soul.Composition3500.FourFactionAlpha'+('.DefenseProof' if a.alpha_defense else '.AttackProof' if a.alpha_attack else '')
 for relative in ['RBSave/Domains/'+slot+'.domain.rbsave','CampaignInputExpectedSnapshot.json']:
  f=a.source/'User/Saved'/relative;t=U/'Saved'/relative;t.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,t)
flags=['-ForceRes','-RenderOffscreen','-unattended','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-DDC=InstalledNoZenLocalFallback','-SoulComposition','-UserDir='+U.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
if a.mode in ['defense','defense-load']:
 flags+=['-SoulSixFactionProof','-SoulControlledBattle=orcs','-SoulHumanDefenseProof','-SoulActivePerSide=15']
 if a.mode=='defense':flags+=['-SoulRosterCapture']
 else:flags+=['-SoulCampaignLoadProof']
 marker='SOUL_CONTROLLED_BATTLE_PASS' if a.mode=='defense' else 'SOUL_CAMPAIGN_COLD_LOAD_PASS'
else:
 flags+=['-SoulFourFactionAlpha','-SoulAlphaQualification','-SoulAlphaSeed='+str(a.seed),'-SoulAlphaTargetDay='+str(a.target_day),'-SoulAlphaCheckpointDay='+str(a.checkpoint_day)]
 if a.mode=='cold':flags+=['-SoulAlphaColdContinue']
 if a.alpha_defense:flags+=['-SoulAlphaDefenseProof']
 if a.alpha_attack:flags+=['-SoulAlphaAttackProof']
 marker='SOUL_ALPHA_DEFENSE_PASS' if a.alpha_defense else 'SOUL_ALPHA_ATTACK_PASS' if a.alpha_attack else 'SOUL_ALPHA_CAMPAIGN_PASS'
cmd=[sys.executable,str(R/'Tools/qualify_soul_vertical.py'),'--ue-exe',str(exe),'--project',str(R/'Soul.uproject'),'--stage','G6' if a.mode=='defense' else 'G0','--expected-active-units','30' if a.mode=='defense' else '0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1920x1080','--max-fps',str(a.fps),'--diagnostic-rhi','d3d11','--duration','60','--startup-timeout','600','--completion-timeout','5400' if a.mode in ['alpha','cold'] else '900','--completion-marker',marker,'--output',str(O/'runtime')]+['--ue-arg='+f for f in flags]
(O/'launch-receipt.json').write_text(json.dumps({'command':cmd,'runtime_kind':'packaged' if a.stage_receipt else 'editor_game','stage_receipt':str(a.stage_receipt),'binary_sha256':hashlib.sha256((exe if a.stage_receipt else R/'Binaries/Win64/UnrealEditor-Soul.dll').read_bytes()).hexdigest(),'performance_qualification':False},indent=2))
raise SystemExit(subprocess.run(cmd,cwd=R).returncode)
