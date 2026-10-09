"""Bounded opt-in candidate launch through the existing guarded qualification runner."""
from pathlib import Path
import argparse,json,subprocess,sys,shutil,hashlib
from stage_manifest import verify_manifest_presence
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
p=argparse.ArgumentParser();p.add_argument('mode',choices=['input','authored','load','battle','profile','traversal','recovery','development','sixstate','rosterposes']);p.add_argument('--run',required=True);p.add_argument('--battle-player-pool',type=int,help='Explicit isolated battle force fixture; never a forced result.');p.add_argument('--expect-defeat',action='store_true');p.add_argument('--battle-visual-proof',action='store_true',help='Existing vertical observer: battle screenshot and normal automatic spell inputs; a separately labeled functional run.');p.add_argument('--viking-proof',action='store_true',help='Exact isolated Viking axe infantry fixture.');p.add_argument('--six-proof',action='store_true',help='Isolated no-AI six-faction state/save profile.');p.add_argument('--orc-proof',action='store_true',help='Exact isolated Orc roster fixture, battle or fresh-load only.');p.add_argument('--source',type=Path);p.add_argument('--stage-receipt',type=Path,help='Verified isolated loose/pak stage receipt; use its actual game executable.');p.add_argument('--visual-fps',type=int,default=20,help='Functional review cap only; profiles remain uncapped.');p.add_argument('--evidence-root',type=Path,default=E);a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to((R/'Evidence').resolve());E.mkdir(parents=True,exist_ok=True)
assert 10<=a.visual_fps<=30
assert not a.orc_proof or a.mode in {'battle','load'}
assert not a.viking_proof or (a.mode in {'battle','load','rosterposes'} and not a.orc_proof and not a.six_proof)
assert not a.six_proof or (a.mode in {'sixstate','load'} and not a.orc_proof)
assert a.mode!='sixstate' or a.six_proof
assert a.mode!='rosterposes' or a.viking_proof
assert not a.battle_visual_proof or a.mode=='battle'
assert a.battle_player_pool is None or (a.mode=='battle' and a.viking_proof and 1<=a.battle_player_pool<=90)
assert not a.expect_defeat or (a.mode=='battle' and a.viking_proof)
assert all(c.isalnum() or c in '-_' for c in a.run)
exe=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe');stage_receipt=None
if a.stage_receipt:
 assert a.stage_receipt.resolve().is_relative_to((E/'Local').resolve())
 stage_receipt=json.loads(a.stage_receipt.read_text(encoding='utf-8'));assert stage_receipt['pass']
 exe=Path(stage_receipt['stage'])/'Windows/Soul/Binaries/Win64/SoulComposition.exe'
 assert exe.is_file() and hashlib.sha256(exe.read_bytes()).hexdigest()==stage_receipt['binary_sha256']
 verify_manifest_presence(a.stage_receipt,exe.parents[3])
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
 slot='Soul.Composition3500.VikingProof' if a.viking_proof else 'Soul.Composition3500.SixFactionProof' if a.six_proof else 'Soul.Composition3500.OrcProof' if a.orc_proof else 'Soul.Composition3500.Founder'
 for relative in ['RBSave/Domains/'+slot+'.domain.rbsave','CampaignInputExpectedSnapshot.json']:
  s=a.source/'User/Saved'/relative;t=U/'Saved'/relative;t.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(s,t)
flags=['-ForceRes','-RenderOffscreen','-unattended','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-DDC=InstalledNoZenLocalFallback','-SoulComposition','-SoulCampaignCapturePrefix='+a.run,'-UserDir='+U.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
if a.six_proof:flags+=['-SoulSixFactionProof']
if a.orc_proof:flags+=['-SoulOrcMatchupProof']
if a.viking_proof:flags+=['-SoulVikingMatchupProof']
if a.viking_proof and a.mode=='battle':flags+=['-SoulRosterCapture']
if a.battle_visual_proof:flags+=['-SoulVerticalQualification']
if a.battle_player_pool is not None:flags+=['-SoulPlayerPool='+str(a.battle_player_pool)]
if a.expect_defeat:flags+=['-SoulCampaignDefeatProof']
markers={'recovery':'SOUL_CAMPAIGN_RECOVERY_PASS','traversal':'SOUL_COMPOSITION_TRAVEL_PASS','input':'SOUL_WORLD_VISUAL_INPUT_PASS','load':'SOUL_CAMPAIGN_COLD_LOAD_PASS','authored':'SOUL_AUTHORED_SETTLEMENT_PASS','battle':'SOUL_CAMPAIGN_ROUNDTRIP_PASS','profile':'SOUL_TERRAIN_BENCHMARK_COMPLETE'}
markers['development']='SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_PASS'
markers['sixstate']='SOUL_SIX_FACTION_STATE_PASS'
markers['rosterposes']='SOUL_POSE_PASS'
flags+= {'rosterposes':['-SoulCampaignQualification','-SoulActivePerSide=15','-SoulBattleReadabilityProof','-SoulAnimationPoseProof'],'sixstate':['-SoulSixFactionQualification'],'development':['-SoulHumanSettlementProof','-SoulSettlementDevelopmentProof','-SoulSettlementDevelopmentQualification'],'recovery':['-SoulCampaignQualification','-SoulCampaignRetryQualification','-SoulCampaignDefeatProof','-SoulAutobattle','-SoulPlayerPool=3','-SoulEnemyPool=30','-SoulActivePerSide=15'],'traversal':['-SoulCompositionTraversal'],'input':['-SoulCampaignVisualProof'],'load':['-SoulCampaignLoadProof'],'authored':['-SoulHumanSettlementProof','-SoulAuthoredSettlementQualification','-SoulAutobattle','-SoulActivePerSide=15'],'battle':['-SoulCampaignQualification','-SoulAutobattle','-SoulActivePerSide=15'],'profile':['-SoulTerrainBenchmark','-SoulTerrainBenchmarkSeconds=60']}[a.mode]
cmd=[sys.executable,str(R/'Tools/qualify_soul_vertical.py'),'--ue-exe',str(exe),'--project',str(R/'Soul.uproject'),'--stage','G6' if a.mode in ['authored','battle','recovery','rosterposes'] else 'G0','--expected-active-units',str(min(15,a.battle_player_pool)+15) if a.battle_player_pool is not None else '18' if a.mode=='recovery' else '30' if a.mode in ['authored','battle','rosterposes'] else '0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1920x1080','--max-fps','0' if a.mode=='profile' else str(a.visual_fps),'--diagnostic-rhi','d3d11','--duration','60','--startup-timeout','600','--completion-timeout','2700' if a.mode=='authored' else '900' if a.mode=='traversal' else '900' if a.mode=='recovery' else '600','--completion-marker',markers[a.mode],'--output',str(O/'runtime')]+['--ue-arg='+f for f in flags]
(O/'launch-receipt.json').write_text(json.dumps({'command':cmd,'runtime_kind':'packaged' if a.stage_receipt else 'editor_game','stage_receipt':str(a.stage_receipt) if a.stage_receipt else None,'binary_sha256':hashlib.sha256((exe if a.stage_receipt else R/'Binaries/Win64/UnrealEditor-Soul.dll').read_bytes()).hexdigest(),'profile_sha256':hashlib.sha256((R/'Data/CampaignComposition/presentation.json').read_bytes()).hexdigest()},indent=2))
raise SystemExit(subprocess.run(cmd,cwd=R).returncode)
