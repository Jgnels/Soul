"""Copy the reviewed three waterfront materials and repair their null default.

Run in a blank live editor after building SoulSettlementAssetAuthoring. No donor
is changed or saved. This creates candidates only; it does not bind the city.
"""
import datetime
import hashlib
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source = '/Game/CastleTown/Materials/MaterialLayer/'
destination = '/Game/Soul/Materials/Settlements/HumanCapital/'
functions = ['ML_BasicTextured_01', 'ML_Slab_Inst1', 'ML_Wood_01_Inst',
             'ML_StoneWall_Inst', 'MLI_Moss', 'MLI_Moss_Inst_02']
materials = ['MI_Stab_Algae', 'MI_Wood_Algae', 'MI_StoneWall_Algae1']
receipt = root/'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006/human-owned-material-repair-r1.json'
assert not receipt.exists()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
assert all(not unreal.EditorAssetLibrary.does_asset_exist(destination+n) for n in functions+materials)

def file_for(package): return root/'Content'/(package.removeprefix('/Game/')+'.uasset')
def digest(path):
    with path.open('rb') as stream: return hashlib.file_digest(stream, 'sha256').hexdigest()

packages = [source+n for n in functions]+[source+'LayerV2/'+n for n in materials]
before = {p: digest(file_for(p)) for p in packages}
copies, replacements = {}, {}
for name in functions:
    original = unreal.load_asset(source+name)
    copied = unreal.EditorAssetLibrary.duplicate_asset(source+name, destination+name)
    assert copied
    copies[name] = copied
    replacements[original] = copied
    if name == 'ML_BasicTextured_01':
        nodes = [e for e in unreal.MaterialEditingLibrary.get_material_function_expressions(copied)
                 if isinstance(e, unreal.MaterialExpressionTextureSampleParameter2D)
                 and str(e.get_editor_property('parameter_name')) == 'Normnal']
        assert len(nodes) == 1 and nodes[0].get_editor_property('texture') is None
        # A valid conventional source normal is required even though the native
        # layers/instances override it with their own original texture values.
        normal = unreal.load_asset('/Game/CastleTown/Textures/WallBrick/T_Slab_Normal')
        assert normal
        nodes[0].set_editor_property('texture', normal)
    else:
        original_parent = original.get_editor_property('parent')
        assert original_parent in replacements, 'Copy the native parent chain first'
        assert unreal.SoulSettlementAssetAuthoring.reparent_owned_material_function(copied, replacements[original_parent])
    unreal.MaterialEditingLibrary.update_material_function(copied)
    assert unreal.EditorAssetLibrary.save_loaded_asset(copied, False)

for name in materials:
    original = unreal.load_asset(source+'LayerV2/'+name)
    assert not unreal.SoulSettlementAssetAuthoring.remap_owned_material_layers(original, replacements), 'Donor-write guard failed'
    copied = unreal.EditorAssetLibrary.duplicate_asset(source+'LayerV2/'+name, destination+name)
    assert copied and unreal.SoulSettlementAssetAuthoring.remap_owned_material_layers(copied, replacements)
    unreal.MaterialEditingLibrary.update_material_instance(copied)
    assert unreal.EditorAssetLibrary.save_loaded_asset(copied, False)
    copies[name] = copied

after = {p: digest(file_for(p)) for p in packages}
assert before == after, 'Donor package bytes changed'
assert all(p.get_path_name().startswith('/Game/Soul/') for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()), 'Donor became dirty'
result = dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    reason='Three native waterfront materials fail SM5 compilation because their shared Normnal texture sample has a null default',
    change='Owned dependency copies; one valid native slab-normal default; original texture overrides and authored layer/blend settings retained',
    donor_before=before, donor_after=after,
    owned={n:dict(path=a.get_path_name(), sha256=digest(file_for(destination+n))) for n,a in copies.items()},
    admission='candidate material copies only; shader/render validation and explicit owned-map binding still required')
receipt.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
print('SOUL_HUMAN_OWNED_MATERIAL_REPAIR', str(receipt))
