"""Local evidence gallery; never renders or invents player-facing assets."""
from pathlib import Path
import argparse,json,html
R=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--evidence-root',type=Path,required=True);p.add_argument('--final',action='store_true');a=p.parse_args();E=a.evidence_root.resolve();assert E.is_relative_to(R/'Evidence')
 esc=html.escape;parts=[]
 def image(path,caption):
  f=E/path
  if not f.is_file():return
  assert f.resolve().is_relative_to(E)
  url=esc(f.relative_to(E).as_posix(),quote=True)
  parts.append(f'<figure><a href="{url}"><img loading="lazy" src="{url}" alt="{esc(caption)}"></a><figcaption>{esc(caption)}</figcaption></figure>')
 status='Evidence closeout' if a.final else 'IN PROGRESS — editor and cooked proof scopes are separate'
 parts.append('<h1>Soul controlled faction qualification</h1><p>'+status+'</p><p>3.5 km foundation unchanged. Full autonomous AI OFF. Functional runs capped and guarded at 85°C; no performance qualification.</p>')
 parts.append('<nav><a href="controlled-action-policy.md">Action policy</a> · <a href="ordered-pair-matrix.md">Ordered matrix</a> · <a href="approach-coverage.json">Approach coverage</a> · <a href="roster-readiness.json">Roster readiness</a></nav>')
 parts.append('<h2>Nature exact infantry</h2><p>Owned Animals Warrior Bear, its native animation set and right-hand blade. The donor weapon package is named Axe, but its visible form, material and textures identify a sword. Staged poses are separate from natural battle qualification.</p><div class="grid">')
 for f in sorted((E/'Local/editor-nature-poses-r3/User/Saved/Screenshots/AnimationPoses').glob('*.png')):image(f.relative_to(E),f.stem+' — paused pose inspection')
 parts.append('</div><h2>Natural ordered battle results</h2>')
 seen=set()
 for f in sorted(E.glob('*-result.json')):
  d=json.loads(f.read_text())
  if not(d.get('pass') and d.get('attacker') and d.get('defender')):continue
  key=(d['attacker'],d['defender'],tuple(x['run'] for x in d['runtime']))
  if key in seen:continue
  seen.add(key);scope=', '.join(sorted({x['runtime_kind'] for x in d['runtime']}))
  parts.append('<section><h3>'+esc(d['attacker']+' → '+d['defender'])+'</h3><p>'+esc(scope+'; '+d['natural_result']+'; owner '+d['owner']+'; return '+d['attacker_return_region'])+'</p><p>Exact bodies and RBCombat contacts verified. F5/F9 plus separate-process byte-identical save restoration. Routed forces use existing effective-survivor rules.</p><a href="'+esc(f.name)+'">Result receipt</a><div class="grid">')
  for row in d['captures']:
   image(Path(row['path']),Path(row['path']).stem)
  parts.append('</div></section>')
 parts.append('<h2>All-six controlled turn</h2><p>Normal neutral captures and AP; replayed proposals must reject. Final status is in the turn receipt.</p><div class="grid">')
 for f in sorted((E/'Local').glob('*controlled-turn*/User/Saved/Screenshots/*.png')):image(f.relative_to(E),f.parent.parent.parent.parent.name+' / '+f.stem)
 parts.append('</div>')
 css='body{background:#171d22;color:#e1e7eb;font:16px system-ui;margin:2rem auto;max-width:1500px;padding:0 1rem}a{color:#87d1e7}nav{margin:1rem 0}section{border-top:1px solid #49555f;padding:1rem 0}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(400px,1fr));gap:1rem}figure{margin:0;background:#232c33}img{width:100%;display:block}figcaption{padding:.6rem}h1,h2,h3{color:#f1d5a2}'
 (E/'visual-review.html').write_text('<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Soul controlled faction review</title><style>'+css+'</style><body>'+''.join(parts)+'</body></html>',encoding='utf-8')
 print(E/'visual-review.html')
if __name__=='__main__':main()
