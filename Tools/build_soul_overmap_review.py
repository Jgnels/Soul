"""Build a local browser review page for the Soul overmap."""
import html
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/"Data"/"soul_world_overmap_v1_20260922.json"
OUT=ROOT/"Evidence"/"WorldOvermap"/"soul_world_overmap_review.html"

d=json.loads(DATA.read_text(encoding="utf-8"))
rows=[]
for n in sorted(d["nodes"],key=lambda x:(x["macro_region"],x["name"])):
    owner=n["owner"] or "neutral"
    rows.append(
        "<tr>"
        f"<td>{html.escape(n['name'])}</td>"
        f"<td>{html.escape(n['macro_region'])}</td>"
        f"<td>{html.escape(owner)}</td>"
        f"<td>{html.escape(n['feature'])}</td>"
        f"<td>{html.escape(n['battle_recipe_hint'])}</td>"
        "</tr>"
    )

page="""<!doctype html><html><head><meta charset="utf-8">
<title>Soul World Overmap Review</title>
<style>
body{font-family:Segoe UI,Arial,sans-serif;margin:0;background:#171a1d;color:#e7e2d8}
header{padding:16px 24px;background:#20252a;position:sticky;top:0;z-index:2}
h1{margin:0 0 6px;font-size:24px} p{margin:4px 0;color:#bfc6ca}
main{padding:18px;max-width:1500px;margin:auto}
.card{background:#22282d;border:1px solid #394149;border-radius:12px;padding:14px;margin-bottom:18px}
img{width:100%;height:auto;background:#eee5d3;border-radius:8px}
.grid{display:grid;grid-template-columns:2fr 1fr;gap:18px}
table{width:100%;border-collapse:collapse;font-size:13px}
th,td{padding:7px;border-bottom:1px solid #3b444c;text-align:left}
th{position:sticky;top:76px;background:#22282d}
a{color:#9ecaf1}
@media(max-width:950px){.grid{grid-template-columns:1fr}}
</style></head><body>
<header><h1>Soul World Overmap — Structural Review</h1>
<p>Non-UE campaign geography. Macro-region and place names remain editable; topology is the current review target.</p></header>
<main>
<div class="card"><img src="soul_world_overmap_v1.svg"></div>
<div class="grid">
<div class="card"><h2>Founder slice</h2><img src="soul_founder_slice_overmap_v1.svg"></div>
<div class="card"><h2>Current gates</h2>
<p><b>36 strategic regions · 51 links · six faction seats.</b></p>
<p>Founder slice: nine regions and three short Human→Orc approaches.</p>
<p>One campaign-region move is one AP. Edge cost 6–10 is logistics/readiness/supply pressure, not AP.</p>
<p><a href="route_analysis.md">Route analysis</a> · <a href="validation.json">Validation</a></p>
</div></div>
<div class="card"><h2>Region catalogue</h2><table>
<tr><th>Region</th><th>Macro region</th><th>Owner</th><th>Feature</th><th>Battle recipe</th></tr>
"""+"".join(rows)+"""</table></div>
</main></body></html>"""
OUT.write_text(page,encoding="utf-8")
print("WROTE",OUT)
