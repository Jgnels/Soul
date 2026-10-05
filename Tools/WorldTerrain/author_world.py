"""Drive the admitted live Nwiro editor; mutate only owned world packages."""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Tools/MountainTerrain'))
from nwiro_client import call
EXTERNAL = Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/WorldTerrain')
EVIDENCE = ROOT/'Evidence/CampaignWorldTerrain-20261005'
ASSET = '/Game/SoulCampaignWorld'
p=argparse.ArgumentParser()
p.add_argument('operation',choices=['import','textures','dress','inventory','state','close'])
p.add_argument('--reimport',action='store_true')
p.add_argument('--diagnostic',action='store_true',help='Import an explicitly unqualified study for rendered inspection; never grants acceptance')
args=p.parse_args()

def invoke(name,arguments):
    result=call(name,arguments,600)
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%f')
    path=EVIDENCE/(stamp+'-'+name+'.json')
    path.write_text(json.dumps(result,indent=2))
    body=result.get('result',{})
    if result.get('error') or body.get('isError'):
        raise RuntimeError(json.dumps(result))
    for item in body.get('content',[]):
        if item.get('type')=='text':
            print(item['text'][:6000],flush=True)
            try:
                data=json.loads(item['text'])
            except ValueError:
                continue
            if isinstance(data,dict) and data.get('success') is False:
                raise RuntimeError(item['text'])
    return result

def py(code):return invoke('execute_python',{'code':code,'execution_mode':'live'})

def ensure_material(name):
    if (ROOT/'Content/SoulCampaignWorld'/(name+'.uasset')).exists():return
    result=py("import unreal as u\nprint('WORLD_ASSET_EXISTS',u.EditorAssetLibrary.does_asset_exist('"+ASSET+'/'+name+"'))")
    exists=any('WORLD_ASSET_EXISTS True' in item.get('text','')
               for item in result.get('result',{}).get('content',[]))
    if not exists:invoke('create_material',{'name':name,'path':ASSET})
    py("import unreal as u\nm=u.load_asset('"+ASSET+'/'+name+"')\nassert m\nassert u.EditorAssetLibrary.save_loaded_asset(m)")

invoke('get_project_info',{})
if args.operation=='state':
    py('import unreal as u\nprint(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_path_name())\nprint([p.get_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_map_packages()])')
elif args.operation=='inventory':
    py((ROOT/'Tools/WorldTerrain/asset_manifest.py').read_text())
elif args.operation=='import':
    profile=json.loads((ROOT/'Data/CampaignWorldTerrain/presentation.json').read_text())
    report=json.loads((EXTERNAL/'qualification.json').read_text())
    assert report['status']=='pass' or args.diagnostic,'Rejected terrain requires explicit --diagnostic for study import'
    height_hash=hashlib.sha256((EXTERNAL/'WorldHeight.r16').read_bytes()).hexdigest()
    assert height_hash==profile['height_sha256']==report['height_sha256']
    t=profile['terrain']; assert t['resolution']==2033
    filename=EXTERNAL/'WorldHeight.png'; assert filename.exists(),filename
    if args.reimport:
        py("import unreal as u\nassert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/SoulCampaignWorld/L_SoulWorld')")
    else:
        py("import unreal as u\nassert not u.EditorAssetLibrary.does_asset_exist('/Game/SoulCampaignWorld/L_SoulWorld')\nassert u.get_editor_subsystem(u.LevelEditorSubsystem).new_level('/Game/SoulCampaignWorld/L_SoulWorld')")
    invoke('create_landscape',{'heightmapPath':str(filename),'sectionSize':127,
        'numSubsections':2,'componentCount':8,'scale':[500,500,512],
        'location':[-508000,-508000,0],'editLayers':False,
        'modify':args.reimport,'landscapeName':'SoulWorldLandscape'})
    py("import unreal as u\nassert u.get_editor_subsystem(u.LevelEditorSubsystem).save_current_level()")
    receipt={'height_sha256':height_hash,'profile_sha256':hashlib.sha256((ROOT/'Data/CampaignWorldTerrain/presentation.json').read_bytes()).hexdigest(),
             'map_sha256':hashlib.sha256((ROOT/'Content/SoulCampaignWorld/L_SoulWorld.umap').read_bytes()).hexdigest(),'terrain':t,
             'offline_status':report['status'],'diagnostic_only':args.diagnostic,'runtime_qualified':False}
    (EXTERNAL/'geometry-import.json').write_text(json.dumps(receipt,indent=2))
elif args.operation=='textures':
    ensure_material('M_WorldMaskCopy')
    py((ROOT/'Tools/WorldTerrain/import_masks.py').read_text())
elif args.operation=='dress':
    for name in ['M_WorldGround','M_WorldWater','M_WorldRoad','M_WorldMasonry']:
        ensure_material(name)
    py((ROOT/'Tools/WorldTerrain/dress_world.py').read_text())
elif args.operation=='close':
    py('import unreal as u\nu.SystemLibrary.quit_editor()')
