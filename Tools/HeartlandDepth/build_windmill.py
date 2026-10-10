"""Compile the owned mill at native relative scale; no donor writes."""
import unreal,json
from pathlib import Path
r=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=r/'Evidence/HumanHeartlandDepth-20261010/Local/windmill-compiled.json'
package='/Game/Soul/CampaignProxies/Heartland/SM_CrossroadsWindmill_r1'
assert not out.exists() and not unreal.EditorAssetLibrary.does_asset_exist(package)
s=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
roots=[];actors=[];manifest=[]
for bp,location in [('/Game/Old_Windmill/Blueprint/BPP_L_PLA_Mill',unreal.Vector()),('/Game/Old_Windmill/Blueprint/BP_windmill',unreal.Vector(0,-478,1398))]:
 cls=unreal.EditorAssetLibrary.load_blueprint_class(bp);assert cls
 a=s.spawn_actor_from_class(cls,location);roots.append(a)
 for c in a.get_components_by_class(unreal.StaticMeshComponent):
  mesh=c.static_mesh
  if not mesh or any(k in mesh.get_name().lower() for k in ('grass','millstone','system_windmill_01')):continue
  transforms=[c.get_instance_transform(i,True) for i in range(c.get_instance_count())] if isinstance(c,unreal.InstancedStaticMeshComponent) else [c.get_world_transform()]
  for t in transforms:
   actor=s.spawn_actor_from_class(unreal.StaticMeshActor,t.translation,t.rotation.rotator());actor.set_actor_scale3d(t.scale3d)
   actor.static_mesh_component.set_static_mesh(mesh);actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
   for i in range(c.get_num_materials()):actor.static_mesh_component.set_material(i,c.get_material(i))
   actors.append(actor)
  manifest.append({'mesh':mesh.get_path_name(),'instances':len(transforms)})
options=unreal.MergeStaticMeshActorsOptions();options.base_package_name=package;options.destroy_source_actors=True;options.spawn_merged_actor=True
settings=unreal.MeshMergingSettings();settings.lod_selection_type=unreal.MeshLODSelectionType.LOWEST_DETAIL_LOD
settings.pivot_type=unreal.MeshMergePivotType.CUSTOM;settings.custom_pivot_location=unreal.Vector()
for k in ('generate_light_map_uv','merge_physics_data','merge_materials','support_ray_tracing','allow_distance_field'):settings.set_editor_property(k,False)
options.mesh_merging_settings=settings
print('SOUL_WINDMILL_MERGE_START',len(actors))
merged=ed.merge_static_mesh_actors(actors,options);assert merged
mesh=merged.static_mesh_component.static_mesh;before=mesh.get_num_triangles(0)
if before>120000:
 reduction=ed.get_lod_reduction_settings(mesh,0);reduction.percent_triangles=120000/before;reduction.max_num_of_triangles=120000;ed.set_lod_reduction_settings(mesh,0,reduction)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
for a in roots:s.destroy_actor(a)
out.write_text(json.dumps({'package':mesh.get_path_name(),'source_instances':len(actors),'sources':manifest,'triangles_before':before,'triangles_after':mesh.get_num_triangles(0),'bounds':str(mesh.get_bounds()),'donor_saved':False,'status':'native derivative, rendered review pending'},indent=2))
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(3500,4200,3000),unreal.MathLibrary.find_look_at_rotation(unreal.Vector(3500,4200,3000),unreal.Vector(0,0,800)))
print('SOUL_WINDMILL_MERGE_DONE',mesh.get_path_name(),mesh.get_num_triangles(0))
