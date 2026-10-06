"""Bounded local editor session for owned campaign-world asset authoring.

Shares the existing qualification telemetry and 85 C cutoff. Never starts a
second Unreal session, edits configuration, or kills another worker's process.
Create OUTPUT/stop to close this child after authoring through Nwiro.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from host_commit import system_commit_sample

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from qualify_soul_vertical import (gpu_sample, process_memory_sample,
                                  conflicting_processes, close_owned_process, utc)

p = argparse.ArgumentParser()
p.add_argument('--project', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--minutes', type=int, default=45)
p.add_argument('--max-private-mib', type=float, default=20000,
               help='Stop this owned editor before cold asset builds exhaust system commit')
p.add_argument('--min-system-commit-mib', type=float, default=4096,
               help='Keep this much host commit headroom; never alter the paging file')
p.add_argument('--rhi', choices=('d3d11', 'd3d12'), default='d3d11')
p.add_argument('--enable-plugin', action='append', default=[])
p.add_argument('--exec-command', action='append', default=[])
p.add_argument('--ini-override', action='append', default=[],
               help='Process-local Unreal ini override, e.g. Engine:[section]:key=value; does not edit project configuration')
p.add_argument('--sm6', action='store_true', help='Request Shader Model 6 for a DX12 donor qualification')
p.add_argument('--world-profile',action='store_true',help='Select the full-world adapter for native automation in this authoring session')
p.add_argument('--runtime-environment-survey', choices=('human',),
               help='Read-only native game load of the intact Human wrapper, without the editor UI or campaign binding')
args = p.parse_args()
assert not conflicting_processes(), 'Another Unreal/Soul session is active'
out = args.output.resolve()
assert not out.exists(), 'Use a new evidence directory'
out.mkdir(parents=True)
gpu = shutil.which('nvidia-smi')
assert gpu and max(x['temperature_c'] for x in gpu_sample(gpu)) < 85
command = ['C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
           str(args.project.resolve()), '/Engine/Maps/Entry', '-'+args.rhi,
           '-RenderOffscreen', '-unattended', '-nosound', '-nosplash',
           '-DisablePlugins=AndroidFileServer', '-DDC=InstalledNoZenLocalFallback',
           '-ExecCmds='+','.join(['t.MaxFPS 20', 'r.VSync 0']+args.exec_command), '-windowed', '-ResX=1920', '-ResY=1080',
           '-ForceRes', '-abslog='+str(out/'unreal.log')]
if args.world_profile:command.append('-SoulWorldTerrain')
cache_seed = []
if args.runtime_environment_survey:
    assert not args.world_profile, 'The environment survey does not load campaign terrain'
    user = out/'User'
    user.mkdir()
    # Reuse only the current validated registry references, as the authored
    # Dwarf runner already does. Cold discovery is not part of this load test.
    cache = args.project.resolve().parent/'Intermediate/CachedAssetRegistry'
    target = user/'Intermediate/CachedAssetRegistry'
    for ref in sorted(cache.glob('*.ref')):
        name = ref.read_text(encoding='utf-8-sig').strip()
        assert Path(name).name == name and name.endswith('.bin')
        source = cache/name
        if not source.is_file(): continue
        target.mkdir(parents=True, exist_ok=True)
        for file in (source, ref):
            destination = target/file.name
            shutil.copy2(file, destination)
            with destination.open('rb') as stream:
                cache_seed.append(dict(path=str(destination), bytes=destination.stat().st_size,
                    sha256=hashlib.file_digest(stream, 'sha256').hexdigest()))
    command[2] = '/Game/Soul/Maps/Settlements/L_HumanCapital_Authored?game=/Script/Engine.GameModeBase'
    command[command.index('-DisablePlugins=AndroidFileServer')] = '-DisablePlugins=AndroidFileServer,NwiroIntegrationKit'
    # This mode is content inspection, never a performance benchmark. Native
    # city preparation can saturate the GPU even at twenty frames per second.
    command = [arg.replace('t.MaxFPS 20', 't.MaxFPS 5') if arg.startswith('-ExecCmds=') else arg for arg in command]
    command += ['-game', '-SoulHumanEnvironmentSurvey', '-SoulCampaignCapturePrefix=human-runtime-survey',
                '-UserDir='+user.as_posix()]
if args.enable_plugin:command.append('-EnablePlugins='+','.join(args.enable_plugin))
if args.sm6:
    assert args.rhi == 'd3d12', 'SM6 qualification requires DX12'
    command.append('-sm6')
for override in args.ini_override:
    assert override.startswith('Engine:[') and '\n' not in override and '\r' not in override
    assert all(part.startswith('[') and ']:' in part and '=' in part
               for part in override.removeprefix('Engine:').split(',')), 'Every override needs its section'
# UE accepts one comma-separated override list per config category. Repeating
# -ini:Engine can lose later startup settings, unlike repeated ExecCmd entries.
if args.ini_override:
    command.append('-ini:Engine:'+','.join(value.removeprefix('Engine:') for value in args.ini_override))
env = os.environ.copy()
scratch = tempfile.mkdtemp(prefix='SoulWorldEditor_')
env['TEMP'] = env['TMP'] = scratch
record = {'command': command, 'started_utc': utc(), 'temporary_directory': scratch}
if cache_seed: record['asset_registry_cache_seed'] = cache_seed
process = None
try:
    with (out/'console.log').open('w') as console, (out/'telemetry.jsonl').open('w') as telemetry:
        process = subprocess.Popen(command, cwd=args.project.resolve().parent, env=env,
                                   stdout=console, stderr=subprocess.STDOUT,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        record['pid'] = process.pid
        (out/'launch.json').write_text(json.dumps(record, indent=2))
        print('Owned authoring editor PID', process.pid, flush=True)
        deadline = time.monotonic() + args.minutes*60
        while process.poll() is None:
            host_memory = system_commit_sample()
            if host_memory['available_commit_mib'] < args.min_system_commit_mib:
                record['stop_reason'] = 'system_commit_headroom'
                record['system_memory_at_stop'] = host_memory
                break
            sample = {'utc': utc(), 'gpu': gpu_sample(gpu),
                      'process_memory': process_memory_sample(process.pid),
                      'system_memory': host_memory}
            telemetry.write(json.dumps(sample)+'\n'); telemetry.flush()
            if max(x['temperature_c'] for x in sample['gpu']) >= 85:
                record['stop_reason'] = 'thermal_cutoff_85c'; break
            if sample['process_memory'].get('private_commit_mib', 0) >= args.max_private_mib:
                record['stop_reason'] = 'private_commit_limit'
                record['private_commit_limit_mib'] = args.max_private_mib
                break
            if (out/'stop').exists():
                record['stop_reason'] = 'requested_authoring_close'; break
            if time.monotonic() >= deadline:
                record['stop_reason'] = 'session_deadline'; break
            time.sleep(3)
except BaseException as exc:
    record['error'] = repr(exc)
    raise
finally:
    # Preserve the guard decision even if a memory-bound editor takes longer
    # than the bounded cleanup wait to exit.
    (out/'session.json').write_text(json.dumps(record, indent=2))
    if process:
        try:
            record['cleanup'] = close_owned_process(process)
        except BaseException as cleanup_error:
            record['cleanup_error'] = repr(cleanup_error)
        record['exit_code'] = process.poll()
    record['finished_utc'] = utc()
    if args.runtime_environment_survey:
        log = (out/'unreal.log').read_text(encoding='utf-8-sig', errors='replace') if (out/'unreal.log').exists() else ''
        record['native_survey_completed'] = (record.get('exit_code') == 0 and not record.get('stop_reason')
            and 'SOUL_HUMAN_ENVIRONMENT_SURVEY_COMPLETE' in log)
    (out/'session.json').write_text(json.dumps(record, indent=2))
    print(json.dumps(record, indent=2), flush=True)
