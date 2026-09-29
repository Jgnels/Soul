"""Run via Nwiro execute_python after every heightmap create/reimport.

Nwiro modify recreates the Landscape and resets dynamic material settings.
Do not save vendor packages, editor defaults or the Entry map.
"""
import unreal as u
ROOT='/Game/Soul/Campaign/TerrainV2'
assert str(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_outer().get_name())==ROOT+'/L_FounderTerrain'
m=u.load_asset(ROOT+'/M_FounderLandscape')
m.set_editor_property('used_with_static_lighting',True)
errors=u.MaterialEditingLibrary.recompile_material(m)
assert not errors, errors
u.EditorAssetLibrary.save_loaded_asset(m)
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
 if isinstance(a,u.LandscapeProxy):
  a.set_editor_property('use_dynamic_material_instance',True)
  # Nwiro assigns the pointer directly. The reflected editor setter also rebuilds
  # the Landscape component material instances and their shader permutations.
  a.call_method('EditorSetLandscapeMaterial',(m,))
assert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()
u.log('SOUL_TERRAIN_IMPORT_FINALIZED')
