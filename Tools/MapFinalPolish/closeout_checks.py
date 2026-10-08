"""Read-only preservation and bounded source-test receipts for this polish pass."""
from pathlib import Path
import json,hashlib,subprocess,sys,datetime
R=Path(__file__).resolve().parents[2];E=R/'Evidence/MapFinalPolish-20261007'
def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(8*1024*1024),b''):h.update(chunk)
    return h.hexdigest()
baseline=json.loads((E/'preservation-baseline.json').read_text());rows=[]
for item in baseline['files']:
    p=Path(item['path']);current=digest(p) if p.exists() else None
    rows.append(dict(path=str(p),before=item['sha256'],after=current,unchanged=current==item['sha256']))
inherited=[]
for item in json.loads((E/'inherited-tracked-hashes.json').read_text()):
    current=digest(R/item['path']);inherited.append(dict(path=item['path'],before=item['sha256'],after=current,unchanged=current==item['sha256'],intentional_own_change=item['path']=='.gitignore'))
assets=[]
for p in sorted((R/'Content/SoulCampaignComposition').rglob('*')):
    if p.is_file():assets.append(dict(path=str(p.relative_to(R)).replace('\\','/'),bytes=p.stat().st_size,sha256=digest(p)))
(E/'preservation-final.json').write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),baseline_count=len(rows),unchanged=sum(x['unchanged'] for x in rows),changed=[x for x in rows if not x['unchanged']],files=rows,inherited_tracked=inherited,candidate_assets=assets),indent=2))
print('PRESERVATION',len(rows),sum(x['unchanged'] for x in rows),'INHERITED_CHANGED',[x['path'] for x in inherited if not x['unchanged']])
cmd=[sys.executable,'-m','unittest','Tools.test_soul_campaign_input','Source.Soul.Private.Tests.test_weekend_campaign_view']
p=subprocess.run(cmd,cwd=R,capture_output=True,text=True)
(E/'source-tests.txt').write_text(p.stdout+p.stderr)
(E/'source-tests.json').write_text(json.dumps(dict(command=cmd,exit_code=p.returncode,log='source-tests.txt',runtime_source_changed=False),indent=2))
print('SOURCE_TESTS',p.returncode)
print(p.stdout+p.stderr)
