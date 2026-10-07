from pathlib import Path
import datetime,hashlib,json,unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()));folder=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006/PopulationSources';lib=unreal.MaterialEditingLibrary;a=unreal.EditorAssetLibrary
prior=json.loads((folder/'ravenhold-sampler-copy-r1.json').read_text());owned={p:unreal.load_asset(q) for p,q in prior['owned'].items()};hashes={};remap={}
def disk(p):return root/'Content'/(p.split('.')[0].removeprefix('/Game/')+'.uasset')
def sha(p):return hashlib.file_digest(disk(p).open('rb'),'sha256').hexdigest()
def parent_copy(obj):
 path=obj.get_path_name().split('.')[0]
 if path in owned:return owned[path]
 if not isinstance(obj,unreal.MaterialInstanceConstant):return None
 parent=parent_copy(obj.get_editor_property('parent'))
 if not parent:return None
 dest='/Game/Soul/Materials/Settlements/Ravenhold/'+obj.get_name()+'_SM5_r1';assert not a.does_asset_exist(dest)
 hashes[path]=sha(path);copy=a.duplicate_asset(path,dest);assert copy
 lib.set_material_instance_parent(copy,parent);lib.update_material_instance(copy);assert a.save_loaded_asset(copy,False);owned[path]=copy;return copy
source='/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r1';dest='/Game/Soul/CampaignProxies/Population/SM_Ravenhold_CurtainWall_r2';assert not a.does_asset_exist(dest);mesh=a.duplicate_asset(source,dest);assert mesh
for i,slot in enumerate(mesh.get_editor_property('static_materials')):
 m=slot.material_interface
 if m:
  replacement=parent_copy(m)
  if replacement:mesh.set_material(i,replacement);remap[m.get_path_name()]=replacement.get_path_name()
assert a.save_loaded_asset(mesh,False)
assert all(sha(p)==h for p,h in hashes.items())
(folder/'ravenhold-wall-sampler-copy-r2.json').write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),source=source,destination=dest,geometry_changes=0,triangles=mesh.get_num_triangles(0),source_hashes=hashes,material_remap=remap,owned_sha256=sha(dest),reason='New native wall material instances use the same existing SM5-safe owned master; source parameters and textures unchanged'),indent=2)+'\n')
print('SOUL_NATIVE_WALL_SM5_COPY',len(hashes),len(remap),dest)
