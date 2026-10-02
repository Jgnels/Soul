"""Summarize Unreal CSV frame timing. Invocation must identify the exercised scenario."""
import csv,json,sys,statistics
from pathlib import Path
p=Path(sys.argv[1])
with p.open() as f:
 rows=list(csv.DictReader(f))
values=[]
for r in rows:
 try:
  v=float(r.get("FrameTime", ""))
  if v>0: values.append(v)
 except (ValueError,TypeError): pass
assert len(values)>=1000,"Expected bounded 1200-frame active qualification sample"
# First 30 profiling setup frames are excluded, independent of measured values.
native="--native" in sys.argv
cut=300 if native else 30
a=sorted(values[cut:]);n=len(a)
q=lambda p:a[min(n-1,int(n*p))]
report={"csv":str(p),"scope":"D3D11 rendered-offscreen Dragon Graveyard, 1280x720, 30 active initial combatants, normal combat and spells; qualification captures and diagnostics enabled", "hardware":"GTX 1080 / i7-7700HQ / 16 GB", "raw_frames":len(values),"excluded_first_frames":cut,"measured_frames":n,"mean_ms":statistics.mean(a),"median_ms":statistics.median(a),"p95_ms":q(.95),"p99_ms":q(.99),"max_ms":max(a),"mean_equivalent_fps":1000/statistics.mean(a),"frames_above_33_33_ms":sum(x>1000/30 for x in a),"limitations":"Short active sample, not a packaged release benchmark or full campaign soak. Screenshot and spell asset activity are included; startup is excluded by capture beginning at deployment resume."}
if native:
 report["scope"]="Native windowed D3D11 Dragon Graveyard, 1280x720, 30 initial combatants; five spells cast by mouse before this capture; combat continues, no screenshot requests during sample"
 report["limitations"]="Short 25-second sample. First 300 frames exclude command entry/setup; console overlay remains open during sample. Diagnostics enabled. Not a clean release benchmark or long soak."
print(json.dumps(report,indent=2))
if len(sys.argv)>2:Path(sys.argv[2]).write_text(json.dumps(report,indent=2))
