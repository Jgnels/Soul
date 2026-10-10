"""Only remove canopy rooted inside the bounded Soul-owned combat clearing."""
import unreal as u,json,hashlib
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));s=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
p='/Game/Soul/Maps/Battles/L_Heartland_Woodland';assert w.get_outermost().get_name()==p
source=r/'Content/Forest_village/Level/L_showcase_level.umap';sha=hashlib.sha256(source.read_bytes()).hexdigest()
removed=[]
def inside(v):return ((v.x+9000)/4800)**2+((v.y+9000)/4800)**2<1
for a in s.get_all_level_actors():
 if isinstance(a,u.StaticMeshActor):
  c=a.static_mesh_component
  if c.static_mesh and c.static_mesh.get_name().startswith('SM_pine_tree_') and inside(a.get_actor_location()):
   removed.append({'actor':a.get_name(),'mesh':c.static_mesh.get_path_name(),'location':str(a.get_actor_location())});s.destroy_actor(a)
 for c in a.get_components_by_class(u.InstancedStaticMeshComponent):
  if c.static_mesh and c.static_mesh.get_name().startswith('SM_pine_tree_'):
   ids=[i for i in range(c.get_instance_count()) if inside(c.get_instance_transform(i,world_space=True).translation)]
   if ids:assert c.remove_instances(ids);removed.append({'component':c.get_name(),'instances':ids})
assert u.EditorLoadingAndSavingUtils.save_map(w,p)
assert hashlib.sha256(source.read_bytes()).hexdigest()==sha
(r/'Evidence/HumanHeartlandDepth-20261010/Local/woodland-clearing-edit.json').write_text(json.dumps({'map':p,'ellipse_radii_cm':[4800,4800],'removed_trees':removed,'ground_unchanged':True,'donor_sha256':sha},indent=2))
print(json.dumps(removed))
