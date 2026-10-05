"""Read-only package-string preflight. This is NOT AssetRegistry qualification.

Walks both external companion trees for every discovered map. Preserves the
reference chain to forbidden/missing packages instead of silently excluding it.
Never loads UE, copies assets, writes donor files, or authorizes migration.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import shutil

SEEDS = {
    "ClifftopMine": "/Game/Environment/Clifftop/Level_Instance/IL_Clifftop_Mine_TownGrid",
    "SulfurBandit": "/Game/Environment/Sulfur/Level_Instances/LI_Sulfur_BanditRestOutpost",
}
FORBIDDEN = (
    "/Game/Blueprint/", "/Game/Characters/", "/Game/Maps/",
    "/Game/__ExternalActors__/Maps/", "/Game/__ExternalObjects__/Maps/",
    "/Script/Titan", "/SampleFramework/", "/InventorySystem/", "/QuestSystem/",
    "/PlayerInteraction/", "/TitanMovement/", "/TitanCamera/", "/TitanRaft/",
    "/DayNightCycle/", "/Game/BlueprintDEV/",
)
REF = re.compile(rb"/(?:Game|Engine|Script|PCG|Niagara|Water|Landmass|VirtualHeightfieldMesh|"
                 rb"SampleFramework|InventorySystem|QuestSystem|PlayerInteraction|TitanMovement|"
                 rb"TitanCamera|TitanRaft|DayNightCycle)/[A-Za-z0-9_/]+")
WIDE_REF = re.compile(rb"/\x00(?:[A-Za-z0-9_]\x00){2,64}/\x00(?:[A-Za-z0-9_/]\x00)+")


def package_file(content, package):
    if not package.startswith("/Game/"):
        return None
    relative = package[6:]
    if any(p in ("", ".", "..") for p in relative.split("/")):
        raise ValueError(f"Invalid package: {package}")
    for extension in (".uasset", ".umap"):
        candidate = content / (relative + extension)
        if candidate.is_file():
            return candidate
    return None


def companions(content, package):
    for tree in ("__ExternalActors__", "__ExternalObjects__"):
        folder = content / tree / package[6:]
        if folder.is_dir():
            for path in sorted(folder.rglob("*")):
                if path.suffix.lower() in (".uasset", ".umap"):
                    yield "/Game/" + path.relative_to(content).with_suffix("").as_posix()


def references(data):
    # UE FNames can be ANSI or UTF-16. Loose matches remain conservative evidence.
    wide = b"\x00".join(s.decode("utf-16-le").encode("ascii") for s in WIDE_REF.findall(data))
    return sorted({m.decode("ascii").rstrip("/") for raw in (data, wide)
                   for m in REF.findall(raw)})


def audit(content, seed, limit=10000):
    queue = collections.deque([seed])
    parents = {seed: None}
    records, forbidden, missing, external = [], [], [], set()
    while queue:
        package = queue.popleft()
        if package.startswith(FORBIDDEN):
            forbidden.append(package)
            continue  # Fail closed at boundary, not full gameplay closure.
        path = package_file(content, package)
        if not path:
            if package.startswith("/Game/"):
                missing.append(package)
            else:
                external.add(package)
            continue
        data = path.read_bytes()
        deps = set(references(data))
        if path.suffix == ".umap":
            deps.update(companions(content, package))
        records.append(dict(package=package, relative_file=path.relative_to(content).as_posix(),
                            bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                            references=sorted(deps)))
        for dep in sorted(deps):
            if dep not in parents:
                parents[dep] = package
                queue.append(dep)
        if len(parents) > limit:
            raise RuntimeError(f"Closure exceeded {limit} nodes; no migration permitted")

    def chain(package):
        result = []
        while package is not None:
            result.append(package)
            package = parents[package]
        return list(reversed(result))

    return dict(seed=seed, method="binary strings; not hard/soft reference authority",
                migration_permitted=False, package_count=len(records),
                bytes=sum(r["bytes"] for r in records),
                forbidden=[dict(package=p, chain=chain(p)) for p in sorted(forbidden)],
                missing=[dict(package=p, chain=chain(p)) for p in sorted(missing)],
                external=sorted(external), records=sorted(records, key=lambda r: r["package"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--donor", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    content = (args.donor / "Content").resolve()
    output = args.output.resolve()
    workspace = Path(__file__).resolve().parents[2]
    if not output.is_relative_to(workspace) or output.is_relative_to(args.donor.resolve()):
        parser.error("Evidence output must be inside this Soul worktree, outside donor")
    results = {name: audit(content, seed) for name, seed in SEEDS.items()}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(donor=str(args.donor.resolve()),
        free_bytes=shutil.disk_usage(workspace).free, donors=results), indent=2), encoding="utf-8")
    for name, result in results.items():
        print(name, json.dumps(dict(package_count=result["package_count"], bytes=result["bytes"],
            forbidden=result["forbidden"], missing_count=len(result["missing"]), external=result["external"])))


if __name__ == "__main__":
    main()
