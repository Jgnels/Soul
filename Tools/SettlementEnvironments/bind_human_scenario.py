"""Bind only presentation fields on the already-proven Human development asset.
Keeps existing construction definitions, starting state and persistence authority.
"""
from pathlib import Path
import json,hashlib,datetime,shutil,unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
e=root/'Evidence/SettlementEnvironmentPlan-20261005'
recipe_path=root/'Data/SettlementEnvironments/CrownsteadDevelopmentProof.json'
recipe=json.loads(recipe_path.read_text())
path=recipe['asset_path'];assert path=='/Game/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof'
disk=root/'Content'/(path.removeprefix('/Game/')+'.uasset')
backup=e/'Local/HumanCapital_DevelopmentProof_before_environment.uasset'
receipt=e/'Population-20261006/human-scenario-binding-r1.json'
assert not backup.exists() and not receipt.exists()
shutil.copy2(disk,backup)
asset=unreal.load_asset(path);assert asset
before_buildings=str(asset.get_editor_property('buildings'))
before_definitions=str(asset.get_editor_property('development_definitions'))
for key in ('owned_environment_map','miniature_base_mesh','miniature_upgrade_mesh'):
    value=recipe[key];assert value.startswith('/Game/Soul/')
    source=unreal.load_asset(value);assert source
    asset.set_editor_property(key,source)
t=unreal.Transform();t.translation=unreal.Vector(*recipe['miniature_translation'])
t.rotation=unreal.Rotator(yaw=recipe.get('miniature_yaw',0)).quaternion()
t.scale3d=unreal.Vector(*([recipe['miniature_scale']]*3))
asset.set_editor_property('miniature_transform',t)
assert before_buildings==str(asset.get_editor_property('buildings'))
assert before_definitions==str(asset.get_editor_property('development_definitions'))
assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
receipt.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),asset=path,sha256=hashlib.sha256(disk.read_bytes()).hexdigest(),prior_sha256=hashlib.sha256(backup.read_bytes()).hexdigest(),recipe_sha256=hashlib.sha256(recipe_path.read_bytes()).hexdigest(),starting_and_development_definitions_unchanged=True,binding={k:recipe[k] for k in ('owned_environment_map','miniature_base_mesh','miniature_upgrade_mesh','miniature_translation','miniature_yaw','miniature_scale')},status='presentation binding saved; runtime proof and visual review required'),indent=2)+'\n')
print('SOUL_HUMAN_SCENARIO_BOUND',str(receipt))
