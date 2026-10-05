"""Bounded local editor session for owned campaign-world asset authoring.

Shares the existing qualification telemetry and 85 C cutoff. Never starts a
second Unreal session, edits configuration, or kills another worker's process.
Create OUTPUT/stop to close this child after authoring through Nwiro.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from qualify_soul_vertical import (gpu_sample, process_memory_sample,
                                  conflicting_processes, close_owned_process, utc)

p = argparse.ArgumentParser()
p.add_argument('--project', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--minutes', type=int, default=45)
p.add_argument('--world-profile',action='store_true',help='Select the full-world adapter for native automation in this authoring session')
args = p.parse_args()
assert not conflicting_processes(), 'Another Unreal/Soul session is active'
out = args.output.resolve()
assert not out.exists(), 'Use a new evidence directory'
out.mkdir(parents=True)
gpu = shutil.which('nvidia-smi')
assert gpu and max(x['temperature_c'] for x in gpu_sample(gpu)) < 85
command = ['C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
           str(args.project.resolve()), '/Engine/Maps/Entry', '-d3d11',
           '-RenderOffscreen', '-unattended', '-nosound', '-nosplash',
           '-DisablePlugins=AndroidFileServer', '-DDC=InstalledNoZenLocalFallback',
           '-ExecCmds=t.MaxFPS 20,r.VSync 0', '-windowed', '-ResX=1920', '-ResY=1080',
           '-ForceRes', '-abslog='+str(out/'unreal.log')]
if args.world_profile:command.append('-SoulWorldTerrain')
env = os.environ.copy()
scratch = tempfile.mkdtemp(prefix='SoulWorldEditor_')
env['TEMP'] = env['TMP'] = scratch
record = {'command': command, 'started_utc': utc(), 'temporary_directory': scratch}
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
            sample = {'utc': utc(), 'gpu': gpu_sample(gpu),
                      'process_memory': process_memory_sample(process.pid)}
            telemetry.write(json.dumps(sample)+'\n'); telemetry.flush()
            if max(x['temperature_c'] for x in sample['gpu']) >= 85:
                record['stop_reason'] = 'thermal_cutoff_85c'; break
            if (out/'stop').exists():
                record['stop_reason'] = 'requested_authoring_close'; break
            if time.monotonic() >= deadline:
                record['stop_reason'] = 'session_deadline'; break
            time.sleep(3)
except BaseException as exc:
    record['error'] = repr(exc)
    raise
finally:
    if process:
        record['cleanup'] = close_owned_process(process)
        record['exit_code'] = process.returncode
    record['finished_utc'] = utc()
    (out/'session.json').write_text(json.dumps(record, indent=2))
    print(json.dumps(record, indent=2), flush=True)
