from pathlib import Path
import json,struct,urllib.request
r=Path(__file__).resolve().parents[1];report={}
for pack in ['coastal','desert','landscapes','rocky']:
 before=json.loads((r/('Evidence/'+pack+'-before.json')).read_text())
 changed=[v['path'] for v in before if not Path(v['path']).exists() or Path(v['path']).stat().st_size!=v['bytes'] or Path(v['path']).stat().st_mtime_ns!=v['mtime_ns']]
 report[pack]={'checked':len(before),'changed':changed};assert not changed
for view in ['evil_harbor','evil_region','overhead','campaign','human_plains','dwarf_foothills','northern_lake']:
 p=r/'TerrainWork/Captures'/('L_evil_waterfront_'+view+'.png');assert struct.unpack('>II',p.read_bytes()[16:24])==(1920,1080)
data=json.loads((r/'TerrainWork/settlement-plan.json').read_text())
assert len(data['sites'])==18 and len(data['routes'])==12 and all(v['pass'] for v in data['routes'])
report['settlement_count']=18;report['terrain_routes_pass']=12
report['gallery_http']=urllib.request.urlopen('http://127.0.0.1:8766/settlement-plan.html').status
(r/'Evidence/waterfront-integrity.json').write_text(json.dumps(report,indent=2));print(report)
