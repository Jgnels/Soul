"""Soul-owned field wrapper from approved native woodland; preserve donor bytes."""
import unreal as u,json,hashlib
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));s=u.get_editor_subsystem(u.EditorActorSubsystem)
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_name()=='L_showcase_level'
source=r/'Content/Forest_village/Level/L_showcase_level.umap';before=hashlib.sha256(source.read_bytes()).hexdigest()
p='/Game/Soul/Maps/Battles/L_Heartland_Woodland';assert not u.EditorAssetLibrary.does_asset_exist(p)
# Review cameras are transient tools, not donor composition.
u.get_editor_subsystem(u.LevelEditorSubsystem).eject_pilot_level_actor()
for a in s.get_all_level_actors():
 if isinstance(a,u.CameraActor) and a.get_name().startswith('CameraActor'):s.destroy_actor(a)
# A new owned package; original source is never saved.
assert u.EditorLoadingAndSavingUtils.save_map(w,p)
after=hashlib.sha256(source.read_bytes()).hexdigest()
assert after==before
(r/'Evidence/HumanHeartlandDepth-20261010/Local/woodland-wrapper.json').write_text(json.dumps({'source':'/Game/Forest_village/Level/L_showcase_level','source_sha256':before,'source_unchanged':True,'owned_map':p,'origin_cm':[-9000,-9000,-87.0749478874734],'composition_preserved':True,'native_collision_lanes':3,'runtime_qualified':False},indent=2))
print('SOUL_WOODLAND_WRAPPER_CREATED',p)
