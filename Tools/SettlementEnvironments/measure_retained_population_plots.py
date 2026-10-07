"""Read-only candidate footprint measurements on accepted Soul terrain.
No topology, routes, heightfield or source assets are changed. Measured parcels
are candidates for owned decorative assemblies, not new settlement authorities.
"""
from pathlib import Path
import array, hashlib, json, math
root=Path(__file__).resolve().parents[2]
e=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006'
output=e/'retained-population-parcels-r1.json';assert not output.exists()
profile=json.loads((root/'Data/CampaignEvilCorridor/presentation.json').read_text())
raw=(root/'Data/CampaignMesaLocal/MesaHeight.r16').read_bytes()
assert hashlib.sha256(raw).hexdigest()==profile['height_sha256']
h=array.array('H');h.frombytes(raw);n=profile['resolution'];assert len(h)==n*n
minimum=profile['minimum_xy_cm'];extent=profile['extent_cm']
def height(x,y):
 u=max(0,min(n-1.001,(x-minimum)/extent*(n-1)));v=max(0,min(n-1.001,(y-minimum)/extent*(n-1)))
 a,b=int(u),int(v);u-=a;v-=b
 z=lambda i,j:(h[j*n+i]-32768)*profile['height_unit_cm']
 return z(a,b)+(z(a+1,b)-z(a,b))*u+(z(a+1,b+1)-z(a+1,b))*v if u>=v else z(a,b)+(z(a+1,b+1)-z(a,b+1))*u+(z(a,b+1)-z(a,b))*v
segments=[(a,b) for r in profile['routes'] for a,b in zip(r['points'],r['points'][1:])]
def distance(p,a,b):
 d=[b[i]-a[i] for i in (0,1)];l=sum(x*x for x in d)
 t=max(0,min(1,sum((p[i]-a[i])*d[i] for i in (0,1))/max(1,l)))
 return math.hypot(*(p[i]-a[i]-t*d[i] for i in (0,1)))
results={}
for region,origin in profile['regions'].items():
 kinds={}
 for size in [(700,700),(1400,1000),(2400,1600)]:
  candidates=[];radius=math.hypot(*size)/2
  for dy in range(-4500,4501,500):
   for dx in range(-4500,4501,500):
    if dx*dx+dy*dy<700**2:continue
    center=[origin[0]+dx,origin[1]+dy]
    if min(center)-radius<minimum or max(center)+radius>minimum+extent:continue
    clearance=min(distance(center,a,b) for a,b in segments)-radius
    if clearance<250:continue
    zs=[height(center[0]+size[0]*(i/8-.5),center[1]+size[1]*(j/8-.5)) for i in range(9) for j in range(9)]
    relief=max(zs)-min(zs)
    if min(zs)<200 or relief>min(size)*.10:continue
    candidates.append(dict(center=[*center,(min(zs)+max(zs))/2],offset=[dx,dy],size_cm=size,relief_cm=relief,road_clearance_cm=clearance,score=relief+math.hypot(dx,dy)*.008))
  candidates.sort(key=lambda q:(q['score'],q['center']))
  kept=[]
  for row in candidates:
   if all(math.dist(row['center'][:2],q['center'][:2])>radius*1.6 for q in kept):kept.append(row)
   if len(kept)==4:break
  kinds['x'.join(map(str,size))]=kept
 results[region]=kinds
output.write_text(json.dumps(dict(terrain=profile['map'],height_sha256=profile['height_sha256'],method='81 samples per unchanged rectangular envelope; actual existing route segment clearance; Landscape triangle interpolation; no flattening or writes',status='Candidate measurements only: native asset extents, existing trees/props and rendered collision/road fit must still be reviewed',regions=results),indent=2)+'\n')
print({region:{k:len(v) for k,v in kinds.items()} for region,kinds in results.items()})
