"""One bounded correction of the four measured residual route exceptions."""
import json,math
import numpy as np
from route_surface import OUT,dense,surface,assess,good
from repair_ford import search
d=json.loads((OUT/'Local/routes-presentation-r3.json').read_text());report=[]
keys={('north_pass','orc_camp'),('orc_watch','orc_camp'),('viking_harbour','viking_fjord_ridge'),('nature_grassland_edge','nature_river_woodland')}
for r in d['routes']:
 if (r['a'],r['b']) not in keys:continue
 for seg in r['segments']:
  if seg['type']!='road':continue
  p=dense(seg['points_m'],.1);h,v=surface(p);ds=np.linalg.norm(np.diff(p,axis=0),axis=1);bad=np.flatnonzero(np.degrees(np.arctan(abs(np.diff(h))/np.maximum(ds,1e-8)))>22.05)
  if not len(bad):continue
  groups=np.split(bad,np.flatnonzero(np.diff(bad)>100)+1)
  for run in reversed(groups):
   lo=max(0,int(run[0])-100);hi=min(len(p)-1,int(run[-1])+101);old=p[lo:hi+1]
   if np.ptp(old,axis=0).max()>100:continue
   fixed=search(old[0],old[-1],spacing=.5,margin=22,grade_limit=20.5)
   ok=fixed is not None and assess(fixed,.1)['max_grade_deg']<=22.05
   report.append(dict(a=r['a'],b=r['b'],start=old[0].tolist(),end=old[-1].tolist(),accepted=ok,before=assess(old,.1),after=assess(fixed,.1) if fixed is not None else None))
   if ok:p=np.concatenate([p[:lo],fixed,p[hi+1:]])
  seg['points_m']=p.tolist()
(OUT/'Local/routes-verified-r4.json').write_text(json.dumps(d,indent=2))
(OUT/'residual-route-repairs.json').write_text(json.dumps({'terrain_changed':False,'repairs':report},indent=2))
print(json.dumps(report,indent=2))
