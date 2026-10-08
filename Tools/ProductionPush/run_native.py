"""Fresh-binary native campaign automation; NullRHI, bounded, one UE process."""
from pathlib import Path
import argparse,subprocess,json,hashlib,time,os,tempfile
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionPush-20261008'
p=argparse.ArgumentParser();p.add_argument('--run',required=True);p.add_argument('--composition',action='store_true');a=p.parse_args();assert all(c.isalnum() or c in '-_' for c in a.run)
O=E/'Local'/a.run;assert not O.exists();O.mkdir(parents=True)
cmd=['C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',str(R/'Soul.uproject'),'-unattended','-nosound','-nullrhi','-nop4','-nosplash','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-DDC=InstalledNoZenLocalFallback','-ExecCmds=Automation RunTests Soul.Integration.CampaignWorld','-TestExit=Automation Test Queue Empty','-ReportExportPath='+str(O/'report'),'-abslog='+str(O/'unreal.log')]
if a.composition:cmd+=['-SoulComposition']
env=os.environ.copy();scratch=tempfile.mkdtemp(prefix='SoulNative_');env['TEMP']=env['TMP']=scratch
start=time.time();receipt={'command':cmd,'binary_sha256':hashlib.sha256((R/'Binaries/Win64/UnrealEditor-Soul.dll').read_bytes()).hexdigest()}
try:
 with (O/'console.log').open('w') as log:
  result=subprocess.run(cmd,cwd=R,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=600,creationflags=subprocess.CREATE_NO_WINDOW);receipt['exit_code']=result.returncode
except subprocess.TimeoutExpired:receipt['timeout']=True
receipt['seconds']=time.time()-start
text=(O/'unreal.log').read_text(encoding='utf-8-sig',errors='replace') if (O/'unreal.log').exists() else ''
receipt['tests']=[l for l in text.splitlines() if 'Test Completed. Result=' in l]
receipt['pass']=receipt.get('exit_code')==0 and len(receipt['tests'])==2 and all('Result={Success}' in l for l in receipt['tests'])
(O/'receipt.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt,indent=2));raise SystemExit(0 if receipt['pass'] else 1)
