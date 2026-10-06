"""Apply reviewed material copies to the owned native waterfront sublevel.

The original meshes, placement, layer parameters and donor packages are retained.
The containing city must subsequently be rebound and rendered independently.
"""
import datetime
import hashlib
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
package = '/Game/Soul/Maps/Settlements/SL_HumanCapital_Waterfront'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split('.')[0] == package
receipt = root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006/human-waterfront-material-binding-r1.json'
assert not receipt.exists()
source = '/Game/CastleTown/Materials/MaterialLayer/LayerV2/'
destination = '/Game/Soul/Materials/Settlements/HumanCapital/'
names = ['MI_Stab_Algae', 'MI_Wood_Algae', 'MI_StoneWall_Algae1']
mapping = {source+n+'.'+n:unreal.load_asset(destination+n) for n in names}
assert all(mapping.values())
changes = []
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    assert actor.get_outermost().get_name().startswith('/Game/Soul/'), 'Unexpected foreign level actor'
    for component in actor.get_components_by_class(unreal.MeshComponent):
        for slot in range(component.get_num_materials()):
            material = component.get_material(slot)
            old_path = material.get_path_name() if material else ''
            if old_path in mapping:
                actor.modify()
                component.modify()
                component.set_material(slot, mapping[old_path])
                changes.append(dict(actor=actor.get_name(), component=component.get_name(), slot=slot,
                    source=old_path, owned=mapping[old_path].get_path_name()))
assert changes
assert unreal.EditorLoadingAndSavingUtils.save_map(world, package)
file = root/'Content'/(package.removeprefix('/Game/')+'.umap')
with file.open('rb') as stream: digest = hashlib.file_digest(stream, 'sha256').hexdigest()
receipt.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    owned_map=package, sha256=digest, changes=changes,
    geometry_or_transform_changes=0, gameplay_binding=False,
    acceptance='owned sublevel saved; full-city runtime image comparison still required'), indent=2)+'\n', encoding='utf-8')
print('SOUL_HUMAN_WATERFRONT_MATERIAL_BINDING', len(changes), str(receipt))
