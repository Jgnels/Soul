"""Produce a hashed, bounded migration plan from a CLEAN native UE audit.

Deliberately has no copy mode: first qualification must inspect native audit
edges/classes and external plugin dependencies. Binary preflight cannot unlock it.
Paths stay /Game/...; .umap companion packages and nested maps are explicit.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

from audit_packages import FORBIDDEN, SEEDS, package_file

METHOD = "UE AssetRegistry per-file hard+soft closure with recursive map companions"
PASSIVE_ROOTS = ("/Game/Environment/", "/Game/__ExternalActors__/Environment/",
                 "/Game/__ExternalObjects__/Environment/")
ENGINE_SCRIPTS = {
    "/Script/CoreUObject", "/Script/Engine", "/Script/NavigationSystem", "/Script/UnrealEd",
    "/Script/BlueprintGraph", "/Script/PhysicsCore", "/Script/Landscape", "/Script/Foliage",
    "/Script/InterchangeCore", "/Script/InterchangeEngine", "/Script/InterchangeFactoryNodes",
    "/Script/InterchangePipelines",
}


def build_plan(report, donor, seed, destination):
    if report.get("method") != METHOD:
        raise ValueError("Requires native UE AssetRegistry audit; binary evidence is insufficient")
    if seed not in SEEDS.values():
        raise ValueError("Seed is outside the two authorized pilot donors")
    candidates = [d for d in report["donors"] if d["seed"] == seed]
    if len(candidates) != 1:
        raise ValueError("Expected exactly one donor record")
    result = candidates[0]
    if result["blocked"] or result["missing"]:
        raise ValueError("Unresolved forbidden or missing packages; donor remains stopped")
    content = (donor / "Content").resolve()
    if Path(report["donor_content"]).resolve() != content:
        raise ValueError("Audit donor root does not match source")
    if destination.resolve().is_relative_to(donor.resolve()):
        raise ValueError("Destination cannot be inside the read-only donor")
    unresolved = [p for p in result["external"] if not p.startswith("/Engine/") and p not in ENGINE_SCRIPTS]
    if unresolved:
        raise ValueError("External dependencies require explicit review: " + ", ".join(unresolved))
    rows = result["packages"]
    packages = {r["package"] for r in rows}
    if len(packages) != len(rows) or seed not in packages:
        raise ValueError("Duplicate packages or missing root map")
    files = []
    for row in rows:
        package = row["package"]
        if package.startswith(FORBIDDEN) or not package.startswith(PASSIVE_ROOTS):
            raise ValueError("Package needs explicit environment-only review: " + package)
        if not row["hard_query_found"] or not row["all_query_found"] or not row["classes"]:
            raise ValueError("Incomplete registry scan: " + package)
        if any("WorldPartitionHLOD" in cls or "/Script/Titan" in cls for cls in row["classes"]):
            raise ValueError("Excluded actor class: " + package)
        for dep in row["hard"] + row["soft"] + row["companions"]:
            if dep.startswith("/Game/") and dep not in packages:
                raise ValueError("Closure is incomplete at: " + dep)
        source = package_file(content, package)
        if source is None or source.resolve() != Path(row["file"]).resolve():
            raise ValueError("Package/source mismatch: " + package)
        # Catch incomplete native reports, including companions omitted at any nested map.
        if source.suffix == ".umap":
            from audit_packages import companions
            if set(companions(content, package)) != set(row["companions"]):
                raise ValueError("Companion tree changed or was omitted: " + package)
        if source.stat().st_size != row["bytes"]:
            raise ValueError("Donor changed since audit: " + package)
        for path in [source] + [source.with_suffix(e) for e in (".uexp", ".ubulk", ".uptnl") if source.with_suffix(e).exists()]:
            relative = path.relative_to(content)
            target = destination / "Content" / relative
            if target.exists():
                raise ValueError("Refuse existing destination, preserve rollback: " + str(target))
            files.append(dict(package=package, relative_file=relative.as_posix(),
                              bytes=path.stat().st_size, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    total = sum(row["bytes"] for row in files)
    if total > 1024**3:
        raise ValueError("Pilot closure exceeded 1 GiB safety budget")
    free = shutil.disk_usage(destination).free
    if free - total < 15 * 1024**3:
        raise ValueError("Plan would consume the 15 GiB build/cook reserve")
    return dict(seed=seed, status="REVIEW_REQUIRED_NO_COPY_PERFORMED", bytes=total,
                package_count=len(rows), file_count=len(files), free_bytes_before=free,
                files=sorted(files, key=lambda f: f["relative_file"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--donor", type=Path, required=True)
    parser.add_argument("--seed", choices=SEEDS, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    workspace = Path(__file__).resolve().parents[2]
    if not args.output.resolve().is_relative_to(workspace):
        parser.error("Plan output must remain inside this Soul worktree")
    plan = build_plan(json.loads(args.audit.read_text(encoding="utf-8-sig")), args.donor,
                      SEEDS[args.seed], workspace)
    args.output.write_text(json.dumps(plan, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in plan.items() if k != "files"}))


if __name__ == "__main__":
    main()
