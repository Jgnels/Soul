"""Build candidate-owned water surfaces. Does not edit source terrain or donors."""
import unreal as u,json
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929')
E=ROOT/'Evidence/ProductionWorldComposition-20261007';ASSET='/Game/SoulCampaignComposition'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();actors=u.get_editor_subsystem(u.EditorActorSubsystem)
assert w.get_path_name().startswith(ASSET+'/L_Composition_3500_r2')
assert not any(x.get_actor_label().startswith('Composition_Water_') for x in actors.get_all_level_actors())
mat=u.EditorAssetLibrary.duplicate_asset(ASSET+'/MI_Composition_Water_r1',ASSET+'/MI_Composition_River_r1');assert mat
lib=u.MaterialEditingLibrary
for name,value in [('Tile',1.),('DepthFadeDistance',80.),('OpacityNear',.48),('OpacityFar',.6)]:lib.set_material_instance_scalar_parameter_value(mat,name,value)
lib.update_material_instance(mat);assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
receipts=[]
for row in json.loads((E/'Local/water-mesh-buffers-r1.json').read_text())['meshes']:
    vertices=[u.Vector(*p) for p in row['vertices_cm']]
    buf=u.GeometryScriptSimpleMeshBuffers(vertices=vertices,triangles=[u.IntVector(*p) for p in row['triangles']],normals=[u.Vector(0,0,1)]*len(vertices),uv0=[u.Vector2D(*p) for p in row['uv']])
    mesh=u.DynamicMesh();u.GeometryScript_MeshEdits.append_buffers_to_mesh(mesh,buf)
    package=ASSET+'/Water/SM_'+row['id']+'_r1';assert not u.EditorAssetLibrary.does_asset_exist(package)
    opt=u.GeometryScriptCreateNewStaticMeshAssetOptions();opt.enable_nanite=False;opt.enable_collision=False
    asset,ok=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh,package,opt)
    assert ok==u.GeometryScriptOutcomePins.SUCCESS and asset
    asset.set_editor_property('static_materials',[u.StaticMaterial(material_interface=mat,material_slot_name=u.Name('Water'))]);assert u.EditorAssetLibrary.save_loaded_asset(asset,False)
    actor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0));actor.set_actor_label('Composition_Water_'+row['id']);actor.static_mesh_component.set_static_mesh(asset);actor.set_actor_enable_collision(False);actor.static_mesh_component.set_cast_shadow(False)
    receipts.append(dict(id=row['id'],asset=package,vertices=len(vertices),triangles=len(row['triangles'])))
    mesh.reset()
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'water-placement-r1.json').write_text(json.dumps({'surfaces':receipts,'ocean_datum_m':0,'status':'editor composition; visual review required'},indent=2))
print('WATER_PLACED',len(receipts))
