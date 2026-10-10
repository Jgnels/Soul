"""Normalize the Soul-owned woodland using the existing presentation adapter."""
import unreal as u,json,hashlib
from pathlib import Path
r=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()));s=u.get_editor_subsystem(u.EditorActorSubsystem)
p="/Game/Soul/Maps/Battles/L_Heartland_Woodland"
assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level(p)
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
source=r/"Content/Forest_village/Level/L_showcase_level.umap";before=hashlib.sha256(source.read_bytes()).hexdigest()
cls=u.load_class(None,"/Script/Soul.SoulSettlementPresentationController")
actors=[a for a in s.get_all_level_actors() if a.get_class()==cls]
controller=actors[0] if actors else s.spawn_actor_from_class(cls,u.Vector())
controller.set_actor_label("Soul Woodland Authored Exposure")
controller.set_editor_property("authored_ev100_exposure",True)
assert u.EditorLoadingAndSavingUtils.save_map(w,p)
assert hashlib.sha256(source.read_bytes()).hexdigest()==before
(r/"Evidence/HumanHeartlandDepth-20261010/Local/woodland-runtime-preparation.json").write_text(json.dumps({"map":p,"existing_exposure_adapter":True,"source_sha256":before,"source_unchanged":True},indent=2))
print("SOUL_WOODLAND_RUNTIME_PREPARED")
