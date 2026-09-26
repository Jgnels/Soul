"""Read-only Dragon Graveyard render-contract audit; run in Unreal -nullrhi.

Loads the actual donor level but never saves an asset, sets a property, builds
geometry, or runs gameplay. Writes only the project-owned JSON evidence file.
"""
import json
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
import unreal

MAP = "/Game/Dragon_graveyard/Level/L_showcase_level"
OUT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())) / "Evidence" / "DragonRenderContract.json"
errors = []


def query(label, fn):
    try:
        return fn()
    except Exception as exc:
        errors.append({"query": label, "error": str(exc)})
        return None


def prop(obj, name):
    return obj.get_editor_property(name)


def vec(value):
    return [value.x, value.y, value.z]


def path(obj):
    return obj.get_path_name() if obj else None


levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(MAP):
    raise RuntimeError("Could not load actual Dragon Graveyard level")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
mesh_editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
mesh_objects = {}
rows = []
classes = Counter()
landscapes = []
for actor in actors:
    label = actor.get_actor_label()
    cls = actor.get_class().get_name()
    classes[cls] += 1
    actor_row = {
        "label": label,
        "class": cls,
        "actor_path": actor.get_path_name(),
        "location": vec(actor.get_actor_location()),
        "scale": vec(actor.get_actor_scale3d()),
    }
    bounds = query(label + ".bounds", lambda: actor.get_actor_bounds(False, True))
    if bounds:
        actor_row["bounds_origin"] = vec(bounds[0])
        actor_row["bounds_extent"] = vec(bounds[1])
    if "Landscape" in cls:
        landscape = dict(actor_row)
        for name in ("landscape_material", "enable_nanite", "lod_distribution_setting", "lod0_distribution_setting"):
            value = query(label + "." + name, lambda n=name: prop(actor, n))
            landscape[name] = path(value) if isinstance(value, unreal.Object) else value
        landscapes.append(landscape)
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = prop(component, "static_mesh")
        if not mesh:
            continue
        mesh_path = mesh.get_path_name()
        mesh_objects[mesh_path] = mesh
        row = dict(actor_row)
        row.update({"component_path": component.get_path_name(), "mesh": mesh_path})
        for name in ("forced_lod_model", "min_lod", "override_min_lod", "visible", "hidden_in_game", "disallow_nanite"):
            row[name] = query(label + "." + name, lambda n=name: prop(component, n))
        row["materials"] = query(label + ".materials", lambda: [path(component.get_material(i)) for i in range(component.get_num_materials())])
        if isinstance(component, unreal.InstancedStaticMeshComponent):
            row["instance_count"] = component.get_instance_count()
        rows.append(row)

meshes = []
for mesh_path, mesh in sorted(mesh_objects.items()):
    record = {"path": mesh_path}
    for name in ("lod_group", "allow_cpu_access"):
        value = query(mesh_path + "." + name, lambda n=name: prop(mesh, n))
        record[name] = str(value) if name == "lod_group" else value
    record["auto_compute_lod_screen_size"] = query(mesh_path + ".auto_compute_lod_screen_size", lambda: mesh.is_lod_screen_size_auto_computed())
    settings = query(mesh_path + ".nanite_settings", lambda: prop(mesh, "nanite_settings"))
    record["nanite"] = {}
    if settings:
        for name in ("enabled", "fallback_relative_error", "fallback_percent_triangles", "fallback_target", "generate_fallback"):
            value = query(mesh_path + ".nanite." + name, lambda n=name: prop(settings, n))
            record["nanite"][name] = value if isinstance(value, (bool, int, float, str)) or value is None else str(value)
    count = query(mesh_path + ".lod_count", lambda: mesh.get_num_lods())
    record["lod_count"] = count
    record["lod_screen_sizes"] = query(mesh_path + ".lod_screen_sizes", lambda: list(mesh_editor.get_lod_screen_sizes(mesh))) if mesh_editor else None
    record["lods"] = []
    for lod in range(count or 0):
        item = {"index": lod}
        item["triangles"] = query(mesh_path + f".lod{lod}.triangles", lambda i=lod: mesh.get_num_triangles(i))
        item["vertices"] = query(mesh_path + f".lod{lod}.vertices", lambda i=lod: mesh.get_num_vertices(i))
        item["sections"] = query(mesh_path + f".lod{lod}.sections", lambda i=lod: mesh.get_num_sections(i))
        reduction = query(mesh_path + f".lod{lod}.reduction", lambda i=lod: mesh_editor.get_lod_reduction_settings(mesh, i)) if mesh_editor else None
        if reduction:
            item["reduction_percent_triangles"] = query(mesh_path + f".lod{lod}.reduction_percent", lambda: prop(reduction, "percent_triangles"))
        record["lods"].append(item)
    meshes.append(record)

report = {
    "schema": 1,
    "generated_utc": datetime.now(timezone.utc).isoformat(),
    "mode": "read_only_editor_load_no_gameplay_no_asset_save",
    "map": MAP,
    "actor_count": len(actors),
    "static_mesh_editor_subsystem_available": mesh_editor is not None,
    "class_counts": dict(classes),
    "static_mesh_component_count": len(rows),
    "unique_mesh_count": len(meshes),
    "unique_meshes": meshes,
    "mesh_actors": rows,
    "landscapes": landscapes,
    "query_errors": errors,
    "limitations": "NullRHI reads asset/render data; it does not prove selected runtime LOD or visual fidelity. Missing queries are null with errors, never assumed false.",
}
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
unreal.log("SOUL_DRAGON_RENDER_CONTRACT: " + json.dumps({"output": str(OUT), "actors": len(actors), "meshes": len(meshes), "query_errors": len(errors)}))
