"""Two owned Human architectural pieces: explicit render-LOD copy for DX11."""
import unreal as u,json
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007';rows=[]
for name,path in [('HumanCobble','/Game/CastleTown/Static_Mesh/Cobblestone_Road/SM_Cobbles_4M'),('HumanQuay','/Game/CastleTown/Static_Mesh/DockWalls/SM_DockWall_4x8_01')]:
 package='/Game/SoulCampaignComposition/FinalPolish/SM_'+name+'_r1';assert not u.EditorAssetLibrary.does_asset_exist(package)
 source=u.load_asset(path);mesh=u.DynamicMesh();opt=u.GeometryScriptCopyMeshFromAssetOptions();opt.apply_build_settings=False;opt.use_build_scale=False;lod=u.GeometryScriptMeshReadLOD();lod.lod_type=u.GeometryScriptLODType.RENDER_DATA;lod.lod_index=0
 _,ok=u.GeometryScript_AssetUtils.copy_mesh_from_static_mesh_v2(source,mesh,opt,lod,False);assert ok==u.GeometryScriptOutcomePins.SUCCESS
 mats,names=u.GeometryScript_AssetUtils.get_material_list_from_static_mesh(source);create=u.GeometryScriptCreateNewStaticMeshAssetOptions();create.enable_collision=False;create.enable_nanite=False
 asset,ok=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh,package,create);assert ok==u.GeometryScriptOutcomePins.SUCCESS
 asset.static_materials=[u.StaticMaterial(material_interface=m,material_slot_name=n) for m,n in zip(mats,names)];u.EditorAssetLibrary.save_loaded_asset(asset,False);rows.append(dict(source=path,derived=package,triangles=mesh.get_triangle_count(),method='unchanged native render LOD0 and materials, non-Nanite Soul-owned derivative'))
(E/'human-owned-details.json').write_text(json.dumps(rows,indent=2));print(rows)
