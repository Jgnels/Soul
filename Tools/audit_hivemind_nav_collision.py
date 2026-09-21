import unreal, json, os, traceback

OUT = r"D:\RefinedBadger\Games\Soul\Evidence"
MANIFEST = os.path.join(OUT, "hivemind_nav_collision_audit.json")
MAPS = [
    ("tavern", "/Game/Medieval_Megapack/Levels/Prefabs/Tavern"),
    ("forge", "/Game/Medieval_Megapack/Levels/Prefabs/Forge"),
]
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def safe_call(obj, name, *args):
    fn = getattr(obj, name, None)
    if not fn:
        return None
    try:
        value = fn(*args)
        return str(value) if not isinstance(value, (int, float, bool)) else value
    except Exception:
        return None

def nav_flag(component):
    for prop in ("can_ever_affect_navigation", "b_can_ever_affect_navigation"):
        try:
            return bool(component.get_editor_property(prop))
        except Exception:
            pass
    return safe_call(component, "can_ever_affect_navigation")

rows = []
errors = []
for label, map_path in MAPS:
    try:
        if not levels.load_level(map_path):
            raise RuntimeError("Could not load " + map_path)
        world = unreal.EditorLevelLibrary.get_editor_world()
        actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
        components = []
        for actor in actors:
            for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
                mesh = comp.get_editor_property("static_mesh")
                if not mesh:
                    continue
                triangles = safe_call(mesh, "get_num_triangles", 0)
                vertices = safe_call(mesh, "get_num_vertices", 0)
                components.append({
                    "actor": actor.get_actor_label(),
                    "component": comp.get_name(),
                    "mesh": mesh.get_path_name(),
                    "triangles_lod0": triangles,
                    "vertices_lod0": vertices,
                    "nav_relevant": nav_flag(comp),
                    "collision_profile": safe_call(comp, "get_collision_profile_name"),
                    "collision_enabled": safe_call(comp, "get_collision_enabled"),
                })
        components.sort(key=lambda x: (-(x["triangles_lod0"] or 0), x["mesh"]))

        nav_triangles = sum(
            (x["triangles_lod0"] or 0)
            for x in components if x["nav_relevant"] is True
        )
        row = {
            "id": label,
            "map": map_path,
            "static_mesh_components": len(components),
            "nav_relevant_components": sum(1 for x in components if x["nav_relevant"] is True),
            "nav_relevant_lod0_triangles": nav_triangles,
            "top_components": components[:40],
        }
        rows.append(row)
        unreal.log("SOUL_HIVEMIND_NAV_AUDIT " + json.dumps({
            "id": label,
            "components": row["static_mesh_components"],
            "nav_components": row["nav_relevant_components"],
            "nav_triangles": nav_triangles,
        }, sort_keys=True))
    except Exception:
        err = traceback.format_exc()
        errors.append({"id": label, "map": map_path, "error": err})
        unreal.log_error("SOUL_HIVEMIND_NAV_AUDIT_ERROR " + label + "\n" + err)

with open(MANIFEST, "w", encoding="utf-8") as handle:
    json.dump({"maps": rows, "errors": errors}, handle, indent=2)
