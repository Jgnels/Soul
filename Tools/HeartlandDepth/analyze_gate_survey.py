"""Summarize measured cooked-city strips; does not create a siege or edit terrain."""
import argparse,csv,json
from collections import Counter,defaultdict
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument("survey",type=Path);p.add_argument("--out",type=Path,required=True);a=p.parse_args()
rows=list(csv.DictReader(a.survey.open(encoding="utf-8-sig")));groups=defaultdict(list)
for row in rows:groups[(row["yaw"],row["lane"])].append(row)
results=[]
for (yaw,lane),samples in sorted(groups.items()):
 missing=[s for s in samples if float(s["z"])<=-9999]
 blocked=[s for s in samples if int(s["blocked"]) or abs(float(s["step"]))>90 or float(s["normal"])<.75]
 results.append({"yaw":float(yaw),"lane_cm":float(lane),"samples":len(samples),"missing_floor":len(missing),"blocked_or_large_step":len(blocked),"max_step_cm":max(abs(float(s["step"])) for s in samples),"floor_relief_cm":max(float(s["z"]) for s in samples)-min(float(s["z"]) for s in samples),"actors":dict(Counter(s["actor"] for s in blocked)),"clear_sampled_strip":len(samples)==31 and not missing and not blocked})
result={"source":str(a.survey.resolve()),"station_spacing_cm":200,"scope":"12 sampled straight strips through measured native gate, not navigation coverage","strips":results,"clear_strips":sum(s["clear_sampled_strip"] for s in results),"siege_implemented":False}
a.out.write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
