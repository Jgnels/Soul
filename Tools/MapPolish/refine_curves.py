"""Round bounded corners only when the unchanged native heightfield accepts them."""
import sys,json,math
import numpy as np
from route_surface import OUT,SITES,good,assess
sys.path.insert(0,str(OUT/'Local/python-libs'))
from shapely.geometry import LineString
data=json.loads((OUT/'Local/routes-consolidated-r1.json').read_text());cache={};report=[]
for r in data['routes']:
    count=0
    for s in r['segments']:
        if s['type']!='road':continue
        p=np.asarray(LineString(s['points_m']).simplify(.002).coords);out=[p[0]]
        for a,b,c in zip(p[:-2],p[1:-1],p[2:]):
            ab=b-a;bc=c-b;la=np.linalg.norm(ab);lb=np.linalg.norm(bc)
            if min(la,lb)<.1:out.append(b);continue
            angle=np.degrees(np.arccos(np.clip(ab@bc/la/lb,-1,1)))
            protected=any(np.linalg.norm(b-site['center_xy_m'])<site['span_m']/2+10 for site in SITES)
            if angle<7 or angle>115 or protected:out.append(b);continue
            key=tuple(np.round(np.concatenate([a,b,c]),4));curve=cache.get(key)
            if curve is None:
                for factor in [1,.5,.25]:
                    trim=min(8.,la*.3,lb*.3)*factor
                    if trim<.3:continue
                    left=b-ab/la*trim;right=b+bc/lb*trim
                    t=np.linspace(0,1,max(8,math.ceil(trim*12)))[:,None]
                    candidate=(1-t)**2*left+2*(1-t)*t*b+t*t*right
                    if good(candidate,22.0):curve=candidate;cache[key]=curve;break
            if curve is None:out.append(b)
            else:out.extend(curve);count+=1
        out.append(p[-1]);result=assess(out)
        original=assess(s['points_m'])
        if result['max_grade_deg']<=max(22.1,original['max_grade_deg']) and result['invalid_samples']<=original['invalid_samples']:
            s['points_m']=np.asarray(out).tolist()
        else:count=0
    report.append(dict(a=r['a'],b=r['b'],rounded_corners=count))
(OUT/'Local/routes-curves-r1.json').write_text(json.dumps(data,indent=2))
(OUT/'curve-refinement.json').write_text(json.dumps({'terrain_edited':False,'maximum_corner_trim_m':8,'routes':report},indent=2))
print('ROUNDED',sum(r['rounded_corners'] for r in report),flush=True)
