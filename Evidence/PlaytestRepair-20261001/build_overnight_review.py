"""Local inspected evidence gallery; never modifies licensed assets."""
from pathlib import Path
import shutil,html,json
ROOT=Path(__file__).resolve().parents[2]
E=Path(__file__).resolve().parent
OUT=Path(r"D:/RefinedBadger/AssetLibraries/SoulTerrainPreview")
IM=OUT/'SoulOvernight-20261002';IM.mkdir(exist_ok=True)
rows=[
('Your reported camera problem',Path(r"C:/Users/Jeff/AppData/Local/Temp/codex-clipboard-a735925e-6dca-47c6-9171-f9021f447ece.png"),'User-supplied before screenshot: camera inside the hero.'),
('Third-person hero',E/'native-final-hero.png','Native mouse click switched from commander to hero. The camera is outside the body, with a forward view.'),
('First-person hero',E/'native-final-first-person.png','Native X key. The owner body is hidden; world units remain visible.'),
('Repeated pause and grounded creatures',E/'native-pause-cycle2.png','Native P resume/pause repeated, then Space resume/P pause. Position, health, ammunition and battle time remained frozen while paused. Kraken now uses mesh bounds for ground placement.'),
('Select a troop',E/'native-unit-selected.png','Clicking an allied troop selects its formation and updates the card.'),
('Focus the selected formation',E/'native-unit-focus.png','Native Focus click brings the selected troops into a closer view.'),
('Place a formation order',E/'native-final-move-marker.png','Native formation card, Move button, and ground click. Gold marker shows the accepted anchor.'),
('Native mouse spell controls',E/'native-chain-target.png','All five spells cast through native mouse controls. Firebolt and Chain Lightning target enemies, Blizzard targets ground, Ward and Tailwind use their buttons. Mana 80 to 18; pause freezes the effects for inspection.'),
('Blizzard and arrows',E/'offscreen-final-profile/blizzard.png','Rendered gameplay: falling ice marks the actual spell area; golden arrow trails show the missile volley.'),
('Chain Lightning',E/'offscreen-final-profile/chain-lightning-engagement.png','Arcs follow the actual accepted chain targets.'),
('Ward and Tailwind',E/'offscreen-pass6/tailwind.png','Ward follows the hero; green wind cues follow affected allies.'),
('Campaign capital and company',E/'defeat-recovery-final/World_initial.png','The capital is larger and surrounded by houses. The company uses an animated hero and escorts.'),
('Movement and bridge',E/'campaign-reviewed-pass3/World_frontier.png','Movement remaining is explicit. The reviewed short bridge and continuous coastal terrain remain intact.'),
('Defeat, recruit and retry',E/'defeat-recovery-final/Vertical_Campaign_Return.png','Physical defeat returns to Bridgeward. Three recruits consume the finite pool and gold; the retry receives a fresh encounter identity and its outcome persists.'),
('1920 by 1080 layout',E/'offscreen-1080-final/deployment-hero.png','Final 1080p run: unobstructed hero view and separate formations. Five casts, 320 accepted contacts, no allied targeting, 72.26-second resolution. Fixed-pixel HUD text remains small at this resolution.'),
('Battle returns to campaign',E/'campaign-reviewed-pass3/Vertical_Campaign_Return.png','100.10-second victory: 33 survivors, 18 mana, captured Ashport. Encounter identity and RBSave return verified.')
]
cards=[]
for i,(title,path,caption) in enumerate(rows):
 if not path.exists():continue
 name=f'{i:02d}-{path.name}';shutil.copy2(path,IM/name)
 url='SoulOvernight-20261002/'+name
 cards.append(f'<figure><h2>{html.escape(title)}</h2><a href="{url}" target="_blank"><img loading="lazy" src="{url}" alt="{html.escape(title)}"></a><figcaption>{html.escape(caption)}</figcaption></figure>')
report=E/'overnight-qualification.json'
receipt=''
if report.exists():
 shutil.copy2(report,IM/'qualification.json');receipt='<p><a href="SoulOvernight-20261002/qualification.json">Full qualification receipt: commits, files, tests and limitations</a></p>'
page=r"""<!doctype html><html><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Soul overnight playtest repair</title><style>body{margin:0;background:#101a1e;color:#e4e8e7;font:17px/1.55 system-ui}main{max-width:1320px;margin:auto;padding:32px}h1,h2{color:#eed9a8}h2{font-size:22px}a{color:#86d9eb}figure{margin:32px 0;padding:20px;background:#1b2b32;border:1px solid #34464c;border-radius:9px}img{display:block;width:100%;height:auto}figcaption{padding-top:12px;color:#c7d1d3}.controls{background:#25373d;padding:18px;border-left:4px solid #d6b671}code{word-break:break-all}.comparison{display:grid;grid-template-columns:1fr 1fr;gap:18px}.comparison figure{min-width:0;margin-bottom:0}@media(max-width:800px){.comparison{grid-template-columns:1fr}}</style><main><h1>Soul: campaign and physical battle playtest</h1><p>October 2, 2026. Actual Unreal and native-input captures, inspected individually. Reference goals: readable campaign navigation, controllable hero and formations, visible ranged combat and magic, physical reinforcement waves, and persistent outcomes.</p><div class="controls"><b>Play:</b> run <code>D:\RefinedBadger\Worktrees\Soul-bannerlord-campaign-map-20260929\PLAY_SOUL_VERTICAL_SLICE.cmd</code>.<br><b>Campaign:</b> click YOUR ARMY or press Home, then choose a highlighted neighboring location. Movement is shown at the top. T opens town; Space advances the day.<br><b>Battle:</b> begins paused. Click an ally or a formation card, then an order. Move here arms a ground click. P/Space or the top button pauses/resumes. C switches commander/hero; X switches first/third person. Spells 1-3 require a target; 4-5 affect your side. R returns selected formations to AI.</div>"""+receipt+'<div class="comparison">'+''.join(cards[:2])+'</div>'+''.join(cards[2:])+"""<figure><h2>Remaining work</h2><p>The slice still needs authored creature attack polish, richer battlefield composition, stronger city art, controller qualification, audio work and wider hardware testing. The first-person view currently hides the hero body and has no dedicated first-person arms. Current evidence proves the bounded Dragon Graveyard slice, not Bannerlord or Total War production scale.</p></figure></main></html>"""
(OUT/'soul-overnight.html').write_text(page,encoding='utf-8')
print(OUT/'soul-overnight.html')
