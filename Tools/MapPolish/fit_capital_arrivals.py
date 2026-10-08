"""Small arrival corrections around the measured, unchanged Human miniature."""
import json
import numpy as np
from route_surface import OUT,dense,assess,good
data=json.loads((OUT/'Local/routes-landings-r1.json').read_text());rows=[]
def rounded(points):
    p=np.asarray(points);out=[p[0]]
    for a,b,c in zip(p[:-2],p[1:-1],p[2:]):
        ab=b-a;bc=c-b;trim=min(5,np.linalg.norm(ab)*.3,np.linalg.norm(bc)*.3)
        left=b-ab/np.linalg.norm(ab)*trim;right=b+bc/np.linalg.norm(bc)*trim
        t=np.linspace(0,1,30)[:,None];out.extend((1-t)**2*left+2*(1-t)*t*b+t*t*right)
    out.append(p[-1]);return np.asarray(out)
for r in data['routes']:
    if r['a']!='human_capital':continue
    s=r['segments'][0];p=dense(s['points_m'],.5);radius=np.linalg.norm(p-p[0],axis=1);end=int(np.flatnonzero(radius>75)[0])
    if r['b']=='crossroads':middle=[[744,1664],[763,1684]]
    elif r['b']=='coastal_ruins':middle=[[716,1662],[714,1687]]
    else:continue
    candidate=rounded([p[0].tolist()]+middle+[p[end].tolist()]);measurement=assess(candidate)
    accepted=good(candidate,22.05)
    if accepted:s['points_m']=np.concatenate([candidate,p[end+1:]]).tolist()
    rows.append(dict(route=[r['a'],r['b']],accepted=accepted,before=assess(p[:end+1]),after=measurement,reason='Arrival follows the open central street and skirts native housing clusters; capital geometry and scale unchanged.'))
spur=rounded([[727.45098,1660.78431],[734,1654],[739.5,1647]])
assert good(spur,22.05)
data['settlement_arrival_spurs']=[dict(id='human_keep_forecourt',type='settlement street, not campaign edge',points_m=spur.tolist(),width_m=2.8)]
(OUT/'Local/routes-arrivals-r1.json').write_text(json.dumps(data,indent=2))
(OUT/'capital-arrival-fit.json').write_text(json.dumps({'routes':rows,'forecourt_spur':assess(spur),'capital_scale_changed':False,'terrain_changed':False,'waterfront_correspondence':'lake retained; no invented island or quay'},indent=2))
print(json.dumps(rows,indent=2))
