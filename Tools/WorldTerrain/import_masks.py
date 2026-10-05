"""Factory-free texture authoring via UE's supported render-target conversion.

The ordinary factory path crashes on this machine. Use the editor-exposed
RenderingLibrary conversion instead. Only owned assets
are written. M_WorldMaskCopy must first be created with create_material.
"""
import json
import hashlib
import shutil
import time
from pathlib import Path
import unreal as u

ROOT='/Game/SoulCampaignWorld'
DATA=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/WorldTerrain')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
lib=u.MaterialEditingLibrary
m=u.load_asset(ROOT+'/M_WorldMaskCopy');assert m
lib.delete_all_material_expressions(m)
m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
sample=lib.create_material_expression(m,u.MaterialExpressionTextureSampleParameter2D)
sample.set_editor_property('parameter_name','Mask')
fallback=u.load_asset(ROOT+'/T_MaskCopyDefault')
if not fallback:
    tiny=u.RenderingLibrary.create_render_target2d(world,16,16,u.TextureRenderTargetFormat.RTF_RGBA8)
    u.RenderingLibrary.clear_render_target2d(world,tiny,u.LinearColor(1,1,1,1))
    fallback=u.RenderingLibrary.render_target_create_static_texture2d_editor_only(tiny,ROOT+'/T_MaskCopyDefault')
fallback.set_editor_property('srgb',False)
assert u.EditorAssetLibrary.save_loaded_asset(fallback)
sample.set_editor_property('texture',fallback)
sample.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
assert lib.connect_material_property(sample,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
errors=lib.recompile_material(m);assert not errors,list(errors)
u.AutomationLibrary.finish_loading_before_screenshot()
receipt=[]
for name in ['WorldMacro','WorldBiome','WorldRegions','WorldDetail','WorldCultivation']:
    source=u.RenderingLibrary.import_file_as_texture2d(world,str(DATA/(name+'.png')))
    assert source,name
    source.set_editor_property('srgb',False)
    instance=u.MaterialLibrary.create_dynamic_material_instance(world,m)
    instance.set_texture_parameter_value('Mask',source)
    rt=u.RenderingLibrary.create_render_target2d(world,2033,2033,u.TextureRenderTargetFormat.RTF_RGBA16F)
    u.RenderingLibrary.draw_material_to_render_target(world,rt,instance)
    target=ROOT+'/T_'+name
    texture=u.load_asset(target)
    if texture:
        # Preserve the saved owned derivative outside Content, then update its
        # pixels in place. Renaming would leave a redirector at the live path.
        previous=DATA.parent/'Content/SoulCampaignWorld'/('T_'+name+'.uasset')
        archive=DATA/'rejected'/('textures-'+str(time.time_ns()))
        archive.mkdir(parents=True)
        assert previous.is_file(),previous
        shutil.copy2(previous,archive/previous.name)
        u.RenderingLibrary.convert_render_target_to_texture2d_editor_only(world,rt,texture)
    else:
        texture=u.RenderingLibrary.render_target_create_static_texture2d_editor_only(
            rt,target,u.TextureCompressionSettings.TC_DEFAULT,u.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    assert texture and texture.get_path_name().startswith(target+'.')
    # This supported conversion stores floating-point source data, for which UE
    # disables the hardware sRGB flag on save. The material explicitly decodes
    # the macro palette; region/biome channels remain linear data.
    texture.set_editor_property('srgb',False)
    texture.set_editor_property('address_x',u.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y',u.TextureAddress.TA_CLAMP)
    assert u.EditorAssetLibrary.save_loaded_asset(texture)
    observations=[]
    for x,y in [(250,250),(1016,1016),(1650,1650)]:
        c=u.RenderingLibrary.read_render_target_raw_pixel(world,rt,x,y,False)
        observations.append({'xy':[x,y],'rgb':[c.r,c.g,c.b]})
    receipt.append({'name':name,'path':texture.get_path_name(),'pixels':observations,
                    'srgb':False,'palette_decode':'material IEC sRGB' if name=='WorldMacro' else 'none; linear mask',
                    'source_sha256':hashlib.sha256((DATA/(name+'.png')).read_bytes()).hexdigest()})
assert u.EditorAssetLibrary.save_loaded_asset(m)
(DATA/'texture-import-receipt.json').write_text(json.dumps(receipt,indent=2))
print('SOUL_WORLD_TEXTURES_IMPORTED '+json.dumps(receipt))
