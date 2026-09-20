import unreal, os, json, traceback

OUT=r"D:\RefinedBadger\Games\Soul\Evidence"
MAP_SOURCE="/Game/JustBStudios/Water_City/Levels/LV_WaterVillage"
MAP_DEST="/Game/Soul/Maps/Cities/LV_Soul_VikingHarbour_Prototype"
DATA_DIR="/Game/Soul/Data/Settlements"
DATA_NAME="DA_Soul_VikingHarbour_Default"
SETTLEMENT="viking_harbour"

actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
tools=unreal.AssetToolsHelpers.get_asset_tools()
def n(x):return unreal.Name(x)
def log(x):unreal.log("SOUL_VIKING_CITY "+x)

def bp(path):
    try:return unreal.EditorAssetLibrary.load_blueprint_class(path)
    except:return None

def anchor():
    w=unreal.EditorLevelLibrary.get_editor_world()
    try:
        starts=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerStart)
        if starts:return starts[0].get_actor_location(),"PlayerStart"
    except:pass
    return unreal.Vector(0,0,0),"Origin"

def spawn(cls,loc,label,rot=None):
    a=actors.spawn_actor_from_class(cls,loc,rot or unreal.Rotator(0,0,0))
    a.set_actor_label(label,False)
    return a

def scenario():
    path=DATA_DIR+"/"+DATA_NAME
    a=unreal.load_asset(path)
    if a:return a
    fac=unreal.DataAssetFactory();fac.set_editor_property("data_asset_class",unreal.SoulSettlementScenarioData)
    a=tools.create_asset(DATA_NAME,DATA_DIR,unreal.SoulSettlementScenarioData,fac)
    a.set_editor_property("settlement_id",n(SETTLEMENT));a.set_editor_property("faction_id",n("vikings"));a.set_editor_property("region_id",n("viking_home"))
    a.set_editor_property("fortification_level",1);a.set_editor_property("wall_integrity_permille",1000)
    ids=["viking.raider_longhouse","viking.hunter_range","viking.shield_hall","viking.berserker_mead_hall","viking.shaman_lodge","viking.huscarl_hall","viking.wolf_kennels","viking.great_hall","viking.shipyard","viking.smithy","viking.market","viking.watch"]
    specs=[]
    for bid in ids:
        s=unreal.SoulInitialBuildingSpec();s.set_editor_property("building_id",n(bid));s.set_editor_property("level",1);s.set_editor_property("integrity_permille",1000);s.set_editor_property("built",True);specs.append(s)
    a.set_editor_property("buildings",specs);unreal.EditorAssetLibrary.save_asset(path,False);return a

