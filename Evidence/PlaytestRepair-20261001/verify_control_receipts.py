"""Verify real runtime pause/control receipts; screenshots still require inspection."""
from pathlib import Path
import re, sys, json
log=Path(sys.argv[1]).read_text(encoding="utf-8",errors="replace")
rows=[dict(re.findall(r"(\w+)=(V\([^)]*\)|R\([^)]*\)|\S+)",line)) for line in log.splitlines() if "SOUL_CONTROL_RECEIPT:" in line]
assert rows, "No runtime control receipts"
frozen=("elapsed","health","cooldown","ammo","positions","hits","mana","casts","alive")
segments=[]; current=[]
for r in rows:
    if r["paused"]=="1": current.append(r)
    elif current: segments.append(current); current=[]
if current: segments.append(current)
long=[s for s in segments if len(s)>=7]
assert long, "Need at least 30 seconds of paused runtime"
for s in long:
    assert all(r["worldPaused"]=="1" and r["arm"]=="1" for r in s)
    baseline=tuple(s[0][k] for k in frozen)
    assert all(tuple(r[k] for k in frozen)==baseline for r in s), "Simulation changed while paused"
paused=[r for s in segments for r in s]
assert any(r["firstPerson"]=="1" for r in paused), "First person not exercised"
assert any(r["commander"]=="1" for r in paused), "Commander not exercised"
assert len({r["camera"] for r in paused if r["commander"]=="1"})>1, "Commander did not move"
assert len({r["rotation"] for r in paused if r["commander"]=="0"})>1, "Hero look did not move"
assert any(r["paused"]=="0" for r in rows), "Resume not exercised"
print(json.dumps({"passed":True,"receipts":len(rows),"long_pause_segments":len(long),"scope":"pause invariants and camera mode/input movement; visual inspection separate"},indent=2))
