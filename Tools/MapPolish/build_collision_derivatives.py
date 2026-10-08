"""Native render meshes with matching complex collision, in Soul-owned packages."""
import unreal as u,json
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapPolish-20261007'
rows=[]
for name,path in [('TimberDeck','/Game/Forest_village/Meshes/Wood_modules/SM_bridge_module'),('StoneBridge','/Game/Kingdom_Capital/Meshes/Bridge/SM_arch_bridge_01')]:
 package='/Game/SoulCampaignComposition/Polish/SM_'+name+'_Collision_r1';assert not u.EditorAssetLibrary.does_asset_exist(package)
 source=u.load_asset(path);mesh=u.DynamicMesh();opt=u.GeometryScriptCopyMeshFromAssetOptions();opt.apply_build_settings=False;opt.use_build_scale=False;lod=u.GeometryScriptMeshReadLOD();lod.lod_type=u.GeometryScriptLODType.RENDER_DATA;lod.lod_index=0
 _,ok=u.GeometryScript_AssetUtils.copy_mesh_from_static_mesh_v2(source,mesh,opt,lod,False);assert ok==u.GeometryScriptOutcomePins.SUCCESS
 mats,names=u.GeometryScript_AssetUtils.get_material_list_from_static_mesh(source)
 create=u.GeometryScriptCreateNewStaticMeshAssetOptions();create.enable_collision=True;create.collision_mode=u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE;create.enable_nanite=False
 asset,ok=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh,package,create);assert ok==u.GeometryScriptOutcomePins.SUCCESS
 asset.static_materials=[u.StaticMaterial(material_interface=m,material_slot_name=n) for m,n in zip(mats,names)];u.EditorAssetLibrary.save_loaded_asset(asset,False)
 rows.append(dict(source=path,derived=package,triangles=mesh.get_triangle_count(),method='Native RenderData copy, unchanged geometry/materials, complex collision matches render triangles'))
a=u.get_editor_subsystem(u.EditorActorSubsystem);wood=u.load_asset(rows[0]['derived']);stone=u.load_asset(rows[1]['derived']);changed=[]
for actor in a.get_all_level_actors():
 label=actor.get_actor_label()
 if label.startswith('Polish_Crossing_') and '_timber_' in label or label.startswith('Polish_Landing_') and '_deck_' in label:
  actor.static_mesh_component.set_static_mesh(wood);actor.static_mesh_component.set_collision_profile_name('BlockAll');actor.set_actor_enable_collision(True);changed.append(label)
 elif label in ['Composition_Bridge_southern_crossing','Composition_Bridge_human_west_bridge']:
  actor.static_mesh_component.set_static_mesh(stone);actor.static_mesh_component.set_collision_profile_name('BlockAll');actor.set_actor_enable_collision(True);changed.append(label)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'crossing-collision-derivatives.json').write_text(json.dumps({'sources':rows,'actors':changed,'donor_packages_saved':False},indent=2))
print('COLLISION_DERIVATIVES',len(changed))
