import sys,json
from pathlib import Path
from nwiro_client import call
root=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview')
variant=sys.argv[1] if len(sys.argv)>1 else 'plains_v1'
plan=json.loads((root/'TerrainWork/terrain-plan.json').read_text())
map_path='/Game/SoulCampaignMountain/L_'+variant
r=call('execute_python',{'code':"import unreal\nunreal.get_editor_subsystem(unreal.LevelEditorSubsystem).new_level('"+map_path+"')"});print(r)
r=call('create_landscape',{'heightmapPath':str(root/'TerrainWork'/(variant+'.png')),'sectionSize':255,'numSubsections':1,'componentCount':8,'scale':[plan['xy_scale_cm'],plan['xy_scale_cm'],64],'location':plan['location_cm'],'editLayers':False,'landscapeName':'SoulMountainPlains'},600)
(root/'Evidence'/('import-'+variant+'.json')).write_text(json.dumps(r,indent=2));print(r)
