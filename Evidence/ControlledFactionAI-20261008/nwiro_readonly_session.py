"""Bounded read-only Nwiro session; never opens or saves a donor map."""
from pathlib import Path
import json,sys,time,subprocess,urllib.request,socket,os,tempfile
R=Path(__file__).resolve().parents[2]
E=R/'Evidence/ControlledFactionAI-20261008'
sys.path.insert(0,str(R/'Tools'))
from qualify_soul_vertical import conflicting_processes,close_owned_process,gpu_sample
assert not conflicting_processes()
with socket.socket() as s: assert s.connect_ex(('127.0.0.1',5353))!=0,'Existing endpoint; do not launch another editor'
O=E/'Local/nwiro-readonly-r1';assert not O.exists();O.mkdir(parents=True)
cmd=['C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',str(R/'Soul.uproject'),'/Engine/Maps/Entry','-nullrhi','-nosound','-unattended','-nosplash','-nop4','-DisablePlugins=AndroidFileServer','-EnablePlugins=NwiroIntegrationKit','-DDC=InstalledNoZenLocalFallback','-UserDir='+str(O/'User'),'-abslog='+str(O/'unreal.log')]
start=time.time();proc=None;result={'command':cmd,'read_only':True,'donor_map_opened':False,'thermal_cutoff_c':85}
try:
 with (O/'console.log').open('w') as log:
  proc=subprocess.Popen(cmd,cwd=R,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
  result['pid']=proc.pid
  while time.time()-start<180 and proc.poll() is None:
   try:
    req=urllib.request.Request('http://127.0.0.1:5353/mcp',data=json.dumps({'jsonrpc':'2.0','id':1,'method':'tools/list','params':{}}).encode(),headers={'Content-Type':'application/json','Accept':'application/json, text/event-stream'})
    with urllib.request.urlopen(req,timeout=3) as response: listing=json.load(response)
    (O/'tools.json').write_text(json.dumps(listing,indent=2));result['ready']=True;print('NWIRO_READY',flush=True);break
   except Exception: time.sleep(2)
  assert result.get('ready'),'Nwiro endpoint did not become available within three minutes'
  while time.time()-start<900 and proc.poll() is None and not (O/'stop').exists():
   gpu=gpu_sample('nvidia-smi');result['peak_gpu_c']=max(result.get('peak_gpu_c',0),*(x['temperature_c'] for x in gpu))
   if result['peak_gpu_c']>=85:result['stop_reason']='thermal_cutoff';break
   time.sleep(2)
finally:
 if proc:result['cleanup']=close_owned_process(proc);result['exit_code']=proc.poll()
 result['seconds']=time.time()-start;(O/'session.json').write_text(json.dumps(result,indent=2));print(json.dumps(result),flush=True)
