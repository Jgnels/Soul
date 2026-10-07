"""Place exact deterministic transforms into candidate-owned foliage types."""
import unreal as u,json,math
from pathlib import Path
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');OUT=ROOT/'Evidence/ProductionWorldComposition-20261007'
a=u.get_editor_subsystem(u.EditorActorSubsystem);w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/')
fa=next(x for x in a.get_all_level_actors() if isinstance(x,u.InstancedFoliageActor));fts=list(fa.get_used_foliage_types());assert len(fts)==2
assert sum(c.get_instance_count() for c in fa.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent))==0,'Refuse duplicate population; the FoliageType transform query can return empty despite populated HISM components.'
fts.sort(key=lambda f:f.get_editor_property('mesh').get_name())
for ft in fts:
 assert ft.get_path_name().startswith(w.get_path_name()+':'),'never modify a donor foliage type'
 assert len(fa.get_instance_transforms(ft))==0
 mesh=ft.get_editor_property('mesh');mats=[s.material_interface for s in mesh.get_editor_property('static_materials')]
 mats=[u.load_asset('/Game/Soul/Campaign/TerrainV2/MI_CampaignPine') if m and m.get_name().startswith('MI_pine_tree') else m for m in mats]
 ft.set_editor_property('override_materials',mats);ft.set_editor_property('cast_dynamic_shadow',True);ft.set_editor_property('cast_static_shadow',False);ft.set_editor_property('affect_distance_field_lighting',False)
 ft.set_editor_property('cull_distance',u.Int32Interval(min=1000000,max=1200000))
data=json.loads((ROOT/'Data/CampaignCompositionLocal/forest-masses-r1.json').read_text());groups=[[],[]];misses=[];offsets=0
for p in data['points']:
 x,y=p['xy_m'];x=x*100-175000;y=y*100-175000
 hit=u.SystemLibrary.line_trace_single(w,u.Vector(x,y,100000),u.Vector(x,y,-100000),u.TraceTypeQuery.ECC_VISIBILITY,False,[],u.DrawDebugTrace.NONE);d=hit.to_dict() if hit else {}
 if not d.get('blocking_hit') or not isinstance(d['hit_actor'],u.Landscape):misses.append([x,y]);continue
 variant=p['variant'];mesh=fts[variant].get_editor_property('mesh');b=mesh.get_bounding_box();scale=p['tree_height_m']*100/(b.max.z-b.min.z);rot=u.Rotator(pitch=0,yaw=p['yaw'],roll=0)
 groups[variant].append(u.Transform(location=u.Vector(x,y,d['impact_point'].z-b.min.z*scale-5),rotation=rot,scale=u.Vector(scale,scale,scale)))
for ft,transforms in zip(fts,groups):u.InstancedFoliageActor.add_instances(w,ft,transforms)
for c in fa.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent):
 c.set_forced_lod_model(3);c.set_cull_distances(1000000,1200000);c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(OUT/'forest-mass-placement-r1.json').write_text(json.dumps({'counts':[len(x) for x in groups],'foliage_types':[f.get_path_name() for f in fts],'collision_misses':misses,'LOD_model':3,'cull_start_end_cm':[1000000,1200000],'terrain_changed':False,'status':'scale/composition test using retained owned pine family'},indent=2));print('FOREST_STUDY_COMPLETE',[len(x) for x in groups],len(misses))
