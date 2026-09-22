import unreal

MAP_PATH = "/Game/Soul/Maps/Playtest/LV_Soul_RealtimeBattleArena"
MODE_PATH = "/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode"

if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
else:
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)

if not world:
    raise RuntimeError("Could not create or load realtime battle arena map")

mode = unreal.load_class(None, MODE_PATH)
if not mode:
    raise RuntimeError("SoulRealtimeArenaGameMode class not found; build editor target first")

world.get_world_settings().set_editor_property("default_game_mode", mode)

if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
    raise RuntimeError("Realtime battle arena map save failed")

print("SOUL_RT_ARENA_MAP_PASS:", MAP_PATH, MODE_PATH)
