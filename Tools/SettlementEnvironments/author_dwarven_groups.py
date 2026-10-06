"""Bind the inspected Citadel caravan-hall group to existing Soul presentation.

The carved enclosure, floor and perimeter architecture remain starting content.
The upgrade installs the authored inner colonnade and gathering-hall furnishings.
No donor package, gameplay state, economy or save schema is written here.
"""
import datetime
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
map_path = '/Game/Soul/Maps/Settlements/L_DwarfHold_Authored'
assert world.get_path_name().split('.')[0] == map_path
sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()
assert not any(isinstance(a, unreal.SoulSettlementBuildingActor) for a in actors)
out = root / 'Evidence/SettlementEnvironmentPlan-20261005/Continuation-20261006/dwarven-authored-groups.json'
assert not out.exists()
group = []
for actor in actors:
    actor.set_is_temporarily_hidden_in_editor(False)
    if not actor.get_path_name().startswith(map_path + '.'):
        continue
    # ChildActorComponent instances are regenerated in the runtime world and
    # must be reached through their persistent parent, not serialized directly.
    if actor.get_parent_actor():
        continue
    origin, extent = actor.get_actor_bounds(False)
    if not (2400 < origin.x < 6900 and 2800 < origin.y < 10300 and origin.z < 5000):
        continue
    cls = actor.get_class().get_name()
    if cls.startswith(('BPP_SideRibvault_05', 'BP_FireStand', 'BP_Candle', 'BP_Torch')):
        group.append(actor)
    elif isinstance(actor, unreal.StaticMeshActor):
        mesh = actor.static_mesh_component.static_mesh
        if mesh and mesh.get_name().startswith(('SM_PB_', 'SM_PR_', 'SM_Chest')):
            group.append(actor)
assert len(group) == 56, 'Reviewed persistent root membership changed; inspect before proceeding'
building = sub.spawn_actor_from_class(unreal.SoulSettlementBuildingActor, unreal.Vector(4580, 6120, 60))
building.set_actor_label('Soul_DwarfHold_CaravanHall_State')
building.set_editor_property('settlement_id', 'dwarf_hold')
building.set_editor_property('building_id', 'dwarf.caravan_hall')
building.set_editor_property('follow_settlement_state', True)
building.set_editor_property('intact_actors', group)
# Never serialize the preview's disabled collision into the initial actor data.
# Runtime visibility reads the canonical saved condition after world entry.
building.apply_condition_name('Intact')
controller = sub.spawn_actor_from_class(unreal.SoulSettlementPresentationController, unreal.Vector())
controller.set_actor_label('Soul_DwarfHold_Presentation')
controller.set_editor_property('settlement_id', 'dwarf_hold')
controller.set_editor_property('authored_ev100_exposure', True)
camera = next(a for a in actors if a.get_actor_label() == 'SideHall_Camera2')
anchor = sub.spawn_actor_from_class(unreal.SoulTownViewAnchor, camera.get_actor_location(), camera.get_actor_rotation())
anchor.set_actor_label('Soul_DwarfHold_CaravanHall_View')
anchor.set_editor_property('settlement_id', 'dwarf_hold')
anchor.get_editor_property('camera').set_field_of_view(70)
world.get_world_settings().set_editor_property('default_game_mode', unreal.SoulSettlementVisitGameMode)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, map_path)
result = dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), map=map_path,
    settlement_id='dwarf_hold', building_id='dwarf.caravan_hall',
    starting='original excavated shell, foundation, perimeter walls, adjoining halls, monumental gate and cliff approach',
    buildable='56 persistent authored root actors plus runtime children: inner colonnade, furnishings, chests, candles, braziers and torches',
    representation_scope='construction within an existing mountain hold; not a new freestanding exterior building',
    actor_group=[dict(name=a.get_name(), label=a.get_actor_label(), actor_class=a.get_class().get_name()) for a in group],
    state_actor=building.get_name(), camera_actor=anchor.get_name(),
    acceptance='authored group binding only; actual purchase/visit/save/battle and miniature still require runtime proof')
out.write_text(json.dumps(result, indent=2) + '\n')
print('SOUL_DWARF_AUTHORED_GROUPS', str(out), len(group))
