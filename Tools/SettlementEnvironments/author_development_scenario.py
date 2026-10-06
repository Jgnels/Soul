"""Author only the opt-in, nonvisual Soul scenario asset. Run in the live editor.

Never loads or saves a donor map, creates visual content, or binds an incomplete
city. The checked-in JSON is an immutable authoring recipe, not saved game state.
Refuses overwrite; future environment/group binding needs a separate reviewed pass.
"""
from pathlib import Path
import datetime
import hashlib
import json
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
recipe_path = root / 'Data/SettlementEnvironments/CrownsteadDevelopmentProof.json'
recipe_bytes = recipe_path.read_bytes()
recipe = json.loads(recipe_bytes)
expected = '/Game/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof'
out = root / 'Evidence/SettlementEnvironmentPlan-20261005/development-scenario-authoring.json'
assert not out.exists(), 'Refusing to overwrite prior receipt'
assert recipe['schema'] == 1 and recipe['asset_path'] == expected
assert recipe['settlement_id'] == recipe['region_id'] == 'human_capital'
assert recipe['faction_id'] == 'humans'
assert recipe['owned_environment_map'] is None, 'Do not implicitly admit an environment'
assert not unreal.EditorAssetLibrary.does_asset_exist(expected), 'Refusing to overwrite an existing scenario'

factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.SoulSettlementScenarioData)
asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    expected.rsplit('/', 1)[1], expected.rsplit('/', 1)[0], unreal.SoulSettlementScenarioData, factory)
assert asset, 'Scenario creation failed'
for key in ('settlement_id', 'faction_id', 'region_id'):
    asset.set_editor_property(key, unreal.Name(recipe[key]))
for key in ('fortification_level', 'wall_integrity_permille'):
    asset.set_editor_property(key, recipe[key])
starts = []
for row in recipe['buildings']:
    spec = unreal.SoulInitialBuildingSpec()
    for key, value in row.items():
        spec.set_editor_property(key, unreal.Name(value) if key == 'building_id' else value)
    starts.append(spec)
asset.set_editor_property('buildings', starts)
definitions = []
for row in recipe['development_definitions']:
    spec = unreal.SoulBuildingDevelopmentSpec()
    for key in ('building_id', 'category'):
        spec.set_editor_property(key, unreal.Name(row[key]))
    for key in ('max_level', 'build_days'):
        spec.set_editor_property(key, row[key])
    spec.set_editor_property('build_cost', {unreal.Name(k): v for k, v in row['build_cost'].items()})
    for key in ('prerequisites', 'unlock_ids'):
        spec.set_editor_property(key, [unreal.Name(v) for v in row[key]])
    definitions.append(spec)
asset.set_editor_property('development_definitions', definitions)
for key in ('settlement_id', 'faction_id', 'region_id'):
    assert str(asset.get_editor_property(key)) == recipe[key]
read_buildings = [
    {key: str(spec.get_editor_property(key)) if key == 'building_id' else spec.get_editor_property(key)
     for key in ('building_id', 'level', 'integrity_permille', 'built')}
    for spec in asset.get_editor_property('buildings')
]
assert read_buildings == recipe['buildings'], 'Native starting state differs from recipe'
read_definitions = []
for spec in asset.get_editor_property('development_definitions'):
    row = {key: str(spec.get_editor_property(key)) for key in ('building_id', 'category')}
    row.update({key: spec.get_editor_property(key) for key in ('max_level', 'build_days')})
    row['build_cost'] = {str(k): v for k, v in spec.get_editor_property('build_cost').items()}
    for key in ('prerequisites', 'unlock_ids'):
        row[key] = [str(v) for v in spec.get_editor_property(key)]
    read_definitions.append(row)
for actual, expected_definition in zip(read_definitions, recipe['development_definitions']):
    assert all(actual[key] == expected_definition[key] for key in actual), 'Native development data differs from recipe'
assert len(read_definitions) == len(recipe['development_definitions'])
assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False), 'Owned scenario save failed'
disk = root / 'Content' / (expected.removeprefix('/Game/') + '.uasset')
receipt = {
    'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'asset_path': expected,
    'asset_file': str(disk),
    'asset_sha256': hashlib.sha256(disk.read_bytes()).hexdigest(),
    'asset_bytes': disk.stat().st_size,
    'recipe_sha256': hashlib.sha256(recipe_bytes).hexdigest(),
    'starting_buildings': [x['building_id'] for x in recipe['buildings'] if x['built']],
    'initially_unbuilt': [x['building_id'] for x in recipe['buildings'] if not x['built']],
    'definition': recipe['development_definitions'],
    'native_readback_buildings': read_buildings,
    'native_readback_definitions': read_definitions,
    'owned_environment_map': None,
    'scope': 'nonvisual development fixture only; no donor package written; no complete settlement acceptance',
}
out.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
print('SOUL_DEVELOPMENT_SCENARIO_AUTHORED', json.dumps(receipt))
