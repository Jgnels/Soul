import unreal, os, json, math, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MAP_SOURCE = "/Game/Medieval_Megapack/Levels/PL_Fortress_Day"
MAP_DEST = "/Game/Soul/Maps/Cities/LV_Soul_HumanCapital_Prototype"
DATA_DIR = "/Game/Soul/Data/Settlements"
DATA_NAME = "DA_Soul_HumanCapital_Default"
SETTLEMENT = "human_capital"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

def n(s): return unreal.Name(s)

def log(msg): unreal.log("SOUL_HUMAN_CITY "+msg)

def find_anchor():
    w=unreal.EditorLevelLibrary.get_editor_world()
    try:
        starts=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerStart)
        if starts:
            return starts[0].get_actor_location(), "PlayerStart"
    except: pass
    try:
        ls=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Landscape)
        if ls:
            o,e=ls[0].get_actor_bounds(False,True)
            return o,"Landscape"
    except: pass
    return unreal.Vector(0,0,0),"Origin"

def spawn(cls,loc,label):
    a=actors.spawn_actor_from_class(cls,loc,unreal.Rotator(0,0,0))
    a.set_actor_label(label,False)
    return a

def ensure_scenario():
    path=DATA_DIR+"/"+DATA_NAME
    existing=unreal.load_asset(path)
    if existing:
        return existing
    factory=unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class",unreal.SoulSettlementScenarioData)
    asset=asset_tools.create_asset(DATA_NAME,DATA_DIR,unreal.SoulSettlementScenarioData,factory)
    if not asset:
        raise RuntimeError("Could not create settlement scenario data asset")
    asset.set_editor_property("settlement_id",n(SETTLEMENT))
    asset.set_editor_property("faction_id",n("humans"))
    asset.set_editor_property("region_id",n("human_home"))
    asset.set_editor_property("fortification_level",2)
    asset.set_editor_property("wall_integrity_permille",1000)
    ids=[
        "human.muster_yard","human.archery_range","human.guardhouse",
        "human.barracks","human.witch_collegium","human.royal_chapterhouse",
        "human.griffon_roost",
        "human.keep","human.forge","human.tavern","human.market","human.walls"
    ]
    specs=[]
    for bid in ids:
        spec=unreal.SoulInitialBuildingSpec()
        spec.set_editor_property("building_id",n(bid))
        spec.set_editor_property("level",1)
        spec.set_editor_property("integrity_permille",1000)
        spec.set_editor_property("built",True)
        specs.append(spec)
    asset.set_editor_property("buildings",specs)
    unreal.EditorAssetLibrary.save_asset(path,False)
    return asset

def level_instance_info():
    rows=[]
    try:
        cls=getattr(unreal,"LevelInstance")
        for a in unreal.GameplayStatics.get_all_actors_of_class(unreal.EditorLevelLibrary.get_editor_world(),cls):
            label=a.get_actor_label()
            row={"label":label,"name":a.get_name(),"location":[a.get_actor_location().x,a.get_actor_location().y,a.get_actor_location().z]}
            for prop in ("world_asset","world_asset_package","world_asset_package_name"):
                try:
                    v=a.get_editor_property(prop)
                    row[prop]=str(v)
                except: pass
            rows.append((a,row))
    except Exception as e:
        log("LEVEL_INSTANCE_QUERY "+repr(e))
    return rows

def bind_by_token(building_actor,instances,tokens):
    matched=[]
    for a,row in instances:
        hay=(" ".join(str(v) for v in row.values())).lower()
        if any(t.lower() in hay for t in tokens):
            matched.append(a)
    if matched:
        building_actor.set_editor_property("intact_actors",matched)
    return [a.get_actor_label() for a in matched]

