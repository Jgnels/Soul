"""Extend land approaches to actual shores; keep both existing ferry passages."""
import json
import numpy as np
from route_surface import OUT,sample,assess,good,dense
from repair_ford import search
data=json.loads((OUT/'Local/routes-curves-r1.json').read_text());ports=data['ferry_passages'];rows=[];fitted={}
for port in ports:
    p,q=np.array(port['endpoints_xy_m']);line=dense([p,q],.25);wet=np.flatnonzero(sample(line)<0)
    shorepoints=[line[wet[0]],line[wet[-1]]];ends=[];approaches=[]
    for start,shore in zip([p,q],shorepoints):
        xx,yy=np.meshgrid(np.arange(-14,15,2),np.arange(-14,15,2));candidates=np.stack([xx.ravel(),yy.ravel()],-1)+shore
        h=sample(candidates);candidates=candidates[(h>.35)&(h<1.4)]
        candidates=sorted(candidates,key=lambda c:np.linalg.norm(c-shore)+np.linalg.norm(c-start)*.05)
        found=None
        for target in candidates[:12]:
            path=search(start,target,spacing=1,margin=24)
            if path is not None and good(path,22.05):found=path;break
        approaches.append(found);ends.append(found[-1].tolist() if found is not None else start.tolist())
    ok=all(a is not None for a in approaches)
    row=dict(id=port['id'],before_endpoints=port['endpoints_xy_m'],after_endpoints=ends,accepted=ok,approaches=[assess(a) if a is not None else None for a in approaches])
    if ok:
        fitted[port['id']]=(np.array(port['endpoints_xy_m']),approaches,ends)
        port['endpoints_xy_m']=ends;port['span_m']=float(np.linalg.norm(np.diff(ends,axis=0)))
        port['water_fraction']=float((sample(dense(ends,.5))<0).mean())
        row['after_water_fraction']=port['water_fraction'];row['after_span_m']=port['span_m']
    rows.append(row);print('FERRY',row,flush=True)
for r in data['routes']:
    for i,s in enumerate(r['segments']):
        if s['type']!='ferry' or s['id'] not in fitted:continue
        old,approaches,ends=fitted[s['id']];first=0 if np.linalg.norm(np.array(s['points_m'][0])-old[0])<1 else 1;last=1-first
        assert i>0 and i+1<len(r['segments'])
        r['segments'][i-1]['points_m']+=approaches[first].tolist()[1:]
        r['segments'][i+1]['points_m']=approaches[last][::-1].tolist()[:-1]+r['segments'][i+1]['points_m']
        s['points_m']=[ends[first],ends[last]]
(OUT/'Local/routes-landings-r1.json').write_text(json.dumps(data,indent=2))
(OUT/'ferry-landing-fit.json').write_text(json.dumps({'passages':rows,'new_legal_edges':0,'terrain_changed':False},indent=2))
