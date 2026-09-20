import hashlib, json, os, shutil, subprocess, tempfile, zipfile
from pathlib import Path

PROJECT = Path(r"D:\RefinedBadger\Games\Soul")
UPROJECT = PROJECT / "Soul.uproject"
FOUNDATION = Path(r"D:\RefinedBadger\Deps\RefinedBadger-Foundation-2")
MANIFEST_PATH = FOUNDATION / "Plugins" / "RBFoundation" / "StackManifest.json"
GH = Path(r"C:\Users\Jeff\Documents\GitHub")
PLUGINS = PROJECT / "Plugins"
CONFIG = PROJECT / "Config"

manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8-sig"))
selected = list(manifest["profiles"]["core"])
products = {p["id"]: p for p in manifest["products"]}

repo_overrides = {
    "RBSave": GH / "RefinedBadger-Save",
    "RBItemEconomy": GH / "RefinedBadger-ItemEconomy",
    "RBRoutine": GH / "RefinedBadger-Routine",
    "RBWeather": GH / "RBWeather",
    "RBOptimization": GH / "RefinedBadger-Optimization",
    "RefinedBadgerCombat": GH / "RefinedBadger-Combat",
}

def run(*args):
    return subprocess.check_output([str(a) for a in args], text=True, stderr=subprocess.STDOUT).strip()

def ensure_commit(repo: Path, commit: str):
    try:
        run("git", "-C", repo, "cat-file", "-e", f"{commit}^{{commit}}")
    except subprocess.CalledProcessError:
        subprocess.check_call(["git", "-C", str(repo), "fetch", "origin", commit])
        run("git", "-C", repo, "cat-file", "-e", f"{commit}^{{commit}}")

def export_paths(repo: Path, commit: str, paths: list[str], dest: Path):
    with tempfile.TemporaryDirectory() as td:
        archive = Path(td) / "payload.zip"
        cmd = ["git", "-C", str(repo), "archive", "--format=zip", "-o", str(archive), commit]
        cmd.extend(paths)
        subprocess.check_call(cmd)
        with zipfile.ZipFile(archive) as z:
            z.extractall(td)
        for rel in paths:
            src = Path(td) / rel
            if not src.exists():
                raise RuntimeError(f"Archive path missing: {repo} {commit} {rel}")
            target = dest / Path(rel).name if len(paths) > 1 else dest
            if len(paths) == 1 and src.is_dir():
                shutil.copytree(src, dest, dirs_exist_ok=False)
            elif src.is_dir():
                shutil.copytree(src, target, dirs_exist_ok=False)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, target)

def tree_hash(root: Path) -> str:
    h = hashlib.sha256()
    for p in sorted(x for x in root.rglob("*") if x.is_file()):
        rel = p.relative_to(root).as_posix().encode()
        h.update(rel); h.update(b"\0")
        with p.open("rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                h.update(chunk)
        h.update(b"\0")
    return h.hexdigest()

PLUGINS.mkdir(parents=True, exist_ok=True)
CONFIG.mkdir(parents=True, exist_ok=True)

components = []
foundation_commit = run("git", "-C", FOUNDATION, "rev-parse", "HEAD")
for pid, srcname in [("RBFoundation","RBFoundation"),("RBFoundationLegacyAdapters","RBFoundationLegacyAdapters")]:
    src = FOUNDATION / "Plugins" / srcname
    dst = PLUGINS / srcname
    if dst.exists():
        raise RuntimeError(f"Refusing existing plugin destination: {dst}")
    shutil.copytree(src, dst)
    components.append({
        "id": pid, "plugin": srcname, "version": manifest["foundation"]["version"],
        "commit": foundation_commit, "payload_hash": tree_hash(dst),
        "source_kind": "foundation" if pid == "RBFoundation" else "foundation-legacy",
    })

for pid in selected:
    p = products[pid]
    repo = repo_overrides[pid]
    ensure_commit(repo, p["commit"])
    dst = PLUGINS / p["plugin"]
    if dst.exists():
        raise RuntimeError(f"Refusing existing plugin destination: {dst}")
    if p.get("payload_paths"):
        dst.mkdir(parents=True, exist_ok=False)
        export_paths(repo, p["commit"], list(p["payload_paths"]), dst)
    else:
        export_paths(repo, p["commit"], [p["source_path"]], dst)
    components.append({
        "id": pid, "plugin": p["plugin"], "version": p["version"],
        "commit": p["commit"], "payload_hash": tree_hash(dst), "source_kind": "manifest",
    })

project = json.loads(UPROJECT.read_text(encoding="utf-8-sig"))
project["Plugins"] = [{"Name": c["plugin"], "Enabled": True} for c in components]
UPROJECT.write_text(json.dumps(project, indent=2) + "\n", encoding="utf-8")

lock = {
    "schema": 2,
    "foundation_version": manifest["foundation"]["version"],
    "engine": manifest["foundation"]["engine"],
    "profile": "core",
    "manifest_sha256": hashlib.sha256(MANIFEST_PATH.read_bytes()).hexdigest(),
    "selected_products": sorted(selected),
    "components": sorted(components, key=lambda x: x["id"]),
    "installer": "Soul Python manifest staging fallback; PowerShell policy unchanged",
}
(CONFIG / "RBFoundation.lock.json").write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
print("RB_STACK_STAGED")
for c in lock["components"]:
    print(c["id"], c["version"], c["commit"][:8], c["payload_hash"][:12])
