"""Inspect one approved native prefab in an unsaved Soul staging world.
POPULATION_PREFAB selects a fixed approved source. No donor or staging map saves.
The exported actual mesh placements feed the existing deterministic derivative tool.
"""
from pathlib import Path
import datetime, hashlib, json, unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
folder=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006/PopulationSources'
folder.mkdir(exist_ok=True)
approved={
 'raven_wall_10':('/Game/Ravenhold/Art/3_BPP/BPP_FCK_CurtainWall_10m','RH','orc_camp'),
 'raven_arch_6':('/Game/Ravenhold/Art/3_BPP/BPP_FCK_Lrg_Wall_Archway_6m','RH','orc_camp'),
 'raven_gate_l':('/Game/Ravenhold/Art/3_BPP/BPP_FCK_GateHouse_Tower_L','RH','orc_camp'),
 'raven_gate_r':('/Game/Ravenhold/Art/3_BPP/BPP_FCK_GateHouse_Tower_R','RH','orc_watch'),
 'nature_02':('/Game/Forest_village/BP/Prefab/BPP_PLA_02','FF','forest_edge'),
 'nature_03':('/Game/Forest_village/BP/Prefab/BPP_PLA_03','FF','forest_edge'),
 'nature_04':('/Game/Forest_village/BP/Prefab/BPP_PLA_04','FF','ancient_shrine'),
 'nature_prefab':('/Game/Forest_village/BP/Prefab/BPP_Prefab_01','FF','forest_edge'),
 'war_outpost_01':('/Game/Medieval_Warzone/Blueprints/BP_Outpost_01','WZ','orc_watch'),
 'war_outpost_02':('/Game/Medieval_Warzone/Blueprints/BP_Outpost_02','WZ','orc_camp'),
 'war_trebuchet':('/Game/Medieval_Warzone/Blueprints/BP_TrebuchetProps','WZ','orc_camp'),
 'war_tent_01':('/Game/Medieval_Warzone/Blueprints/BP_TentProps_01','WZ','orc_camp'),
 'war_tent_02':('/Game/Medieval_Warzone/Blueprints/BP_TentProps_02','WZ','orc_camp'),
}
key=POPULATION_PREFAB;package,family,context=approved[key]
output=folder/(key+'-native-source-r1.json');assert not output.exists()
assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_path_name().startswith('/Engine/Maps/Entry.')
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in sub.get_all_level_actors():
 if actor.get_actor_label().startswith('Soul_Population_Inspection_'):sub.destroy_actor(actor)
cls=unreal.EditorAssetLibrary.load_blueprint_class(package);assert cls
actor=sub.spawn_actor_from_class(cls,unreal.Vector())
assert actor;actor.set_actor_label('Soul_Population_Inspection_'+key)
rows=[];meshes={};pending=[actor];seen=set()
def xyz(v):return [v.x,v.y,v.z]
while pending:
 current=pending.pop()
 if current in seen:continue
 seen.add(current)
 pending.extend(current.get_attached_actors())
 for c in current.get_components_by_class(unreal.ChildActorComponent):
  child=c.get_editor_property('child_actor')
  if child:pending.append(child)
 for c in current.get_components_by_class(unreal.StaticMeshComponent):
  mesh=c.get_editor_property('static_mesh')
  if not mesh:continue
  path=mesh.get_path_name();lod=0  # Lowest auto LODs can lose surfaces; inspect native LOD0 first.
  meshes[path]=dict(lod=lod,triangles=mesh.get_num_triangles(lod),native_lod_triangles=[mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())])
  materials=[c.get_material(i).get_path_name() if c.get_material(i) else None for i in range(c.get_num_materials())]
  transforms=[c.get_instance_transform(i,world_space=True) for i in range(c.get_instance_count())] if isinstance(c,unreal.InstancedStaticMeshComponent) else [c.get_world_transform()]
  for t in transforms:
   r=t.rotation.rotator()
   rows.append(dict(actor=current.get_path_name(),component=c.get_name(),state='base',mesh=path,materials=materials,location=xyz(t.translation),rotation=[r.pitch,r.yaw,r.roll],scale=xyz(t.scale3d)))
center,extent=actor.get_actor_bounds(False,True)
triangles=sum(meshes[r['mesh']]['triangles'] for r in rows)
assert rows and triangles>0
output.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),source_prefab=package,approved_family=family,intended_context=context,common_pivot=[center.x,center.y,center.z-extent.z],bounds_center=xyz(center),bounds_extent=xyz(extent),meshes=meshes,instances=rows,input_triangles=triangles,status='Native prefab instantiated read-only; actual screenshot review and retained-ground fitting required. No capital assignment or visit routing implied.'),indent=2)+'\n')
# Existing inspection lights remain temporary. A source view demonstrates what
# the prefab actually contains before any derivative is accepted.
camera=center+unreal.Vector(-extent.x*2.2,-extent.y*2.2,max(extent.z*1.8,500))
look=unreal.MathLibrary.find_look_at_rotation(camera,center)
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(camera,look)
unreal.AutomationLibrary.set_editor_active_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_invalidate_viewports()
print('SOUL_NATIVE_POPULATION_PREFAB',key,'instances',len(rows),'triangles',triangles,'bounds',xyz(extent),'source',str(output))
