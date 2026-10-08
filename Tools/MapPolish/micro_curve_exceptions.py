"""Sub-four-metre lateral curve trials around measured residual grade samples.

Both ends and their tangents stay fixed. Fine validation prevents grid-sample
aliasing from accepting an apparent improvement. No heightfield writes.
"""
import json,argparse
import numpy as np
from route_surface import OUT,dense,surface,assess
parser=argparse.ArgumentParser();parser.add_argument('--input',default='routes-verified-r4.json');parser.add_argument('--revision',default='r5');parser.add_argument('--fine-shapes',action='store_true');args=parser.parse_args()
d=json.loads((OUT/'Local'/args.input).read_text());report=[]
keys={('north_pass','orc_camp'),('orc_watch','orc_camp'),('viking_harbour','viking_fjord_ridge'),('nature_grassland_edge','nature_river_woodland')}
for r in d['routes']:
 if (r['a'],r['b']) not in keys:continue
 for s in r['segments']:
  if s['type']!='road':continue
  p=dense(s['points_m'],.1);h,_=surface(p);ds=np.linalg.norm(np.diff(p,axis=0),axis=1);bad=np.flatnonzero(np.degrees(np.arctan(abs(np.diff(h))/np.maximum(ds,1e-9)))>22.08)
  groups=np.split(bad,np.flatnonzero(np.diff(bad)>120)+1) if len(bad) else []
  for run in reversed(groups):
   lo=max(0,int(run[0])-120);hi=min(len(p)-1,int(run[-1])+121);old=p[lo:hi+1];chord=old[-1]-old[0];normal=np.array([-chord[1],chord[0]])/np.linalg.norm(chord)
   arc=np.r_[0,np.cumsum(np.linalg.norm(np.diff(old,axis=0),axis=1))];weight=np.sin(np.pi*arc/arc[-1])**2;accepted=None;best=None
   trials=[(power,sign*v) for power in ([2,3,4] if args.fine_shapes else [2]) for v in np.arange(.1,4.01,.025 if args.fine_shapes else .1) for sign in [1,-1]]
   for power,amplitude in trials:
    weight=np.sin(np.pi*arc/arc[-1])**power
    q=old+weight[:,None]*normal*amplitude;result=assess(q,.025)
    if best is None or result['max_grade_deg']<best['max_grade_deg']:best=dict(result,amplitude_m=float(amplitude))
    if result['max_grade_deg']<=22.02 and not result['invalid_samples']:
     accepted=q;report.append(dict(a=r['a'],b=r['b'],accepted=True,amplitude_m=float(amplitude),start=old[0].tolist(),end=old[-1].tolist(),after=result));break
   if accepted is not None:p=np.concatenate([p[:lo],accepted,p[hi+1:]])
   else:report.append(dict(a=r['a'],b=r['b'],accepted=False,start=old[0].tolist(),end=old[-1].tolist(),best=best))
  s['points_m']=p.tolist()
(OUT/('Local/routes-micro-'+args.revision+'.json')).write_text(json.dumps(d,indent=2))
(OUT/('micro-curve-repairs-'+args.revision+'.json')).write_text(json.dumps({'terrain_changed':False,'trials_max_lateral_m':4,'fine_validation_spacing_m':.025,'repairs':report},indent=2))
print(json.dumps(report,indent=2))
