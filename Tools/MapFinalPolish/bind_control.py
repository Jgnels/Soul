"""Separate Soul-owned material binding for final road-only polish."""
import unreal as u,json,hashlib
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
root='/Game/SoulCampaignComposition/FinalPolish';path=root+'/T_RoadControl_final1'
p=u.ImportAssetParameters();p.is_automated=True;p.destination_name='T_RoadControl_final1';p.replace_existing=False
src=u.InterchangeManager.create_source_data(str(E/'Local/Composition_Polish_Control_final1.png'));u.InterchangeManager.get_interchange_manager_scripted().import_asset(root,src,p);tex=u.load_asset(path);assert tex
tex.set_editor_property('srgb',False);tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS);tex.set_editor_property('address_x',u.TextureAddress.TA_CLAMP);tex.set_editor_property('address_y',u.TextureAddress.TA_CLAMP)
mat=u.EditorAssetLibrary.duplicate_asset('/Game/SoulCampaignComposition/Polish/M_Polish_r1',root+'/M_FinalRoads_r1');mi=u.EditorAssetLibrary.duplicate_asset('/Game/SoulCampaignComposition/Polish/MI_Polish_r1',root+'/MI_FinalRoads_r1');assert mat and mi
node=u.find_object(None,mat.get_path_name()+':MaterialExpressionTextureSample_2');assert node;node.set_editor_property('texture',tex);u.MaterialEditingLibrary.set_material_instance_parent(mi,mat);u.MaterialEditingLibrary.recompile_material(mat);u.MaterialEditingLibrary.update_material_instance(mi)
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(actor,u.Landscape):actor.set_editor_property('landscape_material',mi)
for asset in [tex,mat,mi]:u.EditorAssetLibrary.save_loaded_asset(asset)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'road-binding.json').write_text(json.dumps(dict(texture=path,material=mat.get_path_name(),instance=mi.get_path_name(),heightfield_changed=False,previous_material_preserved=True,sha256=hashlib.sha256((E/'Local/Composition_Polish_Control_final1.png').read_bytes()).hexdigest()),indent=2));print('FINAL_ROAD_BINDING_SAVED')
