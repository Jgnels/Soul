"""Bounded SM5 sampler repair on Soul-owned copies of the native gate material.
No textures, geometry, UVs or native parameter values are generated or replaced.
"""
from pathlib import Path
import datetime,hashlib,json,unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
e=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006/PopulationSources'
receipt=e/'ravenhold-sampler-copy-r1.json';assert not receipt.exists()
source=json.loads((e/'raven_gate_l-native-source-r1.json').read_text())
destination='/Game/Soul/Materials/Settlements/Ravenhold/'
master='/Game/Ravenhold/Art/2_Master_Materials/M_FCK_Architecture'
function='/Game/Ravenhold/Art/2_Master_Materials/MF/MF_FCK_TriPlanar'
originals={};copies={};changed=[]
def disk(path):return root/'Content'/(path.split('.')[0].removeprefix('/Game/')+'.uasset')
def digest(path):return hashlib.file_digest(disk(path).open('rb'),'sha256').hexdigest()
def copy(path):
 obj=unreal.load_asset(path);assert obj
 out=destination+obj.get_name()+'_SM5_r1';assert not unreal.EditorAssetLibrary.does_asset_exist(out)
 originals[path]=digest(path);result=unreal.EditorAssetLibrary.duplicate_asset(path,out);assert result;copies[path]=result;return result
f=copy(function)
for node in unreal.MaterialEditingLibrary.get_material_function_expressions(f):
 if isinstance(node,unreal.MaterialExpressionTextureSample):
  node.set_editor_property('sampler_source',unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS);changed.append(node.get_path_name())
unreal.MaterialEditingLibrary.update_material_function(f);assert unreal.EditorAssetLibrary.save_loaded_asset(f,False)
m=copy(master)
for node in unreal.MaterialEditingLibrary.get_material_expressions(m):
 if isinstance(node,unreal.MaterialExpressionMaterialFunctionCall) and node.get_editor_property('material_function').get_path_name().split('.')[0]==function:node.set_editor_property('material_function',f)
 if isinstance(node,unreal.MaterialExpressionTextureSample):
  texture=node.get_editor_property('texture')
  if isinstance(texture,unreal.Texture2D):
   assert texture.get_editor_property('address_x')==unreal.TextureAddress.TA_WRAP and texture.get_editor_property('address_y')==unreal.TextureAddress.TA_WRAP
   node.set_editor_property('sampler_source',unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS);changed.append(node.get_path_name())
unreal.MaterialEditingLibrary.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m,False)
def owned_parent(asset):
 path=asset.get_path_name().split('.')[0]
 if path in copies:return copies[path]
 if not isinstance(asset,unreal.MaterialInstanceConstant):return None
 parent=asset.get_editor_property('parent');replacement=owned_parent(parent)
 if replacement is None:return None
 owned=copy(path);unreal.MaterialEditingLibrary.set_material_instance_parent(owned,replacement);unreal.MaterialEditingLibrary.update_material_instance(owned);assert unreal.EditorAssetLibrary.save_loaded_asset(owned,False);return owned
remap={}
for path in sorted(set(p for row in source['instances'] for p in row['materials'] if p)):
 replacement=owned_parent(unreal.load_asset(path))
 if replacement:remap[path]=replacement.get_path_name()
assert remap and all(digest(p)==sha for p,sha in originals.items())
for row in source['instances']:row['materials']=[remap.get(p,p) for p in row['materials']]
out=e/'raven_gate_l-sampler-source-r1.json';assert not out.exists();out.write_text(json.dumps(source,indent=2)+'\n')
receipt.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),reason='Native SM5 X4510 sampler index exceeds 16',change='Shared wrap sampling in owned native material/function copies; native texture parameters and geometry preserved',changed_nodes=changed,donor_hashes=originals,material_remap=remap,owned={p:a.get_path_name() for p,a in copies.items()},status='Saved owned candidate; shader completion and rendered native comparison required'),indent=2)+'\n')
# Apply only to the unsaved inspection actors for an immediate native comparison.
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 for c in actor.get_components_by_class(unreal.StaticMeshComponent):
  for i in range(c.get_num_materials()):
   old=c.get_material(i)
   if old and old.get_path_name() in remap:c.set_material(i,unreal.load_asset(remap[old.get_path_name()]))
print('SOUL_RAVENHOLD_SAMPLER_COPY',len(changed),len(remap),str(receipt))
