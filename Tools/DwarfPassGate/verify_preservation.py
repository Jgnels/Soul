from pathlib import Path
import hashlib,json,subprocess
R=Path.cwd();E=R/'Evidence/DwarfPassGate-20261007'
def sha(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
b=json.loads((E/'preservation-baseline.json').read_text());rows=[]
for old in b['files']:
 p=Path(old['path']);h=sha(p);rows.append(dict(path=str(p),sha256=h,bytes=p.stat().st_size,unchanged=h==old['sha256'],before_sha256=old['sha256']))
inh=json.loads((E/'inherited-tracked-hashes.json').read_text());dirty=[dict(**v,unchanged=sha(R/v['path'])==v['sha256']) for v in inh]
result=dict(files=rows,inherited_tracked=dirty,unchanged=sum(r['unchanged'] for r in rows),total=len(rows),inherited_unchanged=sum(r['unchanged'] for r in dirty))
(E/'preservation-final.json').write_text(json.dumps(result,indent=2))
assets=[dict(path=str(p),sha256=sha(p),bytes=p.stat().st_size) for p in (R/'Content/SoulCampaignComposition').rglob('*') if p.is_file()]
(E/'candidate-asset-manifest.json').write_text(json.dumps(assets,indent=2))
log=(E/'Local/editor-r1/unreal.log').read_text(errors='replace');lines=[l for l in log.splitlines() if 'CampaignWorld' in l or 'Test Completed.' in l or 'Automation Test Queue' in l]
(E/'native-tests.txt').write_text('\n'.join(lines))
assert sum('Test Completed. Result={Success}' in l for l in lines)==2
assert all(r['unchanged'] for r in dirty)
changed=[r['path'] for r in rows if not r['unchanged']]
assert len(changed)==1 and changed[0].endswith('L_Composition_3500_r2.umap'),changed
print('PRESERVATION',result['unchanged'],'/',len(rows),'INHERITED',result['inherited_unchanged'],'/',len(dirty),'CHANGED',changed)