def main():
    if not levels.load_level(MAP_SOURCE):
        raise RuntimeError("Failed to load donor Hivemind fortress")
    base,anchor_source=find_anchor()
    log("ANCHOR "+anchor_source+" "+str(base))
    if not unreal.EditorLevelLibrary.save_current_level_as(MAP_DEST):
        raise RuntimeError("Failed to duplicate donor map to "+MAP_DEST)
    scenario=ensure_scenario()

    controller=spawn(unreal.SoulSettlementPresentationController,base,"Soul_HumanCapital_Presentation")
    controller.set_editor_property("settlement_id",n(SETTLEMENT))

    boot=spawn(unreal.SoulSettlementBootstrapActor,base,"Soul_HumanCapital_Bootstrap")
    boot.set_editor_property("scenario",scenario)
    boot.set_editor_property("only_create_if_missing",True)

    camloc=unreal.Vector(base.x+6500,base.y-6500,base.z+4200)
    town=spawn(unreal.SoulTownViewAnchor,camloc,"Soul_HumanCapital_TownView")
    town.set_editor_property("settlement_id",n(SETTLEMENT))
    town.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camloc,unreal.Vector(base.x,base.y,base.z+500)),False)
    try:
        town.get_editor_property("camera").set_editor_property("field_of_view",42.0)
    except: pass

    layout=spawn(unreal.SoulBattlefieldLayoutActor,base,"Soul_HumanCapital_BattlefieldLayout")
    layout.set_editor_property("recipe_id",n("human.fortress_outskirts"))
    layout.set_editor_property("biome",n("temperate"))
    layout.set_editor_property("landform",n("fortified_outskirts"))
    layout.set_editor_property("strategic_feature",n("castle_edge"))
    layout.set_editor_property("hex_cell_size",300.0)
    layout.set_editor_property("board_radius",8)
    try:
        layout.get_editor_property("attacker_deployment_root").set_relative_location(unreal.Vector(-3300,0,0))
        layout.get_editor_property("defender_deployment_root").set_relative_location(unreal.Vector(3300,0,0))
        layout.get_editor_property("landmark_root").set_relative_location(unreal.Vector(0,0,0))
    except: pass

    building_ids=[
      "human.muster_yard","human.archery_range","human.guardhouse",
      "human.barracks","human.witch_collegium","human.royal_chapterhouse",
      "human.griffon_roost"
    ]
    offsets=[
      (-1800,-900,0),(-900,-1700,0),(0,-1900,0),(1000,-1500,0),
      (1750,-600,0),(1450,850,0),(350,1800,400)
    ]
    bactors={}
    for bid,(x,y,z) in zip(building_ids,offsets):
        a=spawn(unreal.SoulSettlementBuildingActor,unreal.Vector(base.x+x,base.y+y,base.z+z),"Soul_"+bid.replace(".","_"))
        a.set_editor_property("settlement_id",n(SETTLEMENT))
        a.set_editor_property("building_id",n(bid))
        bactors[bid]=a

    # Exact authored prefabs when represented as LevelInstance actors.
    instances=level_instance_info()
    bindings={}
    bindings["human.witch_collegium"]=bind_by_token(bactors["human.witch_collegium"],instances,["building_a"])
    bindings["human.archery_range"]=bind_by_token(bactors["human.archery_range"],instances,["building_c"])
    bindings["human.barracks"]=bind_by_token(bactors["human.barracks"],instances,["building_d"])
    bindings["human.royal_chapterhouse"]=bind_by_token(bactors["human.royal_chapterhouse"],instances,["building_a_02"])

    wall=spawn(unreal.SoulFortificationSegmentActor,unreal.Vector(base.x-2500,base.y,base.z),"Soul_HumanCapital_WestWall")
    wall.set_editor_property("settlement_id",n(SETTLEMENT))
    wall.set_editor_property("segment_id",n("west_wall"))
    wall.set_editor_property("breach_scar_id",n("west_wall_breach"))

    objectives=[
      ("gatehouse","human.guardhouse","west_wall_breach","OpenGate",False,(-2300,0,0)),
      ("forge","human.forge","","DisableRepair",False,(500,-700,0)),
      ("witch_collegium","human.witch_collegium","","DropWard",False,(1700,-600,0)),
      ("keep","human.keep","","CaptureSettlement",True,(0,1500,0)),
    ]
    for oid,bid,scar,effect,primary,(x,y,z) in objectives:
        a=spawn(unreal.SoulSiegeObjectiveActor,unreal.Vector(base.x+x,base.y+y,base.z+z),"Soul_Objective_"+oid)
        a.set_editor_property("settlement_id",n(SETTLEMENT))
        a.set_editor_property("objective_id",n(oid))
        a.set_editor_property("linked_building_id",n(bid))
        if scar:a.set_editor_property("linked_breach_scar_id",n(scar))
        a.set_editor_property("effect_tag",n(effect))
        a.set_editor_property("primary_victory_objective",primary)

    unreal.EditorLevelLibrary.save_current_level()
    manifest={
      "map":MAP_DEST,"anchor_source":anchor_source,
      "anchor":[base.x,base.y,base.z],
      "level_instances":[row for _,row in instances],
      "bindings":bindings,
      "buildings":building_ids,
      "objectives":[x[0] for x in objectives],
    }
    with open(os.path.join(OUT,"human_capital_prototype_manifest.json"),"w",encoding="utf-8") as f:
        json.dump(manifest,f,indent=2)
    log("DONE map="+MAP_DEST+" instances="+str(len(instances))+" bindings="+str(bindings))

try:
    main()
except Exception:
    err=traceback.format_exc()
    unreal.log_error("SOUL_HUMAN_CITY_ERROR\n"+err)
    with open(os.path.join(OUT,"human_capital_prototype_error.txt"),"w",encoding="utf-8") as f:f.write(err)
