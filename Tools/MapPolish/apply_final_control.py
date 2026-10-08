"""Bind the final road-only mask to Soul-owned polish materials; no terrain edit."""
import unreal as u,json,hashlib
from pathlib import Path
R=Path('D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929')
E=R/'Evidence/MapPolish-20261007'
w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert w.get_path_name().startswith('/Game/SoulCampaignComposition/L_Composition_3500_r2')
path='/Game/SoulCampaignComposition/Polish/T_Polish_Control_r4'
tex=u.load_asset(path)
if not tex:
    p=u.ImportAssetParameters();p.is_automated=True;p.destination_name='T_Polish_Control_r4';p.replace_existing=False
    src=u.InterchangeManager.create_source_data(str(E/'Local/Composition_Polish_Control_r4.png'))
    u.InterchangeManager.get_interchange_manager_scripted().import_asset('/Game/SoulCampaignComposition/Polish',src,p)
    tex=u.load_asset(path)
assert tex
tex.set_editor_property('srgb',False)
tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_MASKS)
tex.set_editor_property('address_x',u.TextureAddress.TA_CLAMP);tex.set_editor_property('address_y',u.TextureAddress.TA_CLAMP)
mat=u.load_asset('/Game/SoulCampaignComposition/Polish/M_Polish_r1')
mi=u.load_asset('/Game/SoulCampaignComposition/Polish/MI_Polish_r1')
node=u.find_object(None,mat.get_path_name()+':MaterialExpressionTextureSample_2')
assert node
node.set_editor_property('texture',tex)
u.MaterialEditingLibrary.recompile_material(mat);u.MaterialEditingLibrary.update_material_instance(mi)
for asset in [tex,mat,mi]:u.EditorAssetLibrary.save_loaded_asset(asset)
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
(E/'final-control-binding.json').write_text(json.dumps(dict(texture=path,material=mat.get_path_name(),instance=mi.get_path_name(),map=w.get_path_name(),control_sha256=hashlib.sha256((E/'Local/Composition_Polish_Control_r4.png').read_bytes()).hexdigest(),heightfield_changed=False),indent=2))
print('POLISH_FINAL_CONTROL_SAVED')
