"""Build a read-only final route review from recorded geometry and qualification receipts.
No terrain generation, route solving, gameplay test or player-facing content.
"""
from pathlib import Path
import hashlib,json,math
import numpy as np
from PIL import Image
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008'
source=E/'Local/routes-final.json';d=json.loads(source.read_text());before=json.loads((E/'Local/routes-baseline.json').read_text())
native=json.loads((E/'native-route-final-summary.json').read_text())
hits_path=E/'Local/native-route-hits-continuation2.json'
assert hashlib.sha256(hits_path.read_bytes()).hexdigest()==native['source_sha256']
hits=json.loads(hits_path.read_text())
# Tie recorded native stations to current route polylines, without rerunning collision/tests.
max_station_xy_error=0.;station_count=0
for r,h in zip(d['routes'],hits['routes']):
 assert (r['a'],r['b'])==(h['a'],h['b'])
 roads=[s for s in r['segments'] if s['type']=='road'];assert len(roads)==len(h['segments'])
 for seg,hit in zip(roads,h['segments']):
  p=np.asarray(seg['points_m']);p=p[np.r_[True,np.linalg.norm(np.diff(p,axis=0),axis=1)>1e-7]];arc=np.r_[0,np.cumsum(np.linalg.norm(np.diff(p,axis=0),axis=1))]
  stations=np.asarray([x['station_m'] for x in hit['hits']]);xy=np.asarray([x['xy_m'] for x in hit['hits']]);interp=np.stack([np.interp(stations,arc,p[:,0]),np.interp(stations,arc,p[:,1])],-1)
  max_station_xy_error=max(max_station_xy_error,float(np.max(np.linalg.norm(xy-interp,axis=1))));station_count+=len(stations)
assert max_station_xy_error<1e-8 and station_count==native['points']
key=lambda a,b:'|'.join(sorted([a,b]))
by_native={key(x['a'],x['b']):x for x in native['routes']}
old={key(x['a'],x['b']):x for x in before['routes']}
meta={key(x['a'],x['b']):x for x in json.loads((E/'route-inventory.json').read_text())['routes']}
journey=json.loads((E/'packaged-runtime-results.json').read_text())['runs'][-1]['journey'];legs=journey['legs']
changes=json.loads((E/'road-curves-r1.json').read_text())['changes']+json.loads((E/'crownspine-curves-r1.json').read_text())['changes']
# A derivative evidence hillshade, never a replacement heightfield or shipped texture.
height=R/'Data/CampaignCompositionLocal/Composition_3500_r2.png'
z=(np.asarray(Image.open(height),dtype=float)-32768)/128
gy,gx=np.gradient(z,3500/2040);v=np.clip((.8-gx*.45-gy*.5)/np.sqrt(1+gx*gx+gy*gy),.15,1)
rgb=np.stack([v*95+25,v*110+30,v*75+24],-1)
W=R/'Evidence/ProductionWorldComposition-20261007/Local'
water=np.maximum(0,np.maximum(np.load(W/'river-water.npy'),np.load(W/'lake-water.npy')))
rgb[z<water]=[36,71,84]
img=E/'Local/final-route-review-hillshade.png';Image.fromarray(rgb.astype(np.uint8)).resize((1400,1400),Image.Resampling.LANCZOS).save(img)
def reduced(s):
 p=s['points_m'];last=len(p)-1;idx=list(range(0,last,4))+[last]
 return {'type':s['type'],'points':[[round(p[i][0],2),round(p[i][1],2)] for i in idx]}
rows=[]
for i,r in enumerate(d['routes']):
 k=key(r['a'],r['b']);n=by_native[k];m=meta[k];o=old[k]
 assert n['passes'] and not n['misses']
 hits=[x for x in legs if key(x['from'],x['to'])==k]
 pts=np.concatenate([np.asarray(s['points_m']) for s in r['segments']]);lo=np.maximum(pts.min(0)-65,0);hi=np.minimum(pts.max(0)+65,3500)
 changed=r['segments']!=o['segments']
 rows.append({'index':i+1,'a':r['a'],'b':r['b'],'hierarchy':r['road_hierarchy'],'width_m':r['road_width_m'],'native_max_grade_deg':n['max_grade_deg'],'native_misses':n['misses'],'crossings':m['crossings'],'length_m':sum(math.dist(a,b) for s in r['segments'] for a,b in zip(s['points_m'][:-1],s['points_m'][1:])),'geometry_changed':changed,'cooked_travel_count':len(hits),'cooked_directions':[x['from']+' -> '+x['to'] for x in hits],'bounds':[float(lo[0]),float(lo[1]),float(hi[0]-lo[0]),float(hi[1]-lo[1])],'segments':[reduced(s) for s in r['segments']],'before':[reduced(s) for s in o['segments']]})
