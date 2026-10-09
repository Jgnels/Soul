"""Verify protected baseline bytes, licensed source/copies and qualified compiled-source bytes."""
from pathlib import Path
import hashlib,json,datetime,time,subprocess
R=Path.cwd();E=R/'Evidence/ControlledFactionAI-20261008'
def digest(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
start=time.monotonic();bad=[];baseline=json.loads((E/'preservation-before.json').read_text())['files'];groups={}
for row in baseline:
 p=Path(row['path']);groups[row['group']]=groups.get(row['group'],0)+1
 if not p.is_file() or digest(p)!=row['sha256']:bad.append({'scope':'baseline','path':str(p)})
nature=json.loads((E/'nature-donor-copy.json').read_text())['rows']
for row in nature:
 for key in ['donor','copy']:
  p=Path(row[key]);p=p if p.is_absolute() else R/p
  if not p.is_file() or digest(p)!=row['sha256']:bad.append({'scope':'nature_'+key,'path':str(p)})
q=json.loads((E/'qualified-builds.json').read_text());print('BASELINE_AND_NATURE',len(baseline),len(nature)*2,'mismatches',len(bad),flush=True)
# The schema is checked explicitly rather than silently accepting an empty snapshot.
assert len(q['source_snapshot'])==162
for row in q['source_snapshot']:
 p=R/row['path']
 if not p.is_file() or digest(p)!=row['sha256']:bad.append({'scope':'compiled_source','path':str(p)})
for row in q['builds']:
 p=R/row['binary']
 if not p.is_file() or digest(p)!=row['sha256']:bad.append({'scope':'fresh_binary','path':str(p)})
for row in q['editor_modules']:
 p=R/row['path']
 if not p.is_file() or digest(p)!=row['sha256']:bad.append({'scope':'editor_module','path':str(p)})
out={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'pass':not bad,'baseline_entries':len(baseline),'baseline_groups':groups,'nature_donor_and_copy_hash_checks':len(nature)*2,'compiled_source_files':len(q['source_snapshot']),'fresh_targets':len(q['builds']),'editor_modules':len(q['editor_modules']),'mismatches':bad,'seconds':time.monotonic()-start,'head':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'donors_saved':False,'performance_qualification':False}
(E/'preservation-final.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out));assert out['pass']
