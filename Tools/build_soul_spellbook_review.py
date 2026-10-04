"""Build the local Soul spellbook review from real, inspected UE captures."""
from pathlib import Path
from html import escape
import shutil

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "Evidence/Spellbook-20261004"
PREVIEW = Path(r"D:\RefinedBadger\AssetLibraries\SoulTerrainPreview")
DEST = PREVIEW / "Spellbook-20261004"
DEST.mkdir(parents=True, exist_ok=True)
shots = EVIDENCE / "ReadabilityUser/Saved/Screenshots/Spellbook"
items = [
    ("Full battle regression: resolved", EVIDENCE / "ReadabilityUser/Saved/Screenshots/Readability/result.png", "result.png"),
    ("Before: previous HUD", ROOT / "Evidence/BattleExpansion-20261003/Inspected-1280/deployment-commander.png", "before.png"),
    ("Compact controls during battle - 1080p", shots / "1920x1080-compact-live.png", "compact-1080.png"),
    ("Spellbook with hover details - 1080p", shots / "1920x1080-open-book.png", "book-1080.png"),
    ("Blizzard targeting preview - 1080p", shots / "1920x1080-ground-target-preview.png", "target-1080.png"),
    ("Compact controls - 720p", shots / "1280x720-compact-live.png", "compact-720.png"),
    ("Spellbook - 720p", shots / "1280x720-open-book.png", "book-720.png"),
    ("Quick-slot hover information - 720p", shots / "1280x720-quick-slot-details.png", "tooltip-720.png"),
    ("Book in hero view - 720p", shots / "1280x720-hero-book.png", "hero-book.png"),
    ("Hero controls restored - 720p", shots / "1280x720-hero-controls-restored.png", "hero-restored.png"),
]
cards = []
for title, source, name in items:
    if not source.is_file():
        raise RuntimeError(f"Capture missing: {source}")
    shutil.copy2(source, DEST / name)
    url = f"Spellbook-20261004/{name}"
    cards.append(f'<figure><h2>{escape(title)}</h2><a href="{url}" target="_blank"><img loading="lazy" src="{url}" alt="{escape(title)}"></a></figure>')
html = """<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Soul - live spellbook and compact battle HUD</title>
<style>
:root{color-scheme:dark}body{margin:0;background:#141a1c;color:#e7e1d0;font:17px/1.55 system-ui,sans-serif}
main{max-width:1440px;margin:auto;padding:30px}h1{font-size:32px;color:#e6bd6b;margin:0}
h2{font-size:20px;margin:0 0 12px}p{max-width:1000px;color:#bfcbc9}strong{color:#eee3bf}
kbd{background:#303a3d;border:1px solid #6c7776;border-radius:4px;padding:2px 7px}
figure{margin:25px 0;padding:16px;background:#20292c;border:1px solid #59605c;border-radius:8px}
img{display:block;width:100%;height:auto}a{color:#9cdbe7}.note{border-left:3px solid #be9853;padding-left:15px}
</style><main><h1>Soul - live spellbook</h1>
<p>Real Dragon Graveyard runtime captures, inspected at 1280 x 720 and 1920 x 1080.
Click a screenshot for full resolution. The closed interface keeps the battlefield clear;
the open grimoire occupies approximately 16% of a 16:9 screen.</p>
<p><kbd>K</kbd> or <strong>Spellbook</strong> opens the book.
Hover a spell to read its effect, target, mana cost and cooldown.
Select a spell, then click the battlefield to confirm. <kbd>1-5</kbd> also ready spells.
<kbd>RMB</kbd> cancels targeting. <kbd>Esc</kbd> closes menus or cancels targeting first.</p>
<p><strong>Opening the book never pauses.</strong> <kbd>P</kbd> / <kbd>Space</kbd> pauses separately.
Spells may be selected while paused, but cast only after resuming.
Disable <strong>Tactical pause</strong> during deployment for a continuously live battle.</p>
<p class="note">This is a HUD/control qualification. Automated runtime checks use actual HUD hitboxes and action handlers.
It does not substitute for Jeff's next hands-on playtest or a physical controller test.
Full battle regression: 40 active slots, five accepted spells, five reinforcement waves, 519 accepted contacts, zero allied targets, resolution in 89.65 seconds. Existing battlefield art and animation limitations remain outside this interface change.</p>
""" + "\n".join(cards) + "</main></html>"
(PREVIEW / "soul-spellbook.html").write_text(html, encoding="utf-8")
print(PREVIEW / "soul-spellbook.html")
