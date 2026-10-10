"""Bounded cooked Heartland checks through the existing guarded qualification runner.
No new gameplay input or state authority lives in this wrapper. Each fresh proof
gets a new UserDir; --cold-user restores only an explicitly supplied proof session.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys,shutil
R=Path(__file__).resolve().parents[2];E=R/'Evidence/HumanHeartlandDepth-20261010'
sys.path.insert(0,str(R/'Tools/ProductionContinuation'));from stage_manifest import verify_manifest_presence
p=argparse.ArgumentParser();p.add_argument('--receipt',type=Path,required=True);p.add_argument('--run',required=True);p.add_argument('--proof',choices=['depth','company','review','diplomacy','city','forest','bridge'],required=True);p.add_argument('--commander',action='store_true');p.add_argument('--mixed-defense',action='store_true');p.add_argument('--cold-user',type=Path);p.add_argument('--copy-user',type=Path);p.add_argument('--minutes',type=int,default=25);a=p.parse_args()
assert all(x.isalnum() or x in '-_' for x in a.run) and 1<=a.minutes<=50
receipt=a.receipt.resolve();s=json.loads(receipt.read_text());assert s['pass'];root=Path(s['stage'])/'Windows';verify_manifest_presence(receipt,root)
exe=root/'Soul/Binaries/Win64/SoulComposition.exe'
with exe.open('rb') as f:assert hashlib.file_digest(f,'sha256').hexdigest()==s['binary_sha256']
out=E/'Local'/a.run;out.mkdir(exist_ok=False)
if a.cold_user:
 user=a.cold_user.resolve();assert user.is_relative_to(E/'Local') and (user/'Saved/CampaignInputExpectedSnapshot.json').is_file()
else:user=out/'User';user.mkdir()
if a.copy_user:
 assert a.proof in ('company','review','diplomacy') and not a.cold_user
 source=a.copy_user.resolve();assert source.is_relative_to(E/'Local')
 (user/'Saved').mkdir();shutil.copytree(source/'Saved/RBSave',user/'Saved/RBSave')
 shutil.copy2(source/'Saved/CampaignInputExpectedSnapshot.json',user/'Saved/CampaignInputExpectedSnapshot.json')
flags=['-ForceRes','-RenderOffscreen','-unattended','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-DDC=InstalledNoZenLocalFallback','-SoulComposition','-SoulFourFactionAlpha','-SoulHeartland','-SoulAlphaSeed=1701','-UserDir='+user.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
if a.mixed_defense:
 assert a.proof in ('city','forest','bridge');flags+=['-SoulHeartlandMixedDefenseQualification']
if a.commander:
 assert a.proof=='city';flags+=['-SoulHeartlandCommanderQualification']
if a.proof not in ('depth','company','review','diplomacy'):
 flags+=['-SoulHeartlandBattleQualification']
 if a.proof!='city':flags+=['-SoulHeartlandField='+a.proof]
if a.cold_user:flags+=['-SoulCampaignLoadProof'];marker='SOUL_CAMPAIGN_COLD_LOAD_PASS'
elif a.proof=='diplomacy':
 flags+=['-SoulAlphaQualification','-SoulHeartlandDepthQualification','-SoulHeartlandDiplomacyProof'];marker='SOUL_DIPLOMACY_PROOF_PASS'
elif a.proof=='review':
 flags+=['-SoulAlphaQualification','-SoulHeartlandDepthQualification','-SoulHeartlandPresentationReview'];marker='SOUL_DEPTH_PRESENTATION_PASS'
elif a.proof=='company':
 flags+=['-SoulAlphaQualification','-SoulHeartlandDepthQualification','-SoulHeartlandCompanyBattle'];marker='SOUL_DEPTH_COMPANY_BATTLE_PASS'
else:
 flags+=['-SoulAlphaQualification','-SoulHeartlandDepthQualification' if a.proof=='depth' else '-SoulHeartlandQualification']
 marker='SOUL_DEPTH_PASS' if a.proof=='depth' else 'SOUL_HEARTLAND_CITY_BATTLE_PASS'
c=[sys.executable,str(R/'Tools/qualify_soul_vertical.py'),'--ue-exe',str(exe),'--project',str(R/'Soul.uproject'),'--stage','G0','--expected-active-units','0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1280x800','--max-fps','10','--diagnostic-rhi','d3d11','--duration','60','--startup-timeout','600','--completion-timeout',str(a.minutes*60),'--completion-marker',marker,'--output',str(out/'runtime')]+['--ue-arg='+f for f in flags]
(out/'launch.json').write_text(json.dumps({'command':c,'stage_receipt':str(receipt),'binary_sha256':s['binary_sha256'],'proof':a.proof,'user':str(user),'performance_qualification':False},indent=2))
raise SystemExit(subprocess.run(c,cwd=R).returncode)
