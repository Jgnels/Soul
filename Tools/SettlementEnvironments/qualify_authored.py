"""Launch the existing guarded UE runner for authored-city proof or fresh load.

No build, donor edit, player-save overwrite, purchase or automatic art acceptance.
An isolated UserDir may reuse the project's asset-registry cache: this is warm
asset discovery, while the game process and gameplay state are newly created.
"""
import argparse
import datetime
import hashlib
import json
import re
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
evidence = root / 'Evidence/SettlementEnvironmentPlan-20261005'
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('mode', choices=('functional', 'fresh-performance', 'battle-restored'))
p.add_argument('--run', required=True)
p.add_argument('--settlement', choices=('dwarf', 'human'), default='dwarf')
p.add_argument('--restore-checkpoint', type=Path)
p.add_argument('--expected-campaign', type=Path)
p.add_argument('--expected-settlement', type=Path)
p.add_argument('--gpu-profile', action='store_true', help='Profile the warmed city GPU after its frame measurement, before return')
a = p.parse_args()
human = a.settlement == 'human'
family = 'HumanCapital' if human else 'DwarfHold'
recipe_name = 'CrownsteadDevelopmentProof' if human else 'DwarfHoldDevelopmentProof'
assert not a.gpu_profile or a.mode == 'fresh-performance'
assert a.run and all(c.isalnum() or c in '-_' for c in a.run)
run = evidence / 'Local' / a.run
assert not run.exists(), 'Never reuse a qualification run or player save'
if a.mode != 'functional':
    for source in (a.restore_checkpoint, a.expected_campaign, a.expected_settlement):
        assert source and source.is_file() and source.resolve().is_relative_to(evidence.resolve()), 'Use an existing isolated evidence checkpoint'
else:
    assert not any((a.restore_checkpoint, a.expected_campaign, a.expected_settlement))
user = run / 'User'
user.mkdir(parents=True)
receipt = dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), mode=a.mode, settlement=a.settlement,
    scope='input/visibility/save/travel observation; review actual screenshots separately', inputs=[], cache_seed=[])

def record(path):
    return dict(path=str(path), bytes=path.stat().st_size,
        sha256=hashlib.file_digest(path.open('rb'), 'sha256').hexdigest())

# Copy only the current cache references and their named binaries, never stale
# orphan files or credentials/configuration. Unreal validates the cache on load.
cache = root / 'Intermediate/CachedAssetRegistry'
target = user / 'Intermediate/CachedAssetRegistry'
for ref in sorted(cache.glob('*.ref')):
    name = ref.read_text(encoding='utf-8-sig').strip()
    assert Path(name).name == name and name.endswith('.bin')
    source = cache / name
    if not source.is_file(): continue
    target.mkdir(parents=True, exist_ok=True)
    for file in (source, ref):
        shutil.copy2(file, target/file.name)
        receipt['cache_seed'].append(record(target/file.name))
if a.mode != 'functional':
    saved = user / 'Saved'
    destinations = (
        (a.restore_checkpoint, saved/'RBSave/Domains/Soul.VerticalCampaign.domain.rbsave'),
        (a.expected_campaign, saved/'expected_Soul.Campaign.json'),
        (a.expected_settlement, saved/'expected_Soul.Settlements.json'))
    for source, destination in destinations:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        receipt['inputs'].append(dict(source=record(source), copied=record(destination)))

files = [
    'Source/Soul/Private/SoulAuthoredSettlementQualification.cpp',
    'Source/Soul/Public/SoulAuthoredSettlementQualification.h',
    'Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp',
    'Source/Soul/Private/SoulSettlementBuildingActor.cpp',
    'Source/Soul/Private/SoulSettlementPresentationController.cpp',
    'Source/Soul/Private/SoulSettlementVisitGameMode.cpp',
    'Source/Soul/Private/SoulPlaytestRegionActor.cpp',
    'Source/Soul/Private/SoulCampaignWorldActor.cpp',
    'Source/Soul/Private/SoulCampaignTerrain.cpp',
    f'Data/SettlementEnvironments/{family}RuntimeProof.json',
    f'Data/SettlementEnvironments/{recipe_name}.json',
    f'Content/Soul/Data/Settlements/DA_Soul_{family}_DevelopmentProof.uasset',
    f'Content/Soul/Maps/Settlements/L_{family}_Authored.umap',
    'Binaries/Win64/UnrealEditor-Soul.dll',
    'Binaries/Win64/UnrealEditor-SoulRealtimeBattle.dll',
    'Source/SoulRealtimeBattle/Private/SoulRealtimeBattleArena.cpp',
    'Source/SoulRealtimeBattle/Public/SoulRealtimeBattleArena.h',
]
if human:
    files += ['Content/Soul/Maps/Settlements/SL_HumanCapital_Houses.umap',
              'Content/Soul/Maps/Settlements/SL_HumanCapital_TownProps.umap',
              'Content/Soul/Maps/Settlements/SL_HumanCapital_Waterfront.umap']
