import json, shutil, subprocess, tempfile, zipfile, hashlib
from pathlib import Path

repo=Path(r"D:\RefinedBadger\Deps\RefinedBadger-UIInput")
project=Path(r"D:\RefinedBadger\Games\Soul")
commit="497b887dd04b7503498fe7e38538c52b70a67ff4"
dest=project/"Plugins"/"RBUIInput"

try:
    subprocess.check_call(["git","-C",str(repo),"cat-file","-e",commit+"^{commit}"])
except subprocess.CalledProcessError:
    subprocess.check_call(["git","-C",str(repo),"fetch","origin",commit])
    subprocess.check_call(["git","-C",str(repo),"cat-file","-e",commit+"^{commit}"])

if dest.exists():
    raise SystemExit("RBUIInput destination already exists")

with tempfile.TemporaryDirectory() as td:
    z=Path(td)/"ui.zip"
    subprocess.check_call(["git","-C",str(repo),"archive","--format=zip","-o",str(z),commit,"RBUIInput.uplugin","Source"])
    with zipfile.ZipFile(z) as arc:
        arc.extractall(td)
    dest.mkdir(parents=True)
    shutil.copy2(Path(td)/"RBUIInput.uplugin",dest/"RBUIInput.uplugin")
    shutil.copytree(Path(td)/"Source",dest/"Source")

up=project/"Soul.uproject"
data=json.loads(up.read_text(encoding="utf-8-sig"))
plugins={x["Name"]:x for x in data.get("Plugins",[])}
for name in ["CommonUI","EnhancedInput","RBUIInput"]:
    plugins[name]={"Name":name,"Enabled":True}
data["Plugins"]=list(plugins.values())
up.write_text(json.dumps(data,indent=2)+"\n",encoding="utf-8")

h=hashlib.sha256()
for p in sorted(x for x in dest.rglob("*") if x.is_file()):
    h.update(p.relative_to(dest).as_posix().encode()); h.update(b"\0"); h.update(p.read_bytes()); h.update(b"\0")
lock={"product":"RBUIInput","version":"1.0.0","commit":commit,"payload_hash":h.hexdigest(),"tier":"Studio Full / RB UI seam"}
(project/"Config"/"SoulStudioStack.lock.json").write_text(json.dumps(lock,indent=2)+"\n",encoding="utf-8")
print("RBUIINPUT_STAGED",h.hexdigest())
