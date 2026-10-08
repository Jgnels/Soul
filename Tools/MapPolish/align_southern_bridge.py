"""Keep traffic inside the native Southern Crossing parapets and through its ends."""
import json,math,argparse
import numpy as np
from route_surface import OUT,SITES,surface,assess,good,dense
import repair_ford
base=surface
def clear_surface(points):
    p=np.asarray(points);h,valid=base(p)
    for c in SITES:
        if c['type']=='ford' or c['gate_id']=='woodland_bridge':continue
        t=math.radians(c['yaw_deg']);axis=np.array([math.cos(t),math.sin(t)]);q=p-c['center_xy_m'];along=abs(q@axis);across=abs(q@np.array([-axis[1],axis[0]]))
        # Native stone parapets start inside the old analytical 5.28 m box.
        conflict=(along<c['span_m']/2+.6)&(across>1.4)&(across<3.2)
        valid &= ~conflict
    return h,valid
repair_ford.surface=clear_surface
parser=argparse.ArgumentParser();parser.add_argument('--site',default='southern_crossing');parser.add_argument('--input',default='routes-presentation-r2.json');parser.add_argument('--revision',default='r3');args=parser.parse_args()
d=json.loads((OUT/'Local'/args.input).read_text());c=next(c for c in SITES if c['gate_id']==args.site);center=np.array(c['center_xy_m']);t=math.radians(c['yaw_deg']);axis=np.array([math.cos(t),math.sin(t)]);half=c['span_m']/2
report=[]
for r in d['routes']:
    for seg in r['segments']:
        if seg['type']!='road':continue
        p=dense(seg['points_m'],.5);ids=np.flatnonzero(np.linalg.norm(p-center,axis=1)<half+26)
        if not len(ids):continue
        chunks=np.split(ids,np.flatnonzero(np.diff(ids)>1)+1)
        for run in reversed(chunks):
            lo=max(0,int(run[0])-1);hi=min(len(p)-1,int(run[-1])+1);start=p[lo];end=p[hi]
            cross0=np.dot(start-center,axis);cross1=np.dot(end-center,axis)
            sign0=1 if cross0>0 else -1;sign1=1 if cross1>0 else -1
            if abs(cross0)>half and abs(cross1)>half and sign0==sign1:continue
            left=center+axis*sign0*(half+2) if np.linalg.norm(start-center)>2 else start
            right=center+axis*sign1*(half+2) if np.linalg.norm(end-center)>2 else end
            a=repair_ford.search(start,left,spacing=.5,margin=20) if np.linalg.norm(start-left)>.01 else np.array([start])
            b=repair_ford.search(right,end,spacing=.5,margin=20) if np.linalg.norm(end-right)>.01 else np.array([end])
            candidate=np.concatenate([a,b]) if a is not None and b is not None else None
            accepted=candidate is not None and good(candidate,22.1)
            report.append(dict(a=r['a'],b=r['b'],accepted=accepted,left_found=a is not None,right_found=b is not None,assessment=assess(candidate) if candidate is not None else None))
            if accepted:p=np.concatenate([p[:lo],candidate,p[hi+1:]])
        seg['points_m']=p.tolist()
(OUT/('Local/routes-crossing-'+args.revision+'.json')).write_text(json.dumps(d,indent=2))
(OUT/(args.site+'-alignment-'+args.revision+'.json')).write_text(json.dumps({'reason':'Actual native crossing seam/parapet hits, not a terrain-grade waiver','terrain_changed':False,'routes':report},indent=2))
print(json.dumps(report,indent=2))
