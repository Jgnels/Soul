from pathlib import Path
import json,subprocess,hashlib,ast
R=Path.cwd();E=R/'Evidence/MapFinalPolish-20261007'
for row in json.loads((E/'inherited-tracked-hashes.json').read_text()):
 assert hashlib.sha256((R/row['path']).read_bytes()).hexdigest()==row['sha256'],row['path']
assert not subprocess.check_output(['git','diff','--cached','--name-only'],text=True).strip(),'Pre-existing staged changes; do not mix'
paths=[p for p in (R/'Tools/MapFinalPolish').iterdir() if p.is_file() and p.suffix in ['.py','.md']]
paths += [p for p in E.iterdir() if p.is_file() and p.suffix in ['.md','.json','.html','.txt'] and p.name not in ['baseline-status.txt','final-head.txt']]
for p in paths:
 assert p.stat().st_size<2*1024*1024,(str(p),p.stat().st_size)
 if p.suffix=='.json':json.loads(p.read_text(encoding='utf-8-sig'))
 if p.suffix=='.py':ast.parse(p.read_text(encoding='utf-8-sig'))
subprocess.run(['git','add','--']+[str(p.relative_to(R)) for p in paths],check=True)
print('STAGED_SCOPED_FILES',len(paths),'BYTES',sum(p.stat().st_size for p in paths))
r=subprocess.run(['git','diff','--cached','--check'],capture_output=True,text=True);print(r.stdout+r.stderr);assert r.returncode==0
