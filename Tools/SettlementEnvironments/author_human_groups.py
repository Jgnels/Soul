"""Bind reviewed native Human tavern roots in Soul-owned map packages.
Uses UE's existing explicit destination-world actor duplication, avoiding a
whole-city editor load. Donors remain unchanged. Runtime visual/input proof is
mandatory; this authoring receipt alone is not acceptance.
"""
import unreal, json, hashlib, shutil, datetime
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
e=root/'Evidence/SettlementEnvironmentPlan-20261005'
receipt=e/'Population-20261006/human-authored-groups-r1.json'
assert not receipt.exists()
city='/Game/Soul/Maps/Settlements/L_HumanCapital_Authored'
houses='/Game/Soul/Maps/Settlements/SL_HumanCapital_Houses'
props='/Game/Soul/Maps/Settlements/SL_HumanCapital_TownProps'
source_houses='/Game/CastleTown/Levels/SubLevels/SL_Houses'
source_props='/Game/CastleTown/Levels/SubLevels/SL_Town_Props'
def file(package): return root/'Content'/(package.removeprefix('/Game/')+'.umap')
def sha(p):
    with p.open('rb') as f: return hashlib.file_digest(f,'sha256').hexdigest()
before={p:sha(file(p)) for p in (source_houses,source_props)}
backup=e/'Local/HumanCapital_before_state.umap'
assert not backup.exists()
shutil.copy2(file(city),backup)
assert not unreal.EditorAssetLibrary.does_asset_exist(props)
original=unreal.load_asset(source_props)
assert unreal.EditorLoadingAndSavingUtils.save_map(original,props)
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Soul/Maps/Settlements/L_HumanTavern_Inspection')
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
prototype=sub.spawn_actor_from_class(unreal.SoulSettlementBuildingActor,unreal.Vector())
worlds={p:unreal.load_asset(p) for p in (city,houses,props)}
assert all(worlds.values())
selection=json.loads((e/'Population-20261006/human-tavern-footprint-independent-roots.json').read_text())
groups={city:[],houses:['LevelInstance_149'],props:[]}
for row in selection['roots']:
    destination=city if row['level']==city else props
    groups[destination].append(row['path'].rsplit('.',1)[1])
assert [len(groups[p]) for p in (city,houses,props)]==[10,1,13]
records=[]
for package,names in groups.items():
    world=worlds[package]
    actors=[unreal.find_object(None,world.get_path_name()+':PersistentLevel.'+name) for name in names]
    assert all(actors) and all(a.get_outermost().get_name()==package for a in actors)
    group=sub.duplicate_actor(prototype,world,unreal.Vector())
    assert group and group.get_outermost().get_name()==package, 'Explicit owned-world duplication failed'
    group.set_actor_label('Soul_HumanCapital_Tavern_'+package.rsplit('_',1)[1]+'_State')
    group.set_editor_property('settlement_id','human_capital')
    group.set_editor_property('building_id','human.tavern')
    group.set_editor_property('follow_settlement_state',True)
    group.set_editor_property('intact_actors',actors)
    group.apply_condition_name('Intact')
    records.append(dict(map=package,state_actor=group.get_name(),roots=names))
# Root-owned view/presentation adapters use the same classes as the first proof.
controller_template=sub.spawn_actor_from_class(unreal.SoulSettlementPresentationController,unreal.Vector())
controller=sub.duplicate_actor(controller_template,worlds[city],unreal.Vector())
assert controller and controller.get_outermost().get_name()==city
controller.set_actor_label('Soul_HumanCapital_Presentation')
controller.set_editor_property('settlement_id','human_capital')
controller.set_editor_property('authored_ev100_exposure',True)
camera_position=unreal.Vector(-7000,-1300,4500)
look=unreal.MathLibrary.find_look_at_rotation(camera_position,unreal.Vector(-4350,2700,900))
camera_template=sub.spawn_actor_from_class(unreal.SoulTownViewAnchor,camera_position,look)
camera=sub.duplicate_actor(camera_template,worlds[city],unreal.Vector())
assert camera and camera.get_outermost().get_name()==city
camera.set_actor_label('Soul_HumanCapital_Tavern_View')
camera.set_editor_property('settlement_id','human_capital')
camera.get_editor_property('camera').set_field_of_view(55)
worlds[city].get_world_settings().set_editor_property('default_game_mode',unreal.SoulSettlementVisitGameMode)
assert unreal.SoulSettlementAssetAuthoring.rebind_unloaded_owned_sublevel(worlds[city],source_houses,houses)
assert unreal.SoulSettlementAssetAuthoring.rebind_unloaded_owned_sublevel(worlds[city],source_props,props)
for package,world in worlds.items(): assert unreal.EditorLoadingAndSavingUtils.save_map(world,package)
assert before=={p:sha(file(p)) for p in before},'Donor changed'
result=dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),settlement='human_capital',building='human.tavern',
    groups=records,persistent_roots=24,expected_groups=3,
    classification=dict(starting='Complete authored city except one native tavern and its 23 independently placed local props/smoke',buildable='LI_Building_05_Fix root and its loaded children, ten root-world dressing/effect actors, thirteen town-prop roots',decoration='All remaining original city composition retained',incompatible='No additional donor actors removed'),
    native_tavern='/Game/CastleTown/Levels/LevelInstances/LI_Building_05_Fix',native_instance='SL_Houses:PersistentLevel.LevelInstance_149',
    donor_hashes=before,owned_hashes={p:sha(file(p)) for p in worlds},backup=str(backup),
    camera_position=[-7000,-1300,4500],camera_target=[-4350,2700,900],
    acceptance='Owned data binding only; absent/present render, real construction, save, visit, miniature and battle qualification still required')
receipt.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('SOUL_HUMAN_GROUPS_AUTHORED',records)
