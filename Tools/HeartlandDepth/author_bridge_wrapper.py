"""A separate battle presentation of the existing Human crossing; never save campaign map."""
import unreal as u,json,hashlib
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));s=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
source='/Game/SoulCampaignComposition/L_Composition_3500_r2';dest='/Game/Soul/Maps/Battles/L_Heartland_RiverBridge'
assert w.get_outermost().get_name()==source and not u.EditorAssetLibrary.does_asset_exist(dest)
f=r/'Content/SoulCampaignComposition/L_Composition_3500_r2.umap';before=hashlib.sha256(f.read_bytes()).hexdigest()
u.get_editor_subsystem(u.LevelEditorSubsystem).eject_pilot_level_actor()
for a in s.get_all_level_actors():
 if isinstance(a,u.CameraActor):s.destroy_actor(a)
assert u.EditorLoadingAndSavingUtils.save_map(w,dest)
assert hashlib.sha256(f.read_bytes()).hexdigest()==before
(r/'Evidence/HumanHeartlandDepth-20261010/Local/river-bridge-wrapper.json').write_text(json.dumps({'source':source,'source_sha256':before,'source_unchanged':True,'owned_map':dest,'origin_cm':[-110853.5755,4792.0679,150],'terrain_transformed':False,'runtime_qualified':False,'note':'Separate battle presentation copy. Deck collision and water movement constraints must qualify before admission.'},indent=2))
print('SOUL_BRIDGE_WRAPPER_CREATED',dest)
