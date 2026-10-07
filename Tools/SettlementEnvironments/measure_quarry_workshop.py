"""Measure inspected owned workshop modules against retained quarry ground."""
from pathlib import Path
import json,math,hashlib
root=Path(__file__).resolve().parents[2]
e=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006'
# Reuse the same triangular height interpolation as the village measurement.
shared=(root/'Tools/SettlementEnvironments/measure_retained_village.py').read_text().split("pivot=root/")[0]
shared=shared.replace("output=e/'retained-native-village-layout-r1.json';assert not output.exists()", "")
exec(compile(shared,'measured-retained-ground','exec'))
meshes=json.loads((e/'PopulationSources/quarry-native-modules-r1.json').read_text())['meshes']
origin=profile['regions']['old_quarry'];rows=[]
placements=[([-650,1350],.55,0),([-430,1570],.8,45),([-650,1640],.6,90)]
for mesh,(offset,scale,yaw) in zip(meshes,placements):
 p=root/'Content'/(mesh['package'].removeprefix('/Game/')+'.uasset');assert hashlib.file_digest(p.open('rb'),'sha256').hexdigest()==mesh['sha256']
 ex,ey=mesh['bounds_extent'][:2];angle=math.radians(yaw);c,s=math.cos(angle),math.sin(angle);zs=[];center=[origin[i]+offset[i] for i in (0,1)]
 for iy in range(13):
  for ix in range(13):
   x,y=ex*(ix/6-1)*scale,ey*(iy/6-1)*scale;zs.append(height(center[0]+x*c-y*s,center[1]+x*s+y*c))
 radius=math.hypot(ex,ey)*scale;clearance=min(distance(center,a,b) for a,b in segments)-radius
 row=dict(package=mesh['package'],sha256=mesh['sha256'],offset=offset,scale=scale,yaw=yaw,radius=radius,ground_relief=max(zs)-min(zs),route_clearance=clearance,dry=min(zs)>200)
 row['accepted']=row['dry'] and row['ground_relief']<=65 and clearance>=250;rows.append(row)
assert all(q['accepted'] for q in rows),rows
out=e/'quarry-workshop-fit-r1.json';assert not out.exists();out.write_text(json.dumps(dict(terrain=profile['map'],height_sha256=profile['height_sha256'],scope='Three minor resource-workshop modules; no capital or gameplay authority',plots=rows),indent=2)+'\n')
print(json.dumps(rows,indent=2))