record={'status':'FINAL_REVIEW_EXISTING_RECEIPTS_NOT_NEW_QUALIFICATION','route_source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'native_hits_source_sha256':native['source_sha256'],'recorded_station_xy_match_max_m':max_station_xy_error,'height_sha256':hashlib.sha256(height.read_bytes()).hexdigest(),'native_routes':51,'native_stations':native['points'],'cooked_unique_edges':journey['unique_undirected_edges'],'cooked_moves':journey['legal_moves'],'changed_route_count':sum(x['geometry_changed'] for x in rows),'curve_locations':changes,'routes':[{k:v for k,v in x.items() if k not in ['segments','before']} for x in rows]}
(E/'final-route-review.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
data=json.dumps({'routes':rows,'anchors':d['anchors'],'changes':changes},separators=(',',':'))
page='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Soul final route review</title><style>
body{margin:0;background:#151c22;color:#e8e8df;font:16px system-ui}header{padding:22px;max-width:1300px}a{color:#9ed9ef}main{display:grid;grid-template-columns:minmax(300px,2fr) minmax(290px,1fr);gap:20px;padding:0 22px 22px}svg{width:100%;height:75vh;background:#122027;border:1px solid #607077}aside{max-height:75vh;overflow:auto}button,select,input{font:inherit}select{width:100%;padding:8px}button{margin:8px 8px 8px 0;padding:6px}label{display:block;margin:7px 0}.route{fill:none;stroke:#9a9b88;stroke-width:2;vector-effect:non-scaling-stroke}.ferry{stroke:#9bbded;stroke-dasharray:6 5}.selected{stroke:#f0bc69;stroke-width:4}.selected.ferry{stroke:#afd9ff}.before{fill:none;stroke:#e184a2;stroke-width:2;stroke-dasharray:3 3;vector-effect:non-scaling-stroke}.anchor{fill:#ddd}.label{fill:white;font-size:14px;paint-order:stroke;stroke:#152024;stroke-width:3}.curve{fill:none;stroke:#df94bd;stroke-width:2;vector-effect:non-scaling-stroke}#details{line-height:1.5}table{border-collapse:collapse;width:100%;font-size:14px}td,th{border-bottom:1px solid #39434c;padding:6px;text-align:left}tr{cursor:pointer}tr:hover{background:#39434c}.note{color:#d5ba87}@media(max-width:800px){main{display:block}svg{height:55vh}}
</style><header><h1>Final accepted routes and cooked travel coverage</h1><p>Read-only analytical evidence over the frozen 3.5 km heightfield. This is not an Unreal render, new terrain or a continuous-slope guarantee. Final native: 51/51 pass, 52,944 stations, zero misses. The cooked journey covers 31 unique edges with 40 moves; unchecked edges below are not runtime failures.</p><p><a href="visual-review.html">Actual rendered before/after gallery</a> · <a href="native-route-final-summary.json">Native receipt</a> · <a href="packaged-runtime-results.json">Cooked receipts</a> · <a href="HANDOFF.md">Handoff</a></p></header><main><div><svg id="map" viewBox="0 0 3500 3500" aria-label="Final canonical route geometry"><image href="Local/final-route-review-hillshade.png" width="3500" height="3500"/><g id="roads"></g><g id="baseline"></g><g id="selected"></g><g id="anchors"></g><g id="curves"></g><g id="labels"></g></svg><p class="note">Gold: selected land route. Blue dashes: ferry. Pink dashes: inherited route comparison. Pink rings: 14 accepted local curve edits. Multiple legal edges can share one visible trunk. Display samples are reduced; full-resolution source remains authoritative.</p></div><aside><select id="pick" aria-label="Select canonical route"></select><button id="prev">Previous</button><button id="next">Next</button><button id="whole">Whole world</button><button id="fit">Fit selected</button><label><input id="showBefore" type="checkbox" checked> Show inherited geometry for selected route</label><label><input id="showLabels" type="checkbox"> Show all anchor IDs</label><label><input id="showCurves" type="checkbox" checked> Show local correction locations</label><div id="details"></div><table><thead><tr><th>Route</th><th>Native max</th><th>Cooked moves</th></tr></thead><tbody id="table"></tbody></table></aside></main><script>
const DATA=__DATA__,ns='http://www.w3.org/2000/svg';let index=0;const $=id=>document.getElementById(id);function el(tag,attrs,parent){const e=document.createElementNS(ns,tag);for(const[k,v]of Object.entries(attrs))e.setAttribute(k,v);parent.append(e);return e}function draw(segments,parent,cls){for(const s of segments)el('polyline',{points:s.points.map(p=>p.join(',')).join(' '),class:cls+(s.type==='road'?'':' ferry')},parent)}
for(const r of DATA.routes){draw(r.segments,$('roads'),'route');const op=document.createElement('option');op.value=r.index-1;op.textContent=String(r.index).padStart(2,'0')+' '+r.a+' → '+r.b;$('pick').append(op);const tr=document.createElement('tr');for(const value of [r.index+' '+r.a+' → '+r.b,r.native_max_grade_deg.toFixed(3)+'°',r.cooked_travel_count||'Not in journey']){const td=document.createElement('td');td.textContent=value;tr.append(td)}tr.onclick=()=>select(r.index-1,true);$('table').append(tr)}
for(const a of DATA.anchors){el('circle',{cx:a.xy_m[0],cy:a.xy_m[1],r:5,class:'anchor'},$('anchors'));el('text',{x:a.xy_m[0]+8,y:a.xy_m[1]-8,class:'label'},$('labels')).textContent=a.id}for(const c of DATA.changes)el('circle',{cx:c.center[0],cy:c.center[1],r:Math.max(10,c.trim_m),class:'curve'},$('curves'));
function fit(){const b=DATA.routes[index].bounds;$('map').setAttribute('viewBox',b.join(' '))}function select(i,zoom){index=(i+51)%51;$('pick').value=index;const r=DATA.routes[index];$('selected').replaceChildren();$('baseline').replaceChildren();draw(r.segments,$('selected'),'route selected');if($('showBefore').checked)draw(r.before,$('baseline'),'before');$('details').replaceChildren();const h=document.createElement('h2');h.textContent=r.a+' → '+r.b;$('details').append(h);for(const t of [r.hierarchy+' · '+r.width_m.toFixed(1)+' m wide · '+r.length_m.toFixed(1)+' m route length','Native sampled maximum '+r.native_max_grade_deg.toFixed(5)+'°; zero misses.','Crossings: '+(r.crossings.join(', ')||'land route'),'Changed route geometry: '+(r.geometry_changed?'yes (may include shared trunk propagation)':'no'),'Cooked journey: '+(r.cooked_travel_count?r.cooked_travel_count+' traversal(s), '+r.cooked_directions.join('; '):'not exercised by this representative journey'),'Passing grade does not certify turning radius, road art or settlement completeness.']){const p=document.createElement('p');p.textContent=t;$('details').append(p)}if(zoom)fit()}
$('pick').onchange=()=>select(Number($('pick').value),true);$('prev').onclick=()=>select(index-1,true);$('next').onclick=()=>select(index+1,true);$('whole').onclick=()=>$('map').setAttribute('viewBox','0 0 3500 3500');$('fit').onclick=fit;$('showBefore').onchange=()=>select(index,false);$('showLabels').onchange=()=>$('labels').style.display=$('showLabels').checked?'':'none';$('showCurves').onchange=()=>$('curves').style.display=$('showCurves').checked?'':'none';$('labels').style.display='none';select(0,false);
</script></html>'''.replace('__DATA__',data)
(E/'final-route-review.html').write_text(page,encoding='utf-8')
print('FINAL_ROUTE_REVIEW',len(rows),'changed routes',record['changed_route_count'],'cooked unique',record['cooked_unique_edges'])
