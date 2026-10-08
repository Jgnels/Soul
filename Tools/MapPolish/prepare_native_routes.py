"""Native route probes: 1 m travel stations, 0.25 m through crossing approaches."""
import json,argparse
import numpy as np
from route_surface import OUT,SITES,sample,surface
parser=argparse.ArgumentParser();parser.add_argument('--input',default='routes-presentation-r2.json');args=parser.parse_args()
d=json.loads((OUT/'Local'/args.input).read_text());out=[]
for r in d['routes']:
    segments=[]
    for s in r['segments']:
        if s['type']!='road':continue
        p=np.array(s['points_m']);lengths=np.linalg.norm(np.diff(p,axis=0),axis=1);keep=np.r_[True,lengths>1e-7];p=p[keep];arc=np.r_[0,np.cumsum(np.linalg.norm(np.diff(p,axis=0),axis=1))]
        stations=np.r_[np.arange(0,arc[-1],1),arc[-1]]
        close=[]
        for c in SITES:
            if c['gate_id']=='woodland_bridge':continue
            indices=np.flatnonzero(np.linalg.norm(p-c['center_xy_m'],axis=1)<c['span_m']/2+14)
            if len(indices):close.extend(np.arange(max(0,arc[indices[0]]-2),min(arc[-1],arc[indices[-1]]+2),.25))
        stations=np.unique(np.r_[stations,close]);pp=np.stack([np.interp(stations,arc,p[:,0]),np.interp(stations,arc,p[:,1])],-1)
        expected=sample(pp)
        segments.append(dict(points=[[float(x),float(y),float(z),float(t)] for (x,y),z,t in zip(pp,expected,stations)]))
    out.append(dict(a=r['a'],b=r['b'],segments=segments))
(OUT/'Local/native-route-input.json').write_text(json.dumps({'routes':out}))
print('NATIVE_POINTS',sum(len(s['points']) for r in out for s in r['segments']))
