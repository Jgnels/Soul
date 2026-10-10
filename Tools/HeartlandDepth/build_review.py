"""Generate the review from explicitly selected real cooked captures."""
from pathlib import Path
from html import escape
import json, argparse
R=Path(__file__).resolve().parents[2]; E=R/'Evidence/HumanHeartlandDepth-20261010'
p=argparse.ArgumentParser();p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
m=json.loads(a.manifest.read_text()); html=['<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Soul Heartland depth</title><style>body{margin:auto;max-width:1400px;padding:20px;background:#151a20;color:#eee;font:17px/1.5 system-ui}a{color:#abd9ff}h1,h2{color:#e8d498}section{margin:32px 0}figure{margin:0}img{width:100%;height:auto;background:#222}figcaption{padding:8px 0}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,480px),1fr));gap:16px}.note{border-left:4px solid #d3a15c;padding:12px 18px;background:#202730}nav{margin:16px 0}</style><h1>Soul - Human Heartland depth</h1><p class="note">'+escape(m['summary'])+'</p><nav><a href="HANDOFF.md">Handoff</a> | <a href="PLAY_HEARTLAND.md">Play instructions</a> | <a href="final-closeout.json">Closeout receipt</a></nav>']
for sec in m['sections']:
 html+=['<section><h2>'+escape(sec['title'])+'</h2><p>'+escape(sec['note'])+'</p><div class="grid">']
 for f in sec['figures']:
  source=E/f['path'];assert source.is_file(),source
  assert source.resolve().is_relative_to(R),source
  rel=source.relative_to(E).as_posix();html+=['<figure><a href="'+escape(rel,quote=True)+'"><img loading="lazy" src="'+escape(rel,quote=True)+'" alt="'+escape(f['caption'],quote=True)+'"></a><figcaption>'+escape(f['caption'])+'</figcaption></figure>']
 html+=['</div></section>']
html+=['<footer>All images are actual Unreal captures. Capped functional runs are not sustained performance qualification. No generative player-facing artwork.</footer></html>']
(E/'visual-review.html').write_text('\n'.join(html),encoding='utf-8');print(E/'visual-review.html')
