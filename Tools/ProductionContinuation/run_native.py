"""Fresh-binary native campaign automation; NullRHI, bounded, one UE process."""
from pathlib import Path
import argparse,subprocess,json,hashlib,time,os,tempfile
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
p=argparse.ArgumentParser();p.add_argument('--run',required=True);p.add_argument('--composition',action='store_true');p.add_argument('--tests',default='Soul.Integration.CampaignWorld');p.add_argument('--expected-tests',type=int,default=2);p.add_argument('--evidence-root',type=Path,default=E);a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to((R/'Evidence').resolve());E.mkdir(parents=True,exist_ok=True);assert all(c.isalnum() or c in '-_' for c in a.run)
assert 1<=a.expected_tests<=200 and all(c.isalnum() or c in '._' for c in a.tests)
O=E/'Local'/a.run;assert not O.exists();O.mkdir(parents=True)
cmd=['C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',str(R/'Soul.uproject'),'-unattended','-nosound','-nullrhi','-nop4','-nosplash','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-DDC=InstalledNoZenLocalFallback','-ExecCmds=Automation RunTests '+a.tests,'-TestExit=Automation Test Queue Empty','-ReportExportPath='+str(O/'report'),'-abslog='+str(O/'unreal.log')]
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
report=O/'report/index.json'
structured=json.loads(report.read_text(encoding='utf-8-sig')) if report.is_file() else {}
completed=structured.get('tests',[])
receipt['structured_tests']=[{'name':t.get('fullTestPath'),'state':t.get('state')} for t in completed]
receipt['report_sha256']=hashlib.sha256(report.read_bytes()).hexdigest() if report.is_file() else None
receipt['pass']=receipt.get('exit_code')==0 and len(completed)==a.expected_tests and structured.get('succeeded',0)+structured.get('succeededWithWarnings',0)==a.expected_tests and structured.get('failed')==0 and structured.get('notRun')==0 and all(t.get('state')=='Success' and t.get('fullTestPath','').startswith(a.tests) for t in completed) and len(receipt['tests'])<=a.expected_tests and all('Result={Success}' in l for l in receipt['tests'])
receipt['text_completion_count']=len(receipt['tests'])
receipt['succeeded_with_warnings']=structured.get('succeededWithWarnings',0)
receipt['warnings']=[{'test':t.get('fullTestPath'),'message':e['event'].get('message')} for t in completed for e in t.get('entries',[]) if e.get('event',{}).get('type')=='Warning']
(O/'receipt.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt,indent=2));raise SystemExit(0 if receipt['pass'] else 1)
