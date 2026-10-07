"""Independent acceptance measurements; report defects rather than hide them."""
from pathlib import Path
import json,math,hashlib,datetime,ast,subprocess,argparse
import numpy as np
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionWorldComposition-20261007';L=E/'Local'
parser=argparse.ArgumentParser();parser.add_argument('--routes',default='route-local-repair-study-r7.json');parser.add_argument('--revision',default='r2');args=parser.parse_args()
world=json.loads((R/'Data/soul_world_overmap_v1_20260922.json').read_text());routes=json.loads((E/args.routes).read_text());anchors={n['id']:n for n in routes['anchors']}
assert set(anchors)=={n['id'] for n in world['nodes']}
assert {tuple(sorted((r['a'],r['b']))) for r in routes['routes']}=={tuple(sorted((r['a'],r['b']))) for r in world['edges']}
z=np.load(L/'composed-height.npy');ocean=np.load(L/'ocean-connected-mask.npy');bridges=json.loads((E/'selected-crossing-sites-r3.json').read_text())['crossings'];decks={b['id']:b for b in json.loads((E/'bridge-placement-r2.json').read_text())['bridges']}
def sample(points):
 p=np.clip(np.asarray(points)/3500*2040,0,2039.999);i=p.astype(int);f=p-i;x,y=i[...,0],i[...,1];a,b,c,d=z[y,x],z[y,x+1],z[y+1,x],z[y+1,x+1];fx,fy=f[...,0],f[...,1];return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)
def dense(points):
 out=[]
 for p,q in zip(points[:-1],points[1:]):
  p,q=np.array(p)[:2],np.array(q)[:2];d=np.linalg.norm(q-p)
  if d<1e-6:continue
  out.extend(p+(q-p)*np.linspace(0,1,max(1,math.ceil(d)),endpoint=False)[:,None])
 return np.array(out+[np.array(points[-1])[:2]])
measurements=[];probes=[]
for r in routes['routes']:
 grades=[];failures=[]
 for s in r['segments']:
  if s['type']!='road':continue
  p=dense(s['points_m']);h=sample(p);deck_mask=np.zeros(len(p),bool)
  for b in bridges:
   if b['type']=='ford':continue
   yaw=math.radians(b['yaw_deg']);axis=np.array([math.cos(yaw),math.sin(yaw)]);delta=p-b['center_xy_m'];along=delta@axis;cross=delta@np.array([-axis[1],axis[0]]);inside=(abs(along)<=b['span_m']/2)&(abs(cross)<=2.64)
   deck=decks[b['gate_id']];h[inside]=np.maximum(h[inside],deck['deck_z_m']+along[inside]*deck['deck_gradient']);deck_mask|=inside
  distances=np.linalg.norm(np.diff(p,axis=0),axis=1);g=np.degrees(np.arctan2(abs(np.diff(h)),np.maximum(distances,.0001)));grades.extend(g.tolist())
  for i in np.where(g>22.1)[0]:failures.append(dict(xy_m=p[i].tolist(),grade_deg=float(g[i]),near_bridge=bool(deck_mask[i] or deck_mask[i+1])))
  probes.extend(p[::max(1,len(p)//10)].tolist())
 measurements.append(dict(a=r['a'],b=r['b'],max_surface_grade_deg=max(grades) if grades else 0,over22_samples=len(failures),worst_samples=sorted(failures,key=lambda x:-x['grade_deg'])[:4]))
river=json.loads((E/'river-skeleton.json').read_text());assert all(np.max(np.diff(np.array(r['points_xyz_m'])[:,2]))<1e-6 for r in river['rivers'])
harbor=np.array(anchors['viking_harbour']['xy_m']);yy,xx=np.where(ocean);dist=float(np.min(np.hypot(xx*3500/(ocean.shape[1]-1)-harbor[0],yy*3500/(ocean.shape[0]-1)-harbor[1])))
checks=[]
for row in json.loads((E/'preservation-baseline.json').read_text())['files']:
 p=Path(row['path']);got=hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None;checks.append(dict(path=str(p),before=row['sha256'],after=got,unchanged=got==row['sha256']))
inherited=[]
for row in json.loads((E/'inherited-tracked-hashes.json').read_text()):
 p=R/row['path'];got=hashlib.sha256(p.read_bytes()).hexdigest();inherited.append(dict(path=row['path'],unchanged=got==row['sha256'],intentional_change=row['path']=='.gitignore'))
assert all(r['unchanged'] for r in checks)
assert all(r['unchanged'] or r['intentional_change'] for r in inherited)
syntax=[]
for p in (R/'Tools/CampaignComposition').glob('*.py'):ast.parse(p.read_text(encoding='utf-8-sig'));syntax.append(p.name)
result=dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=R,text=True).strip(),canonical_ids=36,canonical_edges=51,river_profiles_downhill=True,harbor_distance_to_boundary_connected_ocean_m=dist,pre_channel_grade_max_deg=max(r['max_road_grade_deg'] for r in routes['routes']),post_channel_surface_routes=measurements,post_channel_routes_over22=sum(r['over22_samples']>0 for r in measurements),preserved_files=len(checks),all_preserved=True,inherited_tracked=inherited,syntax_checked=syntax,status='post-channel grade includes bridge deck edges; exceptions are blockers, not waived passes')
(E/('validation-'+args.revision+'.json')).write_text(json.dumps(result,indent=2));(E/'preservation-final.json').write_text(json.dumps({'files':checks,'all_unchanged':True},indent=2))
probes=np.array(probes);rng=np.random.default_rng(7100710);extra=rng.uniform(20,3480,(200,2));probes=np.concatenate([probes[::max(1,len(probes)//500)],extra]);hs=sample(probes)
(L/'native-height-probes.json').write_text(json.dumps({'points':[[float(x),float(y),float(h)] for (x,y),h in zip(probes,hs)]}))
print(json.dumps({k:result[k] for k in ['canonical_ids','canonical_edges','river_profiles_downhill','harbor_distance_to_boundary_connected_ocean_m','pre_channel_grade_max_deg','post_channel_routes_over22','preserved_files','all_preserved']}))
