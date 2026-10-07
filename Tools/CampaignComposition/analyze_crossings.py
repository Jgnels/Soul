from pathlib import Path
import json,numpy as np,math
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionWorldComposition-20261007';L=E/'Local';j=json.loads((E/'route-composition-proposal.json').read_text());w=np.load(L/'river-water.npy');z=np.load(L/'proposed-relief-before-rivers.npy');a={a['id']:a for a in j['anchors']};runs=[]
for route in j['routes']:
 for s in route['segments']:
  if s['type']!='road':continue
  p=[]
  for x,y in zip(s['points_m'][:-1],s['points_m'][1:]):
   x,y=np.array(x),np.array(y);p.extend(x+(y-x)*np.linspace(0,1,max(1,math.ceil(np.linalg.norm(y-x)/1.5)),endpoint=False)[:,None])
  p=np.array(p+[s['points_m'][-1]]);i=np.clip(np.rint(p/3500*2040).astype(int),0,2040);wet=w[i[:,1],i[:,0]]>-900;start=None
  for k,yes in enumerate(np.r_[wet,False]):
   if yes and start is None:start=k
   if not yes and start is not None:
    end=k-1;c=p[(start+end)//2];lo=max(0,start-5);hi=min(len(p)-1,end+5);h=z[i[[lo,hi],1],i[[lo,hi],0]];span=float(np.linalg.norm(p[hi]-p[lo]));angle=math.degrees(math.atan2(*(p[hi]-p[lo])[::-1]));near=min(a,key=lambda key:math.dist(c,a[key]['xy_m']));distance=math.dist(c,a[near]['xy_m']);kind='ford' if near=='river_ford' and distance<35 else 'bridge';runs.append({'center_xy_m':c.tolist(),'endpoints_xy_m':p[[lo,hi]].tolist(),'bank_heights_m':h.tolist(),'length_m':span,'wet_run_m':float(np.linalg.norm(np.diff(p[start:end+1],axis=0),axis=1).sum()),'yaw_deg':angle,'water_z_m':float(w[i[(start+end)//2,1],i[(start+end)//2,0]]),'near_region':near,'distance_to_region_m':distance,'type':kind,'route':[route['a'],route['b']]});start=None
unique=[]
for r in sorted(runs,key=lambda r:r['length_m'],reverse=True):
 if not any(math.dist(r['center_xy_m'],q['center_xy_m'])<25 for q in unique):unique.append(r)
(E/'crossing-proposal.json').write_text(json.dumps({'all_crossing_runs':len(runs),'unique_crossings':unique,'limitations':['bank-to-bank structural spans need reviewed meshes and actual deck alignment','long water-parallel runs are defects, not certified bridges']},indent=2));print(json.dumps(unique,indent=2))
