import unreal, os, json, traceback

OUT=r"D:\RefinedBadger\Games\Soul\Evidence"
RECIPES=[
 {"id":"human.grassland_crossroads","src":"/Game/LandscapePackTwo/Maps/Grassland_01","dst":"/Game/Soul/Maps/Battlefields/BF_Human_GrasslandCrossroads","biome":"temperate","landform":"rolling_plain","feature":"crossroads"},
 {"id":"human.rolling_ridge","src":"/Game/LandscapePackTwo/Maps/Grassland_02","dst":"/Game/Soul/Maps/Battlefields/BF_Human_RollingRidge","biome":"temperate","landform":"ridge","feature":"high_ground"},
 {"id":"viking.snow_pass","src":"/Game/LandscapePackOne/Maps/SnowyMountain_01","dst":"/Game/Soul/Maps/Battlefields/BF_Viking_SnowPass","biome":"snow","landform":"mountain_pass","feature":"narrow_pass"},
 {"id":"dwarf.mountain_pass","src":"/Game/LandscapePackOne/Maps/Mountain_01","dst":"/Game/Soul/Maps/Battlefields/BF_Dwarf_MountainPass","biome":"mountain","landform":"pass","feature":"switchback"},
 {"id":"orc.badlands","src":"/Game/LandscapePackTwo/Maps/Mesa_01","dst":"/Game/Soul/Maps/Battlefields/BF_Orc_Badlands","biome":"badlands","landform":"mesa","feature":"dry_gully"},
 {"id":"dark.ash_plain","src":"/Game/LandscapePackTwo/Maps/Desert_01","dst":"/Game/Soul/Maps/Battlefields/BF_Dark_AshPlain","biome":"ash_waste","landform":"open_plain","feature":"dead_ground"},
 {"id":"neutral.coastal_ruins","src":"/Game/Elite_CoastalRuins/Maps/CoastalRuins_01","dst":"/Game/Soul/Maps/Battlefields/BF_Neutral_CoastalRuins","biome":"coast","landform":"valley_hill","feature":"ruins"},
]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def n(x):return unreal.Name(x)
def log(x):unreal.log("SOUL_BF_BUILD "+x)

def anchor():
    w=unreal.EditorLevelLibrary.get_editor_world()
    try:
        landscapes=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Landscape)
        if landscapes:
            o,e=landscapes[0].get_actor_bounds(False,True)
            return o,"Landscape"
    except:pass
    try:
        starts=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerStart)
        if starts:return starts[0].get_actor_location(),"PlayerStart"
    except:pass
    return unreal.Vector(0,0,0),"Origin"

def main():
    rows=[]
    for recipe in RECIPES:
        log("LOAD "+recipe["id"])
        ok=levels.load_level(recipe["src"])
        row=dict(recipe);row["load_ok"]=bool(ok)
        if not ok:
            rows.append(row);continue
        center,source=anchor()
        row["anchor_source"]=source;row["anchor"]=[center.x,center.y,center.z]
        if not unreal.EditorLevelLibrary.save_current_level_as(recipe["dst"]):
            row["save_ok"]=False;rows.append(row);continue
        row["save_ok"]=True
        layout=actors.spawn_actor_from_class(unreal.SoulBattlefieldLayoutActor,center,unreal.Rotator(0,0,0))
        layout.set_actor_label("Soul_BattlefieldLayout",False)
        layout.set_editor_property("recipe_id",n(recipe["id"]))
        layout.set_editor_property("biome",n(recipe["biome"]))
        layout.set_editor_property("landform",n(recipe["landform"]))
        layout.set_editor_property("strategic_feature",n(recipe["feature"]))
        layout.set_editor_property("hex_cell_size",300.0)
        layout.set_editor_property("board_radius",8)
        try:
            layout.get_editor_property("attacker_deployment_root").set_relative_location(unreal.Vector(-3300,0,0))
            layout.get_editor_property("defender_deployment_root").set_relative_location(unreal.Vector(3300,0,0))
        except:pass
        grid=actors.spawn_actor_from_class(unreal.SoulBattlefieldGridActor,center,unreal.Rotator(0,0,0))
        grid.set_actor_label("Soul_HiddenHexGrid",False)
        grid.set_editor_property("hex_cell_size",300.0)
        grid.set_editor_property("board_radius",8)
        grid.set_editor_property("trace_height",15000.0)
        grid.set_editor_property("trace_depth",30000.0)
        # Bake immediately; cells are saved with the map and can be manually overridden later.
        grid.bake_grid()
        row["baked_cells"]=len(grid.get_editor_property("baked_cells"))
        row["expected_cells"]=grid.get_expected_cell_count()
        unreal.EditorLevelLibrary.save_current_level()
        rows.append(row)
        log("DONE "+recipe["id"]+" cells="+str(row["baked_cells"]))
    with open(os.path.join(OUT,"battlefield_prototype_build_manifest.json"),"w",encoding="utf-8") as f:json.dump(rows,f,indent=2)

try:main()
except Exception:
    e=traceback.format_exc();unreal.log_error("SOUL_BF_BUILD_ERROR\n"+e);open(os.path.join(OUT,"battlefield_prototype_build_error.txt"),"w",encoding="utf-8").write(e)
