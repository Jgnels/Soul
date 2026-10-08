"""Compare protected bytes and inherited files without touching any source asset."""
from pathlib import Path
import hashlib,json,subprocess
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionPush-20261008'
def sha(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
base=json.loads((E/'preservation-baseline.json').read_text())['files'];rows=[]
allowed=R/'Content/SoulCampaignComposition/L_Composition_3500_r2.umap'
source=json.loads((E/'source-delta-working.json').read_text());ours={x['path'] for x in source}
for old in base:
 p=Path(old['path']);h=sha(p) if p.is_file() else None
 rows.append(dict(path=str(p),before=old['sha256'],after=h,unchanged=h==old['sha256'],intentional_candidate_change=p==allowed,intentional_runtime_source_change=p.is_relative_to(R) and p.relative_to(R).as_posix() in ours))
source=json.loads((E/'source-delta-working.json').read_text());ours={x['path'] for x in source}
inherited=[]
for old in json.loads((E/'inherited-tracked-hashes.json').read_text()):
 p=R/old['path'];h=sha(p) if p.is_file() else None
 inherited.append(dict(path=old['path'],before=old['sha256'],after=h,unchanged=h==old['sha256'],mission_source_overlap=old['path'] in ours))
receipt=dict(files=rows,inherited=inherited,unexpected_protected_changes=[x for x in rows if not x['unchanged'] and not x['intentional_candidate_change'] and not x['intentional_runtime_source_change']],unexpected_inherited_changes=[x for x in inherited if not x['unchanged'] and not x['mission_source_overlap']])
receipt['pass']=not receipt['unexpected_protected_changes'] and not receipt['unexpected_inherited_changes']
(E/'preservation-final.json').write_text(json.dumps(receipt,indent=2))
(E/'final-status.txt').write_bytes(subprocess.check_output(['git','status','--short'],cwd=R))
print(json.dumps({k:v for k,v in receipt.items() if k not in ['files','inherited']},indent=2))
