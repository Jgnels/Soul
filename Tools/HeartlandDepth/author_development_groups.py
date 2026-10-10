"""Bind three native city roots to existing development state. Never save donors."""
import unreal,json,hashlib,shutil
from pathlib import Path
r=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
e=r/'Evidence/HumanHeartlandDepth-20261010/Local'
receipt=e/'authored-development-groups.json'
assert not receipt.exists(),'Inspect the existing receipt before another authoring pass'
package='/Game/Soul/Maps/Settlements/SL_HumanCapital_Houses'
path=r/'Content/Soul/Maps/Settlements/SL_HumanCapital_Houses.umap'
before=hashlib.sha256(path.read_bytes()).hexdigest()
backup=e/'SL_HumanCapital_Houses.before-depth.umap'
if backup.exists():assert hashlib.sha256(backup.read_bytes()).hexdigest()==before,'Previous attempt changed the owned map; inspect before retry'
else:shutil.copy2(path,backup)
assert unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_outermost().get_name()=='/Game/Soul/Maps/Settlements/L_HumanTavern_Inspection'
sub=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
prototype=sub.spawn_actor_from_class(unreal.SoulSettlementBuildingActor,unreal.Vector())
world=unreal.load_asset(package)
print('SOUL_DEPTH_GROUP_BEGIN',world.get_path_name())
rows=[]
for building,name,minimum in [('human.arcane_hall','LevelInstance_24',1),('human.barracks','LevelInstance_151',2),('human.market','LevelInstance_10',2)]:
    actor=unreal.find_object(None,world.get_path_name()+':PersistentLevel.'+name)
    assert actor and actor.get_outermost().get_name()==package
    print('SOUL_DEPTH_GROUP_DUPLICATE',building,name)
    group=sub.duplicate_actor(prototype,world,unreal.Vector())
    assert group and group.get_outermost().get_name()==package
    group.set_actor_label('Soul_HeartlandDepth_'+building.replace('.','_'))
    group.set_editor_property('settlement_id','human_capital')
    group.set_editor_property('building_id',building)
    group.set_editor_property('minimum_building_level',minimum)
    group.set_editor_property('follow_settlement_state',True)
    group.set_editor_property('intact_actors',[actor])
    group.apply_condition_name('Intact')
    rows.append({'building':building,'minimum_level':minimum,'root':actor.get_path_name(),'controller':group.get_path_name()})
assert unreal.EditorLoadingAndSavingUtils.save_map(world,package)
receipt.write_text(json.dumps({'map':package,'before_sha256':before,'after_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'backup':str(backup),'groups':rows,'donor_saved':False,'status':'authored; visual qualification pending'},indent=2))
print('SOUL_DEPTH_GROUPS_AUTHORED',len(rows))
