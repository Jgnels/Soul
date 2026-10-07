"""Open the qualified retained population through the existing thermal guard.
Dedicated persistent player profile; qualification/evidence saves stay untouched.
This is a capped interactive review, never a performance measurement.
"""
from pathlib import Path
import argparse, datetime, hashlib, json, shutil, subprocess, sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--dry-run',action='store_true')
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
profile=root/'Saved/EvilCorridorPopulationPlayer'
run=root/'Saved/EvilCorridorPopulationReview'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S-%f')
command=[sys.executable,str(root/'Tools/qualify_soul_vertical.py'),'--ue-exe','C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe','--project',str(root/'Soul.uproject'),'--stage','G0','--expected-active-units','0','--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1920x1080','--diagnostic-rhi','d3d11','--max-fps','40','--duration','1800','--startup-timeout','600','--output',str(run/'runtime')]
flags=['-ForceRes','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-EnablePlugins=HDRIBackdrop','-DDC=InstalledNoZenLocalFallback','-SoulMesaTerrain','-SoulEvilCorridor','-SoulSettlementDevelopmentProof','-UserDir='+profile.as_posix(),'-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
command+=['--ue-arg='+flag for flag in flags]
if a.dry_run:
 print(json.dumps(dict(command=command,profile=str(profile),scope='Visible 30-minute guarded review, 40 FPS interaction cap, unchanged 85 C cutoff; no performance acceptance'),indent=2))
 raise SystemExit(0)
assert not run.exists();run.mkdir(parents=True)
cache=profile/'Intermediate/CachedAssetRegistry';seed=[]
if not cache.exists():
 # Reuse only a validated registry pair; never copy gameplay/save domains.
 sources=[root/'Evidence/SettlementEnvironmentPlan-20261005/Local/retained-population-after-r6/User/Intermediate/CachedAssetRegistry',root/'Intermediate/CachedAssetRegistry']
 for source in sources:
  refs=list(source.glob('*.ref'))
  for ref in refs:
   name=ref.read_text(encoding='utf-8-sig').strip()
   if Path(name).name!=name or not name.endswith('.bin'):continue
   blob=source/name
   if not blob.is_file():continue
   cache.mkdir(parents=True,exist_ok=True)
   for file in (blob,ref):
    target=cache/file.name;shutil.copy2(file,target)
    with target.open('rb') as stream:seed.append(dict(path=str(target),sha256=hashlib.file_digest(stream,'sha256').hexdigest()))
  if seed:break
(run/'launch-plan.json').write_text(json.dumps(dict(command=command,profile=str(profile),registry_seed=seed),indent=2)+'\n')
print('Retained Soul population: 1080p, 40 FPS interactive cap, 85 C thermal stop, 30-minute bound.',flush=True)
print('T services; U construction; Space advances a day; V full city; Esc returns; F5/F9 this separate profile.',flush=True)
print('Human city source loading may take about 14 minutes. Full-city thermal qualification remains failed.',flush=True)
raise SystemExit(subprocess.call(command,cwd=root))
