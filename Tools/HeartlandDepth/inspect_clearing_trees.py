"""Bounded clearing in Soul-owned wrapper; keep authored ground and woodland perimeter."""
import unreal as u,json,hashlib
from pathlib import Path
assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/Soul/Maps/Battles/L_Heartland_Woodland')
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));s=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_outermost().get_name()=='/Game/Soul/Maps/Battles/L_Heartland_Woodland'
rows=[]
for a in s.get_all_level_actors():
 for c in a.get_components_by_class(u.InstancedStaticMeshComponent):
  if c.static_mesh:
   name=c.static_mesh.get_name()
   if 'tree' in name.lower():
    chosen=[]
    for i in range(c.get_instance_count()):
     t=c.get_instance_transform(i,world_space=True);p=t.translation
     if ((p.x+9000)/4800)**2+((p.y+9000)/3800)**2<1:chosen.append(i)
    rows.append({'mesh':c.static_mesh.get_path_name(),'component':c.get_path_name(),'total':c.get_instance_count(),'clearing_count':len(chosen),'indices':chosen})
(r/'Evidence/HumanHeartlandDepth-20261010/Local/woodland-clearing-inventory.json').write_text(json.dumps(rows,indent=2))
print(json.dumps([{k:v for k,v in row.items() if k!='indices'} for row in rows]))
