import json,urllib.request,sys,time
from pathlib import Path
ROOT=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview')
def call(name,args,timeout=180):
    payload=json.dumps({'jsonrpc':'2.0','id':int(time.time()*1000),'method':'tools/call','params':{'name':name,'arguments':args}}).encode()
    req=urllib.request.Request('http://127.0.0.1:5353/mcp',data=payload,headers={'Content-Type':'application/json','Accept':'application/json, text/event-stream'})
    with urllib.request.urlopen(req,timeout=timeout) as response: result=json.load(response)
    return result
if __name__=='__main__':
    if sys.argv[1]=='python':
        source=Path(sys.argv[2]).read_text(encoding='utf-8-sig')
        result=call('execute_python',{'code':source,'execution_mode':'live'},600)
    else:
        result=call(sys.argv[1],json.loads(sys.argv[2]) if len(sys.argv)>2 else {})
    receipt=ROOT/'Evidence'/('nwiro-last-'+sys.argv[1]+'.json')
    receipt.write_text(json.dumps(result,indent=2))
    print(json.dumps(result)[:16000])
