import unreal, json, traceback

OUT=r"D:\RefinedBadger\Games\Soul\Evidence\battlefield_persistence_inspect.json"
MAP="/Game/Soul/Maps/Battlefields/BF_Human_GrasslandCrossroads"
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal_editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
row={"map":MAP}

try:
    row["load_ok"]=bool(levels.load_level(MAP))
    world=unreal_editor.get_editor_world()
    row["world_name"]=world.get_name() if world else None
    row["world_path"]=world.get_path_name() if world else None
    all_actors=actors.get_all_level_actors() if world else []
    row["actor_count"]=len(all_actors)
    row["soul_actors"]=[
        {"label":a.get_actor_label(),"name":a.get_name(),"class":a.get_class().get_name(),"path":a.get_path_name()}
        for a in all_actors
        if "Soul" in a.get_actor_label() or "Soul" in a.get_class().get_name()
    ]
    row["current_level"]=str(levels.get_current_level())
    try:
        row["world_partition"]=str(world.get_editor_property("world_partition"))
    except Exception:
        row["world_partition"]=None
except Exception:
    row["error"]=traceback.format_exc()
    unreal.log_error("SOUL_BF_INSPECT_ERROR\n"+row["error"])

with open(OUT,"w",encoding="utf-8") as f:
    json.dump(row,f,indent=2)
unreal.log("SOUL_BF_INSPECT "+json.dumps(row,sort_keys=True))
