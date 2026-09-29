"""Preserve an immutable compact receipt/screenshots from one qualification run."""
import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
import shutil
import struct

ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--run',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--prefix',required=True)
p.add_argument('--benchmark',action='store_true')
a=p.parse_args()
if a.output.exists():p.error('Immutable receipt directory already exists')
summary=json.loads((a.run/'summary.json').read_text())
started=dt.datetime.fromisoformat(summary['started_utc']).timestamp()
log=(a.run/'unreal.log').read_text(errors='replace')
a.output.mkdir(parents=True)
shutil.copy2(a.run/'summary.json',a.output/'runtime.json')
receipt={'runtime':summary,'images':[], 'height_sha256_at_launch':summary.get('terrain_height_sha256'),
 'height_sha256_at_archive':hashlib.sha256((ROOT/'Data/CampaignTerrainV2/FounderHeight.r16').read_bytes()).hexdigest(),
 'assertions':[line for line in log.splitlines() if 'SOUL_' in line and any(word in line for word in ('PASS','FAIL','BUILT','BENCHMARK_COMPLETE','landscape_loaded'))]}
for f in (ROOT/'Saved/Screenshots').glob(a.prefix+'*.png'):
 if f.stat().st_mtime<started:continue
 raw=f.read_bytes();assert raw[:8]==b'\x89PNG\r\n\x1a\n'
 w,h=struct.unpack('!II',raw[16:24])
 shutil.copy2(f,a.output/f.name)
 receipt['images'].append({'file':f.name,'width':w,'height':h,'sha256':hashlib.sha256(raw).hexdigest()})
if a.benchmark:
 import numpy as np
 for name in ['TerrainBenchmark.json','TerrainBenchmark.csv']:
  src=ROOT/'Saved'/name
  assert src.stat().st_mtime>=started, 'Stale benchmark payload'
  shutil.copy2(src,a.output/name)
 frames=np.loadtxt(a.output/'TerrainBenchmark.csv',skiprows=1)
 longest=run=0;longest_ms=run_ms=0.
 for v in frames:
  run=run+1 if v>1000/30 else 0;longest=max(longest,run)
  run_ms=run_ms+v if v>1000/30 else 0.;longest_ms=max(longest_ms,run_ms)
 receipt['benchmark']=dict(frames=len(frames),mean_ms=float(frames.mean()),p95_ms=float(np.percentile(frames,95)),
  p99_ms=float(np.percentile(frames,99)),mean_fps=float(1000/frames.mean()),below_30=int((frames>1000/30).sum()),
  below_40=int((frames>25).sum()),below_60=int((frames>1000/60).sum()),longest_consecutive_below_30_frames=longest,longest_consecutive_below_30_ms=longest_ms)
telemetry=[json.loads(s) for s in (a.run/'telemetry.jsonl').read_text().splitlines() if s.strip()]
gpus=[g for s in telemetry for g in s.get('gpu',[])]
receipt['peak_gpu_temperature_c']=max((g['temperature_c'] for g in gpus),default=None)
receipt['peak_total_gpu_memory_mib']=max((g['memory_used_mib'] for g in gpus),default=None)
(a.output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({'output':str(a.output),'images':len(receipt['images']),'benchmark':receipt.get('benchmark')},indent=2))
