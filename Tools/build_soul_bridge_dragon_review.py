"""Publish inspected, archived runtime captures; no asset packages are touched."""
from pathlib import Path
import json
P=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview')
R=P/'SoulIntegration/EvilCorridor'
RUN='bridge-dragon-victory-02'
receipt=json.loads((R/RUN/'receipt.json').read_text())
assert receipt['runtime']['completion_marker_observed'] and receipt['runtime']['clean_shutdown']
cards=[('Bridge before','evil-victory-03','Corridor_Bridge_Travel.png'),('Bridge after: one short water crossing',RUN,'Corridor_Bridge_Travel.png'),('Battle before','evil-victory-03','Vertical_Battle.png'),('Battle after: showcase clearing, bones and lava',RUN,'Vertical_Battle.png'),('Campaign return after the physical battle',RUN,'Vertical_Campaign_Return.png')]
cards.append(('70 active combatants, 100-unit pools per side: 720p with a 15 FPS cap','bridge-dragon-stress-720-15fps','Vertical_Battle.png'))
stress=json.loads((R/'bridge-dragon-stress-720-15fps/receipt.json').read_text())
assert stress['runtime']['completion_marker_observed'] and stress['runtime']['clean_shutdown']
body=[]
for title,run,name in cards:
 assert (R/run/name).exists()
 uri=f'SoulIntegration/EvilCorridor/{run}/{name}'
 body.append(f'<section><h2>{title}</h2><a href="{uri}"><img src="{uri}" alt="{title}"></a><p><a href="SoulIntegration/EvilCorridor/{run}/receipt.json">Runtime receipt</a></p></section>')
head='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Soul · bridge and Dragon Graveyard</title><style>body{background:#121b20;color:#e7e5db;font:17px/1.6 system-ui;margin:0}main{max-width:1280px;padding:28px;margin:auto}a{color:#d5bd84}section{margin:32px 0;padding:18px;background:#1d2b31}img{display:block;width:100%;height:auto}p{max-width:980px}code{overflow-wrap:anywhere}</style><main><a href="evil-corridor.html">Full corridor review</a><h1>Short crossing · real Dragon Graveyard showcase</h1><p>The bridge follows the narrow channel, with road over the exposed land. Battle uses the eastern clearing in <code>/Game/Dragon_graveyard/Level/L_showcase_level</code>, facing the authored skull, ribcages and lava. The battle camera, HUD and transient fog settings were adjusted; no donor maps or materials were saved.</p><p>These are Unreal runtime captures from the local worker, not promotional images. The packaged RC2 has not been updated.</p>'''
(P/'bridge-dragon-touchups.html').write_text(head+''.join(body)+'<section><h2>Qualification and limits</h2><p>Editor and game builds passed. 69 Unreal automation tests, 77 tool tests and 5 campaign-view checks passed. The final campaign round trip returned 22 allied survivors, conquered Ashport and reloaded through RBSave.</p><p>The 70-active-unit stress run passed at 720p with a 15 FPS cap. The 30 FPS stress attempts at both 1080p and 720p reached the 85&deg;C cutoff and were stopped; those settings are not qualified for this heavier load. The normal 30-active-unit campaign round trip passed at a 30 FPS cap.</p><p>Lighting and troop readability still need art polish; this does not claim promotional-image parity. <a href="SoulIntegration/bridge-dragon-qualification.json">Exact files, hashes, tests and run receipts</a>.</p></section></main></html>',encoding='utf-8')
print(P/'bridge-dragon-touchups.html')
