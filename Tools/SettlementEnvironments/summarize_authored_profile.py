"""Join native uncapped frame receipts with guarded process/GPU telemetry."""
import argparse
import datetime
import json
from pathlib import Path
import re

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('run', type=Path)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
assert not a.output.exists(), 'Preserve previous evidence'
rows = [json.loads(line) for line in (a.run/'runtime/telemetry.jsonl').read_text().splitlines()]
log = (a.run/'runtime/unreal.log').read_text(encoding='utf-8-sig', errors='replace')
def time(value): return datetime.datetime.fromisoformat(value.replace('Z', '+00:00'))
def peaks(samples):
    assert samples
    return dict(peak_gpu_c=max(g['temperature_c'] for r in samples for g in r['gpu']),
        peak_gpu_used_mib=max(g['memory_used_mib'] for r in samples for g in r['gpu']),
        peak_working_set_mib=max(r['process_memory']['working_set_mib'] for r in samples),
        peak_private_commit_mib=max(r['process_memory']['private_commit_mib'] for r in samples))
profiles = []
for file in sorted((a.run/'User/Saved').glob('*_performance_*.json')):
    data = json.loads(file.read_text())
    start, end = time(data['started_utc']), time(data['finished_utc'])
    samples = [r for r in rows if start <= time(r['utc']) <= end]
    data['warmup_and_sample_telemetry'] = peaks(samples)
    data['receipt'] = str(file)
    data['average_fps_from_intervals'] = 1000/data['mean_ms']
    profiles.append(data)
result = dict(run=str(a.run), profiles=profiles, process=peaks(rows),
    fresh_load_pass='SOUL_AUTHORED_FRESH_LOAD_PASS' in log,
    native_load_times=[dict(map=m, seconds=float(s)) for s,m in re.findall(r'Took ([\d.]+) seconds to LoadMap\(([^)]+)\)', log)],
    limitations=['Editor-game source-content qualification, not a cooked package.',
        'City is one fixed Caravan Hall view; campaign is the three-region opt-in proof view.',
        'GPU memory is device-wide allocation, not exclusive process VRAM.',
        'Native map-load time excludes subsequent asset preparation and visual readiness.',
        'Peak measurements are sampled by the existing guarded runner.'])
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result, indent=2))
