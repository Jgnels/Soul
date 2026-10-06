"""Candidate gate/cliff silhouette plus existing native hall state cutaway.

Selection only: authored transforms, owned meshes and original materials.
No invented geometry or repositioning of individual source structures.
"""
import datetime
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out = root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006'
destination = out/'dwarven-gate-miniature-source-r7.json'
assert not destination.exists()
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name() == 'L_DwarfHold_Authored'
source = json.loads((out/'dwarven-miniature-source.json').read_text())
hall = json.loads((out/'dwarven-miniature-renderlod-base-r5.json').read_text())
bounds = hall['selection_bounds']
excluded = set(hall['excluded_actors'])
rows = []
for row in source['instances']:
    in_hall = (row['actor'] not in excluded and
        all(bounds[2*i] <= row['location'][i] <= bounds[2*i+1] for i in range(3)))
    if row['state'] == 'upgrade' or in_hall:
        rows.append(row)
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
def vec(v): return [v.x, v.y, v.z]
rock_actors, gate_actors = [], []
# EditorActorSubsystem filters out loaded LevelInstance children. Runtime-world
# iteration includes the actual transformed gate, without loading source worlds.
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
gate = next(a for a in actors if a.get_name() == 'LevelInstance_30')
assert gate.is_loaded()
gate_prefix = gate.get_loaded_level().get_path_name()+'.'
for actor in actors:
    is_gate = actor.get_path_name().startswith(gate_prefix)
    center, extent = actor.get_actor_bounds(False)
    rock_bounds = (-15000 < center.x < -3000 and abs(center.y) < 7500 and -1500 < center.z < 4500)
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = component.static_mesh
        if not mesh: continue
        is_rock = rock_bounds and mesh.get_name().startswith(('SM_ExteriorCliff_', 'SM_Rocks_M_'))
        if not (is_gate or is_rock): continue
        path = mesh.get_path_name()
        if path not in source['meshes']:
            lod = sub.get_lod_count(mesh)-1
            source['meshes'][path] = dict(lod=lod, triangles=mesh.get_num_triangles(lod),
                vertices=sub.get_number_verts(mesh, lod))
        transforms = ([component.get_instance_transform(i,True) for i in range(component.get_instance_count())]
            if isinstance(component,unreal.InstancedStaticMeshComponent) else [component.get_world_transform()])
        for t in transforms:
            r = t.rotation.rotator()
            rows.append(dict(actor=actor.get_path_name(), component=component.get_name(), mesh=path, state='base',
                location=vec(t.translation), rotation=[r.pitch,r.yaw,r.roll], scale=vec(t.scale3d),
                materials=[m.get_path_name() if m else None for m in component.get_materials()]))
        (gate_actors if is_gate else rock_actors).append(actor.get_path_name())
source.update(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), instances=rows,
    representation='candidate native exterior gate/cliff frontage and stateful hall cutaway; original relative transforms',
    selected_rock_actors=rock_actors, selected_gate_actors=gate_actors,
    counts={state:sum(r['state']==state for r in rows) for state in ('base','upgrade')})
assert len(rock_actors)>5 and len(gate_actors)>100 and source['counts']['base']>100
destination.write_text(json.dumps(source,indent=2)+'\n')
print('SOUL_GATE_MINIATURE_SOURCE', source['counts'], len(rock_actors), str(destination))
