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
import re

from audit_packages import FORBIDDEN, SEEDS, package_file

METHOD = "UE AssetRegistry per-file hard+soft closure with recursive map companions"
PASSIVE_ROOTS = ("/Game/Environment/", "/Game/__ExternalActors__/Environment/",
                 "/Game/__ExternalObjects__/Environment/")
PASSIVE_SUPPORT = {
    "/Game/Blueprint/FoliageInteraction/MPC_Player": {"/Script/Engine.MaterialParameterCollection"},
    "/Game/Blueprint/FoliageInteraction/RT_Player": {"/Script/Engine.CanvasRenderTarget2D"},
}
# Exact native-class-reviewed material dependencies of Clifftop's base shader.
# This is not permission to copy Landscape actors, HLODs or the whole directory.
PASSIVE_LANDSCAPE = {p: set(classes) for p, classes in json.loads(
    Path(__file__).with_name("clifftop_passive_landscape.json").read_text(encoding="utf-8")).items()}
ENGINE_SCRIPTS = {
    "/Script/CoreUObject", "/Script/Engine", "/Script/NavigationSystem", "/Script/UnrealEd",
    "/Script/BlueprintGraph", "/Script/PhysicsCore", "/Script/Landscape", "/Script/Foliage",
    "/Script/InterchangeCore", "/Script/InterchangeEngine", "/Script/InterchangeFactoryNodes",
    "/Script/InterchangePipelines",
}


def environment_actor_class(row, by_package, covered_companions):
    """Only a covered external actor with a fully scanned environment BP qualifies."""
    package = row["package"]
    if not package.startswith("/Game/__ExternalActors__/Environment/") or package not in covered_companions:
        raise ValueError("Incomplete registry scan outside covered external actor: " + package)
    if len(row["classes"]) != 1:
        raise ValueError("Unresolved external actor class: " + package)
    match = re.fullmatch(r"(/Game/Environment/(?:[A-Za-z0-9_]+/)*([A-Za-z0-9_]+))\.\2_C", row["classes"][0])
    if not match:
        raise ValueError("Unresolved external actor class: " + package)
    class_package = match[1]
    if row.get("class_dependencies") != [class_package]:
        raise ValueError("Missing explicit native class edge: " + package)
    if row.get("serialized_read_complete") is not True or not isinstance(row.get("serialized_dependencies"), list):
        raise ValueError("Incomplete serialized instance dependency read: " + package)
    blueprint = by_package.get(class_package)
    if (not blueprint or not blueprint["hard_query_found"] or not blueprint["all_query_found"]
            or set(blueprint["classes"]) != {"/Script/Engine.Blueprint"}):
        raise ValueError("Class package lacks complete native Blueprint queries: " + class_package)
    return class_package


def build_plan(report, donor, seed, destination, prior_receipt=None):
    existing = {}
    if prior_receipt is not None:
        if prior_receipt.get("status") != "VERIFIED" or prior_receipt.get("seed") != seed:
            raise ValueError("Existing transfer receipt is not verified for this donor")
        existing = {r["relative_file"]: r for r in prior_receipt["files"]}
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
    explicit_companions = {dep for r in rows for dep in r["companions"] if dep.startswith("/Game/")}
    by_package = {r["package"]: r for r in rows}
    reachable, pending = set(), [seed]
    while pending:
        package = pending.pop()
        if package in reachable or package not in by_package:
            continue
        reachable.add(package)
        row = by_package[package]
        pending.extend(row["hard"] + row["soft"] + row["companions"] + row.get("class_dependencies", []) + row.get("serialized_dependencies", []))
    if packages != reachable:
        raise ValueError("Unreferenced packages cannot be admitted: " + ", ".join(sorted(packages - reachable)))
    files, actor_class_fallbacks = [], []
    passive_support = dict(PASSIVE_SUPPORT)
    if seed == SEEDS["ClifftopMine"]:
        passive_support.update(PASSIVE_LANDSCAPE)
    for row in rows:
        package = row["package"]
        is_passive_support = package in passive_support
        if (package.startswith(FORBIDDEN) or not package.startswith(PASSIVE_ROOTS)) and not is_passive_support:
            raise ValueError("Package needs explicit environment-only review: " + package)
        if is_passive_support and set(row["classes"]) != passive_support[package]:
            raise ValueError("Passive support class mismatch: " + package)
        if not row["classes"]:
            raise ValueError("Incomplete registry scan: " + package)
        query_complete = row["hard_query_found"] and row["all_query_found"]
        if not query_complete:
            class_package = environment_actor_class(row, by_package, explicit_companions)
            actor_class_fallbacks.append(dict(package=package, class_package=class_package))
        if any("WorldPartitionHLOD" in cls or "/Script/Titan" in cls for cls in row["classes"]):
            raise ValueError("Excluded actor class: " + package)
        for dep in row["hard"] + row["soft"] + row["companions"] + row.get("class_dependencies", []) + row.get("serialized_dependencies", []):
            if dep.startswith("/Game/") and dep not in packages:
                raise ValueError("Closure is incomplete at: " + dep)
            if not dep.startswith(("/Game/", "/Engine/")) and dep not in ENGINE_SCRIPTS:
                raise ValueError("External dependencies require explicit review: " + dep)
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
            record = dict(package=package, relative_file=relative.as_posix(),
                          bytes=path.stat().st_size, sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            if target.exists():
                if (existing.get(relative.as_posix()) != record
                        or hashlib.sha256(target.read_bytes()).hexdigest() != record["sha256"]):
                    raise ValueError("Refuse existing destination, preserve rollback: " + str(target))
            elif relative.as_posix() in existing:
                raise ValueError("Previously transferred file is missing: " + str(target))
            files.append(record)
    if set(existing) - {r["relative_file"] for r in files}:
        raise ValueError("Prior transfer is not a subset of the current native closure")
    total = sum(row["bytes"] for row in files)
    if total > 1024**3:
        raise ValueError("Pilot closure exceeded 1 GiB safety budget")
    free = shutil.disk_usage(destination).free
    if free - total < 15 * 1024**3:
        raise ValueError("Plan would consume the 15 GiB build/cook reserve")
    return dict(seed=seed, status="REVIEW_REQUIRED_NO_COPY_PERFORMED", bytes=total,
                package_count=len(rows), file_count=len(files), free_bytes_before=free,
                actor_class_fallbacks=actor_class_fallbacks,
                verified_existing_files=sorted(existing.values(), key=lambda f: f["relative_file"]),
                files=sorted(files, key=lambda f: f["relative_file"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--donor", type=Path, required=True)
    parser.add_argument("--seed", choices=SEEDS, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--prior-receipt", type=Path)
    args = parser.parse_args()
    workspace = Path(__file__).resolve().parents[2]
    if not args.output.resolve().is_relative_to(workspace):
        parser.error("Plan output must remain inside this Soul worktree")
    plan = build_plan(json.loads(args.audit.read_text(encoding="utf-8-sig")), args.donor,
                      SEEDS[args.seed], workspace,
                      json.loads(args.prior_receipt.read_text(encoding="utf-8-sig")) if args.prior_receipt else None)
    args.output.write_text(json.dumps(plan, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in plan.items() if k not in ("files", "verified_existing_files", "actor_class_fallbacks")}))


if __name__ == "__main__":
    main()
