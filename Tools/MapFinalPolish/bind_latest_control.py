"""Bind a new road-only mask revision into this pass's isolated materials."""
import unreal as u,json,hashlib
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929');E=R/'Evidence/MapFinalPolish-20261007';revision=globals().get('ROAD_REVISION','final4');root='/Game/SoulCampaignComposition/FinalPolish';name='T_RoadControl_'+revision
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
tex=u.load_asset(root+'/'+name)
if not tex:
 p=u.ImportAssetParameters();p.is_automated=True;p.destination_name=name;p.replace_existing=False;src=u.InterchangeManager.create_source_data(str(E/('Local/Composition_Polish_Control_'+revision+'.png')));u.InterchangeManager.get_interchange_manager_scripted().import_asset(root,src,p);tex=u.load_asset(root+'/'+name)
assert tex
tex.set_editor_property('srgb',False);tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS);tex.set_editor_property('address_x',u.TextureAddress.TA_CLAMP);tex.set_editor_property('address_y',u.TextureAddress.TA_CLAMP)
mat=u.load_asset(root+'/M_FinalRoads_r1');mi=u.load_asset(root+'/MI_FinalRoads_r1');u.find_object(None,mat.get_path_name()+':MaterialExpressionTextureSample_2').set_editor_property('texture',tex);u.MaterialEditingLibrary.recompile_material(mat);u.MaterialEditingLibrary.update_material_instance(mi)
for asset in [tex,mat,mi]:u.EditorAssetLibrary.save_loaded_asset(asset)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'road-binding-final.json').write_text(json.dumps(dict(texture=tex.get_path_name(),material=mat.get_path_name(),instance=mi.get_path_name(),revision=revision,sha256=hashlib.sha256((E/('Local/Composition_Polish_Control_'+revision+'.png')).read_bytes()).hexdigest(),heightfield_changed=False),indent=2));print('ROAD_BOUND',revision)
