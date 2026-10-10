"""Isolated cooked gate-assault proof; uses existing capped thermal guard and real input handlers."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[2];E=R/'Evidence/HumanCapitalSiege-20261010'
sys.path.insert(0,str(R/'Tools/ProductionContinuation'));from stage_manifest import verify_manifest_presence
p=argparse.ArgumentParser();p.add_argument('--receipt',type=Path,required=True);p.add_argument('--run',required=True);p.add_argument('--cold-user',type=Path);p.add_argument('--outnumbered',action='store_true');p.add_argument('--minutes',type=int,default=15);a=p.parse_args()
assert all(c.isalnum() or c in '-_' for c in a.run) and 1<=a.minutes<=15
receipt=a.receipt.resolve();s=json.loads(receipt.read_text());assert s['pass'];root=Path(s['stage'])/'Windows';verify_manifest_presence(receipt,root)
exe=root/'Soul/Binaries/Win64/SoulComposition.exe'
with exe.open('rb') as f:assert hashlib.file_digest(f,'sha256').hexdigest()==s['binary_sha256']
out=E/'Local'/a.run;out.mkdir(exist_ok=False)
if a.cold_user:
 user=a.cold_user.resolve();assert user.is_relative_to(E/'Local') and (user/'Saved/SiegeExpectedCampaign.json').is_file()
else:user=out/'User';user.mkdir()
flags=['-ForceRes','-RenderOffscreen','-unattended','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-DDC=InstalledNoZenLocalFallback','-SoulComposition','-SoulFourFactionAlpha','-SoulHeartland','-SoulSiegeV0','-SoulSiegeQualification','-SoulAlphaSeed=1701','-UserDir='+user.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.MotionBlurQuality=0,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
if a.outnumbered:flags+=['-SoulSiegeOutnumbered']
marker='SOUL_SIEGE_QUAL_PASS'
if a.cold_user:flags+=['-SoulSiegeColdRestore'];marker='SOUL_SIEGE_COLD_PASS'
c=[sys.executable,str(R/'Tools/qualify_soul_vertical.py'),'--ue-exe',str(exe),'--project',str(R/'Soul.uproject'),'--stage','G0','--expected-active-units','0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1280x800','--max-fps','10','--diagnostic-rhi','d3d11','--duration','60','--startup-timeout','600','--completion-timeout',str(a.minutes*60),'--completion-marker',marker,'--output',str(out/'runtime')]+['--ue-arg='+f for f in flags]
(out/'launch.json').write_text(json.dumps({'command':c,'stage_receipt':str(receipt),'binary_sha256':s['binary_sha256'],'user':str(user),'performance_qualification':False},indent=2))
raise SystemExit(subprocess.run(c,cwd=R).returncode)
