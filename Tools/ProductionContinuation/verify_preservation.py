"""Final read-only preservation check after cook/runtime; never repairs or restores files."""
from pathlib import Path
import json,hashlib,datetime,time
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008';before=json.loads((E/'preservation-baseline.json').read_text());previous=json.loads((E/'preservation-final.json').read_text());allowed=set(previous['expected_changed_paths']);rows=[];unexpected=[];started=time.monotonic()
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
for i,row in enumerate(before['files']):
 p=Path(row['path']);after=digest(p);same=after==row['sha256'];rows.append({**row,'after_sha256':after,'unchanged':same})
 if not same:
  relative=p.relative_to(R).as_posix() if p.is_relative_to(R) else str(p)
  if relative not in allowed:unexpected.append(relative)
 if (i+1)%50==0:print('PRESERVATION_CHECKED',i+1,flush=True)
out={'checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'phase':'After all completed cook/runtime activity','files':rows,'expected_changed_paths':sorted(allowed),'unexpected_changes':unexpected,'pass':not unexpected,'unchanged_count':sum(x['unchanged'] for x in rows),'no_donor_saves':not unexpected,'elapsed_seconds':round(time.monotonic()-started,2)}
(E/'preservation-final.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8');assert not unexpected,unexpected
inherited=json.loads((E/'inherited-preservation-final.json').read_text())
for row in inherited['files']:row['after_sha256']=digest(R/row['path']);row['changed']=row['after_sha256']!=row['sha256']
inherited['unexpected_changes']=[r['path'] for r in inherited['files'] if r['changed'] and not r['intentional_change']];inherited['pass']=not inherited['unexpected_changes'];inherited['checked_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();assert inherited['pass'];(E/'inherited-preservation-final.json').write_text(json.dumps(inherited,indent=2)+'\n',encoding='utf-8')
recovery=json.loads((E/'final-local-recovery.json').read_text());mismatches=[]
for row in recovery['files']:
 if digest(R/row['path'])!=row['sha256']:mismatches.append(row['path'])
assert not mismatches,mismatches
(E/'recovery-final-verification.json').write_text(json.dumps({'pass':True,'checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':'All 241 recorded candidate/source/data files still match the verified recovery archive after cooked runtime.','mismatches':mismatches,'archive_sha256':recovery['archive_sha256']},indent=2)+'\n',encoding='utf-8')
print('FINAL_PRESERVATION_PASS',len(rows),'protected;',len(inherited['files']),'inherited;',len(recovery['files']),'recovery files;',out['elapsed_seconds'],'seconds',flush=True)
