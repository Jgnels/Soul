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

def n(x): return unreal.Name(x)
def log(x): unreal.log("SOUL_BF_BUILD "+x)

def anchor():
    w=unreal.EditorLevelLibrary.get_editor_world()
    try:
        landscapes=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Landscape)
        if landscapes:
            o,_=landscapes[0].get_actor_bounds(False,True)
            return o,"Landscape"
    except: pass
    try:
        starts=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerStart)
        if starts: return starts[0].get_actor_location(),"PlayerStart"
    except: pass
    return unreal.Vector(0,0,0),"Origin"

def current_soul_actors():
    rows=actors.get_all_level_actors()
    layouts=[a for a in rows if a.get_class().get_name()=="SoulBattlefieldLayoutActor"]
    grids=[a for a in rows if a.get_class().get_name()=="SoulBattlefieldGridActor"]
    return layouts,grids

def configure_layout(layout,recipe):
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
    except: pass

def configure_grid(grid):
    grid.set_actor_label("Soul_HiddenHexGrid",False)
    grid.set_editor_property("hex_cell_size",300.0)
    grid.set_editor_property("board_radius",8)
    grid.set_editor_property("trace_height",15000.0)
    grid.set_editor_property("trace_depth",30000.0)

def main():
    rows=[]
    for recipe in RECIPES:
        row=dict(recipe)
        try:
            exists=unreal.EditorAssetLibrary.does_asset_exist(recipe["dst"])
            row["destination_preexisted"]=bool(exists)
            if exists:
                log("REPAIR_LOAD "+recipe["id"])
                ok=levels.load_level(recipe["dst"])
                row["mode"]="repair_existing_destination"
            else:
                log("DONOR_LOAD "+recipe["id"])
                ok=levels.load_level(recipe["src"])
                row["mode"]="create_from_donor"
                if ok:
                    world=unreal.EditorLevelLibrary.get_editor_world()
                    row["initial_snapshot_save_ok"]=bool(unreal.EditorLoadingAndSavingUtils.save_map(world,recipe["dst"]))
                    ok=row["initial_snapshot_save_ok"] and bool(levels.load_level(recipe["dst"]))
            row["load_ok"]=bool(ok)
            if not ok:
                rows.append(row)
                continue

            center,source=anchor()
            row["anchor_source"]=source
            row["anchor"]=[center.x,center.y,center.z]

            layouts,grids=current_soul_actors()
            row["pre_layout_count"]=len(layouts)
            row["pre_grid_count"]=len(grids)
            if len(layouts)>1 or len(grids)>1:
                raise RuntimeError("Duplicate Soul battlefield actors already exist in "+recipe["dst"])

            changed=False
            if layouts:
                layout=layouts[0]
            else:
                layout=actors.spawn_actor_from_class(unreal.SoulBattlefieldLayoutActor,center,unreal.Rotator(0,0,0))
                if not layout: raise RuntimeError("Could not spawn layout actor")
                changed=True
            configure_layout(layout,recipe)

            if grids:
                grid=grids[0]
            else:
                grid=actors.spawn_actor_from_class(unreal.SoulBattlefieldGridActor,center,unreal.Rotator(0,0,0))
                if not grid: raise RuntimeError("Could not spawn grid actor")
                changed=True
            configure_grid(grid)

            expected=int(grid.get_expected_cell_count())
            before=len(grid.get_editor_property("baked_cells"))
            row["baked_cells_before"]=before
            row["expected_cells"]=expected
            if before!=expected:
                grid.bake_grid()
                changed=True
            row["baked_cells_after"]=len(grid.get_editor_property("baked_cells"))
            row["changed"]=changed

            if changed:
                row["save_after_actor_update_ok"]=bool(unreal.EditorLoadingAndSavingUtils.save_current_level())
                if not row["save_after_actor_update_ok"]:
                    raise RuntimeError("save_current_level failed for "+recipe["dst"])
            else:
                row["save_after_actor_update_ok"]=True

            # Acceptance: reload the actual saved destination and inspect it again.
            row["reload_ok"]=bool(levels.load_level(recipe["dst"]))
            if not row["reload_ok"]:
                raise RuntimeError("Could not reload saved destination "+recipe["dst"])
            post_layouts,post_grids=current_soul_actors()
            row["post_layout_count"]=len(post_layouts)
            row["post_grid_count"]=len(post_grids)
            if len(post_grids)==1:
                row["post_baked_cells"]=len(post_grids[0].get_editor_property("baked_cells"))
                row["post_expected_cells"]=int(post_grids[0].get_expected_cell_count())
            if row["post_layout_count"]!=1 or row["post_grid_count"]!=1:
                raise RuntimeError("Saved destination failed actor persistence verification")
            if row.get("post_baked_cells")!=row.get("post_expected_cells"):
                raise RuntimeError("Saved destination failed baked-cell verification")

            log("DONE "+recipe["id"]+" cells="+str(row["post_baked_cells"])+" mode="+row["mode"])
        except Exception:
            row["error"]=traceback.format_exc()
            unreal.log_error("SOUL_BF_BUILD_RECIPE_ERROR "+recipe["id"]+"\n"+row["error"])
        rows.append(row)

    with open(os.path.join(OUT,"battlefield_prototype_build_manifest.json"),"w",encoding="utf-8") as f:
        json.dump(rows,f,indent=2)

    errors=[r for r in rows if r.get("error") or not r.get("load_ok")]
    if errors:
        raise RuntimeError(str(len(errors))+" battlefield recipe(s) failed; see manifest")

try:
    main()
except Exception:
    e=traceback.format_exc()
    unreal.log_error("SOUL_BF_BUILD_ERROR\n"+e)
    with open(os.path.join(OUT,"battlefield_prototype_build_error.txt"),"w",encoding="utf-8") as f:
        f.write(e)
    raise
