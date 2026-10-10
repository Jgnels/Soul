import unreal as u,json
from pathlib import Path
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();rows=[]
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if a.get_actor_label()=='Composition_Bridge_human_west_bridge':
  for c in a.get_components_by_class(u.StaticMeshComponent):
   rows.append({'mesh':c.static_mesh.get_path_name(),'collision':str(c.get_collision_enabled()),'object':str(c.get_collision_object_type()),'profile':str(c.get_collision_profile_name()),'body':str(c.static_mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag'))})
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));(r/'Evidence/HumanHeartlandDepth-20261010/Local/bridge-collision-detail.json').write_text(json.dumps(rows,indent=2));print(rows)
