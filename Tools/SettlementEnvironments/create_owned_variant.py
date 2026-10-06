"""Save one Soul-owned copy of the loaded, inspected authored map.

Run through the live editor after setting SOURCE_MAP and DESTINATION_MAP in
the execution namespace. This does not duplicate meshes/textures/sublevels,
save any donor package, or change actors. State-group authoring is a separate
step against the newly owned map. Existing destinations are never overwritten.
"""
from pathlib import Path
import datetime
import hashlib
import json
import unreal

assert SOURCE_MAP.startswith('/Game/') and not SOURCE_MAP.startswith('/Game/Soul/')
assert DESTINATION_MAP.startswith('/Game/Soul/Maps/Settlements/')
assert not unreal.EditorAssetLibrary.does_asset_exist(DESTINATION_MAP), 'Destination already exists'
subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = subsystem.get_editor_world()
assert world.get_path_name().split('.')[0] == SOURCE_MAP, 'Load and inspect the donor first'
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Unsaved map edits must be resolved explicitly'
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages(), 'Unsaved asset edits must be resolved explicitly'
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source_file = root/'Content'/(SOURCE_MAP.removeprefix('/Game/')+'.umap')
before = hashlib.sha256(source_file.read_bytes()).hexdigest()
actor_count = len(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
assert unreal.EditorLoadingAndSavingUtils.save_map(world, DESTINATION_MAP), 'Owned Save As failed'
assert hashlib.sha256(source_file.read_bytes()).hexdigest() == before, 'Source package changed unexpectedly'
# UE's Python save_map writes a renamed copy but can leave the source world open.
# Load the explicit destination before allowing any later actor edits.
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(DESTINATION_MAP)
owned = subsystem.get_editor_world()
assert owned.get_path_name().split('.')[0] == DESTINATION_MAP, 'The owned world did not load'
assert hashlib.sha256(source_file.read_bytes()).hexdigest() == before, 'Source package changed unexpectedly'
dest = root/'Content'/(DESTINATION_MAP.removeprefix('/Game/')+'.umap')
record = dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    source_map=SOURCE_MAP, source_sha256=before, destination_map=DESTINATION_MAP,
    destination_sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),
    destination_bytes=dest.stat().st_size, loaded_actor_count_before=actor_count,
    loaded_actor_count_after=len(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()),
    duplicated_scope='one map package; original mesh, material and sublevel references retained',
    physical_state_groups='not authored by this operation')
out=root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006'/('owned-variant-'+owned.get_name()+'.json')
assert not out.exists(), 'Receipt already exists'
out.write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record))
