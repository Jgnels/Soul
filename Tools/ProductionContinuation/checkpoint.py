from pathlib import Path
import json,hashlib,subprocess,shutil,datetime
R=Path.cwd();E=R/'Evidence/ProductionContinuation-20261008';P=R/'Evidence/ProductionPush-20261008'
assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()=='8ea786c180f2f8e431831b92b7d8867e2237c818'
assert not (E/'clock.json').exists()
start=datetime.datetime.now(datetime.timezone.utc);(E/'clock.json').write_text(json.dumps({'start_utc':start.isoformat(),'target_end_utc':(start+datetime.timedelta(hours=5)).isoformat(),'starting_head':'8ea786c180f2f8e431831b92b7d8867e2237c818','dwarf_minutes_authorized':0},indent=2))
(E/'baseline-status.txt').write_bytes(subprocess.check_output(['git','status','--porcelain=v1','--untracked-files=all']))
rows=[]
for n in subprocess.check_output(['git','diff','--name-only'],text=True).splitlines():
 p=R/n
 if p.is_file():
  d=E/'Local/Inherited'/n;d.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,d);rows.append({'path':n,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(E/'inherited-tracked-hashes.json').write_text(json.dumps(rows,indent=2))
paths={Path(x['path']) for x in json.loads((P/'preservation-baseline.json').read_text())['files']};paths.update(x for x in (R/'Content/SoulCampaignComposition').rglob('*') if x.is_file())
files=[]
for p in sorted(paths):
 with p.open('rb') as f:h=hashlib.file_digest(f,'sha256').hexdigest()
 files.append({'path':str(p),'sha256':h,'bytes':p.stat().st_size})
(E/'preservation-baseline.json').write_text(json.dumps({'files':files},indent=2));shutil.copytree(R/'Content/SoulCampaignComposition',E/'Local/Before/Content/SoulCampaignComposition')
for n in ['Data/CampaignComposition/presentation.json','Data/CampaignComposition/RuntimeProof.json','Data/CampaignComposition/HumanRuntimeProof.json']:
 d=E/'Local/Before'/n;d.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(R/n,d)
shutil.copy2(P/'Local/routes-final.json',E/'Local/routes-baseline.json')
print('PRESERVED',len(files),'files',len(rows),'inherited tracked deltas',start.isoformat())
