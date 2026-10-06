"""Prepare the reviewed Human castle/town derivative on retained terrain.
Run only after intact-parts-r2-plan jobs complete. This writes source/fit receipts,
not terrain, source packages, runtime state or the full visitable city.
"""
from pathlib import Path
import array, hashlib, itertools, json, math, unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
p=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006/HumanMiniature'
out=p/'human-retained-source-r6.json'
assert not out.exists()
plans=json.loads((p/'parts-plan.json').read_text())
intact={j['source']:j for j in json.loads((p/'intact-parts-r2-plan.json').read_text())['jobs']}
intact.update({j['source']:j for j in json.loads((p/'repaired-parts-r3-plan.json').read_text())['jobs']})
profile=json.loads((root/'Data/CampaignEvilCorridor/presentation.json').read_text())
height_path=root/'Data/CampaignMesaLocal/MesaHeight.r16'
raw=height_path.read_bytes()
assert hashlib.sha256(raw).hexdigest()==profile['height_sha256']
heights=array.array('H');heights.frombytes(raw)
N=profile['resolution'];assert len(heights)==N*N
origin=profile['regions']['human_capital'];scale=.12
# Native XY orientation stays intact. Move the castle precinct beside, rather
# than onto, the campaign road endpoint. Full visit city remains at scale one.
pivot=[10000,-6500,0];translation=[-500,0,0]
def height(x,y):
 u=max(0,min(N-1.001,(x-profile['minimum_xy_cm'])/profile['extent_cm']*(N-1)))
 v=max(0,min(N-1.001,(y-profile['minimum_xy_cm'])/profile['extent_cm']*(N-1)))
 a,b=int(u),int(v);u-=a;v-=b
 z=lambda x,y:(heights[y*N+x]-32768)*profile['height_unit_cm']
 # Match UE Landscape triangular interpolation used by SoulCampaignTerrain::Height.
 return z(a,b)+(z(a+1,b)-z(a,b))*u+(z(a+1,b+1)-z(a+1,b))*v if u>=v else z(a,b)+(z(a+1,b+1)-z(a,b+1))*u+(z(a,b+1)-z(a,b))*v

def native_transform(row,position=True):
 t=unreal.Transform();t.translation=unreal.Vector(*(row['location'] if position else [0,0,0]))
 pitch,yaw,roll=row['rotation'];t.rotation=unreal.Rotator(pitch=pitch,yaw=yaw,roll=roll).quaternion()
 t.scale3d=unreal.Vector(*row['scale']);return t

def xyz(v):return [v.x,v.y,v.z]
def corners(mesh,t):
 b=mesh.get_bounding_box()
 return [xyz(unreal.MathLibrary.transform_location(t,unreal.Vector(x,y,z)))
  for x,y,z in itertools.product((b.min.x,b.max.x),(b.min.y,b.max.y),(b.min.z,b.max.z))]

