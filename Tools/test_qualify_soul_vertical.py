"""Safety regression tests for the GPU stage runner. Never launches Unreal."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
SPEC = importlib.util.spec_from_file_location(
    "soul_gpu_runner", Path(__file__).with_name("qualify_soul_vertical.py"))
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


class CompletionSafetyTests(unittest.TestCase):
    def scenario(self, *, ticks=(100, 100, 160, 170, 170, 170), exit_at=170,
                 exit_code=0, marker=True, expect_marker=True, crash=False,
                 hot=False, completion_timeout=300, resolution="1280x720", packaged=False):
        with tempfile.TemporaryDirectory(prefix="soul-runner-mock-") as directory:
            root = Path(directory).resolve()
            self.assertEqual(root.parent, Path(tempfile.gettempdir()).resolve())
            executable = root / "UnrealEditor.exe"
            if packaged:
                executable = root / "Archive/Windows/Soul/Binaries/Win64/Soul.exe"
                executable.parent.mkdir(parents=True)
            executable.touch()
            project = root / "Soul.uproject"
            project.write_text("{}", encoding="utf-8")
            output = root / "run"
            current_time = [100]
            times = iter(ticks)
            proc = types.SimpleNamespace(pid=999999, returncode=None)

            def clock():
                current_time[0] = next(times)
                return current_time[0]

            def poll():
                if current_time[0] >= exit_at:
                    proc.returncode = exit_code
                    return exit_code
                return None

            proc.poll = poll

            def fake_start(command, **unused):
                if packaged:
                    self.assertEqual(unused['cwd'], executable.parent.parent.parent)
                    self.assertNotIn(str(project), command)
                    self.assertNotIn('-game', command)
                log = Path(next(arg.split("=", 1)[1] for arg in command
                                if arg.startswith("-abslog=")))
                text = "LogLoad: Took 1.0 seconds to LoadMap(" + RUNNER.DONOR_MAP + ")\n"
                if marker:
                    text += "SOUL_MOCK_COMPLETED\n"
                if crash:
                    text += "DXGI_ERROR_DEVICE_HUNG\n"
                log.write_text(text, encoding="utf-8")
                return proc

            def close(p):
                if p.returncode is not None:
                    return "already_exited"
                p.returncode = 0
                return "wm_close"

            def gpu(unused):
                return [{"temperature_c": 85 if hot and current_time[0] >= 160 else 50}]

            arguments = ["runner", "--ue-exe", str(executable), "--project", str(project),
                         "--stage", "G0", "--duration", "60", "--output", str(output),
                         "--resolution", resolution]
            if expect_marker:
                arguments += ["--completion-marker", "SOUL_MOCK_COMPLETED",
                              "--completion-timeout", str(completion_timeout)]
            if packaged:
                arguments += ['--ue-arg=-UserDir=' + str(root / 'isolated-user')]
            with patch.object(sys, "argv", arguments), \
                    patch.dict(os.environ, {"COMPUTERNAME": "DESKTOP-Q1S3RPU"}), \
                    patch.object(RUNNER, "conflicting_processes", return_value=[]), \
                    patch.object(RUNNER.shutil, "which", return_value="mock"), \
                    patch.object(RUNNER, "gpu_sample", side_effect=gpu), \
                    patch.object(RUNNER, "process_memory_sample",
                                 return_value={"working_set_mib": 100}), \
                    patch.object(RUNNER.subprocess, "Popen", side_effect=fake_start), \
                    patch.object(RUNNER, "close_owned_process", side_effect=close), \
                    patch.object(RUNNER, "preserve_new_crashes", return_value=[]) as crashes, \
                    patch.object(RUNNER.time, "sleep"), \
                    patch.object(RUNNER.time, "monotonic", side_effect=clock), \
                    contextlib.redirect_stdout(io.StringIO()):
                result = RUNNER.main()
                if packaged:
                    self.assertEqual(crashes.call_args.args[0], root / 'isolated-user')
            return result, json.loads((output / "summary.json").read_text(encoding="utf-8"))

    def test_composition_executable_participates_in_one_process_guard(self):
        tasks = '\n'.join(['"Soul.exe","101"', '"SoulComposition.exe","102"',
                           '"SoulComposition-Win64-Shipping.exe","103"',
                           '"UnrealEditor-Cmd.exe","104"', '"notepad.exe","105"'])
        with patch.object(RUNNER, "hidden", return_value=tasks):
            self.assertEqual([r["pid"] for r in RUNNER.conflicting_processes()], [101, 102, 103, 104])

    def test_explicit_marker_accepts_clean_observed_exit(self):
        result, record = self.scenario()
        self.assertEqual(result, 0)
        self.assertEqual(record["stop_reason"], "completion_marker_process_exit")
        self.assertEqual(record["alive_observed_after_ready_seconds"], 60)
        self.assertFalse(record["automatic_acceptance"])

    def test_packaged_process_uses_archive_and_isolated_crash_directory(self):
        result, record = self.scenario(packaged=True)
        self.assertEqual(result, 0)
        self.assertEqual(record['runtime_kind'], 'packaged')
        self.assertEqual(len(record['executable_sha256']), 64)

    def test_full_hd_uses_bounded_resolution_without_timing_change(self):
        result, record = self.scenario(resolution="1920x1080")
        self.assertEqual(result, 0)
        self.assertIn("-ResX=1920", record["command"])
        self.assertIn("-ResY=1080", record["command"])
        self.assertEqual(record["observation_seconds_after_ready"], 60)

    def test_marker_does_not_accept_early_exit(self):
        result, record = self.scenario(ticks=(100, 100, 155, 155, 155), exit_at=155)
        self.assertEqual(result, 1)
        self.assertEqual(record["stop_reason"], "process_exited")

    def test_dead_polling_interval_cannot_satisfy_minimum(self):
        result, record = self.scenario(ticks=(100, 100, 155, 160, 160, 160), exit_at=160)
        self.assertEqual(result, 1)
        self.assertEqual(record["observed_after_ready_seconds"], 60)
        self.assertEqual(record["alive_observed_after_ready_seconds"], 55)

    def test_missing_marker_rejects_clean_exit(self):
        result, record = self.scenario(marker=False)
        self.assertEqual(result, 1)
        self.assertFalse(record["completion_marker_observed"])

    def test_nonzero_exit_rejected(self):
        self.assertEqual(self.scenario(exit_code=7)[0], 1)

    def test_crash_signature_overrides_marker(self):
        result, record = self.scenario(ticks=(100, 100, 100, 100), crash=True)
        self.assertEqual(result, 1)
        self.assertEqual(record["stop_reason"], "crash_signature")

    def test_thermal_failure_overrides_marker(self):
        result, record = self.scenario(ticks=(100, 100, 160, 160, 160), hot=True)
        self.assertEqual(result, 1)
        self.assertEqual(record["stop_reason"], "thermal_cutoff_85c")

    def test_marker_wait_is_bounded(self):
        result, record = self.scenario(ticks=(100, 100, 160, 220, 220, 220),
                                       exit_at=999, completion_timeout=120)
        self.assertEqual(result, 1)
        self.assertEqual(record["stop_reason"], "completion_timeout")

    def test_original_duration_mode_preserved(self):
        result, record = self.scenario(ticks=(100, 100, 160, 160, 160),
                                       exit_at=999, expect_marker=False)
        self.assertEqual(result, 0)
        self.assertEqual(record["stop_reason"], "observation_duration_reached")


if __name__ == "__main__":
    unittest.main(verbosity=2)
