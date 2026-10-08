"""Build a local review gallery from real captured images; no image synthesis."""
from pathlib import Path
from html import escape
import json,os
R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionPush-20261008'
views=['whole_labels_minimized','ordinary_campaign','human_roads','human_capital_approach','mountain_pass','dwarf_approach','viking_coast','orc_routes','nature_paths','dark_region','broken_bridge','broken_bridge_close','river_ford','major_stone_bridge','minor_bridge','ferry_landing','close_road']
html=['<!doctype html><html><meta charset="utf-8"><title>Soul Production Push — reviewed evidence</title><style>body{background:#15191d;color:#eee;font:17px system-ui;max-width:1600px;margin:auto;padding:30px}img{width:100%;height:auto}section{margin:36px 0}a{color:#a8d5ff}.pair{display:grid;grid-template-columns:1fr 1fr;gap:12px}p{max-width:1100px}figcaption{padding:8px}figure{margin:0}</style><h1>Soul production push</h1><p>WORLD FOUNDATION: YES (future, opt-in). DWARF: PROVISIONAL. RUNTIME: PASS within founder/Human fixtures. PERFORMANCE: FAIL due to observation-window mismatch. Frozen 3.5 km heightfield; not promoted. See the handoff for scope and limitations.</p><p><a href="HANDOFF.md">Authoritative handoff</a> · <a href="road-review.md">Road review</a> · <a href="runtime-scope.md">Runtime scope</a></p>']
def fig(p,label):
 rel=Path(os.path.relpath(p,E)).as_posix();return f'<figure><a href="{escape(rel)}"><img loading="lazy" src="{escape(rel)}"></a><figcaption>{escape(label)}</figcaption></figure>'
for name in views:
 before=R/'Evidence/DwarfPassGate-20261007/Local/captures-final'/f'{name}.png'
 if name=='broken_bridge':before=before.with_name('broken_bridge_wide.png')
 if not before.exists():before=R/'Evidence/MapFinalPolish-20261007/Local/captures-final6'/f'{name}.png'
 final=E/'Local/captures-final'/f'{name}.png'
 if not final.exists():final=E/'Local/captures-roads-push1'/f'{name}.png'
 if not final.exists():continue
 html.append('<section><h2>'+escape(name.replace('_',' '))+'</h2><div class="pair">')
 if before.exists():html.append(fig(before,'Prior accepted capture (view matching where available)'))
 html.append(fig(final,'Current actual Unreal capture'));html.append('</div></section>')
detail=E/'Local/captures-bridge-detail/broken_bridge_detail.png'
if detail.exists():html.append('<section><h2>Broken Bridge centered detail</h2>'+fig(detail,'Owned masonry finish; unchanged timber travel lane; art remains provisional')+'</section>')
for run in ['runtime-recovery-r1','runtime-input-r3','runtime-load-r1','runtime-traversal-r3','runtime-human-r1','runtime-human-resume-r1']:
 folder=E/'Local'/run/'User/Saved/Screenshots'
 images=sorted(folder.glob('*.png')) if folder.exists() else []
 if run=='runtime-input-r3':images=[p for p in images if any(p.stem.endswith(x) for x in ['initial','wide_bounds','restored','controller_company'])]
 if run=='runtime-traversal-r3':images=[p for p in images if any(p.stem.startswith('traversal_'+x) for x in ['03_','04_','17_','24_','35_','39_'])]
 if run=='runtime-human-r1':images=[p for p in images if any(p.stem.endswith(x) for x in ['city_start_clean','city_completed_clean','miniature_start','miniature_completed','authored_battle_warm','campaign_after_battle'])]
 if not images:continue
 html.append('<section><h2>'+escape(run)+'</h2><p>Runtime evidence; acceptance is recorded separately in the handoff.</p><div class="pair">')
 for p in images:html.append(fig(p,p.stem))
 html.append('</div></section>')
profile=E/'Local/runtime-profile-r1/User/Saved/Screenshots/TerrainBenchmark.png'
if profile.exists():html.append('<section><h2>Single native 1080p measurement</h2>'+fig(profile,'82.55 FPS mean; P95 12.85 ms; peak 84 C. Overall qualification FAIL: benchmark exited before wrapper observation minimum. No rerun; not a sustained performance pass.')+'</section>')
html.append('</html>');(E/'visual-review.html').write_text('\n'.join(html),encoding='utf-8');print(E/'visual-review.html')
