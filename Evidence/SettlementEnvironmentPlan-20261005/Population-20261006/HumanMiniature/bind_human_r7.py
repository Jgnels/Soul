from pathlib import Path
import json,hashlib,datetime,shutil,unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
e=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006';p=root/'Data/SettlementEnvironments/CrownsteadDevelopmentProof.json'
recipe=json.loads(p.read_text());fit=json.loads((e/'HumanMiniature/human-retained-fit-r7.json').read_text())
asset=unreal.load_asset(recipe['asset_path']);assert asset
before=(str(asset.get_editor_property('buildings')),str(asset.get_editor_property('development_definitions')))
disk=root/'Content/Soul/Data/Settlements/DA_Soul_HumanCapital_DevelopmentProof.uasset';old=hashlib.sha256(disk.read_bytes()).hexdigest()
backup=e.parent/'Local/HumanCapital_DevelopmentProof_before_r7.uasset';assert not backup.exists();shutil.copy2(disk,backup)
for field,part in [('miniature_base_mesh','Base'),('miniature_upgrade_mesh','Upgrade')]:
 path='/Game/Soul/CampaignProxies/Human/SM_HumanCapital_'+part+'_r7';mesh=unreal.load_asset(path);assert mesh
 asset.set_editor_property(field,mesh);recipe[field]=path
for field in ['miniature_scale','miniature_translation','miniature_yaw']:recipe[field]=fit[field]
t=unreal.Transform();t.translation=unreal.Vector(*fit['miniature_translation']);t.scale3d=unreal.Vector(*([fit['miniature_scale']]*3));t.rotation=unreal.Rotator(yaw=fit['miniature_yaw']).quaternion();asset.set_editor_property('miniature_transform',t)
assert before==(str(asset.get_editor_property('buildings')),str(asset.get_editor_property('development_definitions')))
assert unreal.EditorAssetLibrary.save_loaded_asset(asset,False)
recipe['miniature_quality_status']='R7 campaign readability candidate: native central keep and thirty authored blocks at uniform 0.24; detached island gate omitted, exact native tavern shared. Each foundation refit to retained Landscape. Full city unchanged. Actual campaign render and miniature-state review pending.'
recipe['development_definitions'][0]['campaign_miniature_group']='SM_HumanCapital_Upgrade_r7: same native tavern, same human.tavern state'
p.write_text(json.dumps(recipe,indent=2)+'\n')
(e/'human-miniature-r7-binding.json').write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),asset=recipe['asset_path'],before_sha256=old,after_sha256=hashlib.sha256(disk.read_bytes()).hexdigest(),definitions_unchanged=True,fit=fit,qualifier='Inspection candidate; campaign-render gates still pending'),indent=2)+'\n')
print('HUMAN_R7_PRESENTATION_BOUND',recipe['miniature_base_mesh'])
