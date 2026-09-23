import unreal, json, os, time

OUT = r"D:\RefinedBadger\Worktrees\Soul-battlefield-environments-20260922\Evidence\EnvironmentCaptures\DragonGraveyard\dragon_navigation_qualification_async.json"
actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.EditorLevelLibrary.get_editor_world()
result = {"schema": 1, "world": str(world.get_path_name()) if world else None, "paths": []}
spawned = []
state = {"handle": None, "started": time.time(), "finished": False}

def finish():
    if state["finished"]:
        return
    state["finished"] = True
    for actor in reversed(spawned):
        try:
            actor_sub.destroy_actor(actor)
        except Exception:
            pass
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2)
    unreal.log("SOUL_DRAGON_NAV_ASYNC " + json.dumps({
        "pass": result.get("pass"),
        "success": result.get("success_count"),
        "error": result.get("error"),
        "wait_seconds": result.get("wait_seconds"),
    }))
    try:
        if state["handle"] is not None:
            unreal.unregister_slate_pre_tick_callback(state["handle"])
    except Exception:
        pass
    try:
        unreal.SystemLibrary.quit_editor()
    except Exception as exc:
        unreal.log_error("SOUL_DRAGON_NAV_ASYNC quit failed: " + repr(exc))

def evaluate():
    query_extent = unreal.Vector(1200, 1200, 4000)
    for i in range(24):
        y = -4600.0 + i * (9200.0 / 23.0)
        s = unreal.Vector(-8000, y, 1200)
        e = unreal.Vector(8000, -y, 1200)
        sp = unreal.NavigationSystemV1.project_point_to_navigation(world, s, None, None, query_extent)
        ep = unreal.NavigationSystemV1.project_point_to_navigation(world, e, None, None, query_extent)
        rec = {"i": i, "start_projected": sp is not None, "end_projected": ep is not None}
        if sp is not None and ep is not None:
            path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, sp, ep)
            pts = list(path.path_points) if path else []
            rec["path_points"] = len(pts)
            rec["valid"] = bool(path and path.is_valid() and len(pts) >= 2)
            if pts:
                length = 0.0
                for a, b in zip(pts[:-1], pts[1:]):
                    dx, dy, dz = b.x-a.x, b.y-a.y, b.z-a.z
                    length += (dx*dx + dy*dy + dz*dz) ** 0.5
                rec["path_length"] = length
        result["paths"].append(rec)
    result["success_count"] = sum(1 for x in result["paths"] if x.get("valid"))
    result["target_count"] = 24
    result["deployment_zones"] = 2
    result["pass"] = result["success_count"] >= 22

def tick(_delta):
    try:
        elapsed = time.time() - state["started"]
        building = bool(unreal.NavigationSystemV1.is_navigation_being_built_or_locked(world))
        result["wait_seconds"] = elapsed
        result["nav_building"] = building
        if building and elapsed < 60.0:
            return
        if building:
            result["error"] = "navigation build did not settle within 60 seconds"
        else:
            evaluate()
    except Exception as exc:
        result["error"] = repr(exc)
    finish()

try:
    nav_bounds = actor_sub.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0, 0, 1000))
    spawned.append(nav_bounds)
    nav_bounds.set_actor_label("Soul_DragonGraveyard_QualificationNav", False)
    _, ext = nav_bounds.get_actor_bounds(False, True)
    nav_bounds.set_actor_scale3d(unreal.Vector(
        12000.0 / max(abs(ext.x), 1.0),
        12000.0 / max(abs(ext.y), 1.0),
        5000.0 / max(abs(ext.z), 1.0)))
    attacker = actor_sub.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-8000, 0, 1000))
    defender = actor_sub.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(8000, 0, 1000))
    spawned.extend([attacker, defender])
    attacker.set_actor_label("Soul_Human_Deployment", False)
    defender.set_actor_label("Soul_Dwarf_Deployment", False)
    nav = unreal.NavigationSystemV1.get_navigation_system(world)
    nav.on_navigation_bounds_updated(nav_bounds)
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
    state["handle"] = unreal.register_slate_pre_tick_callback(tick)
    unreal.log("SOUL_DRAGON_NAV_ASYNC waiting for navigation build")
except Exception as exc:
    result["error"] = repr(exc)
    finish()
