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
guard = json.loads((a.run/'runtime/summary.json').read_text(encoding='utf-8-sig'))
native_pass = 'SOUL_AUTHORED_FRESH_LOAD_PASS' in log
human = '-SoulHumanSettlementProof' in guard.get('command', [])
guard_pass = (guard.get('clean_shutdown') is True and guard.get('exit_code') == 0
    and guard.get('stop_reason') == 'completion_marker_process_exit')
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
    data['native_1080p_scale_verified'] = (data.get('viewport') == [1920,1080]
        and data.get('primary_screen_percentage') == 100
        and data.get('secondary_screen_percentage') == 100
        and data.get('dynamic_resolution_mode') == 0)
    profiles.append(data)
result = dict(run=str(a.run), profiles=profiles, process=peaks(rows),
    taa_resolution_observations=[dict(input=[int(w),int(h)],output=[int(ow),int(oh)])
        for w,h,ow,oh in sorted(set(re.findall(r'TAA[^\n]*?(\d+)x(\d+) -> (\d+)x(\d+)',log)))],
    native_fresh_load_marker=native_pass,
    fresh_load_pass=native_pass and guard_pass,
    guarded_run=dict(clean_shutdown=guard.get('clean_shutdown'), exit_code=guard.get('exit_code'),
        stop_reason=guard.get('stop_reason'), accepted=guard_pass),
    native_load_times=[dict(map=m, seconds=float(s)) for s,m in re.findall(r'Took ([\d.]+) seconds to LoadMap\(([^)]+)\)', log)],
    limitations=['Editor-game source-content qualification, not a cooked package.',
        ('City is one fixed native Human tavern view; campaign is the three-region opt-in proof view.' if human else 'City is one fixed Caravan Hall view; campaign is the three-region opt-in proof view.'),
        'GPU memory is device-wide allocation, not exclusive process VRAM.',
        'Native map-load time excludes subsequent asset preparation and visual readiness.',
        'Older reports without explicit 100% scale checks establish viewport size only, not native scene resolution.',
        'Peak measurements are sampled by the existing guarded runner.'])
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result, indent=2))
