"""Deterministic Human castle source-LOD repair; no donor writes.
Each mesh is independently welded and reduced before native prefab assembly.
"""
from pathlib import Path
import json,hashlib,unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
p=root/'Evidence/SettlementEnvironmentPlan-20261005/Population-20261006/HumanMiniature'
output=p/'source-lod0-repaired-r1.json';assert not output.exists()
meshes={}
for j in json.loads((p/'intact-parts-r2-plan.json').read_text())['jobs']:
 if '/Castle/' not in j['source'] or '/LI_CurtainWall' in j['source']:continue
 r=json.loads(Path(j['recipe']).read_text());meshes.update(json.loads((root/r['source']).read_text())['meshes'])
records=[]
for i,(path,meta) in enumerate(sorted(meshes.items())):
 source=unreal.load_asset(path);assert source
 package='/Game/Soul/CampaignProxies/Human/SourceLOD0/'+source.get_name()+'_weld_r1'
 assert not unreal.EditorAssetLibrary.does_asset_exist(package)
 d=unreal.DynamicMesh();opt=unreal.GeometryScriptCopyMeshFromAssetOptions();opt.apply_build_settings=False;opt.use_build_scale=False
 lod=unreal.GeometryScriptMeshReadLOD();lod.lod_type=unreal.GeometryScriptLODType.RENDER_DATA;lod.lod_index=0
 _,ok=unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh_v2(source,d,opt,lod,False);assert ok==unreal.GeometryScriptOutcomePins.SUCCESS
 before=d.get_triangle_count();assert 0<before<3000000
 weld=unreal.GeometryScriptWeldEdgesOptions();weld.tolerance=.01
 unreal.GeometryScript_MeshRepair.weld_mesh_edges(d,weld)
 options=unreal.GeometryScriptSimplifyMeshOptions();options.method=unreal.GeometryScriptRemoveMeshSimplificationType.ATTRIBUTE_AWARE_V2
 target=max(128,meta['triangles'])
 if before>target:unreal.GeometryScript_MeshSimplification.apply_simplify_to_triangle_count(d,target,options)
 o=unreal.GeometryScriptCreateNewStaticMeshAssetOptions();o.enable_nanite=False;o.enable_collision=False
 mesh,ok=unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(d,package,o);assert ok==unreal.GeometryScriptOutcomePins.SUCCESS
 mesh.set_editor_property('static_materials',source.get_editor_property('static_materials'));assert unreal.EditorAssetLibrary.save_loaded_asset(mesh,False)
 disk=root/'Content'/(package.removeprefix('/Game/')+'.uasset')
 records.append(dict(source=path,source_lod=0,before=before,target=target,after=mesh.get_num_triangles(0),mesh=mesh.get_path_name(),sha256=hashlib.sha256(disk.read_bytes()).hexdigest()))
 output.write_text(json.dumps(dict(method='Native RenderLOD0, weld tolerance 0.01 cm, attribute-aware V2, per-mesh native materials; no donor writes',complete=False,records=records),indent=2)+'\n')
 print('SOURCE_LOD0_REPAIR',i,len(meshes),source.get_name(),before,mesh.get_num_triangles(0))
 d.reset();unreal.SystemLibrary.collect_garbage()
output.write_text(json.dumps(dict(method='Native RenderLOD0, weld tolerance 0.01 cm, attribute-aware V2, per-mesh native materials; no donor writes',complete=True,records=records),indent=2)+'\n')
