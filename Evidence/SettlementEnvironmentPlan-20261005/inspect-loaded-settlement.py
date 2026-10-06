"""Read only: inspect an ALREADY LOADED authored scene. Root alone loads it.

Required SOUL_SCENE_EXPECTED_MAP, e.g.
'/Game/CastleTown/Levels/Persistant/PL_CastleTown'. No scene/asset load, save,
spawn, property mutation, DDC/LOD query, simulation or component registration.
Optional SOUL_SCENE_OUTPUT points inside this evidence directory.
"""
import collections
import datetime
import json
from pathlib import Path
import unreal


def path(obj):
    return obj.get_path_name() if obj else None


def vec(v):
    return [float(v.x), float(v.y), float(v.z)]


def transform(t):
    q = t.rotation
    return {'translation_cm': vec(t.translation), 'quaternion_xyzw': [float(q.x), float(q.y), float(q.z), float(q.w)], 'scale': vec(t.scale3d)}


def query(row, key, fn):
    try:
        row[key] = fn()
    except Exception as exc:
        row.setdefault('unavailable', {})[key] = str(exc)


project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
evidence = project / 'Evidence/SettlementEnvironmentPlan-20261005'
expected = globals().get('SOUL_SCENE_EXPECTED_MAP')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not expected or not world or path(world).split('.')[0] != expected:
    raise RuntimeError('No inspection: expected explicit map %r, loaded %r' % (expected, path(world)))
output = Path(globals().get('SOUL_SCENE_OUTPUT', evidence / ('loaded-' + expected.rsplit('/', 1)[-1] + '.json'))).resolve()
if not output.is_relative_to(evidence.resolve()):
    raise RuntimeError('Output must remain under the settlement evidence directory')
if output.exists():
    raise RuntimeError('Refusing to overwrite prior inspection: ' + str(output))
actors = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
if len(actors) > 30000:
    raise RuntimeError('Loaded actor count exceeds bounded inspection limit: %d' % len(actors))
report = {'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'world': path(world), 'expected_map': expected,
          'mode': 'READ_ONLY_LOADED_ACTORS_ONLY', 'loaded_actor_count': len(actors), 'actors': [], 'streaming_levels': [],
          'limits': ['Unloaded sublevels, World Partition cells and nested LevelInstances may be absent.',
                     'Zero/missing actors is incomplete loading evidence, not an invalid donor.',
                     'No triangle/LOD/vertex data, individual foliage-instance transforms or bulk asset loads requested.',
                     'Labels/folders are review hints; exact GUID/path membership must be approved before state binding.']}
query(report, 'world_partition', lambda: path(world.get_editor_property('world_partition')))
query(report, 'world_settings', lambda: path(world.get_world_settings()))
query(report, 'dirty_maps_before', lambda: [path(x) for x in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()])
try:
    levels = list(world.get_editor_property('streaming_levels'))
    for level in levels:
        item = {'path': path(level), 'class': level.get_class().get_name()}
        query(item, 'world_asset', lambda: path(level.get_world_asset()))
        query(item, 'loaded_level', lambda: path(level.get_loaded_level()))
        query(item, 'is_loaded', level.is_level_loaded)
        query(item, 'is_visible', level.is_level_visible)
        for prop in ('should_be_loaded', 'should_be_visible', 'level_transform'):
            query(item, prop, lambda p=prop: str(level.get_editor_property(p)))
        report['streaming_levels'].append(item)
except Exception as exc:
    report['streaming_levels_unavailable'] = str(exc)
classes = collections.Counter()
levels_seen = collections.Counter()
mesh_components = collections.Counter()
instance_total = 0
for actor in actors:
    row = {'path': path(actor), 'class': actor.get_class().get_name()}
    classes[row['class']] += 1
    query(row, 'label', actor.get_actor_label)
    query(row, 'folder', lambda: str(actor.get_folder_path()))
    query(row, 'guid', lambda: str(actor.get_editor_property('actor_guid')))
    query(row, 'level', lambda: path(actor.get_outer()))
    if row.get('level'):
        levels_seen[row['level']] += 1
    query(row, 'transform', lambda: transform(actor.get_actor_transform()))
    query(row, 'bounds_world', lambda: [vec(x) for x in actor.get_actor_bounds(False, True)])
    query(row, 'attachment_parent', lambda: path(actor.get_attach_parent_actor()))
    query(row, 'tags', lambda: [str(x) for x in actor.tags])
    for prop in ('layers', 'data_layer_assets', 'is_spatially_loaded', 'is_editor_only_actor', 'hidden'):
        query(row, prop, lambda p=prop: str(actor.get_editor_property(p)))
    if 'LevelInstance' in row['class'] or 'BPP_' in row['class']:
        query(row, 'world_asset', lambda: str(actor.get_editor_property('world_asset')))
    components = []
    for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
        item = {'path': path(comp), 'class': comp.get_class().get_name()}
        mesh = comp.get_editor_property('static_mesh')
        item['mesh'] = path(mesh)
        if mesh:
            mesh_components[item['mesh']] += 1
        query(item, 'transform_world', lambda: transform(comp.get_component_transform()))
        query(item, 'materials', lambda: [path(comp.get_material(i)) for i in range(comp.get_num_materials())])
        query(item, 'material_overrides', lambda: [path(m) for m in comp.get_editor_property('override_materials')])
        query(item, 'collision_enabled', lambda: str(comp.get_collision_enabled()))
        for prop in ('visible', 'hidden_in_game', 'mobility', 'cast_shadow'):
            query(item, prop, lambda p=prop: str(comp.get_editor_property(p)))
        if isinstance(comp, unreal.InstancedStaticMeshComponent):
            query(item, 'instance_count', comp.get_instance_count)
            instance_total += item.get('instance_count', 0)
        if isinstance(comp, unreal.SplineMeshComponent):
            item['spline_mesh_requires_deformation_preservation'] = True
        components.append(item)
    row['static_mesh_components'] = components
    report['actors'].append(row)
report['class_counts'] = dict(classes)
report['loaded_level_actor_counts'] = dict(levels_seen)
report['mesh_component_counts'] = dict(mesh_components)
report['reported_instanced_population'] = instance_total
registry = unreal.AssetRegistryHelpers.get_asset_registry()
query(report, 'direct_package_dependencies', lambda: [str(x) for x in registry.get_dependencies(expected, unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True, include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False))])
query(report, 'dirty_maps_after', lambda: [path(x) for x in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()])
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print('SOUL_AUTHORED_SCENE_READ_ONLY', json.dumps({'output': str(output), 'world': report['world'], 'actors': len(actors), 'classes': dict(classes), 'loaded_levels': dict(levels_seen), 'unique_meshes': len(mesh_components), 'instanced_population': instance_total}))
