"""Measure short opposing-bank spans inside already selected crossing reaches."""
from pathlib import Path
import json,numpy as np,math
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionWorldComposition-20261007';L=E/'Local';z=np.load(L/'composed-height.npy');g=json.loads((E/'designed-crossing-gates.json').read_text());rivers={r['id']:np.array(r['points_xyz_m']) for r in json.loads((E/'river-skeleton.json').read_text())['rivers']}
def sample(p):
 p=np.clip(np.asarray(p)/3500*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];a,b,c,d=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1];fx,fy=f[...,0],f[...,1];return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
rows=[]
for gate in g['gates']:
 p=rivers[gate['river_id']];start=np.array(gate['center_xy_m']);near=np.where(np.linalg.norm(p[:,:2]-start,axis=1)<45)[0];options=[]
 for j in near[::2]:
  center=p[j,:2];height=p[j,2];t=p[min(len(p)-1,j+6),:2]-p[max(0,j-6),:2];t/=np.linalg.norm(t);normal=np.array([-t[1],t[0]])
  for angle in [-12,-6,0,6,12]:
   q=math.radians(angle);n=np.array([normal[0]*math.cos(q)-normal[1]*math.sin(q),normal[0]*math.sin(q)+normal[1]*math.cos(q)]);banks=[]
   for sign in [-1,1]:
    for distance in np.arange(3,23,.5):
     point=center+sign*n*distance;hh=float(sample(point));inner=float(sample(point-sign*n*1.5));outer=float(sample(point+sign*n*1.5));slope=math.degrees(math.atan(abs(outer-inner)/3))
     if hh>height+.15 and slope<=28 and min(inner,outer)>height+.05:banks.append((point,hh,distance));break
   if len(banks)!=2:continue
   length=banks[0][2]+banks[1][2]
   if abs(banks[0][1]-banks[1][1])/length>math.tan(math.radians(10)):continue
   options.append({'center_xy_m':((banks[0][0]+banks[1][0])/2).tolist(),'bank_endpoints_xy_m':[b[0].tolist() for b in banks],'bank_z_m':[b[1] for b in banks],'span_m':length,'yaw_deg':math.degrees(math.atan2(n[1],n[0])),'water_z_m':float(height),'distance_from_selected_reach_m':float(np.linalg.norm(center-start))})
 if options:
  best=min(options,key=lambda x:(x['span_m'],abs(x['bank_z_m'][1]-x['bank_z_m'][0])));rows.append(dict(gate_id=gate['id'],type=gate['type'],candidate_count=len(options),**best))
 else:rows.append({'gate_id':gate['id'],'type':gate['type'],'status':'no stable short span found in bounded reach; do not fabricate a long bridge'})
(E/'shortest-bank-spans.json').write_text(json.dumps({'method':'Search within 45 m of the selected crossing reach, near perpendicular to river, choose shortest span with dry stable opposing banks and <=10 degree bank elevation differential. Final asset/deck seating still requires native inspection.','crossings':rows},indent=2));print(json.dumps(rows,indent=2))
