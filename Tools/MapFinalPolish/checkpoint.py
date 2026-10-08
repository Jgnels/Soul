from pathlib import Path
import hashlib,json,subprocess,shutil,datetime
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007'
assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()=='e714535172049f7ebfddb7729ea583d8a53ee03b'
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
(E/'baseline-status.txt').write_bytes(subprocess.check_output(['git','status','--short']))
names=subprocess.check_output(['git','diff','--name-only'],text=True).splitlines()
(E/'inherited-tracked-hashes.json').write_text(json.dumps([dict(path=n,sha256=digest(R/n)) for n in names if (R/n).is_file()],indent=2))
old=json.loads((R/'Evidence/MapPolish-20261007/preservation-final.json').read_text())
paths={Path(x['path']) for x in old['files']};paths.update(p for p in (R/'Content/SoulCampaignComposition').rglob('*') if p.is_file())
rows=[dict(path=str(p),sha256=digest(p),bytes=p.stat().st_size) for p in sorted(paths)]
(E/'preservation-baseline.json').write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),files=rows),indent=2))
dest=E/'Local/Before/Content/SoulCampaignComposition';assert not dest.exists();shutil.copytree(R/'Content/SoulCampaignComposition',dest)
print('CHECKPOINT',len(rows),'files',len(names),'inherited tracked changes')
