import unreal, os, json, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence\EnvironmentCaptures"
os.makedirs(OUT, exist_ok=True)

MAPS = {
    "human_hivemind": "/Game/Medieval_Megapack/Levels/PL_Fortress_Day",
    "viking_watercity": "/Game/JustBStudios/Water_City/Levels/LV_WaterVillage",
    "viking_village": "/Game/Viking_Village/Levels/MainVillage/LV_MainVillage",
    "ravenhold": "/Game/Ravenhold/Scenes/HM-FortCastle_Kit_Demo",
    "mountain_01": "/Game/LandscapePackOne/Maps/Mountain_01",
    "snowy_01": "/Game/LandscapePackOne/Maps/SnowyMountain_01",
    "grassland_01": "/Game/LandscapePackTwo/Maps/Grassland_01",
    "mesa_01": "/Game/LandscapePackTwo/Maps/Mesa_01",
    "desert_01": "/Game/LandscapePackTwo/Maps/Desert_01",
    "coastal_ruins_01": "/Game/Elite_CoastalRuins/Maps/CoastalRuins_01",
}
KEY = os.environ.get("SOUL_CAPTURE_KEY","human_hivemind").strip()
MAP = MAPS.get(KEY)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
camera = None
result = {"key":KEY,"map":MAP,"load_ok":False}

def log(s):
    unreal.log("SOUL_SAFE_"+s)

def world():
    return unreal.EditorLevelLibrary.get_editor_world()

def class_actors(cls):
    try:
        return unreal.GameplayStatics.get_all_actors_of_class(world(), cls)
    except Exception:
        return []

def stable_anchor():
    # Deliberately avoid enumerating every actor: that crashed Hivemind after streaming.
    starts = class_actors(unreal.PlayerStart)
    if starts:
        loc=starts[0].get_actor_location()
        return loc, 22000.0, "PlayerStart"
    cams = class_actors(unreal.CameraActor)
    if cams:
        loc=cams[0].get_actor_location()
        return loc, 22000.0, "CameraActorLocation"
    try:
        landscapes = class_actors(unreal.Landscape)
    except Exception:
        landscapes=[]
    if landscapes:
        origin, extent = landscapes[0].get_actor_bounds(False, True)
        span=max(float(extent.x*2),float(extent.y*2))
        span=max(8000.0,min(span,90000.0))
        return origin, span, "LandscapeBounds"
    return unreal.Vector(0,0,0), 25000.0, "OriginFallback"

def make_camera(center, span, overview):
    global camera
    if camera:
        try: actors.destroy_actor(camera)
        except: pass
    if overview:
        loc=unreal.Vector(center.x+span*.55,center.y-span*.55,center.z+span*.55)
    else:
        loc=unreal.Vector(center.x+span*.42,center.y-span*.64,center.z+span*.26)
    camera=actors.spawn_actor_from_class(unreal.CameraActor,loc,unreal.Rotator(0,0,0))
    target=unreal.Vector(center.x,center.y,center.z+span*.02)
    camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc,target),False)
    cc=camera.get_component_by_class(unreal.CameraComponent)
    if overview:
        cc.set_editor_property("projection_mode",unreal.CameraProjectionMode.ORTHOGRAPHIC)
        cc.set_editor_property("ortho_width",float(max(6500,min(span*1.25,100000))))
    else:
        cc.set_editor_property("projection_mode",unreal.CameraProjectionMode.PERSPECTIVE)
        cc.set_editor_property("field_of_view",50.0)

def shot(suffix):
    filename=f"{KEY}_{suffix}.png"
    task=unreal.AutomationLibrary.take_high_res_screenshot(
        1280,720,os.path.join(OUT,filename),camera,False,False,
        unreal.ComparisonTolerance.LOW,"",0.0,True)
    log("CAPTURE_REQUEST "+filename+" valid="+str(task.is_valid_task()))
    return task

state={"phase":"load","ticks":0,"task":None,"busy":False,"center":None,"span":0}

def finish():
    manifest=os.path.join(OUT,"safe_capture_manifest.json")
    rows=[]
    if os.path.exists(manifest):
        try: rows=json.load(open(manifest,"r",encoding="utf-8"))
        except: rows=[]
    rows=[r for r in rows if r.get("key")!=KEY]
    rows.append(result)
    with open(manifest,"w",encoding="utf-8") as f: json.dump(rows,f,indent=2)
    open(os.path.join(OUT,f"safe_done_{KEY}.txt"),"w").write("done\n")
    log("DONE "+KEY)
    state["phase"]="done"

def tick(dt):
    if state["busy"]: return
    state["busy"]=True
    try:
        state["ticks"]+=1
        ph=state["phase"]
        if ph=="done": return
        if ph=="load":
            state["phase"]="loading"
            log("LOAD_BEGIN "+KEY+" "+str(MAP))
            ok=bool(MAP and levels.load_level(MAP))
            result["load_ok"]=ok
            log("LOAD_RESULT "+KEY+" "+str(ok))
            if not ok: finish(); return
            state["phase"]="settle"; state["ticks"]=0
            return
        if ph=="settle":
            # Hivemind streams multiple sublevels. Give all editor-side compilation/streaming time to settle.
            if state["ticks"]<900: return
            center,span,source=stable_anchor()
            state["center"]=center; state["span"]=span
            result["anchor_source"]=source
            result["center"]=[center.x,center.y,center.z]
            result["span"]=span
            log("ANCHOR "+source+" center="+str(center)+" span="+str(span))
            make_camera(center,span,True)
            state["phase"]="overview_warm"; state["ticks"]=0
            return
        if ph=="overview_warm" and state["ticks"]>=60:
            state["phase"]="overview_capture"
            state["task"]=shot("overview")
            return
        if ph=="overview_capture":
            if state["task"] and state["task"].is_task_done():
                make_camera(state["center"],state["span"],False)
                state["task"]=None; state["phase"]="perspective_warm"; state["ticks"]=0
            return
        if ph=="perspective_warm" and state["ticks"]>=60:
            state["phase"]="perspective_capture"
            state["task"]=shot("perspective")
            return
        if ph=="perspective_capture":
            if state["task"] and state["task"].is_task_done():
                result["overview"]=f"{KEY}_overview.png"
                result["perspective"]=f"{KEY}_perspective.png"
                finish()
            return
    except Exception:
        err=traceback.format_exc()
        unreal.log_error("SOUL_SAFE_EXCEPTION\n"+err)
        result["error"]=err
        with open(os.path.join(OUT,f"safe_error_{KEY}.txt"),"w",encoding="utf-8") as f:f.write(err)
        state["phase"]="done"
    finally:
        state["busy"]=False

if not MAP:
    result["error"]="Unknown capture key"
    finish()
else:
    try:
        w=world()
        for cmd in [
            "r.DynamicGlobalIlluminationMethod 0","r.ReflectionMethod 0",
            "r.Lumen.DiffuseIndirect.Allow 0","r.Lumen.Reflections.Allow 0",
            "r.MotionBlurQuality 0","r.ShadowQuality 2",
            "foliage.DensityScale 0.65","grass.DensityScale 0.65"]:
            unreal.SystemLibrary.execute_console_command(w,cmd)
    except: pass
    handle=unreal.register_slate_post_tick_callback(tick)
    print("SOUL_SAFE_READY",KEY,handle)
