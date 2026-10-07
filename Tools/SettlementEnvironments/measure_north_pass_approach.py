"""Bounded read-only pass outwork relocation study on the accepted heightfield."""
from pathlib import Path
import json,hashlib,math,numpy as np
root=Path(__file__).resolve().parents[2];e=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006'
p=json.loads((root/'Data/CampaignEvilCorridor/presentation.json').read_text());raw=(root/'Data/CampaignMesaLocal/MesaHeight.r16').read_bytes();assert hashlib.sha256(raw).hexdigest()==p['height_sha256'];n=p['resolution'];terrain=np.frombuffer(raw,dtype='<u2').reshape(n,n).astype(np.float64)
def height(q):
 q=np.asarray(q);uv=np.clip((q-p['minimum_xy_cm'])/p['extent_cm']*(n-1),0,n-1.001);ij=uv.astype(int);t=uv-ij;a,b=ij[...,0],ij[...,1];u,v=t[...,0],t[...,1];z=lambda x,y:(terrain[y,x]-32768)*p['height_unit_cm'];return np.where(u>=v,z(a,b)+(z(a+1,b)-z(a,b))*u+(z(a+1,b+1)-z(a+1,b))*v,z(a,b)+(z(a+1,b+1)-z(a,b+1))*u+(z(a,b+1)-z(a,b))*v)
segments=np.array([(a[:2],b[:2]) for r in p['routes'] for a,b in zip(r['points'],r['points'][1:])]);A=segments[:,0];D=segments[:,1]-A;length2=(D*D).sum(axis=1)
def near(q):
 t=np.clip(((q-A)*D).sum(axis=1)/np.maximum(1,length2),0,1);points=A+D*t[:,None];ds=np.linalg.norm(points-q,axis=1);i=ds.argmin();return points[i],float(ds[i])
def lane(q,road,radius):
 d=road-q;end=q+d/np.linalg.norm(d)*radius*.72;delta=end-road;l=np.linalg.norm(delta)
 if not 100<l<6500:return None
 steps=max(4,math.ceil(l/90));t=np.linspace(0,1,steps+1);xy=road+delta*t[:,None]+np.array([-delta[1],delta[0]])*(.075*np.sin(t*np.pi))[:,None];z=height(xy);grade=math.degrees(math.atan(float((np.abs(np.diff(z))/np.linalg.norm(np.diff(xy,axis=0),axis=1)).max())))
 return dict(grade=grade,length=float(l),route=road.tolist())
original=[x for x in json.loads((e/'retained-native-population-layout-r3.json').read_text())['plots'] if x['region']=='north_pass'];origin=np.array(p['regions']['north_pass'][:2]);result=[]
for item in original:
 seed=np.array(item['offset']);angle=math.radians(item['yaw']);rotation=np.array([[math.cos(angle),-math.sin(angle)],[math.sin(angle),math.cos(angle)]]);item['radius']*=1.05;extent=np.array(item['extent_xy'])*1.05;samples=np.array([(x,y) for x in np.linspace(-1,1,13) for y in np.linspace(-1,1,13)])*extent;samples=samples@rotation.T
 initialq=origin+seed;rd,dist=near(initialq);before=lane(initialq,rd,item['radius']);candidates=[]
 for dy in range(-3000,3001,125):
  for dx in range(-3000,3001,125):
   off=seed+[dx,dy];q=origin+off
   if any(np.linalg.norm(off-np.array(r['selected']['offset']))<item['radius']+r['radius']+150 for r in result if r['selected']):continue
   road,dist=near(q)
   if dist<item['radius']+250:continue
   z=height(q+samples);lo=float(z.min());relief=float(z.max()-lo)
   if lo<200 or relief>55:continue
   run=lane(q,road,item['radius'])
   if not run or run['grade']>18:continue
   candidates.append(dict(offset=off.tolist(),relief=relief,low=lo,clearance=dist-item['radius'],**run,score=math.hypot(dx,dy)+run['grade']*3))
 candidates.sort(key=lambda x:x['score']);result.append(dict(key=item['key'],radius=item['radius'],before_offset=item['offset'],before_lane=before,candidate_count=len(candidates),selected=candidates[0] if candidates else None,top=candidates[:5]))
out=e/'north-pass-approach-fit-r2.json';assert not out.exists();out.write_text(json.dumps(dict(terrain=p['map'],height_sha256=p['height_sha256'],search='<=30m per axis on unchanged ground, 1.25m samples; conservative 5 percent enlarged native envelope, <=55cm relief and <=18 degree lane; native curve/grade algorithm; no topology change',plots=result),indent=2)+'\n');print(json.dumps(result,indent=2))
