#!/usr/bin/env python3
"""One-process Soul graphical qualification. No retries and no product config writes.

The JSON record is observation, not a graphical PASS. Inspect the world and
gameplay before updating Evidence/SOUL_GPU_STABILITY_20260925.md.
Use --ue-arg=-Flag for arguments beginning with '-'. This runner defaults to the
project D3D12 RHI; a diagnostic RHI requires --diagnostic-rhi and is labeled.
"""
import argparse
import csv
import hashlib
import ctypes
from ctypes import wintypes
import datetime as dt
import io
import json
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import time

DONOR_MAP = "/Game/Dragon_graveyard/Level/L_showcase_level"
CRASH = re.compile(
    r"GPU crash detected|GPU Crashed or D3D Device Removed|"
    r"DXGI_ERROR_DEVICE_(?:HUNG|REMOVED|RESET)|Fatal error[:!]|"
    r"Assertion failed:|Unhandled Exception:", re.I)
CREATE_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)


def utc():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def hidden(command):
    return subprocess.run(command, capture_output=True, text=True, timeout=10,
                          creationflags=CREATE_NO_WINDOW, check=True).stdout


def gpu_sample(executable):
    raw = hidden([executable, "--query-gpu=name,memory.total,memory.used,"
                  "temperature.gpu,utilization.gpu", "--format=csv,noheader,nounits"])
    rows = []
    for row in csv.reader(io.StringIO(raw)):
        if len(row) < 5:
            raise ValueError("Incomplete nvidia-smi telemetry")
        rows.append(dict(name=row[0].strip(), memory_total_mib=float(row[1]),
                         memory_used_mib=float(row[2]), temperature_c=float(row[3]),
                         utilization_percent=float(row[4])))
    if not rows:
        raise ValueError("No GPUs reported")
    return rows



class ProcessMemoryCounters(ctypes.Structure):
    _fields_ = [("cb", wintypes.DWORD), ("page_fault_count", wintypes.DWORD)] + [
        (name, ctypes.c_size_t) for name in (
            "peak_working_set", "working_set", "quota_peak_paged_pool",
            "quota_paged_pool", "quota_peak_nonpaged_pool", "quota_nonpaged_pool",
            "pagefile_usage", "peak_pagefile_usage", "private_usage")]


def process_memory_sample(pid):
    """Read this owned process's working set/private commit; no UI or mutation."""
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    psapi.GetProcessMemoryInfo.argtypes = [
        wintypes.HANDLE, ctypes.POINTER(ProcessMemoryCounters), wintypes.DWORD]
    psapi.GetProcessMemoryInfo.restype = wintypes.BOOL
    handle = kernel.OpenProcess(0x1000 | 0x0010, False, pid)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        counters = ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        if not psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
            raise ctypes.WinError(ctypes.get_last_error())
        return {key + "_mib": round(getattr(counters, field) / 1048576, 2)
                for key, field in (
                    ("working_set", "working_set"), ("peak_working_set", "peak_working_set"),
                    ("private_commit", "private_usage"), ("pagefile", "pagefile_usage"))}
    finally:
        kernel.CloseHandle(handle)


def conflicting_processes():
    rows = csv.reader(io.StringIO(hidden(["tasklist", "/FO", "CSV", "/NH"])))
    return [{"name": r[0], "pid": int(r[1])} for r in rows if len(r) > 1 and
            re.match(r"^(UnrealEditor|Soul(?:Composition)?)(?:-|\.|$)", r[0], re.I)]


