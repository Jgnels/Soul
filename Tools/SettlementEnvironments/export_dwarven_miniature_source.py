"""Record real authored Citadel transforms for deterministic cutaway miniatures.

No scene/model generation. The reduced representation omits cinematic effects,
rock enclosure, ceiling skins and fine floor trim so the underground architecture
and stateful inner colonnade can be read from the campaign camera.
"""
from pathlib import Path
import datetime
import json
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_name() == 'L_DwarfHold_Authored'
# Include transformed children of loaded level instances, which the editor's
# actor-list helper filters out. Never dereference a soft source-world asset.
actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
building = next(a for a in actors if isinstance(a, unreal.SoulSettlementBuildingActor))
building.apply_condition_name('Intact')
upgrade = {a.get_name() for a in building.get_editor_property('intact_actors')}
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
out = root / 'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006/dwarven-miniature-source.json'
assert not out.exists()
rows = []
lod_counts = {}
def vector(v): return [v.x, v.y, v.z]
for actor in actors:
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = component.static_mesh
        if not mesh: continue
        name = mesh.get_name()
        if not name.startswith(('SM_ST_', 'SM_PR_Statue')): continue
        if any(part in name.lower() for part in ('ceilling', 'ceiling', 'trimmesh')): continue
        path = mesh.get_path_name()
        if path not in lod_counts:
            lod = sub.get_lod_count(mesh) - 1
            lod_counts[path] = dict(lod=lod, triangles=mesh.get_num_triangles(lod), vertices=sub.get_number_verts(mesh, lod))
        transforms = ([component.get_instance_transform(i, True) for i in range(component.get_instance_count())]
                      if isinstance(component, unreal.InstancedStaticMeshComponent) else [component.get_world_transform()])
        for transform in transforms:
            assert transform
            rotation = transform.rotation.rotator()
            rows.append(dict(actor=actor.get_name(), component=component.get_name(), mesh=path,
                state='upgrade' if actor.get_name() in upgrade else 'base',
                location=vector(transform.translation), scale=vector(transform.scale3d),
                rotation=[rotation.pitch, rotation.yaw, rotation.roll],
                materials=[m.get_path_name() if m else None for m in component.get_materials()]))
counts = {state: sum(r['state'] == state for r in rows) for state in ('base', 'upgrade')}
triangles = {state: sum(lod_counts[r['mesh']]['triangles'] for r in rows if r['state'] == state) for state in counts}
assert counts['base'] > 100 and counts['upgrade'] > 10
out.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    source_map=world.get_path_name(), representation='authored underground city cutaway; no replacement city geometry',
    common_pivot=[4500, 0, 0], counts=counts, lowest_lod_triangles=triangles,
    meshes=lod_counts, instances=rows), indent=2) + '\n')
print('SOUL_DWARF_MINIATURE_SOURCE', counts, triangles, str(out))
