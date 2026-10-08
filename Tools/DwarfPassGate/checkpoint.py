from pathlib import Path
import subprocess,json,hashlib,shutil
R=Path.cwd();E=R/'Evidence/DwarfPassGate-20261007';B=R/'Evidence/MapFinalPolish-20261007'
assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()==(B/'final-head.txt').read_text().strip()
(E/'baseline-status.txt').write_bytes(subprocess.check_output(['git','status','--short']))
rows=[]
for n in subprocess.check_output(['git','diff','--name-only'],text=True).splitlines():
 p=R/n
 if p.is_file():rows.append(dict(path=n,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
(E/'inherited-tracked-hashes.json').write_text(json.dumps(rows,indent=2))
prev=json.loads((B/'preservation-final.json').read_text());paths={Path(v['path']) for v in prev['files']};paths.update(p for p in (R/'Content/SoulCampaignComposition').rglob('*') if p.is_file());out=[]
for p in sorted(paths):
 with p.open('rb') as f:h=hashlib.file_digest(f,'sha256').hexdigest()
 out.append(dict(path=str(p),sha256=h,bytes=p.stat().st_size))
(E/'preservation-baseline.json').write_text(json.dumps(dict(files=out),indent=2))
shutil.copytree(R/'Content/SoulCampaignComposition',E/'Local/Before/Content/SoulCampaignComposition')
print('BASELINE',len(out),'INHERITED',len(rows))
