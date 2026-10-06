"""Bound tiny native fire effects in the owned Citadel, preserving near detail.

Uses native component distance culling and the donor's existing one-second
inactivity suspension. No particle/material asset or gameplay state is edited.
Save only the Soul-owned map, with an exact recoverable package backup.
"""
import datetime
import hashlib
import json
import shutil
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
package = '/Game/Soul/Maps/Settlements/L_DwarfHold_Authored'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split('.')[0] == package
evidence = root/'Evidence/SettlementEnvironmentPlan-20261005'
receipt = evidence/'Continuation-20261006/dwarven-decorative-culling-r1.json'
backup = evidence/'Local/DwarfHold_before_decorative_culling.umap'
disk = root/'Content/Soul/Maps/Settlements/L_DwarfHold_Authored.umap'
assert not receipt.exists() and not backup.exists()
shutil.copy2(disk, backup)
rows = []
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for component in actor.get_components_by_class(unreal.ParticleSystemComponent):
        assert actor.get_path_name().startswith(package+'.'), 'Never modify donor levels'
        assert component.template.get_path_name() == '/Game/DwarvenCitadel/VFX/P_Fire.P_Fire'
        assert component.get_editor_property('seconds_before_inactive') == 1
        before = component.get_editor_property('ld_max_draw_distance')
        assert before == 0, 'Unexpected existing culling policy'
        actor.modify()
        component.modify()
        component.set_cull_distance(4000)
        rows.append(dict(actor=actor.get_name(), component=component.get_name(),
                         before_distance=before, after_distance=4000,
                         seconds_before_inactive=1))
assert len(rows) == 40
assert unreal.EditorLoadingAndSavingUtils.save_map(world, package)
receipt.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    map=package, before_sha256=hashlib.sha256(backup.read_bytes()).hexdigest(),
    after_sha256=hashlib.sha256(disk.read_bytes()).hexdigest(), components=rows,
    scope='tiny decorative fire particles draw within 40m; original near appearance and illumination retained',
    status='candidate; requires runtime image and performance comparison'), indent=2)+'\n')
print('SOUL_DWARF_DECORATIVE_CULLING', len(rows), str(receipt))
