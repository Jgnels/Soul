"""Short owned timber jetties at the four measured ferry shores."""
import json,math
import numpy as np
from route_surface import OUT,sample,dense
d=json.loads((OUT/'Local/routes-presentation-r1.json').read_text());rows=[]
for p in d['ferry_passages']:
    for index in range(2):
        start=np.array(p['endpoints_xy_m'][index]);opposite=np.array(p['endpoints_xy_m'][1-index]);axis=(opposite-start)/np.linalg.norm(opposite-start)
        line=dense([start,start+axis*30],.25);wet=np.flatnonzero(sample(line)<-.08)
        assert len(wet),'No near-shore water'
        span=min(26,float(np.linalg.norm(line[wet[0]]-start))+3)
        rows.append(dict(id=p['id']+'_'+str(index),start_xy_m=start.tolist(),center_xy_m=(start+axis*span/2).tolist(),span_m=span,width_m=3,deck_z_m=float(sample(start))+.08,yaw_deg=math.degrees(math.atan2(axis[1],axis[0])),end_xy_m=(start+axis*span).tolist()))
(OUT/'ferry-jetty-placement.json').write_text(json.dumps({'jetties':rows,'sea_bridges':0},indent=2))
print(rows)
