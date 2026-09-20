import re,json
from pathlib import Path
log=Path(r"D:\RefinedBadger\Games\Soul\Evidence\EnvironmentCaptures\human_hivemind_safe_ue.log")
text=log.read_text(errors="ignore") if log.exists() else ""
rows=[]
for m in re.finditer(r"Building static mesh ([^\r\n]+?) \(Required Memory Estimate: ([0-9.]+) MB\)",text):
    rows.append({"mesh":m.group(1).strip(),"required_mb":float(m.group(2))})
# dedup max
by={}
for r in rows:
    by[r["mesh"]]=max(r["required_mb"],by.get(r["mesh"],0))
rows=[{"mesh":k,"required_mb":v} for k,v in by.items()]
rows.sort(key=lambda x:-x["required_mb"])
print("TOP REQUIRED MEMORY")
for r in rows[:40]: print(f'{r["required_mb"]:10.1f} MB | {r["mesh"]}')
built=[]
for m in re.finditer(r"Built static mesh \[([0-9.]+)s\] ([^\r\n]+)",text):
    built.append({"seconds":float(m.group(1)),"asset":m.group(2).strip()})
built.sort(key=lambda x:-x["seconds"])
print("\nTOP BUILD TIME")
for r in built[:40]: print(f'{r["seconds"]:8.2f}s | {r["asset"]}')
out={"top_memory":rows[:100],"top_build_time":built[:100]}
Path(r"D:\RefinedBadger\Games\Soul\Evidence\hivemind_heavy_asset_audit.json").write_text(json.dumps(out,indent=2),encoding="utf-8")
