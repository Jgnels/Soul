"""Build one deterministic miniature piece from explicit native RenderData LODs.

Set RECIPE_PATH to a reviewed local recipe in the live execution namespace.
Unlike the rejected actor merge, this never reconstructs high-resolution source
MeshDescriptions or creates thousands of temporary actors. Donors stay read-only.
"""
import collections
import datetime
import hashlib
import json
from pathlib import Path
import unreal

# RECIPE_PATH is an explicit Soul-owned local recipe supplied to the live call.
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
recipe_path = Path(RECIPE_PATH).resolve()
allowed_roots=[root/'Evidence/SettlementEnvironmentPlan-20261005',root/'Evidence/HumanHeartlandDepth-20261010']
assert any(recipe_path.is_relative_to(p) for p in allowed_roots)
recipe = json.loads(recipe_path.read_text(encoding='utf-8-sig'))
MINIATURE_STATE = recipe['state']
assert MINIATURE_STATE in ('base', 'upgrade')
source = (root/recipe['source']).resolve()
assert any(source.is_relative_to(p) for p in allowed_roots)
data = json.loads(source.read_text(encoding='utf-8-sig'))
package = recipe['package']
assert package.startswith('/Game/Soul/CampaignProxies/') and '..' not in package
receipt = (root/recipe['receipt']).resolve()
assert any(receipt.is_relative_to(p) for p in allowed_roots)
bounds = recipe.get('bounds', [-1000000,1000000]*3)
assert len(bounds) == 6
budget_override = recipe['triangle_budget']
assert 0 < budget_override <= 3000000
excluded_actors = set(recipe.get('excluded_actors', []))
assert not receipt.exists() and not unreal.EditorAssetLibrary.does_asset_exist(package)

groups = collections.defaultdict(list)
for row in data['instances']:
    if row['state'] != MINIATURE_STATE: continue
    if row['actor'] in excluded_actors: continue
    if not all(bounds[2*i] <= row['location'][i] <= bounds[2*i+1] for i in range(3)): continue
    groups[(row['mesh'], tuple(row['materials']))].append(row)
estimated_input = sum(data['meshes'][mesh]['triangles'] * len(rows) for (mesh, materials), rows in groups.items())
assert 0 < estimated_input <= 3000000, 'Compile large scenes as native prefab pieces before whole-city assembly'
target = unreal.DynamicMesh()
piece = unreal.DynamicMesh()
materials = []
constant = unreal.Transform()
constant.translation = unreal.Vector(*[-v for v in data['common_pivot']])
copied = []
for index, ((mesh_path, material_paths), rows) in enumerate(sorted(groups.items())):
    source_mesh = unreal.load_asset(mesh_path)
    assert source_mesh
    lod = data['meshes'][mesh_path]['lod']
    read = unreal.GeometryScriptMeshReadLOD()
    read.set_editor_property('lod_type', unreal.GeometryScriptLODType.RENDER_DATA)
    read.set_editor_property('lod_index', lod)
    options = unreal.GeometryScriptCopyMeshFromAssetOptions()
    options.set_editor_property('apply_build_settings', False)
    options.set_editor_property('use_build_scale', False)
    piece.reset()
    _, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh_v2(source_mesh, piece, options, read, False)
    assert outcome == unreal.GeometryScriptOutcomePins.SUCCESS
    count = piece.get_triangle_count()
    assert 0 < count <= data['meshes'][mesh_path]['triangles'], 'Requested render LOD was not respected'
    transforms = []
    for row in rows:
        transform = unreal.Transform()
        transform.translation = unreal.Vector(*row['location'])
        pitch, yaw, roll = row['rotation']
        transform.rotation = unreal.Rotator(pitch=pitch, yaw=yaw, roll=roll).quaternion()
        transform.scale3d = unreal.Vector(*row['scale'])
        transforms.append(transform)
    source_materials = [unreal.load_asset(p) if p and p != 'None' else None for p in material_paths]
    _, materials = unreal.GeometryScript_MeshEdits.append_mesh_transformed_with_materials(
        target, materials, piece, source_materials, transforms, constant, False, True)
    copied.append(dict(mesh=mesh_path, lod=lod, triangles=count, instances=len(rows)))
    if index % 25 == 0:
        print('SOUL_MINIATURE_APPEND', MINIATURE_STATE, index, len(groups), target.get_triangle_count())
        unreal.SystemLibrary.collect_garbage()
before = target.get_triangle_count()
budget = 180000 if MINIATURE_STATE == 'base' else 20000
if budget_override is not None: budget = budget_override
if budget > 0 and before > budget:
    unreal.GeometryScript_MeshSimplification.apply_simplify_to_triangle_count(
        target, budget, unreal.GeometryScriptSimplifyMeshOptions())
reduced = target.get_triangle_count()
assert reduced > 0
options = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
options.set_editor_property('enable_nanite', False)
options.set_editor_property('enable_collision', False)
mesh, outcome = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(target, package, options)
assert outcome == unreal.GeometryScriptOutcomePins.SUCCESS and mesh
mesh.set_editor_property('static_materials', [unreal.StaticMaterial(material_interface=m, material_slot_name=unreal.Name('Mat_' + str(i))) for i, m in enumerate(materials)])
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
disk = root / 'Content' / (package.removeprefix('/Game/') + '.uasset')
receipt.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    state=MINIATURE_STATE, mesh=mesh.get_path_name(), sha256=hashlib.sha256(disk.read_bytes()).hexdigest(),
    source_manifest_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    method='explicit native RenderData LOD copy, native transformed append, attribute-aware deterministic reduction',
    pivot=data['common_pivot'], selection_bounds=bounds, triangle_budget=budget,
    excluded_actors=sorted(excluded_actors),
    source_instances=sum(x['instances'] for x in copied),
    source_render_lods=copied, input_triangles=before, reduced_triangles=reduced,
    saved_triangles=mesh.get_num_triangles(0), material_count=len(materials),
    recipe_sha256=hashlib.sha256(recipe_path.read_bytes()).hexdigest(),
    status='derived mesh saved; rendered state comparison and campaign placement pending'), indent=2) + '\n')
print('SOUL_OWNED_RENDERLOD_MINIATURE', MINIATURE_STATE, before, reduced, mesh.get_num_triangles(0), str(receipt))
target.reset()
piece.reset()
