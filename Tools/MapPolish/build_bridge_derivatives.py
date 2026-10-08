"""Cut the owned stone bridge into two ruined ends. Source package stays read-only."""
from pathlib import Path
import unreal as u,json
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007'
source=u.load_asset('/Game/Kingdom_Capital/Meshes/Bridge/SM_arch_bridge_01');assert source
materials,names=u.GeometryScript_AssetUtils.get_material_list_from_static_mesh(source)
rows=[]
for name,x,flip in [('West',-1350,True),('East',-660,False)]:
 package='/Game/SoulCampaignComposition/Polish/SM_RuinedBridge_'+name+'_r1'
 assert not u.EditorAssetLibrary.does_asset_exist(package)
 mesh=u.DynamicMesh();opt=u.GeometryScriptCopyMeshFromAssetOptions();opt.apply_build_settings=False;opt.use_build_scale=False
 lod=u.GeometryScriptMeshReadLOD();lod.lod_type=u.GeometryScriptLODType.RENDER_DATA;lod.lod_index=0
 _,ok=u.GeometryScript_AssetUtils.copy_mesh_from_static_mesh_v2(source,mesh,opt,lod,False);assert ok==u.GeometryScriptOutcomePins.SUCCESS
 frame=u.Transform();frame.translation=u.Vector(x,0,0);frame.rotation=u.Rotator(pitch=90).quaternion()
 cut=u.GeometryScriptMeshPlaneCutOptions();cut.fill_holes=True;cut.fill_spans=True;cut.flip_cut_side=flip;cut.hole_fill_material_id=0
 u.GeometryScript_MeshBooleans.apply_mesh_plane_cut(mesh,frame,cut)
 box=u.GeometryScript_MeshQueries.get_mesh_bounding_box(mesh)
 # A wrong cutting side must never be written as the accepted ruined end.
 if name=='West':assert box.max.x<=x+1 and box.min.x<-1900,str(box)
 else:assert box.min.x>=x-1 and box.max.x>-1,str(box)
 create=u.GeometryScriptCreateNewStaticMeshAssetOptions();create.enable_nanite=False;create.enable_collision=True;create.collision_mode=u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
 asset,ok=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh,package,create);assert ok==u.GeometryScriptOutcomePins.SUCCESS
 asset.static_materials=[u.StaticMaterial(material_interface=m,material_slot_name=n) for m,n in zip(materials,names)]
 assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
 rows.append(dict(asset=asset.get_path_name(),triangles=mesh.get_triangle_count(),cut_local_x_cm=x,bounds=str(box)))
(E/'bridge-derivatives.json').write_text(json.dumps({'source':source.get_path_name(),'method':'deterministic plane cuts of owned rendered mesh; original UVs/materials retained','donor_saved':False,'derivatives':rows},indent=2))
print('RUINED_BRIDGE_DERIVATIVES',rows)
