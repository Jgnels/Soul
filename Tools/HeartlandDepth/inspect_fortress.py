"""Read the approved local woodland demo, actor geometry and dependencies. No save."""
import unreal,json,hashlib,time
from pathlib import Path
r=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=r/'Evidence/HumanHeartlandDepth-20261010/Local/fortress-source-inspection.json'
package='/Game/Medieval_Megapack/Levels/PL_Fortress_Day'
source=r/'Content/Medieval_Megapack/Levels/PL_Fortress_Day.umap';before=hashlib.sha256(source.read_bytes()).hexdigest()
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(package)
s=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);rows=[]
def xyz(v):return [v.x,v.y,v.z]
for a in s.get_all_level_actors():
 row={'name':a.get_name(),'label':a.get_actor_label(),'class':a.get_class().get_name(),'location':xyz(a.get_actor_location()),'rotation':str(a.get_actor_rotation())}
 center,extent=a.get_actor_bounds(False);row['center']=xyz(center);row['extent']=xyz(extent)
 meshes=[]
 for c in a.get_components_by_class(unreal.StaticMeshComponent):
  if c.static_mesh:meshes.append({'mesh':c.static_mesh.get_path_name(),'transform':str(c.get_world_transform()),'collision':str(c.get_collision_enabled())})
 if meshes:row['meshes']=meshes
 rows.append(row)
reg=unreal.AssetRegistryHelpers.get_asset_registry();opts=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
seen=set();todo=[package]
while todo:
 p=todo.pop()
 if p in seen:continue
 seen.add(p)
 todo.extend(str(q) for q in reg.get_dependencies(p,opts) if str(q).startswith('/Game/') and str(q) not in seen)
assert before==hashlib.sha256(source.read_bytes()).hexdigest()
out.write_text(json.dumps({'map':package,'source_sha256':before,'source_saved':False,'actors':rows,'dependencies':sorted(seen)},indent=2))
print('SOUL_FORTRESS_INSPECTED',len(rows),len(seen))