def main():
    if not levels.load_level(MAP_SOURCE):raise RuntimeError("Water City map failed to load")
    base,src=anchor()
    if not unreal.EditorLevelLibrary.save_current_level_as(MAP_DEST):raise RuntimeError("Could not save Viking prototype map")
    sc=scenario()
    c=spawn(unreal.SoulSettlementPresentationController,base,"Soul_VikingHarbour_Presentation");c.set_editor_property("settlement_id",n(SETTLEMENT))
    b=spawn(unreal.SoulSettlementBootstrapActor,base,"Soul_VikingHarbour_Bootstrap");b.set_editor_property("scenario",sc);b.set_editor_property("only_create_if_missing",True)
    cam=unreal.Vector(base.x+6000,base.y-7000,base.z+3800)
    tv=spawn(unreal.SoulTownViewAnchor,cam,"Soul_VikingHarbour_TownView");tv.set_editor_property("settlement_id",n(SETTLEMENT));tv.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(cam,unreal.Vector(base.x,base.y,base.z+400)),False)
    layout=spawn(unreal.SoulBattlefieldLayoutActor,base,"Soul_VikingHarbour_BattlefieldLayout");layout.set_editor_property("recipe_id",n("viking.harbour_edge"));layout.set_editor_property("biome",n("cold_coast"));layout.set_editor_property("landform",n("cliff_harbour"));layout.set_editor_property("strategic_feature",n("shore_bridge"));layout.set_editor_property("hex_cell_size",300.0);layout.set_editor_property("board_radius",8)

    visible={
      "viking.great_hall":("/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Big_house_05",(1200,1400,500)),
      "viking.shield_hall":("/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Big_house_03",(1700,300,150)),
      "viking.berserker_mead_hall":("/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Big_house_01",(900,-900,100)),
      "viking.shaman_lodge":("/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Small_House_V04",(-1500,1200,100)),
      "viking.huscarl_hall":("/Game/Viking_Village/Meshes/ModularBuildings/BPs/BP_HouseBuilding_003",(-500,500,100)),
      "viking.raider_longhouse":("/Game/Viking_Village/Meshes/ModularBuildings/BPs/BP_HouseBuilding_002",(-1200,-300,50)),
    }
    building_actors={}
    for bid,(asset,(x,y,z)) in visible.items():
        logic=spawn(unreal.SoulSettlementBuildingActor,unreal.Vector(base.x+x,base.y+y,base.z+z),"Soul_"+bid.replace(".","_"));logic.set_editor_property("settlement_id",n(SETTLEMENT));logic.set_editor_property("building_id",n(bid));building_actors[bid]=logic
        cls=bp(asset)
        if cls:
            geom=spawn(cls,unreal.Vector(base.x+x,base.y+y,base.z+z),"SoulGeom_"+bid.replace(".","_"))
            logic.set_editor_property("intact_actors",[geom])

    # Remaining open-yard recruitment structures.
    for bid,(x,y,z) in {
      "viking.hunter_range":(-1700,-1300,0),"viking.wolf_kennels":(2000,-1200,0)
    }.items():
        logic=spawn(unreal.SoulSettlementBuildingActor,unreal.Vector(base.x+x,base.y+y,base.z+z),"Soul_"+bid.replace(".","_"));logic.set_editor_property("settlement_id",n(SETTLEMENT));logic.set_editor_property("building_id",n(bid));building_actors[bid]=logic

    # Function-readable props.
    target_cls=bp("/Game/Viking_Village/Meshes/Props/BPs/BP_StrawArcheryTarget_01a")
    if target_cls:
        for dy in (-250,0,250):spawn(target_cls,unreal.Vector(base.x-2000,base.y-1300+dy,base.z),"SoulGeom_HunterTarget")
    boat_cls=bp("/Game/Viking_Village/Meshes/Props/BPs/BP_VikingBoat_01a")
    if boat_cls:spawn(boat_cls,unreal.Vector(base.x-800,base.y-2600,base.z),"SoulGeom_HarbourBoat")

    wall=spawn(unreal.SoulFortificationSegmentActor,unreal.Vector(base.x-2400,base.y,base.z),"Soul_VikingHarbour_BridgeWatch");wall.set_editor_property("settlement_id",n(SETTLEMENT));wall.set_editor_property("segment_id",n("bridge_watch"));wall.set_editor_property("breach_scar_id",n("bridge_watch_broken"))

    objs=[
      ("bridge_watch","viking.watch","bridge_watch_broken","OpenUpperRoute",False,(-2200,0,0)),
      ("shipyard","viking.shipyard","","DisableHarbourSupport",False,(-700,-2300,0)),
      ("shaman_lodge","viking.shaman_lodge","","DropWard",False,(-1500,1200,100)),
      ("great_hall","viking.great_hall","","CaptureSettlement",True,(1200,1400,500))]
    for oid,bid,scar,effect,primary,(x,y,z) in objs:
        a=spawn(unreal.SoulSiegeObjectiveActor,unreal.Vector(base.x+x,base.y+y,base.z+z),"Soul_Objective_"+oid);a.set_editor_property("settlement_id",n(SETTLEMENT));a.set_editor_property("objective_id",n(oid));a.set_editor_property("linked_building_id",n(bid));a.set_editor_property("effect_tag",n(effect));a.set_editor_property("primary_victory_objective",primary)
        if scar:a.set_editor_property("linked_breach_scar_id",n(scar))
    unreal.EditorLevelLibrary.save_current_level()
    with open(os.path.join(OUT,"viking_harbour_prototype_manifest.json"),"w",encoding="utf-8") as f:json.dump({"map":MAP_DEST,"anchor_source":src,"anchor":[base.x,base.y,base.z],"visible_buildings":list(visible),"objectives":[x[0] for x in objs]},f,indent=2)
    log("DONE "+MAP_DEST)

try:main()
except Exception:
    e=traceback.format_exc();unreal.log_error("SOUL_VIKING_CITY_ERROR\n"+e);open(os.path.join(OUT,"viking_harbour_prototype_error.txt"),"w",encoding="utf-8").write(e)
