"""Read-only survey of the currently loaded authored environment; never saves assets."""
import collections
import datetime
import json
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
rows = []
def vec(v):
    return [round(v.x, 3), round(v.y, 3), round(v.z, 3)]
for actor in actors:
    origin, extent = actor.get_actor_bounds(False)
    row = dict(path=actor.get_path_name(), label=actor.get_actor_label(),
        class_name=actor.get_class().get_name(), location=vec(actor.get_actor_location()),
        rotation=str(actor.get_actor_rotation()), scale=vec(actor.get_actor_scale3d()),
        origin=vec(origin), extent=vec(extent), folder=str(actor.get_folder_path()),
        meshes=[])
    for c in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = c.get_editor_property('static_mesh')
        if not mesh:
            continue
        entry = dict(component=c.get_name(), mesh=mesh.get_path_name(),
                     materials=[m.get_path_name() if m else None for m in c.get_materials()])
        if isinstance(c, unreal.InstancedStaticMeshComponent):
            entry['instances'] = c.get_instance_count()
        row['meshes'].append(entry)
    if isinstance(actor, unreal.LevelInstance):
        row['world_asset'] = str(actor.get_editor_property('world_asset'))
    rows.append(row)
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out = root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006'
stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
dest = out/('survey-'+world.get_name()+'-'+stamp+'.json')
result = dict(utc=stamp, world=world.get_path_name(), actor_count=len(rows),
    classes=dict(collections.Counter(r['class_name'] for r in rows)), actors=rows,
    dirty_content=[x.get_path_name() for x in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()],
    dirty_maps=[x.get_path_name() for x in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()])
dest.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(path=str(dest), world=result['world'], actor_count=len(rows), classes=result['classes'])))
