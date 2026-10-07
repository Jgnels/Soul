"""Read-only candidate footprint measurements on accepted Soul terrain.
No topology, routes, heightfield or source assets are changed. Measured parcels
are candidates for owned decorative assemblies, not new settlement authorities.
"""
from pathlib import Path
import array, hashlib, json, math
root=Path(__file__).resolve().parents[2]
e=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006'
output=e/'retained-outwork-fit-r6.json';assert not output.exists()
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

o=profile['regions']['orc_camp'];plots=[]
def fit(name,offset,ext,scale,yaw):
 a=math.radians(yaw);c=math.cos(a);s=math.sin(a);zs=[]
 for iy in range(13):
  for ix in range(13):
   dx=ext[0]*(ix/6-1)*scale;dy=ext[1]*(iy/6-1)*scale
   zs.append(height(o[0]+offset[0]+dx*c-dy*s,o[1]+offset[1]+dx*s+dy*c))
 radius=math.hypot(*ext)*scale;clearance=min(distance([o[0]+offset[0],o[1]+offset[1]],a,b) for a,b in segments)-radius
 row=dict(name=name,offset=offset,scale=scale,yaw=yaw,radius=radius,relief=max(zs)-min(zs),low=min(zs),clearance=clearance,accepted=min(zs)>200 and max(zs)-min(zs)<=65 and clearance>=250);plots.append(row)
fit('tower_left',[-1100,-800],[689.58548,992.718994],.25,6.88)
fit('tower_right',[1800,-450],[689.58548,992.718994],.25,186.88)
for x in [-750,-320,110,540,970,1400]:fit('rear_wall',[x,-800+(x+1100)*350/2900],[500.993195,368.733276],.43,6.88)
fit('tent_left',[-875,225],[749.850035,763.447772],.5,100)
fit('defenses_right',[1000,-100],[335.4686707027701, 499.47269240505693],.4,0)
output.write_text(json.dumps(dict(height_sha256=profile['height_sha256'],terrain=profile['map'],plots=plots),indent=2)+'\n')
print(json.dumps(plots,indent=2))
