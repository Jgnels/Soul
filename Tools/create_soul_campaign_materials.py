"""Generate only project-owned campaign materials; never saves donor packages."""
import unreal
ROOT = '/Game/Soul/Campaign'
for name, vertex in [('M_CampaignSurface', False), ('M_CampaignTerrain', True)]:
    path = ROOT + '/' + name
    material = unreal.load_asset(path)
    if material:
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    else:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVertexColor if vertex else unreal.MaterialExpressionVectorParameter)
    if not vertex:
        color.set_editor_property('parameter_name', 'Tint')
        color.set_editor_property('default_value', unreal.LinearColor(0.3, 0.4, 0.2, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant)
    rough.set_editor_property('r', 0.92)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('used_with_instanced_static_meshes', True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
unreal.log('SOUL_CAMPAIGN_MATERIALS_CREATED')
