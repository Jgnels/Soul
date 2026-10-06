"""Bind the inspected owned Citadel and deterministic hall derivative, once.

Only creates the explicit qualification DataAsset. No donor or saved-game writes.
"""
from pathlib import Path
import datetime
import hashlib
import json
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
recipe_path = root / 'Data/SettlementEnvironments/DwarfHoldDevelopmentProof.json'
recipe = json.loads(recipe_path.read_text())
path = recipe['asset_path']
assert path == '/Game/Soul/Data/Settlements/DA_Soul_DwarfHold_DevelopmentProof'
assert not unreal.EditorAssetLibrary.does_asset_exist(path)
for key in ('owned_environment_map', 'miniature_base_mesh', 'miniature_upgrade_mesh'):
    assert unreal.EditorAssetLibrary.does_asset_exist(recipe[key]), key
factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.SoulSettlementScenarioData)
asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit('/', 1)[1], path.rsplit('/', 1)[0], unreal.SoulSettlementScenarioData, factory)
assert asset
for key in ('settlement_id', 'region_id', 'faction_id', 'fortification_level', 'wall_integrity_permille'):
    asset.set_editor_property(key, recipe[key])
buildings = []
for row in recipe['buildings']:
    spec = unreal.SoulInitialBuildingSpec()
    for key, value in row.items(): spec.set_editor_property(key, value)
    buildings.append(spec)
asset.set_editor_property('buildings', buildings)
definitions = []
for row in recipe['development_definitions']:
    spec = unreal.SoulBuildingDevelopmentSpec()
    for key, value in row.items(): spec.set_editor_property(key, value)
    definitions.append(spec)
asset.set_editor_property('development_definitions', definitions)
for key in ('owned_environment_map', 'miniature_base_mesh', 'miniature_upgrade_mesh'):
    asset.set_editor_property(key, unreal.load_asset(recipe[key]))
transform = unreal.Transform()
transform.translation = unreal.Vector(*recipe['miniature_translation'])
transform.rotation = unreal.Rotator(yaw=recipe.get('miniature_yaw', 0)).quaternion()
transform.scale3d = unreal.Vector(*([recipe['miniature_scale']] * 3))
asset.set_editor_property('miniature_transform', transform)
assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
disk = root / 'Content' / (path.removeprefix('/Game/') + '.uasset')
out = root / 'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006/dwarven-scenario-authoring.json'
assert not out.exists()
out.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    asset_path=path, sha256=hashlib.sha256(disk.read_bytes()).hexdigest(),
    recipe_sha256=hashlib.sha256(recipe_path.read_bytes()).hexdigest(),
    binding=recipe, status='authored and saved; runtime acceptance pending'), indent=2) + '\n')
print('SOUL_DWARF_SCENARIO_AUTHORED', str(out))
