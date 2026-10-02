"""Run inside the authorized Soul editor only after prepare_soul_battle_audio.py.
Imports new local derivative packages; never overwrites or saves donor assets.
"""
import hashlib
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
if (root / "Soul.uproject").is_file() is False:
    raise RuntimeError("Run only in the Soul worker project")
manifest = json.loads((root / "Data/BattleAudioLocal/manifest.json").read_text(encoding="utf-8"))
if len(manifest) != 11 or len({x["name"] for x in manifest}) != len(manifest):
    raise RuntimeError("Expected eleven distinct reviewed battle clips")
tasks = []
for clip in manifest:
    path = Path(clip["file"]).resolve()
    if path.parent != (root / "Data/BattleAudioLocal").resolve():
        raise RuntimeError("Prepared recording must be inside this project's local audio directory")
    if hashlib.sha256(path.read_bytes()).hexdigest() != clip["sha256"]:
        raise RuntimeError("Prepared audio checksum changed: " + clip["name"])
    package = clip["package"]
    if package != "/Game/Soul/Audio/Battle/" + clip["name"] or not clip["name"].isalnum():
        raise RuntimeError("Unreviewed output package")
    if unreal.EditorAssetLibrary.does_asset_exist(package):
        raise RuntimeError("Existing package requires explicit review; no overwrite: " + package)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", "/Game/Soul/Audio/Battle")
    task.set_editor_property("destination_name", clip["name"])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    task.set_editor_property("factory", unreal.SoundFactory())
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
results = []
for clip, task in zip(manifest, tasks):
    asset = unreal.EditorAssetLibrary.load_asset(clip["package"])
    if not isinstance(asset, unreal.SoundWave):
        raise RuntimeError("Imported sound missing: " + clip["package"])
    results.append({"package": clip["package"], "class": asset.get_class().get_name()})
(root / "Data/BattleAudioLocal/import-receipt.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
unreal.log("SOUL_AUDIO_IMPORT_READY: %d; listening and runtime qualification still required" % len(results))
