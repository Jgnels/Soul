"""Import the final bounded road mask into candidate-owned materials only."""
import unreal as u,json
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/ProductionWorldComposition-20261007';A='/Game/SoulCampaignComposition'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith(A+'/L_Composition_3500_r2')
p=u.ImportAssetParameters();p.is_automated=True;p.destination_name='T_Composition_Control_r5';p.replace_existing=False
src=u.InterchangeManager.create_source_data(str(R/'Data/CampaignCompositionLocal/Composition_Control_r5.png'))
assets=u.InterchangeManager.get_interchange_manager_scripted().import_asset(A+'/Controls',src,p);assert assets;tex=assets[0]
for name,value in [('srgb',False),('compression_settings',u.TextureCompressionSettings.TC_MASKS),('address_x',u.TextureAddress.TA_CLAMP),('address_y',u.TextureAddress.TA_CLAMP)]:tex.set_editor_property(name,value)
assert u.EditorAssetLibrary.save_loaded_asset(tex,False)
m=u.load_asset(A+'/M_Composition_Review_r3');n=u.find_object(m,'MaterialExpressionTextureSample_2');n.set_editor_property('texture',tex);u.MaterialEditingLibrary.recompile_material(m);u.EditorAssetLibrary.save_loaded_asset(m,False)
mi=u.load_asset(A+'/MI_Composition_Review_r4');u.MaterialEditingLibrary.update_material_instance(mi);u.EditorAssetLibrary.save_loaded_asset(mi,False)
actors={a.get_actor_label():a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
row=next(a for a in json.loads((E/'route-local-repair-study-r7.json').read_text())['anchors'] if a['id']=='orc_broken_bridge');x,y=[v*100-175000 for v in row['xy_m']]
h=u.SystemLibrary.line_trace_single(w,u.Vector(x,y,100000),u.Vector(x,y,-100000),u.TraceTypeQuery.ECC_VISIBILITY,False,[],u.DrawDebugTrace.NONE).to_dict();assert h.get('blocking_hit')
actors['Composition_Anchor_orc_broken_bridge'].set_actor_location(u.Vector(x,y,h['impact_point'].z),False,False)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'final-presentation-binding.json').write_text(json.dumps({'map':w.get_path_name(),'material':mi.get_path_name(),'control':tex.get_path_name(),'routes':'route-local-repair-study-r7.json','bridge_receipt':'bridge-placement-r2.json','water_receipts':['lake-shore-refinement-r2.json','river-overlap-refinement-r2.json'],'runtime_bound':False,'production_promoted':False},indent=2));print('FINAL_CANDIDATE_CONTROL')
