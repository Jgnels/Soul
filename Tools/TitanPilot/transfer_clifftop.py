"""Exact Clifftop-only package transfer. No overwrite, source writes or deletion."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

from audit_packages import SEEDS
from prepare_migration import build_plan


def transfer(plan, donor, workspace, receipt_path):
    if plan["seed"] != SEEDS["ClifftopMine"]:
        raise ValueError("Only Clifftop is admitted for this transfer")
    content = (donor / "Content").resolve()
    destination = (workspace / "Content").resolve()
    pairs = []
    # Verify the whole set before writing the first file. Recheck each source
    # during transfer to catch concurrent donor changes.
    for row in plan["files"]:
        relative = Path(row["relative_file"])
        src, dst = (content / relative).resolve(), (destination / relative).resolve()
        if not src.is_relative_to(content) or not dst.is_relative_to(destination) or relative.is_absolute():
            raise ValueError("Path escapes approved roots")
        if dst.exists():
            raise ValueError("No-overwrite collision: " + str(dst))
        if src.stat().st_size != row["bytes"] or hashlib.sha256(src.read_bytes()).hexdigest() != row["sha256"]:
            raise ValueError("Source fingerprint changed: " + str(src))
        pairs.append((row, src, dst))
    if shutil.disk_usage(workspace).free - plan["bytes"] < 15 * 1024**3:
        raise ValueError("15 GiB reserve would be violated")
    receipt = dict(started_utc=datetime.now(timezone.utc).isoformat(), seed=plan["seed"],
                   status="COPYING", free_bytes_before=shutil.disk_usage(workspace).free, files=[])

    def save():
        receipt_path.write_text(json.dumps(receipt, indent=2), encoding="utf-8")

    save()
    try:
        for row, src, dst in pairs:
            data = src.read_bytes()
            if hashlib.sha256(data).hexdigest() != row["sha256"]:
                raise ValueError("Source changed during transfer: " + str(src))
            dst.parent.mkdir(parents=True, exist_ok=True)
            with dst.open("xb") as out:
                out.write(data)
            receipt["files"].append(row)
            save()
            if hashlib.sha256(dst.read_bytes()).hexdigest() != row["sha256"]:
                raise ValueError("Destination verification failed: " + str(dst))
        receipt.update(status="VERIFIED", bytes=sum(r["bytes"] for r in receipt["files"]),
                       file_count=len(receipt["files"]), free_bytes_after=shutil.disk_usage(workspace).free)
    except Exception as error:
        receipt.update(status="FAILED_PARTIAL_PRESERVED", error=str(error))
        raise
    finally:
        save()
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--audit", type=Path, required=True)
    args = parser.parse_args()
    workspace = Path(__file__).resolve().parents[2]
    if str(workspace).lower() != r"D:\RefinedBadger\Worktrees\Soul-titan-pilot-20261005".lower():
        parser.error("Wrong worktree")
    branch = subprocess.check_output(["git", "branch", "--show-current"], cwd=workspace, text=True).strip()
    if branch != "codex/soul-titan-pilot-20261005":
        parser.error("Wrong branch")
    donor = Path("D:/Unreal Projects/ProjectTitan")
    plan = json.loads(args.plan.read_text(encoding="utf-8-sig"))
    native = json.loads(args.audit.read_text(encoding="utf-8-sig"))
    current = build_plan(native, donor, SEEDS["ClifftopMine"], workspace)
    for key in ("seed", "bytes", "package_count", "file_count", "files", "actor_class_fallbacks"):
        if current[key] != plan[key]:
            raise ValueError("Plan changed; refuse transfer: " + key)
    receipt = transfer(plan, donor, workspace, workspace / "Evidence/TitanPilot-20261005/clifftop_transfer.json")
    print(json.dumps({k: v for k, v in receipt.items() if k != "files"}, indent=2))


if __name__ == "__main__":
    main()
