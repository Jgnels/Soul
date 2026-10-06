"""Read-only donor inventory and package hashes before Soul wrapper authoring."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--content', type=Path, required=True)
p.add_argument('--family', required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
assert not a.output.exists(), 'Preserve earlier receipt'
roots = [a.content / a.family, a.content / '__ExternalActors__' / a.family,
         a.content / '__ExternalObjects__' / a.family]
rows = []
for root in roots:
    if not root.exists():
        continue
    for file in sorted(root.rglob('*')):
        if not file.is_file():
            continue
        stat = file.stat()
        row = dict(path=file.relative_to(a.content).as_posix(), bytes=stat.st_size,
                   mtime_ns=stat.st_mtime_ns)
        if file.suffix == '.umap' or root != roots[0]:
            row['sha256'] = hashlib.file_digest(file.open('rb'), 'sha256').hexdigest()
        rows.append(row)
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    content=str(a.content), family=a.family, files=rows,
    hash_scope='Every map and external actor/object; other files size/mtime only'), indent=2)+'\n')
print(json.dumps(dict(files=len(rows), bytes=sum(r['bytes'] for r in rows),
                     hashed=sum('sha256' in r for r in rows), output=str(a.output))))
