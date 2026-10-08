"""Make one additive local recovery archive; never restores, removes or stages payloads."""
from pathlib import Path
import hashlib,json,zipfile,shutil,datetime,subprocess
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008';out=E/'Local/final-candidate-recovery.zip'
assert not out.exists(),'Preserve existing recovery archive; choose a separate reviewed iteration if inputs changed.'
files=sorted(p for p in (R/'Content/SoulCampaignComposition').rglob('*') if p.is_file())
files+=sorted(p for p in (R/'Source').rglob('*') if p.is_file() and p.suffix.lower() in {'.cpp','.h','.cs'})
files+=sorted(p for p in (R/'Data/CampaignComposition').glob('*') if p.is_file())
files.append(R/'Data/CampaignCompositionLocal/Composition_3500_r2.r16')
assert len(files)==len(set(files));size=sum(p.stat().st_size for p in files);assert shutil.disk_usage(E).free>size+2*2**30
rows=[]
for p in files:
 with p.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
 rows.append({'path':p.relative_to(R).as_posix(),'bytes':p.stat().st_size,'sha256':digest})
with zipfile.ZipFile(out,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=3) as z:
 for p,row in zip(files,rows):z.write(p,row['path'])
with zipfile.ZipFile(out) as z:
 assert z.testzip() is None
 for row in rows:
  with z.open(row['path']) as f:assert hashlib.file_digest(f,'sha256').hexdigest()==row['sha256'],row['path']
with out.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
record={'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'worker_head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=R,text=True).strip(),'archive':out.relative_to(E).as_posix(),'archive_sha256':digest,'archive_bytes':out.stat().st_size,'source_bytes':size,'file_count':len(rows),'verified_all_members':True,'local_only':True,'clean_checkout_reproduction_claimed':False,'scope':'Candidate assets, exact current Source tree (including preserved inherited inactive drafts), composition data/height. Not a promotion, redistribution or clean-source admission. Source config, tokens, donor projects and full authored cities excluded. Owned external dependencies still required.','files':rows}
(E/'final-local-recovery.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8');print('LOCAL_RECOVERY_VERIFIED',len(rows),out.stat().st_size)