def close_owned_process(process):
    """WM_CLOSE only windows owned by this exact child PID, then bounded stop."""
    action = "already_exited"
    if process.poll() is None:
        action = "wm_close"
        if os.name == "nt":
            user32 = ctypes.windll.user32
            user32.GetWindowThreadProcessId.argtypes = [
                wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
            user32.PostMessageW.argtypes = [
                wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
            callback_type = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p,
                                              ctypes.c_void_p)

            @callback_type
            def close_window(hwnd, unused):
                owner = ctypes.c_ulong()
                user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
                if owner.value == process.pid:
                    user32.PostMessageW(hwnd, 0x0010, 0, 0)
                return True
            user32.EnumWindows(close_window, 0)
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            action = "terminate_owned_child"
            process.terminate()
            process.wait(timeout=10)
    return action


def preserve_new_crashes(project_dir, output, started):
    copied = []
    crash_root = project_dir / "Saved" / "Crashes"
    if crash_root.exists():
        for source in crash_root.iterdir():
            if source.is_dir() and source.stat().st_mtime >= started - 2:
                target = output / "crashes" / source.name
                shutil.copytree(source, target, dirs_exist_ok=True)
                copied.append(str(target))
    log_root = project_dir / "Saved" / "Logs"
    if log_root.exists():
        for source in log_root.glob("*.nv-gpudmp"):
            if source.stat().st_mtime >= started - 2:
                target = output / source.name
                shutil.copy2(source, target)
                copied.append(str(target))
    return copied


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--ue-exe", type=Path, required=True)
    p.add_argument("--project", type=Path, required=True)
    p.add_argument("--stage", choices=[f"G{i}" for i in range(7)], required=True)
    p.add_argument("--map-url", default=None)
    p.add_argument("--resolution", choices=["1280x720", "1920x1080"], default="1280x720")
    p.add_argument("--max-fps", type=int, default=30, help="0 for uncapped campaign qualification")
    p.add_argument("--ue-arg", action="append", default=[])
    p.add_argument("--expected-active-units", type=int, default=0)
    p.add_argument("--duration", type=float, default=90,
                   help="Seconds after map-ready marker; minimum 60")
    p.add_argument("--startup-timeout", type=float, default=300)
    p.add_argument("--ready-marker", default=None,
                   help="Optional exact ready log marker; otherwise LoadMap completion")
    p.add_argument("--completion-marker", default=None,
                   help="Exact log marker required for an intentional exit after observation")
    p.add_argument("--completion-timeout", type=float, default=300,
                   help="Maximum seconds after readiness in completion-marker mode")
    p.add_argument("--diagnostic-rhi", choices=["d3d11", "d3d12"])
    p.add_argument("--output", type=Path)
    p.add_argument("--dry-run", action="store_true")
    return p


def main():
    p = parser()
    args = p.parse_args()
    if os.name != "nt":
        p.error("This qualification runner requires the authorized Windows machine")
    if os.environ.get("COMPUTERNAME", socket.gethostname()).upper() != "DESKTOP-Q1S3RPU":
        p.error("Authorized machine is DESKTOP-Q1S3RPU")
    if args.duration < 60 or not 1 <= args.startup_timeout <= 600:
        p.error("Require duration >=60 seconds and startup-timeout in [1,600]")
    # The intact Human city crosses several real asynchronous map loads in
    # the existing construction/visit/save/battle proof. Keep ordinary runs
    # at fifteen minutes; only that explicit observer gets a two-hour bound.
    authored_human = ("-SoulHumanSettlementProof" in args.ue_arg
        and "-SoulAuthoredSettlementQualification" in args.ue_arg
        and args.completion_marker in ("SOUL_AUTHORED_SETTLEMENT_PASS", "SOUL_AUTHORED_FRESH_LOAD_PASS"))
    completion_limit = 7200 if authored_human else 900
    if args.completion_marker and not args.duration <= args.completion_timeout <= completion_limit:
        p.error(f"completion-timeout must be >=duration and <={completion_limit} seconds")
    if args.stage != "G0" and (not args.map_url or args.expected_active_units < 2):
        p.error("Combat stages need an explicit map URL and expected-active-units >=2")
    if args.stage == "G0" and args.expected_active_units != 0:
        p.error("G0 requires zero combatants")
    # Resolution, RHI, and bounded timing must not be overridden accidentally.
    for arg in args.ue_arg:
        if re.match(r"^-?(?:execCmds|res[xy]|fullscreen|windowed|d3d|dx1|vulkan|"
                    r"nullrhi|benchmark|seconds|abslog)", arg, re.I):
            p.error("Use runner settings, not conflicting rendering/timing arguments")
    project = args.project.resolve()
    executable = args.ue_exe.resolve()
    if not project.is_file() or not executable.is_file():
        p.error("UE executable and project must exist")
    if executable.name.lower() != "unrealeditor.exe" and not (
            executable.name.lower().startswith("soul") and
            executable.parent.name.lower() == "win64" and
            executable.parent.parent.name.lower() == "binaries"):
        p.error("Use UnrealEditor.exe or the real Binaries/Win64/Soul executable; "
                "a packaged bootstrap could leave an unowned child running")
    map_url = args.map_url or DONOR_MAP + "?game=/Script/Engine.GameModeBase"
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    output = (args.output or project.parent / "Evidence" / "VerticalRuns" /
              (stamp + "_" + args.stage)).resolve()
    if output.exists():
        p.error("Output directory already exists; never overwrite an earlier run")
    log = output / "unreal.log"
    command = [str(executable)]
    is_editor = executable.name.lower().startswith("unrealeditor")
    runtime_root = project.parent if is_editor else executable.parent.parent.parent
    user_dir = next((Path(a.split("=", 1)[1]).resolve() for a in args.ue_arg
                     if a.lower().startswith("-userdir=")), runtime_root)
    if is_editor:
        command += [str(project), map_url, "-game"]
    else:
        command += [map_url]
    width, height = args.resolution.split("x")
    command += ["-windowed", f"-ResX={width}", f"-ResY={height}", "-nosplash",
                f"-ExecCmds=t.MaxFPS {args.max_fps},r.VSync 0", f"-abslog={log}"]
    if args.diagnostic_rhi:
        command.append("-" + args.diagnostic_rhi)
    command += args.ue_arg
    preview = dict(stage=args.stage, command=command, output=str(output),
                   runtime_kind="editor_game" if is_editor else "packaged",
                   working_directory=str(runtime_root),
                   executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                   expected_active_units=args.expected_active_units,
                   diagnostic_rhi=args.diagnostic_rhi,
                   observation_seconds_after_ready=args.duration,
                   completion_marker=args.completion_marker,
                   completion_timeout=args.completion_timeout if args.completion_marker else None)
    if "-SoulTerrainV2" in args.ue_arg:
        height_file=project.parent / "Data/CampaignTerrainV2/FounderHeight.r16"
        preview["terrain_height_sha256"]=hashlib.sha256(height_file.read_bytes()).hexdigest()
        payloads=[project.parent/"Data/CampaignTerrainV2/presentation.json",
                  project.parent/"Binaries/Win64/UnrealEditor-Soul.dll"]
        payloads+=sorted((project.parent/"Content/Soul/Campaign/TerrainV2").glob("*"))
        preview["terrain_payload_sha256"]={str(f.relative_to(project.parent)).replace("\\","/"):
            hashlib.sha256(f.read_bytes()).hexdigest() for f in payloads if f.is_file()}
    if "-SoulMesaTerrain" in args.ue_arg:
        payloads = [project.parent / p for p in (
            'Data/CampaignMesa/presentation.json', 'Data/CampaignMesaLocal/MesaHeight.r16',
            'Content/SoulCampaignMountain/L_evil_waterfront.umap',
            'Binaries/Win64/UnrealEditor-Soul.dll')]
        preview['mesa_payload_sha256'] = {str(f.relative_to(project.parent)).replace('\\','/'):
            hashlib.sha256(f.read_bytes()).hexdigest() for f in payloads}
    if "-SoulEvilCorridor" in args.ue_arg:
        payload = project.parent / 'Data/CampaignEvilCorridor/presentation.json'
        preview['evil_corridor_profile_sha256'] = hashlib.sha256(payload.read_bytes()).hexdigest()
    if args.dry_run:
        print(json.dumps(preview, indent=2))
        return 0
    if conflicts := conflicting_processes():
        p.error("Existing Unreal/Soul process; no launch: " + json.dumps(conflicts))
    nvidia = shutil.which("nvidia-smi")
    if not nvidia:
        p.error("nvidia-smi unavailable; require temperature telemetry")
    before = gpu_sample(nvidia)
    if max(g["temperature_c"] for g in before) >= 85:
        p.error("GPU already at thermal stop threshold 85 C")
    output.mkdir(parents=True)
    record = dict(preview, started_utc=utc(), initial_gpu=before,
                  automatic_acceptance=False)
    (output / "launch.json").write_text(json.dumps(record, indent=2), encoding="utf-8")
    process = None
    started = time.time()
    monotonic_started = time.monotonic()
    ready_at = None
    last_observed_alive_at = None
    stop_reason = "unknown"
    cleanup = "not_started"
    try:
        with (output / "console.log").open("w", encoding="utf-8") as console, \
                (output / "telemetry.jsonl").open("w", encoding="utf-8") as telemetry:
            process = subprocess.Popen(command, cwd=runtime_root, stdout=console,
                                       stderr=subprocess.STDOUT,
                                       creationflags=CREATE_NO_WINDOW)
            record["pid"] = process.pid
            print(f"{args.stage} launched owned PID {process.pid}; {output}", flush=True)
            while True:
                now = time.monotonic()
                text = log.read_text(encoding="utf-8", errors="replace") if log.exists() else ""
                crash_lines = [line[:1000] for line in text.splitlines() if CRASH.search(line)]
                if crash_lines:
                    record["crash_signatures"] = crash_lines
                    stop_reason = "crash_signature"
                    break
                ready = (args.ready_marker in text if args.ready_marker else
                         bool(re.search(r"LogLoad: Took .+ seconds to LoadMap\(" +
                                        re.escape(map_url.split("?")[0]) + r"\)", text)))
                if ready_at is None and ready:
                    ready_at = now
                    record["ready_utc"] = utc()
                    print(f"{args.stage} map ready; starting bounded observation", flush=True)
                if process.poll() is not None:
                    stop_reason = "process_exited"
                    break
                sample = dict(utc=utc(), elapsed_seconds=round(now-monotonic_started, 2))
                try:
                    sample["gpu"] = gpu_sample(nvidia)
                    sample["process_memory"] = process_memory_sample(process.pid)
                    if authored_human:
                        from WorldTerrain.host_commit import system_commit_sample
                        sample["system_memory"] = system_commit_sample()
                except Exception as exc:
                    sample["telemetry_error"] = str(exc)
                telemetry.write(json.dumps(sample) + "\n")
                telemetry.flush()
                if "telemetry_error" in sample:
                    stop_reason = "telemetry_unavailable"
                    break
                if max(g["temperature_c"] for g in sample["gpu"]) >= 85:
                    stop_reason = "thermal_cutoff_85c"
                    break
                if authored_human and (sample["process_memory"].get("private_commit_mib", 0) >= 20000
                        or sample["system_memory"]["available_commit_mib"] < 4096):
                    stop_reason = "authored_environment_memory_guard"
                    break
                if process.poll() is not None:
                    stop_reason = "process_exited"
                    break
                last_observed_alive_at = now
                if ready_at is not None:
                    if args.completion_marker:
                        if now-ready_at >= args.completion_timeout:
                            stop_reason = "completion_timeout"
                            break
                    elif now-ready_at >= args.duration:
                        stop_reason = "observation_duration_reached"
                        break
                if ready_at is None and now-monotonic_started >= args.startup_timeout:
                    stop_reason = "startup_timeout"
                    break
                time.sleep(5)
    except KeyboardInterrupt:
        stop_reason = "operator_interrupt"
    except Exception as exc:
        stop_reason = "runner_error"
        record["runner_error"] = repr(exc)
    finally:
        observation_stopped = time.monotonic()
        if process is not None:
            if process.poll() is not None and stop_reason == "observation_duration_reached":
                stop_reason = "process_exited_before_cleanup"
            try:
                cleanup = close_owned_process(process)
            except Exception as exc:
                cleanup = "cleanup_error"
                record["cleanup_error"] = repr(exc)
            record["exit_code"] = process.returncode
            record["clean_shutdown"] = cleanup == "wm_close" and process.returncode == 0
        record.update(stopped_utc=utc(), stop_reason=stop_reason, cleanup=cleanup,
                      elapsed_seconds=round(time.monotonic()-monotonic_started, 2),
                      observed_after_ready_seconds=(round(observation_stopped-ready_at, 2)
                                                    if ready_at else 0))
        final_log = ""
        if log.exists():
            final_log = log.read_text(encoding="utf-8", errors="replace")
            record["crash_signatures"] = [line[:1000] for line in final_log.splitlines()
                                         if CRASH.search(line)]
            record["rhi_log_lines"] = [line for line in final_log.splitlines()
                                      if "Using Default RHI:" in line or
                                      "Creating D3D" in line]
        record["completion_marker_observed"] = bool(
            args.completion_marker and args.completion_marker in final_log)
        alive_duration = (max(0, last_observed_alive_at-ready_at)
                          if ready_at is not None and last_observed_alive_at is not None else 0)
        record["alive_observed_after_ready_seconds"] = round(alive_duration, 2)
        # Never count the final polling interval after an already-dead process
        # toward the observation minimum. A marker alone cannot grant success.
        if (args.completion_marker and stop_reason == "process_exited" and
                record.get("exit_code") == 0 and record["completion_marker_observed"] and
                alive_duration >= args.duration and not record.get("crash_signatures")):
            record["stop_reason"] = "completion_marker_process_exit"
            record["clean_shutdown"] = True
        try:
            record["preserved_crashes"] = preserve_new_crashes(user_dir, output, started)
        except Exception as exc:
            record["crash_copy_error"] = str(exc)
        (output / "summary.json").write_text(json.dumps(record, indent=2), encoding="utf-8")
    print(json.dumps(record, indent=2))
    return 0 if (record["stop_reason"] in (
                    "observation_duration_reached", "completion_marker_process_exit") and
                 record.get("clean_shutdown") and not record.get("crash_signatures")) else 1


if __name__ == "__main__":
    raise SystemExit(main())
