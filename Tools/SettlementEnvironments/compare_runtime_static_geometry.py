"""Compare loaded static-mesh actor placement across two native surveys.

Ignores package renaming and material overrides. Does not claim to compare
foliage instance transforms, component-level blueprint geometry or collision.
"""
import argparse
from collections import Counter
import datetime
import hashlib
import json
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--before', required=True, type=Path)
p.add_argument('--after', required=True, type=Path)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
assert not a.output.exists(), 'Preserve previous receipts'

def inventory(path):
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    actors = [r for r in data['actors'] if r['class'] == '/Script/Engine.StaticMeshActor']
    result = Counter((r['transform'], tuple(r['meshes'])) for r in actors)
    with path.open('rb') as stream: digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    return result, dict(path=str(path), sha256=digest, static_mesh_actors=len(actors), total_actors=len(data['actors']))

before, b = inventory(a.before)
after, c = inventory(a.after)
removed, added = before-after, after-before
result = dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), before=b, after=c,
    matching=sum((before & after).values()), removed=sum(removed.values()), added=sum(added.values()),
    unchanged=before == after,
    scope='StaticMeshActor native transforms and mesh paths including duplicate multiplicity; material overrides intentionally excluded. No foliage/component/collision equality claim.',
    removed_examples=[dict(transform=k[0], meshes=k[1], count=v) for k,v in list(removed.items())[:20]],
    added_examples=[dict(transform=k[0], meshes=k[1], count=v) for k,v in list(added.items())[:20]])
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
print(json.dumps({k:result[k] for k in ('matching','removed','added','unchanged')}))
raise SystemExit(0 if result['unchanged'] else 1)
