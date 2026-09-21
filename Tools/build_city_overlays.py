import unreal, os, json, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
OVERLAY_MANIFEST = os.path.join(OUT, "city_overlay_build_manifest.json")

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def n(x): return unreal.Name(x)

def log(msg): unreal.log("SOUL_CITY_OVERLAY " + msg)

def v(xyz):
    return unreal.Vector(float(xyz[0]), float(xyz[1]), float(xyz[2]))

def add(a, delta):
    return unreal.Vector(a.x + delta[0], a.y + delta[1], a.z + delta[2])

def spawn(cls, loc, label, rot=None):
    actor = actors.spawn_actor_from_class(cls, loc, rot or unreal.Rotator(0, 0, 0))
    if not actor:
        raise RuntimeError("Could not spawn " + label)
    actor.set_actor_label(label, False)
    return actor

def add_donor_stream(donor):
    world = unreal.EditorLevelLibrary.get_editor_world()
    streaming = unreal.EditorLevelUtils.add_level_to_world(world, donor, unreal.LevelStreamingAlwaysLoaded)
    if not streaming:
        raise RuntimeError("Could not add donor streaming level " + donor)
    return streaming

def load_required(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Missing required asset " + path)
    return asset

def spawn_common(config):
    base = v(config["anchor"])
    scenario = load_required(config["scenario"])

    controller = spawn(unreal.SoulSettlementPresentationController, base, "Soul_" + config["slug"] + "_Presentation")
    controller.set_editor_property("settlement_id", n(config["settlement"]))

    boot = spawn(unreal.SoulSettlementBootstrapActor, base, "Soul_" + config["slug"] + "_Bootstrap")
    boot.set_editor_property("scenario", scenario)
    boot.set_editor_property("only_create_if_missing", True)

    camloc = add(base, config["camera_offset"])
    town = spawn(unreal.SoulTownViewAnchor, camloc, "Soul_" + config["slug"] + "_TownView")
    town.set_editor_property("settlement_id", n(config["settlement"]))
    town.set_actor_rotation(
        unreal.MathLibrary.find_look_at_rotation(camloc, add(base, config.get("camera_target_offset", (0,0,400)))),
        False,
    )
    try:
        town.get_editor_property("camera").set_editor_property("field_of_view", 42.0)
    except Exception:
        pass

    layout = spawn(unreal.SoulBattlefieldLayoutActor, base, "Soul_" + config["slug"] + "_BattlefieldLayout")
    layout.set_editor_property("recipe_id", n(config["battlefield"]["recipe_id"]))
    layout.set_editor_property("biome", n(config["battlefield"]["biome"]))
    layout.set_editor_property("landform", n(config["battlefield"]["landform"]))
    layout.set_editor_property("strategic_feature", n(config["battlefield"]["strategic_feature"]))
    layout.set_editor_property("hex_cell_size", 300.0)
    layout.set_editor_property("board_radius", 8)

    building_actors = {}
    for building in config["buildings"]:
        loc = v(building["location"]) if "location" in building else add(base, building["offset"])
        logic = spawn(
            unreal.SoulSettlementBuildingActor,
            loc,
            "Soul_" + building["id"].replace(".", "_"),
        )
        logic.set_editor_property("settlement_id", n(config["settlement"]))
        logic.set_editor_property("building_id", n(building["id"]))
        building_actors[building["id"]] = logic

        geom_path = building.get("geometry_blueprint")
        if geom_path:
            cls = unreal.EditorAssetLibrary.load_blueprint_class(geom_path)
            if cls:
                geom = spawn(cls, loc, "SoulGeom_" + building["id"].replace(".", "_"))
                logic.set_editor_property("intact_actors", [geom])
            else:
                log("GEOMETRY_MISSING " + building["id"] + " " + geom_path)

    wall_cfg = config["wall"]
    wall = spawn(unreal.SoulFortificationSegmentActor, add(base, wall_cfg["offset"]), "Soul_" + config["slug"] + "_" + wall_cfg["segment_id"])
    wall.set_editor_property("settlement_id", n(config["settlement"]))
    wall.set_editor_property("segment_id", n(wall_cfg["segment_id"]))
    wall.set_editor_property("breach_scar_id", n(wall_cfg["breach_scar_id"]))

    for obj in config["objectives"]:
        goal = spawn(unreal.SoulSiegeObjectiveActor, add(base, obj["offset"]), "Soul_Objective_" + obj["id"])
        goal.set_editor_property("settlement_id", n(config["settlement"]))
        goal.set_editor_property("objective_id", n(obj["id"]))
        goal.set_editor_property("linked_building_id", n(obj.get("building_id", "")))
        if obj.get("scar_id"):
            goal.set_editor_property("linked_breach_scar_id", n(obj["scar_id"]))
        goal.set_editor_property("effect_tag", n(obj["effect"]))
        goal.set_editor_property("primary_victory_objective", bool(obj.get("primary", False)))

    return {
        "building_count": len(building_actors),
        "building_ids": list(building_actors),
        "objective_count": len(config["objectives"]),
    }

def build_one(config):
    log("START " + config["id"])
    if unreal.EditorAssetLibrary.does_asset_exist(config["map"]):
        if not unreal.EditorAssetLibrary.delete_asset(config["map"]):
            raise RuntimeError("Could not replace existing overlay level " + config["map"])
    if not levels.new_level(config["map"]):
        raise RuntimeError("Could not create overlay level " + config["map"])

    # Spawn Soul-owned actors while the overlay is still the current persistent
    # level. Adding a streaming donor can switch the editor current level.
    common = spawn_common(config)

    # Viking cultural dressing remains Soul-owned overlay content while the
    # Water City geography is a read-only streamed donor.
    for extra in config.get("decorations", []):
        cls = unreal.EditorAssetLibrary.load_blueprint_class(extra["blueprint"])
        if cls:
            spawn(cls, add(v(config["anchor"]), extra["offset"]), extra["label"])
        else:
            log("DECORATION_MISSING " + extra["blueprint"])

    overlay_world = unreal.EditorLevelLibrary.get_editor_world()
    streaming = add_donor_stream(config["donor"])
    if not unreal.EditorLoadingAndSavingUtils.save_map(overlay_world, config["map"]):
        raise RuntimeError("Could not save overlay map " + config["map"])

    streaming_world = ""
    try:
        streaming_world = str(streaming.get_editor_property("world_asset"))
    except Exception:
        try:
            streaming_world = str(streaming.get_world_asset_package_name())
        except Exception:
            pass

    package_file = unreal.SystemLibrary.get_project_content_directory()
    disk_rel = config["map"].replace("/Game/", "").replace("/", os.sep) + ".umap"
    disk_path = os.path.join(package_file, disk_rel)
    size = os.path.getsize(disk_path) if os.path.exists(disk_path) else None
    row = {
        "id": config["id"],
        "map": config["map"],
        "donor": config["donor"],
        "streaming_class": str(streaming.get_class().get_name()),
        "streaming_world": streaming_world,
        "saved": True,
        "disk_bytes": size,
        **common,
    }
    log("DONE " + json.dumps(row, sort_keys=True))
    return row

HUMAN_ANCHOR = (-0.985946212142153, 0.0, -770.390625)
VIKING_ANCHOR = (-246.96229749141216, -664.8880967846303, 226.4148006698507)

CONFIGS = [
    {
        "id": "human_capital_overlay",
        "slug": "HumanCapital",
        "settlement": "human_capital",
        "map": "/Game/Soul/Maps/Cities/LV_Soul_HumanCapital_Overlay",
        "donor": "/Game/Medieval_Megapack/Levels/PL_Fortress_Day",
        "scenario": "/Game/Soul/Data/Settlements/DA_Soul_HumanCapital_Default",
        "anchor": HUMAN_ANCHOR,
        "camera_offset": (6500, -6500, 4200),
        "battlefield": {
            "recipe_id": "human.fortress_outskirts",
            "biome": "temperate",
            "landform": "fortified_outskirts",
            "strategic_feature": "castle_edge",
        },
        "buildings": [
            {"id":"human.muster_yard", "offset":(-1800,-900,0)},
            {"id":"human.archery_range", "location":(-5412.919818180731,-4723.643935318234,406.16905134258775)},
            {"id":"human.guardhouse", "offset":(0,-1900,0)},
            {"id":"human.barracks", "location":(-2754.7141016796368,1638.6544158710872,382.1965733019671)},
            {"id":"human.witch_collegium", "location":(-1492.0026177228206,-2321.8399605942423,270.89971984956884)},
            {"id":"human.royal_chapterhouse", "offset":(1450,850,0)},
            {"id":"human.griffon_roost", "offset":(350,1800,400)},
            {"id":"human.keep", "offset":(0,1500,0)},
            {"id":"human.forge", "location":(-3788.365588539835,-1079.8040315995174,217.36980408686662)},
            {"id":"human.tavern", "location":(-4294.624327,-3196.921604,67.198978)},
            {"id":"human.market", "offset":(900,-250,0)},
            {"id":"human.walls", "offset":(-2500,0,0)},
        ],
        "wall": {"segment_id":"WestWall", "breach_scar_id":"west_wall_breach", "offset":(-2500,0,0)},
        "objectives": [
            {"id":"gatehouse","building_id":"human.guardhouse","scar_id":"west_wall_breach","effect":"OpenGate","offset":(-2300,0,0)},
            {"id":"forge","building_id":"human.forge","effect":"DisableRepair","offset":(500,-700,0)},
            {"id":"witch_collegium","building_id":"human.witch_collegium","effect":"DropWard","offset":(1700,-600,0)},
            {"id":"keep","building_id":"human.keep","effect":"CaptureSettlement","primary":True,"offset":(0,1500,0)},
        ],
    },
    {
        "id": "viking_harbour_overlay",
        "slug": "VikingHarbour",
        "settlement": "viking_harbour",
        "map": "/Game/Soul/Maps/Cities/LV_Soul_VikingHarbour_Overlay",
        "donor": "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage",
        "scenario": "/Game/Soul/Data/Settlements/DA_Soul_VikingHarbour_Default",
        "anchor": VIKING_ANCHOR,
        "camera_offset": (6000,-7000,3800),
        "battlefield": {
            "recipe_id":"viking.harbour_edge",
            "biome":"cold_coast",
            "landform":"cliff_harbour",
            "strategic_feature":"shore_bridge",
        },
        "buildings": [
            {"id":"viking.great_hall","offset":(1200,1400,500),"geometry_blueprint":"/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Big_house_05"},
            {"id":"viking.shield_hall","offset":(1700,300,150),"geometry_blueprint":"/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Big_house_03"},
            {"id":"viking.berserker_mead_hall","offset":(900,-900,100),"geometry_blueprint":"/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Big_house_01"},
            {"id":"viking.shaman_lodge","offset":(-1500,1200,100),"geometry_blueprint":"/Game/JustBStudios/Water_City/Blueprints/Houses/BP_Small_House_V04"},
            {"id":"viking.huscarl_hall","offset":(-500,500,100),"geometry_blueprint":"/Game/Viking_Village/Meshes/ModularBuildings/BPs/BP_HouseBuilding_003"},
            {"id":"viking.raider_longhouse","offset":(-1200,-300,50),"geometry_blueprint":"/Game/Viking_Village/Meshes/ModularBuildings/BPs/BP_HouseBuilding_002"},
            {"id":"viking.hunter_range","offset":(-1700,-1300,0)},
            {"id":"viking.wolf_kennels","offset":(2000,-1200,0)},
            {"id":"viking.shipyard","offset":(-700,-2300,0)},
            {"id":"viking.smithy","offset":(-250,250,0)},
            {"id":"viking.market","offset":(300,-600,0)},
            {"id":"viking.watch","offset":(-2200,0,0)},
        ],
        "wall": {"segment_id":"BridgeWatch", "breach_scar_id":"bridge_watch_broken", "offset":(-2400,0,0)},
        "objectives": [
            {"id":"bridge_watch","building_id":"viking.watch","scar_id":"bridge_watch_broken","effect":"OpenUpperRoute","offset":(-2200,0,0)},
            {"id":"shipyard","building_id":"viking.shipyard","effect":"DisableHarbourSupport","offset":(-700,-2300,0)},
            {"id":"shaman_lodge","building_id":"viking.shaman_lodge","effect":"DropWard","offset":(-1500,1200,100)},
            {"id":"great_hall","building_id":"viking.great_hall","effect":"CaptureSettlement","primary":True,"offset":(1200,1400,500)},
        ],
        "decorations": [
            {"blueprint":"/Game/Viking_Village/Meshes/Props/BPs/BP_StrawArcheryTarget_01a","offset":(-2000,-1550,0),"label":"SoulGeom_HunterTarget_A"},
            {"blueprint":"/Game/Viking_Village/Meshes/Props/BPs/BP_StrawArcheryTarget_01a","offset":(-2000,-1300,0),"label":"SoulGeom_HunterTarget_B"},
            {"blueprint":"/Game/Viking_Village/Meshes/Props/BPs/BP_StrawArcheryTarget_01a","offset":(-2000,-1050,0),"label":"SoulGeom_HunterTarget_C"},
            {"blueprint":"/Game/Viking_Village/Meshes/Props/BPs/BP_VikingBoat_01a","offset":(-800,-2600,0),"label":"SoulGeom_HarbourBoat"},
        ],
    },
]

rows = []
errors = []
for config in CONFIGS:
    try:
        rows.append(build_one(config))
    except Exception:
        error = traceback.format_exc()
        unreal.log_error("SOUL_CITY_OVERLAY_ERROR " + config["id"] + "\n" + error)
        errors.append({"id": config["id"], "error": error})

with open(OVERLAY_MANIFEST, "w", encoding="utf-8") as f:
    json.dump({"overlays": rows, "errors": errors}, f, indent=2)

if errors:
    raise RuntimeError("One or more city overlay builds failed; see manifest")
