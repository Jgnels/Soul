"""Preserve source fingerprints and test output without changing donor state."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
from datetime import datetime, timezone

WORKSPACE = Path(__file__).resolve().parents[2]
EVIDENCE = WORKSPACE / "Evidence/TitanPilot-20261005"
DONOR = Path("D:/Unreal Projects/ProjectTitan")
AUDIT_FILES = ["DONOR_AUDIT.md", "PASS3.md", "PASS4.md", "PASS5.md", "integration_manifest_v5.json",
               "clifftop_sulfur_contamination_scan.csv", "pilot_dependency_closure.json", "pilot_dependency_hard_only.json"]


def main():
    result = dict(recorded_utc=datetime.now(timezone.utc).isoformat(), worktree=str(WORKSPACE),
                  branch=subprocess.check_output(["git", "branch", "--show-current"], cwd=WORKSPACE, text=True).strip(),
                  base="6d53714559d34a123b5394206866925a165373c4",
                  free_bytes=shutil.disk_usage(WORKSPACE).free, donor_inputs=[])
    for name in AUDIT_FILES:
        path = DONOR / "Saved/RefinedBadgerAudit" / name
        data = path.read_bytes()
        result["donor_inputs"].append(dict(file=str(path), bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
    preflight = json.loads((EVIDENCE / "binary_preflight.json").read_text(encoding="utf-8"))
    files = {row["relative_file"]: row for donor in preflight["donors"].values() for row in donor["records"]}
    changed = []
    for relative, row in files.items():
        if hashlib.sha256((DONOR / "Content" / relative).read_bytes()).hexdigest() != row["sha256"]:
            changed.append(relative)
    result["rechecked_unique_donor_files"] = len(files)
    result["donor_files_changed_since_scan"] = changed
    result["binary_census_unique_bytes"] = sum(row["bytes"] for row in files.values())
    result["migrated_package_count"] = 0
    result["migrated_bytes"] = 0
    tests = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "Tools/TitanPilot", "-p", "test_*.py", "-v"],
                           cwd=WORKSPACE, capture_output=True, text=True)
    (EVIDENCE / "tooling-tests.txt").write_text(tests.stdout + tests.stderr, encoding="utf-8")
    result["tooling_test_exit_code"] = tests.returncode
    (EVIDENCE / "source_verification.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in result.items() if k != "donor_inputs"}, indent=2))
    return 1 if changed or tests.returncode else 0


if __name__ == "__main__":
    raise SystemExit(main())
