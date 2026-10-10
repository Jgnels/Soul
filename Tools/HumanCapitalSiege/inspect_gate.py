import unreal as u,json
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));e=r/'Evidence/HumanCapitalSiege-20261010/Local'
s=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
def xyz(v):return [v.x,v.y,v.z]
rows=[]
for a in s.get_all_level_actors():
 for c in a.get_components_by_class(u.StaticMeshComponent):
  m=c.static_mesh
  if not m:continue
  if any(x in m.get_name().lower() for x in ['gate','portcullis','castle_entrance']):
   rows.append({'actor':a.get_name(),'label':a.get_actor_label(),'mesh':m.get_path_name(),'component':c.get_name(),'location':xyz(c.get_world_location()),'rotation':str(c.get_world_rotation()),'transform':str(c.get_world_transform()),'bounds':str(c.get_local_bounds()),'collision':str(c.get_collision_enabled())})
(e/'native-gates.json').write_text(json.dumps({'world':w.get_name(),'actors':len(s.get_all_level_actors()),'gates':rows},indent=2))
print('SIEGE_GATE_INSPECTION',len(rows))
