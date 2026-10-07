"""Run only after a fresh editor process loads the saved candidate map."""
from pathlib import Path
import unreal as u,json,math
ROOT=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');OUT=ROOT/'Evidence/TerrainFoundation-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignFoundation/')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors();fit=json.loads((OUT/'dense-route-fit-r5.json').read_text());expected={x['id']:x for x in fit['anchors']};checks=[]
def check(name,ok,detail=None):checks.append({'name':name,'pass':bool(ok),'detail':detail})
anchors=[x for x in actors if x.get_actor_label().startswith('Foundation_Anchor_')]
check('exactly 36 canonical TargetPoints',len(anchors)==36 and all(isinstance(x,u.TargetPoint) for x in anchors))
ids=[x.get_actor_label().removeprefix('Foundation_Anchor_') for x in anchors];check('no duplicate or missing region IDs',len(set(ids))==36 and set(ids)==set(expected))
errors=[];misses=[]
for actor in anchors:
 node=expected[actor.get_actor_label().removeprefix('Foundation_Anchor_')];p=actor.get_actor_location();errors.append(math.hypot(p.x-(node['xy_m'][0]*100-175000),p.y-(node['xy_m'][1]*100-175000)))
 hit=u.SystemLibrary.sphere_trace_single(w,u.Vector(p.x,p.y,100000),u.Vector(p.x,p.y,-100000),.25,u.TraceTypeQuery.ECC_VISIBILITY,False,[],u.DrawDebugTrace.NONE)
 d=hit.to_dict() if hit else {}
 if not d.get('blocking_hit') or not isinstance(d.get('hit_actor'),u.Landscape):misses.append(actor.get_actor_label())
check('anchor XY matches dense proposal',max(errors,default=1e9)<.01,max(errors,default=None));check('native sphere-sweep landscape alignment',not misses,misses)
roads=[x for x in actors if x.get_actor_label().startswith('Foundation_Route_r2_')]
check('51 saved active road actors',len(roads)==51 and all(x.get_actor_scale3d().x>0 and x.static_mesh_component.get_editor_property('static_mesh') for x in roads))
old=[x for x in actors if x.get_actor_label().startswith('Foundation_Route_') and x not in roads]
check('rejected roads remain inactive after reload',all(x.get_actor_scale3d().x==0 and x.get_editor_property('is_editor_only_actor') for x in old),len(old))
forests=[]
for a in actors:
 if isinstance(a,u.InstancedFoliageActor):
  for c in a.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent):forests.append({'mesh':c.get_editor_property('static_mesh').get_path_name(),'count':c.get_instance_count(),'lod':c.get_editor_property('forced_lod_model'),'cull_start':c.get_editor_property('instance_start_cull_distance'),'cull_end':c.get_editor_property('instance_end_cull_distance')})
check('saved woodland HISM instances',sum(c['count'] for c in forests)==7848,forests)
check('woodland review LOD/culling persisted',all(c['lod']==3 and c['cull_end']==1200000 for c in forests))
lands=[x for x in actors if isinstance(x,u.Landscape)];check('one 64-component Landscape',len(lands)==1 and len(lands[0].get_components_by_class(u.LandscapeComponent))==64)
if lands:
 material=lands[0].get_editor_property('landscape_material');check('owned native-surface material',material and material.get_path_name().startswith('/Game/SoulCampaignFoundation/MI_Mountain05_Auto_r1'),material.get_path_name() if material else None)
for label in ['Foundation_Scale_human_capital','Foundation_Scale_human_tavern','Foundation_Scale_dwarf_hold','Foundation_Scale_Army']:
 check('saved '+label,sum(a.get_actor_label()==label for a in actors)==1)
receipt={'map':w.get_path_name(),'all_pass':all(c['pass'] for c in checks),'checks':checks,'scope':'saved editor content only; not campaign travel, gameplay, save/load or performance qualification','mutated_assets':False}
(OUT/'fresh-editor-validation.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt));assert receipt['all_pass']
