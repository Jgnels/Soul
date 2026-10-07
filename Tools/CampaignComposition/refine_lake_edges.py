"""Replace only candidate lake mesh references with clipped shoreline meshes."""
import unreal as u,json
from pathlib import Path
E=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929/Evidence/ProductionWorldComposition-20261007');A='/Game/SoulCampaignComposition'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith(A+'/L_Composition_3500_r2')
actors={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()};mat=u.load_asset(A+'/MI_Composition_River_r1');records=[]
for row in json.loads((E/'Local/water-mesh-buffers-r2.json').read_text())['meshes']:
 if u.EditorAssetLibrary.does_asset_exist(A+'/Water/SM_'+row['id']+'_r2'):continue # Preserve already-qualified candidate surfaces on resume.
 verts=[u.Vector(*p) for p in row['vertices_cm']];buf=u.GeometryScriptSimpleMeshBuffers(vertices=verts,triangles=[u.IntVector(*p) for p in row['triangles']],normals=[u.Vector(0,0,1)]*len(verts),uv0=[u.Vector2D(*p) for p in row['uv']]);mesh=u.DynamicMesh();u.GeometryScript_MeshEdits.append_buffers_to_mesh(mesh,buf)
 package=A+'/Water/SM_'+row['id']+'_r2';assert not u.EditorAssetLibrary.does_asset_exist(package)
 opt=u.GeometryScriptCreateNewStaticMeshAssetOptions();opt.enable_nanite=False;opt.enable_collision=False
 asset,ok=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh,package,opt);assert ok==u.GeometryScriptOutcomePins.SUCCESS and asset
 asset.set_editor_property('static_materials',[u.StaticMaterial(material_interface=mat,material_slot_name=u.Name('Water'))]);assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
 actors['Composition_Water_'+row['id']].static_mesh_component.set_static_mesh(asset);records.append(dict(id=row['id'],asset=package,triangles=len(row['triangles'])));mesh.reset()
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'river-overlap-refinement-r2.json').write_text(json.dumps({'rivers':records,'method':'omit translucent river faces over lake/ocean surfaces','heightfield_changed':False},indent=2));print('RIVER_OVERLAP_REFINED',records)
