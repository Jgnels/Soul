import os, json, traceback, unreal

ROOT = r"D:\RefinedBadger\Games\Soul"
TOOLS = os.path.join(ROOT, "Tools")
OUT = os.path.join(ROOT, "Evidence")
MANIFEST = os.path.join(OUT, "visual_acceptance_batch_manifest.json")

STEPS = [
    "build_siege_damage_proof.py",
    "verify_siege_damage_proof.py",
    "build_city_overlays.py",
    "verify_city_overlays.py",
]

results = []
for filename in STEPS:
    path = os.path.join(TOOLS, filename)
    unreal.log("SOUL_VISUAL_ACCEPTANCE START " + filename)
    try:
        namespace = {
            "__file__": path,
            "__name__": "__main__",
        }
        with open(path, "r", encoding="utf-8") as handle:
            code = compile(handle.read(), path, "exec")
        exec(code, namespace, namespace)
        results.append({"step": filename, "pass": True})
        unreal.log("SOUL_VISUAL_ACCEPTANCE PASS " + filename)
    except Exception:
        err = traceback.format_exc()
        results.append({"step": filename, "pass": False, "error": err})
        unreal.log_error("SOUL_VISUAL_ACCEPTANCE FAIL " + filename + "\n" + err)
        break

passed = len(results) == len(STEPS) and all(row["pass"] for row in results)
optional = []
OPTIONAL_STEPS = [
    "audit_hivemind_nav_collision.py",
    "build_battlefield_overlays.py",
]
if passed:
    for filename in OPTIONAL_STEPS:
        path = os.path.join(TOOLS, filename)
        unreal.log("SOUL_VISUAL_ACCEPTANCE OPTIONAL_START " + filename)
        try:
            namespace = {"__file__": path, "__name__": "__main__"}
            with open(path, "r", encoding="utf-8") as handle:
                exec(compile(handle.read(), path, "exec"), namespace, namespace)
            optional.append({"step": filename, "pass": True})
            unreal.log("SOUL_VISUAL_ACCEPTANCE OPTIONAL_PASS " + filename)
        except Exception:
            err = traceback.format_exc()
            optional.append({"step": filename, "pass": False, "error": err})
            unreal.log_warning("SOUL_VISUAL_ACCEPTANCE OPTIONAL_FAIL " + filename + "\n" + err)

with open(MANIFEST, "w", encoding="utf-8") as handle:
    json.dump({"pass": passed, "steps": results, "optional": optional}, handle, indent=2)

if not passed:
    raise RuntimeError("Soul visual acceptance batch failed; see manifest")

unreal.log("SOUL_VISUAL_ACCEPTANCE DONE")
