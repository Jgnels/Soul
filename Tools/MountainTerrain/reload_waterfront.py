import unreal as u
from pathlib import Path
s=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert s.save_current_level()
assert s.load_level('/Game/SoulCampaignMountain/L_evil_waterfront')
exec(Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/Scripts/verify_waterfront_collision.py').read_text())
print('WATERFRONT_RELOAD_COLLISION_PASS')
u.SystemLibrary.quit_editor()
