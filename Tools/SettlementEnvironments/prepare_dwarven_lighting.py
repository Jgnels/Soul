"""Make the owned Citadel's twelve unbaked stationary lights fully dynamic.

The donor already uses 277 movable lights. This completes that lighting model
only in the owned wrapper, without changing intensities, colors or shadows.
Native distance fading bounds small local lights well beyond their lit surfaces.
"""
from pathlib import Path
import datetime
import hashlib
import json
import shutil
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
package = '/Game/Soul/Maps/Settlements/L_DwarfHold_Authored'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split('.')[0] == package
evidence = root / 'Evidence/SettlementEnvironmentPlan-20261005'
receipt = evidence/'Continuation-20261006/dwarven-dynamic-lighting-r1.json'
backup = evidence/'Local/DwarfHold_before_dynamic_lighting.umap'
assert not receipt.exists() and not backup.exists()
disk = root/'Content/Soul/Maps/Settlements/L_DwarfHold_Authored.umap'
shutil.copy2(disk, backup)
lights = [(a, c) for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
          for c in a.get_components_by_class(unreal.LightComponent)]
assert len(lights) == 289
assert all(a.get_path_name().startswith(package+'.') for a,c in lights)
stationary = [(a,c) for a,c in lights if c.mobility == unreal.ComponentMobility.STATIONARY]
assert len(stationary) == 12
rows = []
for actor, component in lights:
    before = dict(mobility=str(component.mobility))
    if isinstance(component, unreal.LocalLightComponent):
        radius = component.get_editor_property('attenuation_radius')
        before['max_draw_distance'] = component.get_editor_property('max_draw_distance')
        before['max_distance_fade_range'] = component.get_editor_property('max_distance_fade_range')
        # Big architectural lights retain their native unlimited reach. Only
        # fixtures with <=20m influence fade past at least 50m from the camera.
        if radius <= 2000 and before['max_draw_distance'] == 0:
            component.set_editor_property('max_draw_distance', max(5000, radius*4))
            component.set_editor_property('max_distance_fade_range', 1000)
    if component.mobility == unreal.ComponentMobility.STATIONARY:
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
    rows.append(dict(actor=actor.get_name(), component=component.get_name(), before=before,
        mobility=str(component.mobility), max_draw_distance=component.get_editor_property('max_draw_distance')
        if isinstance(component, unreal.LocalLightComponent) else None))
assert unreal.EditorLoadingAndSavingUtils.save_map(world, package)
receipt.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    map=package, before_sha256=hashlib.sha256(backup.read_bytes()).hexdigest(),
    after_sha256=hashlib.sha256(disk.read_bytes()).hexdigest(), lights=rows,
    status='owned-map native dynamic lighting; rendered/performance qualification pending'),indent=2)+'\n')
print('SOUL_DWARF_DYNAMIC_LIGHTING', len(stationary), str(receipt))
