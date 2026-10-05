"""Reject unresolved loads; distinguish the exact resolved editor HLOD warning."""
import argparse
import json
from pathlib import Path
import re


def review(text):
    errors, resolved_editor_warnings = [], []
    module_loaded = "InternalLoadLibrary: 'WorldPartitionHLODUtilities'" in text
    for line in text.splitlines():
        if not re.search(r"LoadErrors:(?! New Page:)|Fatal error:|LogLinker: Warning:.*(?:Failed|Can.t find)", line):
            continue
        if (module_loaded and "LogLinker: Warning: [AssetLog]" in line
                and "Engine/Content/Maps/Templates/HLODs/HLODLayer_Merged.uasset" in line.replace("\\", "/")
                and "VerifyImport: Failed to find script package for import object 'Package /Script/WorldPartitionHLODUtilities'" in line):
            resolved_editor_warnings.append(line)
        else:
            errors.append(line)
    return dict(unresolved_load_errors=errors, resolved_editor_warnings=resolved_editor_warnings,
                runtime_pass="SOUL_TITAN_PASS " in text and "SOUL_TITAN_FAIL " not in text)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--runtime", action="store_true")
    args = parser.parse_args()
    result = review(args.log.read_text(encoding="utf-8-sig", errors="replace"))
    print(json.dumps(result, indent=2))
    raise SystemExit(bool(result["unresolved_load_errors"]) or (args.runtime and not result["runtime_pass"]))
