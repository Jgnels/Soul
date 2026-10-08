import unreal as u,json,math,hashlib
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/ProductionContinuation-20261008';d=json.loads((E/'dark-cues-plan.json').read_text());a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
labels={x.get_actor_label() for x in a.get_all_level_actors()};assert all(p['label'] not in labels for p in d['placements'])
meshes={};hashes={};placed=[]
for row in d['placements']:
 path=row['mesh']
 if path not in meshes:
  file=R/'Content'/(path.removeprefix('/Game/')+'.uasset')
  with file.open('rb') as f:hashes[str(file)]=hashlib.file_digest(f,'sha256').hexdigest()
  meshes[path]=u.load_asset(path);assert meshes[path]
 mesh=meshes[path];b=mesh.get_bounding_box();sc=u.Vector(*row['scale']);rot=u.Rotator(pitch=0,yaw=row['yaw'],roll=0);center=u.Vector((b.min.x+b.max.x)*.5*sc.x,(b.min.y+b.max.y)*.5*sc.y,0);theta=math.radians(row['yaw']);offset=u.Vector(center.x*math.cos(theta)-center.y*math.sin(theta),center.x*math.sin(theta)+center.y*math.cos(theta),0)
 x,y=row['xy_m'];z=row['ground_z_m']*100-b.min.z*sc.z
 actor=a.spawn_actor_from_class(u.StaticMeshActor,u.Vector(x*100-175000-offset.x,y*100-175000-offset.y,z),rot);actor.set_actor_label(row['label']);actor.set_actor_scale3d(sc);actor.static_mesh_component.set_static_mesh(mesh);actor.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION);actor.static_mesh_component.set_mobility(u.ComponentMobility.STATIC);actor.set_actor_enable_collision(False)
 placed.append({'label':row['label'],'mesh':path,'location':list(actor.get_actor_location().to_tuple()),'role':row['role'],'collision':False})
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
for file,sha in hashes.items():
 with Path(file).open('rb') as f:assert hashlib.file_digest(f,'sha256').hexdigest()==sha
(E/'dark-cues-placement.json').write_text(json.dumps({'actors':placed,'donor_hashes':hashes,'status':'REQUIRES_RENDERED_REVIEW','terrain_changed':False,'road_collision_changed':False},indent=2));print('REGIONAL_CUES_PLACED',len(placed))
