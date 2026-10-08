"""Bounded opt-in candidate launch through the existing guarded qualification runner."""
from pathlib import Path
import argparse,json,subprocess,sys,shutil,hashlib
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
p=argparse.ArgumentParser();p.add_argument('mode',choices=['input','authored','load','battle','profile','traversal','recovery','development']);p.add_argument('--run',required=True);p.add_argument('--source',type=Path);p.add_argument('--stage-receipt',type=Path,help='Verified isolated loose/pak stage receipt; use its actual game executable.');a=p.parse_args()
assert all(c.isalnum() or c in '-_' for c in a.run)
exe=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe');stage_receipt=None
if a.stage_receipt:
 assert a.stage_receipt.resolve().is_relative_to((E/'Local').resolve())
 stage_receipt=json.loads(a.stage_receipt.read_text(encoding='utf-8'));assert stage_receipt['pass']
 exe=Path(stage_receipt['stage'])/'Windows/Soul/Binaries/Win64/SoulComposition.exe'
 assert exe.is_file() and hashlib.sha256(exe.read_bytes()).hexdigest()==stage_receipt['binary_sha256']
 assert a.mode!='profile',"The continuation's single corrected profile has already been attempted; no thermal rerun."
O=E/'Local'/a.run;assert not O.exists();U=O/'User';U.mkdir(parents=True)
cache=R/'Intermediate/CachedAssetRegistry';dest=U/'Intermediate/CachedAssetRegistry'
for ref in ([] if a.stage_receipt else cache.glob('*.ref')):
 name=ref.read_text(encoding='utf-8-sig').strip();assert Path(name).name==name
 if (cache/name).is_file():
  dest.mkdir(parents=True,exist_ok=True)
  for f in [ref,cache/name]:shutil.copy2(f,dest/f.name)
if a.source:
 assert a.mode=='load' and a.source.resolve().is_relative_to(E.resolve())
 for relative in ['RBSave/Domains/Soul.Composition3500.Founder.domain.rbsave','CampaignInputExpectedSnapshot.json']:
  s=a.source/'User/Saved'/relative;t=U/'Saved'/relative;t.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(s,t)
flags=['-ForceRes','-RenderOffscreen','-unattended','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-DDC=InstalledNoZenLocalFallback','-SoulComposition','-SoulCampaignCapturePrefix='+a.run,'-UserDir='+U.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
markers={'recovery':'SOUL_CAMPAIGN_RECOVERY_PASS','traversal':'SOUL_COMPOSITION_TRAVEL_PASS','input':'SOUL_WORLD_VISUAL_INPUT_PASS','load':'SOUL_CAMPAIGN_COLD_LOAD_PASS','authored':'SOUL_AUTHORED_SETTLEMENT_PASS','battle':'SOUL_CAMPAIGN_ROUNDTRIP_PASS','profile':'SOUL_TERRAIN_BENCHMARK_COMPLETE'}
markers['development']='SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_PASS'
flags+= {'development':['-SoulHumanSettlementProof','-SoulSettlementDevelopmentProof','-SoulSettlementDevelopmentQualification'],'recovery':['-SoulCampaignQualification','-SoulCampaignRetryQualification','-SoulCampaignDefeatProof','-SoulAutobattle','-SoulPlayerPool=3','-SoulEnemyPool=30','-SoulActivePerSide=15'],'traversal':['-SoulCompositionTraversal'],'input':['-SoulCampaignVisualProof'],'load':['-SoulCampaignLoadProof'],'authored':['-SoulHumanSettlementProof','-SoulAuthoredSettlementQualification','-SoulAutobattle','-SoulActivePerSide=15'],'battle':['-SoulCampaignQualification','-SoulAutobattle','-SoulActivePerSide=15'],'profile':['-SoulTerrainBenchmark','-SoulTerrainBenchmarkSeconds=60']}[a.mode]
cmd=[sys.executable,str(R/'Tools/qualify_soul_vertical.py'),'--ue-exe',str(exe),'--project',str(R/'Soul.uproject'),'--stage','G6' if a.mode in ['authored','battle','recovery'] else 'G0','--expected-active-units','18' if a.mode=='recovery' else '30' if a.mode in ['authored','battle'] else '0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1920x1080','--max-fps','0' if a.mode=='profile' else '20','--diagnostic-rhi','d3d11','--duration','60','--startup-timeout','600','--completion-timeout','2700' if a.mode=='authored' else '900' if a.mode=='traversal' else '900' if a.mode=='recovery' else '600','--completion-marker',markers[a.mode],'--output',str(O/'runtime')]+['--ue-arg='+f for f in flags]
(O/'launch-receipt.json').write_text(json.dumps({'command':cmd,'runtime_kind':'packaged' if a.stage_receipt else 'editor_game','stage_receipt':str(a.stage_receipt) if a.stage_receipt else None,'binary_sha256':hashlib.sha256((exe if a.stage_receipt else R/'Binaries/Win64/UnrealEditor-Soul.dll').read_bytes()).hexdigest(),'profile_sha256':hashlib.sha256((R/'Data/CampaignComposition/presentation.json').read_bytes()).hexdigest()},indent=2))
raise SystemExit(subprocess.run(cmd,cwd=R).returncode)
