"""Summarize exercised pilot evidence; a process exit alone never qualifies it."""
import hashlib
import json
from pathlib import Path
import re
import shutil
from review_unreal_log import review


def main():
    root = Path(__file__).resolve().parents[2]
    evidence = root / "Evidence/TitanPilot-20261005"
    read = lambda name: json.loads((evidence / name).read_text(encoding="utf-8-sig"))
    receipt = read("clifftop_supplement_transfer.json")
    mismatches = []
    for row in receipt["files"]:
        file = root / "Content" / row["relative_file"]
        if not file.is_file() or hashlib.sha256(file.read_bytes()).hexdigest() != row["sha256"]:
            mismatches.append(row["relative_file"])
    runtime = (evidence / "Runtime.log").read_text(encoding="utf-8-sig", errors="replace")
    mapcheck = (evidence / "CreatePilot.log").read_text(encoding="utf-8-sig", errors="replace")
    markers = [line for line in runtime.splitlines() if "SOUL_TITAN_" in line]
    runtime_review = review(runtime)
    load_errors = runtime_review["unresolved_load_errors"]
    required = ("BEGIN", "LOADED", "NAV", "INTERIOR", "INPUT", "TRAVERSED", "PERSISTENCE_PASS", "PASS")
    missing = [name for name in required if not any("SOUL_TITAN_" + name + " " in line for line in markers)]
    stages = {name: read(name + "-attempt.json") for name in ("BuildEditor", "Audit", "CreatePilot", "Runtime")}
    passed = (not mismatches and not load_errors and not missing
              and all(r.get("status") == "EXIT_ZERO_REQUIRES_EVIDENCE_REVIEW" for r in stages.values())
              and not any("SOUL_TITAN_FAIL" in line for line in markers))
    result = dict(status="PASS_CLIFFTOP_TRAVERSAL_PILOT" if passed else "FAIL",
                  map="/Game/Soul/Maps/Soul_TitanPilot", donor=receipt["seed"],
                  donor_files=receipt["file_count"], donor_bytes=receipt["bytes"],
                  hash_mismatches=mismatches, runtime_load_errors=load_errors,
                  resolved_editor_warnings=runtime_review["resolved_editor_warnings"],
                  missing_runtime_markers=missing, runtime_markers=markers,
                  mapcheck_markers=[line for line in mapcheck.splitlines() if "SOUL_TITAN_MAP_CHECK" in line],
                  mapcheck_warning_count=sum(bool(re.search(r"\]MapCheck: Warning:", line)) for line in mapcheck.splitlines()),
                  mapcheck_error_count=sum(bool(re.search(r"\]MapCheck: Error:", line)) for line in mapcheck.splitlines()),
                  free_bytes_after=shutil.disk_usage(root).free,
                  limitations=["Clifftop only; Sulfur remains stopped.",
                               "Traversal pilot does not exercise Dragon Graveyard battle handoff.",
                               "HLOD generation intentionally deferred for this compact staging world."])
    if result["mapcheck_error_count"] or result["free_bytes_after"] < 15 * 1024**3:
        result["status"] = "FAIL"
    cook = evidence / "CookWindows-attempt.json"
    if cook.exists(): result["cook"] = read(cook.name)
    (evidence / "acceptance.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    return 0 if result["status"].startswith("PASS") else 1


if __name__ == "__main__":
    raise SystemExit(main())
