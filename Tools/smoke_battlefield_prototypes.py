import unreal, json, os, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MAPS = [
    ("human.grassland_crossroads", "/Game/Soul/Maps/Battlefields/BF_Human_GrasslandCrossroads"),
    ("human.rolling_ridge", "/Game/Soul/Maps/Battlefields/BF_Human_RollingRidge"),
    ("viking.snow_pass", "/Game/Soul/Maps/Battlefields/BF_Viking_SnowPass"),
    ("dwarf.mountain_pass", "/Game/Soul/Maps/Battlefields/BF_Dwarf_MountainPass"),
    ("orc.badlands", "/Game/Soul/Maps/Battlefields/BF_Orc_Badlands"),
    ("dark.ash_plain", "/Game/Soul/Maps/Battlefields/BF_Dark_AshPlain"),
    ("neutral.coastal_ruins", "/Game/Soul/Maps/Battlefields/BF_Neutral_CoastalRuins"),
]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
rows = []
try:
    for recipe_id, path in MAPS:
        unreal.log("SOUL_BF_SMOKE START " + recipe_id)
        ok = bool(levels.load_level(path))
        row = {"id": recipe_id, "map": path, "load_ok": ok}
        if ok:
            world = unreal.EditorLevelLibrary.get_editor_world()
            grids = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoulBattlefieldGridActor)
            layouts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SoulBattlefieldLayoutActor)
            row.update(grid_count=len(grids), layout_count=len(layouts))
            if len(grids) == 1:
                row["baked_cells"] = len(grids[0].get_editor_property("baked_cells"))
                row["expected_cells"] = grids[0].get_expected_cell_count()
        rows.append(row)
        unreal.log("SOUL_BF_SMOKE DONE " + recipe_id + " " + json.dumps(row, sort_keys=True))
    with open(os.path.join(OUT, "battlefield_smoke_manifest.json"), "w", encoding="utf-8") as f:
        json.dump(rows, f, indent=2)
except Exception:
    error = traceback.format_exc()
    unreal.log_error("SOUL_BF_SMOKE_ERROR\n" + error)
    open(os.path.join(OUT, "battlefield_smoke_error.txt"), "w", encoding="utf-8").write(error)