def campaign_xy(point):return [origin[i]+translation[i]+(point[i]-pivot[i])*scale for i in range(2)]
# The same native tavern selected in the full city is mandatory in the derivative.
upgrade_root='SL_HumanCapital_Houses:PersistentLevel.LevelInstance_149'
tavern=[q for plan in plans for q in plan.get('placements',[]) if q['actor'].endswith(upgrade_root)]
assert len(tavern)==1
tavern=tavern[0]
town=[q for plan in plans if '/Castle/' not in plan['source'] and '/LI_Building_' in plan['source'] for q in plan.get('placements',[]) if not q['actor'].endswith(upgrade_root)]
town.sort(key=lambda q:(sum((q['location'][i]-tavern['location'][i])**2 for i in (0,1)),q['actor']))
selected={q['actor'] for q in town[:30]}|{tavern['actor']}
rows=[];meshes={};fits=[];castle=[]
for plan in plans:
 is_castle='/Castle/' in plan['source']
 # Keep the complete central keep. The outer island curtain wall assemblies
 # depended on native coastal geology, absent from this campaign location.
 if is_castle and '/LI_CurtainWall' in plan['source']:continue
 placements=plan.get('placements',[])
 placements=[q for q in placements if is_castle or q['actor'] in selected]
 if not placements:continue
 job=intact.get(plan['source']);package=job['package'] if job else plan['package']
 if job:
  recipe=json.loads(Path(job['recipe']).read_text());assert (root/recipe['receipt']).exists()
 mesh=unreal.load_asset(package);assert mesh
 path=mesh.get_path_name();meshes[path]=dict(lod=0,triangles=mesh.get_num_triangles(0))
 materials=[s.material_interface.get_path_name() if s.material_interface else None for s in mesh.get_editor_property('static_materials')]
 inverse=unreal.MathLibrary.invert_transform(native_transform(plan['representative'],False))
 for placement in placements:
  t=unreal.MathLibrary.compose_transforms(inverse,native_transform(placement))
  r=t.rotation.rotator();points=corners(mesh,t)
  row=dict(actor=placement['actor'],state='upgrade' if placement['actor'].endswith(upgrade_root) else 'base',mesh=path,materials=materials,location=xyz(t.translation),rotation=[r.pitch,r.yaw,r.roll],scale=xyz(t.scale3d))
  rows.append(row)
  if is_castle:castle.append((row,points));continue
  ground=[height(*campaign_xy(q)) for q in points]
  minimum=min(q[2] for q in points)
  # Place this native building's foundation near the low ground with a small
  # buried margin. Record complete corner relief for later visual review.
  target=(min(ground)-origin[2]-translation[2]-3)/scale
  dz=target-minimum;row['location'][2]+=dz
  fits.append(dict(actor=row['actor'],state=row['state'],native_min_z=minimum,delta_z=dz,ground_relief_cm=max(ground)-min(ground),campaign_center=campaign_xy(placement['location'])))
assert castle
# Castle modules share one rigid vertical shift; never split walls/towers apart.
points=[q for row,points in castle for q in points]
minimum=min(q[2] for q in points)
ground=[height(*campaign_xy(q)) for q in points]
dz=(min(ground)-origin[2]-translation[2]-3)/scale-minimum
for row,points in castle:row['location'][2]+=dz
fits.append(dict(actor='complete native central keep assembly',parts=len(castle),native_min_z=minimum,delta_z=dz,ground_relief_cm=max(ground)-min(ground)))
assert sum(r['state']=='upgrade' for r in rows)==1
counts={state:sum(meshes[r['mesh']]['triangles'] for r in rows if r['state']==state) for state in ('base','upgrade')}
assert max(counts.values())<=3000000,counts
out.write_text(json.dumps(dict(common_pivot=pivot,representation='Native complete central keep and thirty nearby authored town blocks; selected tavern is shared state. Campaign-only vertical foundation fitting; no canal walls, terrain or unrelated fabricated buildings. Full authored visit city unchanged.',meshes=meshes,instances=rows),indent=2)+'\n')
(p/'human-retained-fit-r6.json').write_text(json.dumps(dict(height_sha256=hashlib.sha256(raw).hexdigest(),origin=origin,miniature_scale=scale,miniature_translation=translation,miniature_yaw=0,counts=counts,fit=fits,status='Candidate: requires rendered review, road clearance and physical state comparison'),indent=2)+'\n')
for state in ('base','upgrade'):
 recipe=p/('human-retained-'+state+'-r6-recipe.json');assert not recipe.exists()
 recipe.write_text(json.dumps(dict(source=str(out.relative_to(root)),package='/Game/Soul/CampaignProxies/Human/SM_HumanCapital_'+state.title()+'_r6',state=state,triangle_budget=3000000,receipt=str((p/('human-retained-'+state+'-r6-receipt.json')).relative_to(root))),indent=2)+'\n')
print('SOUL_HUMAN_RETAINED_COMPOSITION',counts,len(rows),'No scenario binding or terrain changed')
