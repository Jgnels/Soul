"""Candidate native shadow resolution budget for small owned-map fixtures.

Preserves every light, shadow, intensity, color and attenuation radius. Large
architectural lights retain full resolution. Donor packages are never saved.
Requires rendered and native-resolution performance comparison before acceptance.
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
receipt = evidence/'Continuation-20261006/dwarven-shadow-budget-r1.json'
backup = evidence/'Local/DwarfHold_before_shadow_budget.umap'
disk = root/'Content/Soul/Maps/Settlements/L_DwarfHold_Authored.umap'
assert not receipt.exists() and not backup.exists()
shutil.copy2(disk, backup)
rows = []
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    for component in actor.get_components_by_class(unreal.LocalLightComponent):
        assert actor.get_path_name().startswith(package+'.'), 'Never edit donor levels'
        radius = component.get_editor_property('attenuation_radius')
        if not component.get_editor_property('cast_shadows') or radius > 3000:
            continue
        before = component.get_editor_property('shadow_resolution_scale')
        assert before == 1.0
        actor.modify()
        component.modify()
        component.set_editor_property('shadow_resolution_scale', 0.5)
        rows.append(dict(actor=actor.get_name(), label=actor.get_actor_label(),
            radius=radius, before=before, after=0.5))
assert rows
assert unreal.EditorLoadingAndSavingUtils.save_map(world, package)
receipt.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    map=package, before_sha256=hashlib.sha256(backup.read_bytes()).hexdigest(),
    after_sha256=hashlib.sha256(disk.read_bytes()).hexdigest(), lights=rows,
    status='candidate; all shadows retained; native image/performance review required'), indent=2)+'\n', encoding='utf-8')
print('SOUL_DWARF_SHADOW_BUDGET', len(rows))
