import hashlib
import json
import shutil
from pathlib import Path

PROJECT = Path(r"D:\RefinedBadger\Worktrees\Soul-realtime-battle-20260921")
RBMAGIC = Path(r"D:\RefinedBadger\Tools\RBMagic")
BRIDGE = Path(r"D:\RefinedBadger\Tools\RBMagicFabBridge")
PLUGIN_DEST = PROJECT / "Plugins" / "RBMagic"
PROVIDER_SRC = BRIDGE / "Content" / "MagicSpells"
PROVIDER_DEST = PROJECT / "Content" / "MagicSpells"

def tree_hash(root: Path) -> str:
    h = hashlib.sha256()
    for p in sorted(x for x in root.rglob("*") if x.is_file()):
        h.update(p.relative_to(root).as_posix().encode("utf-8"))
        h.update(b"\0")
        with p.open("rb") as handle:
            for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                h.update(chunk)
        h.update(b"\0")
    return h.hexdigest()

if PLUGIN_DEST.exists():
    raise SystemExit(f"Refusing existing plugin destination: {PLUGIN_DEST}")
if PROVIDER_DEST.exists():
    raise SystemExit(f"Refusing existing provider destination: {PROVIDER_DEST}")
PLUGIN_DEST.mkdir(parents=True)
shutil.copy2(RBMAGIC / "RBMagic.uplugin", PLUGIN_DEST / "RBMagic.uplugin")
for rel in ["Source", "Content", "Config"]:
    src = RBMAGIC / rel
    if src.exists():
        shutil.copytree(src, PLUGIN_DEST / rel)

shutil.copytree(PROVIDER_SRC, PROVIDER_DEST)

up = PROJECT / "Soul.uproject"
project = json.loads(up.read_text(encoding="utf-8-sig"))
plugins = {item["Name"]: item for item in project.get("Plugins", [])}
plugins["RBMagic"] = {"Name": "RBMagic", "Enabled": True}
project["Plugins"] = list(plugins.values())
up.write_text(json.dumps(project, indent=2) + "\n", encoding="utf-8")

lock = {
    "schema": 1,
    "source": str(RBMAGIC),
    "provider_source": str(PROVIDER_SRC),
    "plugin_hash": tree_hash(PLUGIN_DEST),
    "provider_hash": tree_hash(PROVIDER_DEST),
    "qualified_provider_assets": 305,
    "qualified_niagara_systems": 71,
    "qualified_load_failures": 0,
}
lock_path = PROJECT / "Config" / "RBMagic.lock.json"
lock_path.write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
print("RBMAGIC_STAGED")
print(json.dumps(lock, indent=2))

