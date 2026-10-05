"""Archive and qualify World terrain through the existing one-process runner.

Default is a read-only command preview. Add --execute to run after the parent
releases its editor/build session. No product settings, player saves, existing
runs, thermal guard or underlying runner semantics are changed.

Examples (use a fresh --run name for every execution):
  python Tools/WorldTerrain/qualify_world.py captures --run art-01 --execute
  python Tools/WorldTerrain/qualify_world.py input --run input-01 --execute
  python Tools/WorldTerrain/qualify_world.py load --run load-01 --load-from input-01 --execute
  python Tools/WorldTerrain/qualify_world.py performance --run perf-world-01 --focus overview --execute
  python Tools/WorldTerrain/qualify_world.py performance --run perf-heartland-01 --focus human_capital --execute

Performance numbers are the existing runtime TerrainBenchmark samples. A
technical PASS does not constitute visual approval or a frame-rate target PASS.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
PREVIEW = Path("D:/RefinedBadger/AssetLibraries/SoulTerrainPreview")
EVIDENCE = Path("Evidence/CampaignWorldTerrain-20261005")
PROFILE = Path("Data/CampaignWorldTerrain/presentation.json")
NAME = re.compile(r"^[A-Za-z0-9_-]+$")
FAILURE = re.compile(
    r"\bSOUL_[A-Z0-9_]*(?:_FAIL|_ERROR)\b|"
    r"SOUL_TERRAIN[^\n]*(?:invalid/missing|missing local licensed mesh)|"
    r"Log\w+:\s+Error:|Failed to compile Material", re.I)
MARKERS = {
    "captures": "SOUL_WORLD_CAPTURE_PASS",
    "input": "SOUL_WORLD_VISUAL_INPUT_PASS",
    "load": "SOUL_CAMPAIGN_COLD_LOAD_PASS",
    "victory": "SOUL_CAMPAIGN_ROUNDTRIP_PASS",
    "defeat": "SOUL_CAMPAIGN_ROUNDTRIP_PASS",
    "performance": "SOUL_TERRAIN_BENCHMARK_COMPLETE",
}
VIEWS = (
    "overview", "world_campaign", "human_heartland", "northern_fjords", "crownspine",
    "eastern_badlands", "greenwood", "ashen_south", "river_ford",
    "north_pass", "human_capital", "orc_frontier", "forest_close", "capital_close",
)


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def file_record(path):
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": sha(path)}


def snapshot(project, profile):
    """Hash source, profile, height, all owned terrain packages and loaded refs."""
    paths = {project / PROFILE, project / "Soul.uproject"}
    required = {project / PROFILE, project / "Content/SoulCampaignWorld/L_SoulWorld.umap"}
    terrain = profile.get("terrain", profile)
    height = project / terrain.get("heightfield_path", "Data/CampaignWorldLocal/WorldHeight.r16")
    paths.add(height)
    required.add(height)
    paths.update(project.glob("Config/*.ini"))
    paths.update(project.glob("Source/Soul/Private/SoulCampaign*.cpp"))
    paths.update(project.glob("Source/Soul/Private/SoulFounderPlaytest*.cpp"))
    paths.update(project.glob("Source/Soul/Public/SoulCampaign*.h"))
    paths.update(project.glob("Source/Soul/Public/SoulFounderPlaytest*.h"))
    paths.update(project.glob("Source/Soul/Private/SoulPlaytestRegionActor.cpp"))
    paths.update(project.glob("Source/Soul/Public/SoulPlaytestRegionActor.h"))
    paths.add(project / "Source/Soul/Soul.Build.cs")
    paths.update(project.glob("Tools/WorldTerrain/*.py"))
    paths.add(project / "Tools/qualify_soul_vertical.py")
    paths.add(project / "Binaries/Win64/UnrealEditor-Soul.dll")
    paths.update(project.glob("Data/soul_*world*.json"))
    paths.update(project.glob("Data/soul_*settlement*.json"))
    for package in ("Content/SoulCampaignWorld", "Content/Soul/Campaign/TerrainV2"):
        paths.update(p for p in (project / package).rglob("*") if p.is_file())
    paths.add(project / "Content/SoulCampaignMountain/L_evil_waterfront.umap")
    # Direct donor/material/texture references used by the presentation are
    # included without recursively copying or hashing the whole licensed library.
    for source in list(paths):
        if source.suffix not in (".cpp", ".py") or not source.is_file():
            continue
        for match in re.findall(r"/Game/[A-Za-z0-9_/]+", source.read_text(encoding="utf-8", errors="replace")):
            for suffix in (".uasset", ".umap"):
                candidate = project / "Content" / (match[6:] + suffix)
                if candidate.is_file():
                    paths.add(candidate)
    for name in ("WorldHeight.r16", "WorldHeight.png", "WorldColor.png", "WorldMasks.png", "qualification.json"):
        candidate = PREVIEW / "WorldTerrain" / name
        if candidate.is_file():
            paths.add(candidate)
    files = {str(p.relative_to(project)) if p.is_relative_to(project) else str(p): file_record(p)
             for p in sorted(paths) if p.is_file()}
    return {"files": files, "missing_required": [str(p) for p in sorted(required) if not p.is_file()]}


def player_saves(project):
    paths = []
    for subdir in ("Saved/RBSave", "Saved/SaveGames"):
        paths.extend(p for p in (project / subdir).rglob("*") if p.is_file())
    for name in ("Saved/CampaignInputExpectedSnapshot.json", "Saved/CampaignRoundtripExpectedSnapshot.json"):
        path = project / name
        if path.is_file():
            paths.append(path)
    return {str(p.relative_to(project)): sha(p) for p in sorted(paths)}


def seed_load(local_root, source_name, destination):
    """Copy only a successful isolated input run; never read player saves."""
    source = local_root / source_name
    receipt = read_json(source / "qualification.json")
    if receipt.get("mode") != "input" or receipt.get("technical_result") != "PASS":
        raise ValueError("--load-from must name a technically passed input run")
    saved = source / "User/Saved"
    expected = saved / "CampaignInputExpectedSnapshot.json"
    domain = saved / "RBSave/Domains/Soul.VerticalCampaign.domain.rbsave"
    if not expected.is_file() or not domain.is_file():
        raise ValueError("Prior input run lacks its expected snapshot or RBSave domain")
    destination_saved = destination / "Saved"
    destination_saved.mkdir(parents=True)
    shutil.copy2(expected, destination_saved / expected.name)
    shutil.copytree(saved / "RBSave", destination_saved / "RBSave")
    return {"source_run": str(source), "expected_snapshot": file_record(expected),
            "domain": file_record(domain)}


def png_dimensions(path):
    with path.open("rb") as stream:
        header = stream.read(24)
    if len(header) != 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Invalid PNG header: " + str(path))
    return list(struct.unpack(">II", header[16:24]))


def archive_outputs(run_root):
    saved, archive = run_root / "User/Saved", run_root / "artifacts"
    records = []
    candidates = list((saved / "Screenshots").rglob("*.png"))
    candidates += [saved / name for name in (
        "TerrainBenchmark.json", "TerrainBenchmark.csv", "CampaignInputExpectedSnapshot.json")]
    for source in candidates:
        if not source.is_file():
            continue
        relative = source.relative_to(saved)
        target = archive / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        row = {"saved_relative": str(relative).replace("\\", "/"), **file_record(target)}
        if source.suffix.lower() == ".png":
            try:
                row["dimensions"] = png_dimensions(target)
            except ValueError as error:
                row["error"] = str(error)
        records.append(row)
    return records


def telemetry_summary(path):
    samples, errors = [], []
    if path.is_file():
        for line in path.read_text(encoding="utf-8").splitlines():
            try:
                samples.append(json.loads(line))
            except ValueError as error:
                errors.append(str(error))
    gpu = [g for s in samples for g in s.get("gpu", [])]
    memory = [s["process_memory"] for s in samples if "process_memory" in s]
    def peak(rows, key):
        values = [r[key] for r in rows if isinstance(r.get(key), (int, float))]
        return max(values) if values else None
    return {
        "samples": len(samples), "parse_errors": errors,
        "gpu_peak_c": peak(gpu, "temperature_c"),
        "gpu_peak_memory_used_mib": peak(gpu, "memory_used_mib"),
        "process_peak_working_set_mib": peak(memory, "peak_working_set_mib"),
        "process_peak_private_commit_mib": peak(memory, "private_commit_mib"),
        "scope": "GPU memory is whole-device nvidia-smi usage, not process-only VRAM; RAM is owned process telemetry.",
    }


def benchmark_summary(saved, failures):
    path, csv_path = saved / "TerrainBenchmark.json", saved / "TerrainBenchmark.csv"
    if not path.is_file() or not csv_path.is_file():
        failures.append("Missing existing runtime benchmark JSON/CSV")
        return None
    report = read_json(path)
    fields = ("frames", "mean_ms", "p95_ms", "p99_ms", "below_30", "below_40", "below_60")
    if any(not isinstance(report.get(key), (int, float)) or not math.isfinite(report[key]) for key in fields):
        failures.append("Incomplete/non-finite runtime benchmark metrics")
        return report
    rows = csv_path.read_text(encoding="utf-8-sig").splitlines()
    if report["frames"] <= 0 or len(rows) - 1 != report["frames"]:
        failures.append("Runtime benchmark frame count differs from raw CSV")
    if report.get("warmup_seconds") != 20 or report.get("sample_seconds") != 60:
        failures.append("Unexpected runtime benchmark observation interval")
    report["mean_fps_from_mean_frame_ms"] = 1000 / report["mean_ms"] if report["mean_ms"] > 0 else None
    report["below_fps_percent"] = {str(fps): 100 * report["below_" + str(fps)] / max(1, report["frames"])
                                   for fps in (30, 40, 60)}
    report["scope"] = "Existing gameplay-thread wall-clock frame intervals; no separate GPU-frame profiler. Uncapped, 1080p. No throughput target automatically accepted."
    return report


def required_shots(mode, prefix):
    if mode == "captures":
        return [prefix + "_" + view + ".png" for view in VIEWS]
    if mode == "input":
        return [prefix + "_" + view + ".png" for view in ("initial", "travel", "frontier", "restored")]
    if mode == "load":
        return ["Campaign_Cold_Load.png"]
    if mode == "performance":
        return ["TerrainBenchmark.png"]
    return ["Vertical_Campaign_Return.png"] + (["Campaign_Stronghold.png"] if mode == "victory" else [])


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=tuple(MARKERS))
    parser.add_argument("--run", required=True)
    parser.add_argument("--project", type=Path, default=ROOT / "Soul.uproject")
    parser.add_argument("--ue-exe", type=Path, default=Path("C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe"))
    parser.add_argument("--focus", default="human_capital", help="Performance: overview or an existing all-world region ID")
    parser.add_argument("--load-from", help="Prior successful input run name in this lane's Local directory")
    parser.add_argument("--diagnostic-rhi", choices=("d3d11", "d3d12"), help="Omit to preserve project default RHI")
    parser.add_argument("--execute", action="store_true", help="Run once; default only prints the reviewable command")
    args = parser.parse_args(argv)
    if not NAME.fullmatch(args.run) or (args.load_from and not NAME.fullmatch(args.load_from)):
        parser.error("Use plain evidence names containing letters, digits, hyphens or underscores")
    if (args.mode == "load") != bool(args.load_from):
        parser.error("Load mode requires --load-from; other modes must omit it")
    project = args.project.resolve().parent
    local = project / EVIDENCE / "Local"
    run_root = local / args.run
    user = run_root / "User"
    profile = read_json(project / PROFILE) if (project / PROFILE).is_file() else {}
    if args.mode == "performance" and profile and args.focus != "overview" and args.focus not in profile.get("regions", {}):
        parser.error("Performance focus must name an existing world profile region")
    battle = args.mode in ("victory", "defeat")
    max_fps = 0 if args.mode == "performance" else 30 if battle else 40
    flags = ["-ForceRes", "-RenderOffscreen", "-unattended", "-nosound",
             "-DisablePlugins=AndroidFileServer,NwiroIntegrationKit", "-DDC=InstalledNoZenLocalFallback",
             "-SoulWorldTerrain", "-UserDir=" + str(user), "-SoulCampaignCapturePrefix=" + args.run]
    flags += {
        "captures": ["-SoulWorldCapture"], "input": ["-SoulCampaignVisualProof"],
        "load": ["-SoulCampaignLoadProof"],
        "victory": ["-SoulCampaignMouseRoundtrip", "-SoulRealtimeVisualUnits", "-SoulRealtimeMagicProof"],
        "defeat": ["-SoulCampaignMouseRoundtrip", "-SoulCampaignDefeatProof", "-SoulRealtimeVisualUnits"],
        "performance": ["-SoulTerrainBenchmark", "-SoulTerrainBenchmarkFocus=" + args.focus],
    }[args.mode]
    command = [sys.executable, str(project / "Tools/qualify_soul_vertical.py"),
               "--ue-exe", str(args.ue_exe), "--project", str(args.project.resolve()),
               "--stage", "G6" if battle else "G0",
               "--map-url", "/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode",
               "--resolution", "1920x1080", "--max-fps", str(max_fps),
               "--expected-active-units", str(27 if args.mode == "defeat" else 30 if battle else 0),
               "--duration", "60", "--startup-timeout", "600",
               "--completion-marker", MARKERS[args.mode], "--completion-timeout", "900" if battle else "600",
               "--output", str(run_root / "runtime")]
    if args.diagnostic_rhi:
        command += ["--diagnostic-rhi", args.diagnostic_rhi]
    command += ["--ue-arg=" + flag for flag in flags]
    plan = {"mode": args.mode, "run": args.run, "command": command,
            "run_root": str(run_root), "max_fps": max_fps, "resolution": [1920, 1080],
            "focus": args.focus if args.mode == "performance" else None,
            "load_from": args.load_from, "execute": args.execute,
            "required_screenshots": required_shots(args.mode, args.run),
            "acceptance_scope": "36 regions/51 routes are world presentation; nine founders remain gameplay. Runtime evidence does not grant art approval."}
    if not args.execute:
        print(json.dumps(plan, indent=2))
        return 0
    if run_root.exists():
        parser.error("Run directory already exists; preserve it and choose a fresh --run")
    if not profile:
        parser.error("World profile is missing; complete the authorized bake/import first")
    inputs = snapshot(project, profile)
    if inputs["missing_required"]:
        parser.error("Missing world runtime payloads: " + json.dumps(inputs["missing_required"]))
    preserved_saves = player_saves(project)
    run_root.mkdir(parents=True)
    write_json(run_root / "plan.json", plan)
    write_json(run_root / "inputs-before.json", inputs)
    write_json(run_root / "player-saves-before.json", preserved_saves)
    seed = seed_load(local, args.load_from, user) if args.load_from else None
    if not user.exists():
        user.mkdir(parents=True)
    if seed:
        write_json(run_root / "load-seed.json", seed)
    # Asset discovery is independent of campaign state. Reuse the editor's
    # validated project registry cache so each isolated save test does not scan
    # 17,000 unchanged licensed packages again. UE still invalidates changed files.
    cache_source=project / "Intermediate/CachedAssetRegistry"
    cache_target=user / "Intermediate/CachedAssetRegistry"
    cache_records=[]
    if cache_source.is_dir():
        cache_target.mkdir(parents=True,exist_ok=True)
        for cached in cache_source.iterdir():
            if cached.is_file() and cached.suffix in (".bin", ".ref"):
                shutil.copy2(cached,cache_target / cached.name)
                cache_records.append(file_record(cached))
    write_json(run_root / "asset-registry-cache-seed.json",cache_records)
    env = os.environ.copy()
    temporary = run_root / "Temp"
    temporary.mkdir()
    env["TEMP"] = env["TMP"] = str(temporary)
    # All Unreal ownership, conflict checks, telemetry and thermal shutdown stay
    # inside the established runner. This wrapper starts no additional UE process.
    with (run_root / "runner-output.log").open("w", encoding="utf-8") as output:
        completed = subprocess.run(command, cwd=project, env=env, stdout=output,
                                   stderr=subprocess.STDOUT, check=False)
    runtime_path = run_root / "runtime/summary.json"
    runtime = read_json(runtime_path) if runtime_path.is_file() else {}
    log_path = run_root / "runtime/unreal.log"
    log = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
    failures = [line[:1500] for line in log.splitlines() if FAILURE.search(line)]
    if completed.returncode != 0 or runtime.get("stop_reason") != "completion_marker_process_exit":
        failures.append("Existing runner did not qualify a clean, observed completion")
    if not runtime.get("clean_shutdown") or not runtime.get("completion_marker_observed"):
        failures.append("Clean shutdown/completion marker missing")
    for marker in (MARKERS[args.mode], "SOUL_TERRAIN_V2_ALIGNMENT_PASS", "SOUL_TERRAIN_COAST_ALIGNMENT_PASS hits=121/121"):
        if marker not in log:
            failures.append("Missing required runtime marker: " + marker)
    if args.mode == "captures" and not re.search(r"SOUL_WORLD_CAPTURE_PASS views=14\b", log):
        failures.append("World capture did not confirm all 14 review views")
    context = re.findall(r"SOUL_WORLD_CONTEXT regions=(\d+) routes=(\d+) settlements=(\d+) trees=(\d+)(?: broadleaf=\d+)? rocks=(\d+) gameplay_regions=(\d+)", log)
    if not context or any((int(row[0]), int(row[1]), int(row[5])) != (36, 51, 9) for row in context):
        failures.append("World context marker did not confirm 36 regions/51 routes and nine gameplay founders")
    if battle:
        expected = "1" if args.mode == "victory" else "0"
        outcomes = re.findall(r"SOUL_CAMPAIGN_ROUNDTRIP_PASS[^\n]*victory=(\d)", log)
        if not outcomes or outcomes[-1] != expected:
            failures.append("Roundtrip outcome does not match requested " + args.mode)
        if "SOUL_CAMPAIGN_STRATEGIC_RETURN_PASS" not in log:
            failures.append("Missing strategic return invariant proof")
    artifacts = archive_outputs(run_root)
    by_name = {Path(row["path"]).name: row for row in artifacts}
    for name in required_shots(args.mode, args.run):
        if by_name.get(name, {}).get("dimensions") != [1920, 1080]:
            failures.append("Missing or incorrectly sized 1080p capture: " + name)
    after = snapshot(project, profile)
    write_json(run_root / "inputs-after.json", after)
    changed = [key for key in inputs["files"].keys() | after["files"].keys()
               if inputs["files"].get(key, {}).get("sha256") != after["files"].get(key, {}).get("sha256")]
    if changed:
        failures.append("Source or presentation payload changed during the run")
    saves_after = player_saves(project)
    write_json(run_root / "player-saves-after.json", saves_after)
    if saves_after != preserved_saves:
        failures.append("Non-isolated player save files changed")
    benchmark = benchmark_summary(user / "Saved", failures) if args.mode == "performance" else None
    receipt = {**plan, "technical_result": "FAIL" if failures else "PASS",
               "visual_approval": "pending human/image review", "failures": failures,
               "runner_exit_code": completed.returncode, "runtime_summary": str(runtime_path),
               "world_context_markers": context, "source_payload_changes": changed,
               "player_saves_unchanged": saves_after == preserved_saves,
               "artifacts": artifacts, "telemetry": telemetry_summary(run_root / "runtime/telemetry.jsonl"),
               "benchmark": benchmark}
    write_json(run_root / "qualification.json", receipt)
    print(json.dumps({"technical_result": receipt["technical_result"], "receipt": str(run_root / "qualification.json"),
                      "failures": failures, "telemetry": receipt["telemetry"], "benchmark": benchmark}, indent=2))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