recipe = json.loads((root/f'Data/SettlementEnvironments/{recipe_name}.json').read_text())
for key in ('miniature_base_mesh', 'miniature_upgrade_mesh'):
    path = recipe[key]
    assert path.startswith('/Game/Soul/CampaignProxies/') and '..' not in path
    files.append('Content/'+path.removeprefix('/Game/')+'.uasset')
receipt['payload'] = [record(root/file) for file in files]
command = [sys.executable, str(root/'Tools/qualify_soul_vertical.py'),
    '--ue-exe', 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
    '--project', str(root/'Soul.uproject'), '--stage', 'G0' if a.mode == 'fresh-performance' else 'G6',
    '--expected-active-units', '0' if a.mode == 'fresh-performance' else '30',
    '--map-url', '/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode',
    # Native performance observer removes this cap for each complete warmup
    # and measurement window, verifies zero/VSync off, then restores it during
    # travel. Do not spend the thermal budget rendering loading UI uncapped.
    '--resolution', '1920x1080', '--max-fps', '20',
    '--diagnostic-rhi', 'd3d11', '--duration', '60', '--startup-timeout', '600', '--completion-timeout', '7200' if human else '900',
    '--completion-marker', 'SOUL_AUTHORED_FRESH_LOAD_PASS' if a.mode == 'fresh-performance' else 'SOUL_AUTHORED_SETTLEMENT_PASS',
    '--output', str(run/'runtime')]
flags = ['-ForceRes', '-RenderOffscreen', '-unattended', '-nosound',
    '-DisablePlugins=AndroidFileServer,NwiroIntegrationKit', '-EnablePlugins=HDRIBackdrop',
    '-DDC=InstalledNoZenLocalFallback', '-SoulMesaTerrain', '-SoulEvilCorridor',
    '-SoulHumanSettlementProof' if human else '-SoulDwarfSettlementProof', '-SoulAuthoredSettlementQualification', '-SoulAutobattle',
    '-SoulActivePerSide=15', '-SoulCampaignCapturePrefix='+a.run, '-UserDir='+user.as_posix(),
    # English source-content qualification does not exercise translated package
    # remapping. The installed CoreUObject documents this startup optimization;
    # apply only to this process, never protected project configuration.
    '-ini:Engine:[SystemSettings]:r.AntiAliasingMethod=2,[SystemSettings]:r.Streaming.PoolSize=1600,[SystemSettings]:localization.EnablePackageRemapping=0,[SystemSettings]:r.ScreenPercentage=100,[SystemSettings]:r.SecondaryScreenPercentage.GameViewport=100,[SystemSettings]:r.DynamicRes.OperationMode=0']
if a.mode != 'functional': flags.append('-SoulAuthoredSettlementFreshLoad')
if a.mode == 'battle-restored': flags.append('-SoulAuthoredSettlementBattleResume')
if a.gpu_profile: flags.append('-SoulAuthoredGPUProfile')
command += ['--ue-arg='+flag for flag in flags]
receipt['command'] = command
(run/'source-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
result = subprocess.run(command, cwd=root, check=False)
if result.returncode == 0 and a.mode != 'fresh-performance':
    # A completed battle/result loop alone missed an authored-floor height
    # defect: all allied reserve entries could fail while enemy waves arrived.
    # This fixed 45/30, cap-15 fixture must demonstrate both real reserve paths.
    log = (run/'runtime/unreal.log').read_text(encoding='utf-8-sig', errors='replace')
    waves = re.findall(r'SOUL_RT_REINFORCEMENT_WAVE: side=(\d) bodies=(\d+) wave=(\d+)', log)
    counts = [sum(int(bodies) for side,bodies,wave in waves if int(side)==s) for s in (0,1)]
    gate = dict(bodies_arrived=counts, both_sides_observed=all(n>0 for n in counts),
        scope='actual existing reinforcement events in this fixed authored-environment fixture')
    (run/'reinforcement-gate.json').write_text(json.dumps(gate,indent=2)+'\n')
    print(json.dumps(gate), flush=True)
    if not gate['both_sides_observed']: sys.exit(2)
sys.exit(result.returncode)
