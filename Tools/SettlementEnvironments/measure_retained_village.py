"""Read-only candidate footprint measurements on accepted Soul terrain.
No topology, routes, heightfield or source assets are changed. Measured parcels
are candidates for owned decorative assemblies, not new settlement authorities.
"""
from pathlib import Path
import array, hashlib, json, math
root=Path(__file__).resolve().parents[2]
e=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006'
output=e/'retained-native-village-layout-r1.json';assert not output.exists()
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


pivot=root/'Evidence/CampaignWorldTerrain-20261005/Pivot';meshes={};plots=[]
for letter in 'ABC':
 p=pivot/('SM_Medieval_Building_'+letter+'_r1-'+('lods-finish' if letter=='A' else 'derivation')+'.json');d=json.loads(p.read_text());mesh=root/'Content/SoulCampaignProxies'/('SM_Medieval_Building_'+letter+'_r1.uasset');assert hashlib.file_digest(mesh.open('rb'),'sha256').hexdigest()==d['owned_file']['sha256'];meshes[letter]=dict(bounds=d['owned_bounds_cm'],sha256=d['owned_file']['sha256'],package='/Game/SoulCampaignProxies/SM_Medieval_Building_'+letter+'_r1',lod=1,triangles=d['lods_actual'][1]['triangles'])
def nearest(p):
 best=None
 for a,b in segments:
  dx,dy=b[0]-a[0],b[1]-a[1];t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/max(dx*dx+dy*dy,1)));q=[a[0]+t*dx,a[1]+t*dy];row=(math.dist(p,q),q)
  if best is None or row[0]<best[0]:best=row
 return best
requests=[('crossroads','A',.36,[-1600,-600]),('crossroads','B',.6,[-1500,1000]),('crossroads','C',.48,[-400,1600]),('crossroads','B',.55,[1600,1000]),('crossroads','C',.48,[2400,2200]),('crossroads','B',.55,[1300,-1000]),('old_quarry','A',.42,[1200,400]),('old_quarry','B',.6,[-1600,700])]
existing={'crossroads':[([875,-2500],482),([0,-4000],391)],'old_quarry':[([-1000,-500],482)]}
for region,letter,scale,seed in requests:
 origin=profile['regions'][region];native_extent=meshes[letter]['bounds']['extent'];radius=math.hypot(*native_extent[:2])*scale;candidates=[]
 for dy in range(-750,751,125):
  for dx in range(-750,751,125):
   offset=[seed[0]+dx,seed[1]+dy];center=[origin[i]+offset[i] for i in (0,1)]
   if any(math.dist(offset,q['offset'])<radius+q['radius']+150 for q in plots if q['region']==region):continue
   if any(math.dist(offset,p)<radius+r+150 for p,r in existing[region]):continue
   rd,rp=nearest(center)
   if rd<radius+250:continue
   front=math.degrees(math.atan2(rp[1]-center[1],rp[0]-center[0]));yaw=front-({'A':180,'B':-90,'C':0}[letter]);a=math.radians(yaw);c,s=math.cos(a),math.sin(a);zs=[]
   for iy in range(13):
    for ix in range(13):
     x,y=native_extent[0]*(ix/6-1)*scale,native_extent[1]*(iy/6-1)*scale;zs.append(height(center[0]+x*c-y*s,center[1]+x*s+y*c))
   relief=max(zs)-min(zs)
   if min(zs)<200 or relief>65:continue
   candidates.append(dict(region=region,variant=letter,package=meshes[letter]['package'],offset=offset,world_scale=scale,yaw=yaw,radius=radius,low=min(zs),high=max(zs),relief=relief,nearest_route=rp,clearance=rd-radius,score=math.hypot(dx,dy)+relief*2))
 assert candidates,(region,letter,seed)
 candidates.sort(key=lambda q:q['score']);plots.append(candidates[0])
output.write_text(json.dumps(dict(terrain=profile['map'],height_sha256=profile['height_sha256'],method='Actual qualified owned mesh bounds, native frontage, 169 rotated ground samples, existing-route clearance and non-overlap; no terrain/topology changes',meshes=meshes,plots=plots),indent=2)+'\n')
print(json.dumps(plots,indent=2))
