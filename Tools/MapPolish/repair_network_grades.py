"""Fine-sample all legal routes; repair only bounded failing approach sections."""
import json, math
import numpy as np
from route_surface import OUT, dense, surface, assess, good
from repair_ford import search

data=json.loads((OUT/'Local/routes-woodland-r1.json').read_text()); repairs=[]; cache={}
for route in data['routes']:
    for seg in route['segments']:
        if seg['type']!='road':continue
        p=dense(seg['points_m'],.25);h,valid=surface(p);ds=np.linalg.norm(np.diff(p,axis=0),axis=1)
        bad=(np.degrees(np.arctan(abs(np.diff(h))/np.maximum(ds,1e-9)))>22.05)|(~valid[:-1])|(~valid[1:])
        ids=np.flatnonzero(bad)
        chunks=np.split(ids,np.where(np.diff(ids)>48)[0]+1) if len(ids) else []
        for chunk in reversed(chunks):
            lo=max(0,int(chunk[0])-48);hi=min(len(p)-1,int(chunk[-1])+49)
            old=p[lo:hi+1]; key=tuple(np.round(np.concatenate([old[0],old[-1]]),4)); reverse=key[2:]+key[:2]
            repair=cache.get(key)
            if repair is None and reverse in cache:repair=cache[reverse][::-1]
            if repair is None:
                if np.ptp(old,axis=0).max()>140:
                    repairs.append(dict(a=route['a'],b=route['b'],accepted=False,reason='Exceeds local repair budget',start=old[0].tolist(),end=old[-1].tolist()));continue
                repair=search(old[0],old[-1],spacing=.5,margin=20)
            accepted=repair is not None and good(repair,22.05)
            repairs.append(dict(a=route['a'],b=route['b'],start=old[0].tolist(),end=old[-1].tolist(),before=assess(old),after=assess(repair) if accepted else None,accepted=accepted))
            if accepted:
                cache[key]=repair;p=np.concatenate([p[:lo],repair,p[hi+1:]])
        seg['points_m']=p.tolist()
    print('REPAIRED',route['a'],route['b'],max(assess(s['points_m'])['max_grade_deg'] for s in route['segments'] if s['type']=='road'),flush=True)
(OUT/'Local/routes-fine-r1.json').write_text(json.dumps(data,indent=2))
(OUT/'fine-grade-repairs.json').write_text(json.dumps({'terrain_edited':False,'repairs':repairs,'accepted':sum(r['accepted'] for r in repairs),'rejected':sum(not r['accepted'] for r in repairs)},indent=2))
