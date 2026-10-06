"""Run an explicit script through the installed loopback Unreal MCP endpoint.
Uses existing client/server permissions; never changes permission configuration,
starts an editor, or retries a timed-out mutation.
"""
import argparse, datetime, json, sys
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--script',type=Path,required=True)
p.add_argument('--receipt',type=Path,required=True)
p.add_argument('--timeout',type=int,default=120)
a=p.parse_args()
assert a.script.is_file() and not a.receipt.exists()
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'MountainTerrain'))
from nwiro_client import call
started=datetime.datetime.now(datetime.timezone.utc).isoformat()
try:
    source=a.script.read_text(encoding='utf-8-sig')
    code="exec(compile(%r, %r, 'exec'), %r)" % (source, str(a.script), {"__name__":"__main__", "__file__":str(a.script)})
    result=call('execute_python',{'code':code,'execution_mode':'live'},a.timeout)
except Exception as error:
    result={'transport_error':str(error),'execution_outcome':'unknown; inspect editor logs before retry'}
a.receipt.parent.mkdir(parents=True,exist_ok=True)
a.receipt.write_text(json.dumps(dict(started_utc=started,script=str(a.script),result=result),indent=2)+'\n',encoding='utf-8')
print(json.dumps(result)[:16000])
