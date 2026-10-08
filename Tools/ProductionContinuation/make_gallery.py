"""Real Unreal before/after evidence; no synthesized player-facing content."""

from pathlib import Path

from html import escape

import json,os

R=Path(__file__).resolve().parents[2];E=R/'Evidence/ProductionContinuation-20261008';prior=R/'Evidence/ProductionPush-20261008'

notes={

'whole_labels_minimized':'Frozen 3.5 km heightfield. Materials, shore masks and regional population remain provisional.',

'ordinary_campaign':'Canonical topology and all 36 physical anchors preserved. Roads remain over-regular in several open areas.',

'human_roads':'Existing shared trunks retained; no new Human city or terrain edits.',

'human_capital_approach':'Capital size and arrival preserved. Human authored-city framework unchanged.',

'mountain_pass':'Six local Crownspine curve improvements pass grade sampling; angular switchbacks remain.',

'dwarf_approach':'UNCHANGED PROVISIONAL exterior. No additional Dwarf research or miniature iteration.',

'viking_coast':'Cooler native pine variants; harbor landing detail below. Full Viking capital remains unintegrated.',

'orc_routes':'Native field defenses, local road fillets and restrained Mesa material feathering. Source transition remains visible.',

'nature_paths':'Exact forest XY placements retained; owned pine/broadleaf variants replace the two-family uniformity. Sharp river-bank masks remain.',

'dark_region':'A local road fillet retained. Trial masonry rejected after close review; fortress-horizon identity remains unfinished.',

'broken_bridge':'Prior qualified masonry/timber crossing preserved.',

'river_ford':'Unbridged and unchanged; 21.76054 degrees native sampled maximum.',

'ferry_landing':'Both physical ferry passages retained; seven legal pairs use them. No boat animation claimed.',

'close_road':'Road widths and native terrain relief preserved; materials remain provisional.'}

views=['whole_labels_minimized','ordinary_campaign','human_roads','human_capital_approach','mountain_pass','dwarf_approach','viking_coast','orc_routes','nature_paths','dark_region','broken_bridge','broken_bridge_close','river_ford','major_stone_bridge','minor_bridge','ferry_landing','close_road']

html=['<!doctype html><html><meta charset="utf-8"><title>Soul production continuation</title><style>body{background:#15191d;color:#eee;font:17px system-ui;max-width:1600px;margin:auto;padding:30px}img{width:100%;height:auto}section{margin:36px 0}a{color:#a8d5ff}.pair{display:grid;grid-template-columns:1fr 1fr;gap:12px}p{max-width:1150px}figcaption{padding:8px}figure{margin:0}.warning{border-left:4px solid #e5ac58;padding:15px}</style><h1>Soul — five-hour continuation</h1><p class="warning">Frozen geography; candidate remains opt-in. Dwarf exterior, road surfaces, shore masks and sparse regional art remain provisional. The corrected 60-second performance attempt hit the unchanged 85 C cutoff before completion. Cooked input, fresh-load and Human authored battle/return pass. These capped functional runs are not performance acceptance.</p><p><a href="HANDOFF.md">Handoff</a> · <a href="save-compatibility.md">Save compatibility</a> · <a href="six-faction-groundwork.md">Six-faction groundwork</a> · <a href="native-route-final-summary.json">Native route receipt</a></p>']

def fig(p,label):

 rel=Path(os.path.relpath(p,E)).as_posix();return f'<figure><a href="{escape(rel)}"><img loading="lazy" src="{escape(rel)}"></a><figcaption>{escape(label)}</figcaption></figure>'

for name in views:

 before=prior/'Local/captures-final'/f'{name}.png'

 if not before.exists():before=prior/'Local/captures-roads-push1'/f'{name}.png'

 final=E/'Local/captures-final-r2'/f'{name}.png'

 if not final.exists():continue

 html.append('<section><h2>'+escape(name.replace('_',' '))+'</h2><p>'+escape(notes.get(name,'Existing qualified crossing function preserved; art remains provisional.'))+'</p><div class="pair">')

 if before.exists():html.append(fig(before,'Before: previous completed production push'))

 html.append(fig(final,'After: current actual Unreal capture'));html.append('</div></section>')

p=E/'Local/captures-final-r2/viking_harbor_close.png'

if p.exists():html.append('<section><h2>Viking harbor landing</h2>'+fig(p,'Owned wood modules, piles seated in actual bed, grade-qualified local arrival and approximately 26 m jetty. This is harbor context, not a completed Viking capital.')+'</section>')

for run in ['runtime-input-r1','runtime-load-r1','runtime-development-r1','packaged-input-r2','packaged-load-r1','packaged-human-r1']:

 images=sorted((E/'Local'/run/'User/Saved/Screenshots').glob('*.png'))

 if run in ['runtime-input-r1','packaged-input-r2']:images=[p for p in images if any(p.stem.endswith(x) for x in ['initial','wide_bounds','restored','controller_company'])]

 if run=='packaged-human-r1':images=[p for p in images if any(p.stem.endswith(x) for x in ['miniature_start','miniature_completed','city_start_clean','city_completed_clean','authored_battle_warm','campaign_after_battle'])]

 if not images:continue

 html.append('<section><h2>'+escape(run)+'</h2><div class="pair">')

 for p in images:html.append(fig(p,p.stem))

 html.append('</div></section>')

p=E/'Local/runtime-profile-60s-r1/User/Saved/Screenshots/TerrainBenchmark.png'

if p.exists():html.append('<section><h2>Corrected 60-second profile</h2>'+fig(p,'See performance-result.json for measured window, thermal status and acceptance. Only one corrected profile is authorized this continuation.')+'</section>')

html.append('<section><h2>Performance limit</h2><p>The single corrected native 1080p attempt requested 20 seconds of warmup and 60 measured seconds. It stopped at 85 C before producing a complete sample; no average/P95/P99 result is claimed. <a href="performance-result.json">Thermal receipt</a>.</p></section>')

html.append('<section><h2>Measured thermal trace</h2>'+fig(E/'performance-thermal.svg','Actual telemetry from the one failed attempt. The map-ready marker is not the benchmark sample start.')+'</section>')

html.append('<section><h2>Rejected Dark fragments</h2><p>These two trial fragments are persistently hidden in game with collision disabled; the editor-session hide must be reapplied if reopening them. They read as isolated thin walls, not convincing ruins. Preserved as rejection evidence only.</p><div class="pair">')

for name in ['dark_approach_ruin','dark_causeway_remnant']:

 p=E/'Local/captures-dark-final'/f'{name}.png'

 if p.exists():html.append(fig(p,'REJECTED trial — not accepted campaign art'))

html.append('</div></section></html>');(E/'visual-review.html').write_text('\n'.join(html),encoding='utf-8');print(E/'visual-review.html')
