from pathlib import Path
import subprocess,json,hashlib,shutil,datetime
R=Path.cwd();E=R/'Evidence/ProductionPush-20261008';B=R/'Evidence/DwarfPassGate-20261007'
head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip();assert head==(B/'final-head.txt').read_text().strip()
(E/'clock.json').write_text(json.dumps(dict(start_utc='2026-10-08T06:44:57Z',dwarf_deadline_utc='2026-10-08T07:44:57Z',target_end_utc='2026-10-08T14:44:57Z',starting_head=head),indent=2))
(E/'baseline-status.txt').write_bytes(subprocess.check_output(['git','status','--short']))
rows=[]
for n in subprocess.check_output(['git','diff','--name-only'],text=True).splitlines():
 p=R/n
 if p.is_file():
  rows.append(dict(path=n,sha256=hashlib.sha256(p.read_bytes()).hexdigest()));d=E/'Local/Inherited'/n;d.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,d)
(E/'inherited-tracked-hashes.json').write_text(json.dumps(rows,indent=2))
base=json.loads((B/'preservation-final.json').read_text());paths={Path(x['path']) for x in base['files']};paths.update(p for p in (R/'Content/SoulCampaignComposition').rglob('*') if p.is_file());out=[]
for p in sorted(paths):
 with p.open('rb') as f:h=hashlib.file_digest(f,'sha256').hexdigest()
 out.append(dict(path=str(p),sha256=h,bytes=p.stat().st_size))
(E/'preservation-baseline.json').write_text(json.dumps(dict(files=out),indent=2))
shutil.copytree(R/'Content/SoulCampaignComposition',E/'Local/Before/Content/SoulCampaignComposition')
print('BASELINE',len(out),'INHERITED',len(rows),flush=True)
