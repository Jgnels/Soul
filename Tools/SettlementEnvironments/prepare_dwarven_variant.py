"""Omit demo-only cinematic actors from the single Soul-owned Citadel copy.

Run after create_owned_variant.py. The original architecture, lighting, ordinary
flags and decoration remain. This does not admit the environment for gameplay.
"""
from pathlib import Path
import datetime
import json
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
expected = '/Game/Soul/Maps/Settlements/L_DwarfHold_Authored'
assert world.get_path_name().split('.')[0] == expected, 'Only the Soul-owned variant may change'
out = root / 'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006/dwarven-cinematic-omissions.json'
assert not out.exists(), 'Preserve the previous mutation receipt'
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()
dragon = next(a for a in actors if a.get_name() == 'SkeletalMeshActor_24')
component = dragon.get_component_by_class(unreal.SkeletalMeshComponent)
assert component.get_editor_property('skeletal_mesh_asset').get_path_name() == '/Game/DwarvenCitadel/Meshes/SKM_Dragon_LOD0.SKM_Dragon_LOD0'
remove = [dragon] + [a for a in actors if isinstance(a, unreal.LevelSequenceActor)]
assert all(a.get_path_name().startswith(expected + '.') for a in remove), 'Never edit donor sublevels'
rows = [dict(path=a.get_path_name(), label=a.get_actor_label(), actor_class=a.get_class().get_name()) for a in remove]
for actor in remove:
    assert sub.destroy_actor(actor), 'Failed to omit cinematic actor'
assert unreal.EditorLoadingAndSavingUtils.save_map(world, expected)
dirty = [p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
assert not any(p.startswith('/Game/DwarvenCitadel/') for p in dirty), 'Unexpected dirty donor map'
out.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    map=expected, omitted=rows, retained_scope='authored architecture, lighting, ordinary flags and decoration',
    acceptance='resource qualification and physical construction groups still pending',
    dirty_maps=dirty), indent=2) + '\n')
print('SOUL_CITADEL_CINEMATIC_OMISSIONS', len(rows), str(out))
