"""Replace presentation-only foliage instances with regional owned variants; no terrain edits."""
from pathlib import Path
import unreal as u,json,hashlib,math
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/ProductionContinuation-20261008';root='/Game/SoulCampaignComposition/Continuation'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
assert not (E/'foliage-r1.json').exists()
fa=next(x for x in a.get_all_level_actors() if isinstance(x,u.InstancedFoliageActor));oldfts=list(fa.get_used_foliage_types());assert len(oldfts)==2
original=[];groups={};source_hashes={}
def preserve(path):
 p=R/'Content'/(path.split('.')[0].removeprefix('/Game/')+'.uasset')
 with p.open('rb') as f:source_hashes[str(p)]=hashlib.file_digest(f,'sha256').hexdigest()
meshes={}
for key,path in {'p1':'/Game/Forest_village/Meshes/Vegetation/SM_pine_tree_01','p2':'/Game/Forest_village/Meshes/Vegetation/SM_pine_tree_02','p3':'/Game/Forest_village/Meshes/Vegetation/SM_pine_tree_03','b1':'/Game/Kingdom_Capital/Meshes/Tree/SM_tree_01','b3':'/Game/Kingdom_Capital/Meshes/Tree/SM_tree_03'}.items():
 preserve(path);meshes[key]=u.load_asset(path);assert meshes[key]
print('TREE_TRIANGLES',{k:[m.get_num_triangles(i) for i in range(m.get_num_lods())] for k,m in meshes.items()})
for key in ['b1','b3']:
 dest=root+'/SM_Deciduous_'+key+'_r1';assert not u.EditorAssetLibrary.does_asset_exist(dest)
 owned=u.EditorAssetLibrary.duplicate_asset(meshes[key].get_path_name(),dest);assert owned
 options=u.StaticMeshReductionOptions(auto_compute_lod_screen_size=True,reduction_settings=[u.StaticMeshReductionSettings(percent_triangles=1,screen_size=1),u.StaticMeshReductionSettings(percent_triangles=.35,screen_size=.25),u.StaticMeshReductionSettings(percent_triangles=.12,screen_size=.07)])
 assert u.get_editor_subsystem(u.StaticMeshEditorSubsystem).set_lods(owned,options)==3
 u.EditorAssetLibrary.save_loaded_asset(owned);meshes[key]=owned
 print('OWNED_TREE_LODS',key,[owned.get_num_triangles(i) for i in range(owned.get_num_lods())])

for c in fa.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent):
 oldmin=c.static_mesh.get_bounding_box().min.z;oldheight=c.static_mesh.get_bounding_box().max.z-oldmin
 for i in range(c.get_instance_count()):
  tr=c.get_instance_transform(i,world_space=True);loc=tr.translation;sc=tr.scale3d;x=(loc.x+175000)/100;y=(loc.y+175000)/100
  seed=int(abs(x*137+y*71))
  region='Boreal' if y<1250 else 'Greenwood' if y>2250 else 'Heartland'
  variant='p3' if region=='Boreal' and seed%3==0 else 'p1' if seed%2==0 else 'p2'
  if region in ['Greenwood','Heartland'] and seed%9==0:variant='b1' if seed%2==0 else 'b3'
  mesh=meshes[variant];b=mesh.get_bounding_box();height=oldheight*sc.z
  if variant.startswith('b'):height=min(height,1350)
  scale=height/(b.max.z-b.min.z);ground=loc.z+oldmin*sc.z
  nt=u.Transform(location=u.Vector(loc.x,loc.y,ground-b.min.z*scale),rotation=tr.rotation.rotator(),scale=u.Vector(scale,scale,scale))
  groups.setdefault((region,variant),[]).append(nt)
  original.append({'mesh':c.static_mesh.get_path_name(),'location':list(loc.to_tuple()),'scale':list(sc.to_tuple()),'rotation':list(tr.rotation.to_tuple())})
(E/'Local/foliage-before.json').write_text(json.dumps(original));assert len(original)==18524
mats={};source='/Game/Soul/Campaign/TerrainV2/MI_CampaignPine';preserve(source)
for region,color in {'Boreal':(.18,.285,.22),'Greenwood':(.275,.385,.185),'Heartland':(.33,.405,.205)}.items():
 dest=root+'/MI_Pine_'+region+'_r1';assert not u.EditorAssetLibrary.does_asset_exist(dest)
 mi=u.EditorAssetLibrary.duplicate_asset(source,dest);u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi,'Base Color',u.LinearColor(*color,0));u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi,'SSS',.35);u.MaterialEditingLibrary.update_material_instance(mi);u.EditorAssetLibrary.save_loaded_asset(mi);mats[region]=mi
new=[]
for (region,variant),transforms in sorted(groups.items()):
 name='FT_'+region+'_'+variant+'_r1';ft=u.AssetToolsHelpers.get_asset_tools().create_asset(name,root,u.FoliageType_InstancedStaticMesh,u.FoliageType_InstancedStaticMeshFactory());assert ft
 mesh=meshes[variant];ft.set_editor_property('mesh',mesh)
 materials=[x.material_interface for x in mesh.get_editor_property('static_materials')]
 if variant.startswith('p'):materials=[mats[region] if m and m.get_name().startswith('MI_pine_tree') else m for m in materials]
 ft.set_editor_property('override_materials',materials);ft.set_editor_property('cast_dynamic_shadow',True);ft.set_editor_property('cast_static_shadow',False);ft.set_editor_property('affect_distance_field_lighting',False);ft.set_editor_property('cull_distance',u.Int32Interval(min=1000000,max=1200000));u.EditorAssetLibrary.save_loaded_asset(ft);new.append((ft,transforms));print('FOREST_GROUP',name,len(transforms))
# Old map-local types remain preserved in the before map; donors are never altered.
for ft in oldfts:u.InstancedFoliageActor.remove_all_instances(w,ft)
for ft,transforms in new:u.InstancedFoliageActor.add_instances(w,ft,transforms)
for c in fa.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent):
 c.set_forced_lod_model(min(3,c.static_mesh.get_num_lods()));c.set_cull_distances(1000000,1200000);c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
assert sum(c.get_instance_count() for c in fa.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent))==len(original)
for path,sha in source_hashes.items():
 with Path(path).open('rb') as f:assert hashlib.file_digest(f,'sha256').hexdigest()==sha
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'foliage-r1.json').write_text(json.dumps({'count_before':len(original),'count_after':len(original),'groups':[{ 'type':ft.get_path_name(),'count':len(ts)} for ft,ts in new],'donor_hashes':source_hashes,'placement_xy_unchanged':True,'terrain_changed':False,'status':'REQUIRES_RENDERED_REVIEW'},indent=2));print('REGIONAL_FOLIAGE_COMPLETE')
