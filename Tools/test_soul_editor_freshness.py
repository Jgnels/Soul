"""Execute read-only PowerShell preflight against synthetic files; never starts UE."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / 'Tools/Test-SoulEditorFreshness.ps1'


def quote(value):
    return "'" + str(value).replace("'", "''") + "'"


class EditorFreshnessTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='soul-editor-preflight-')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'project'
        self.engine = Path(self.temp.name) / 'engine'
        self.old = time.time() - 10000
        self.version = dict(MajorVersion=5, MinorVersion=8, PatchVersion=2,
                            Changelist=56702186, CompatibleChangelist=55116800, BuildId='fixture')
        self.write(self.engine / 'Engine/Build/Build.version', self.version)
        self.write(self.engine / 'Engine/Binaries/Win64/UnrealEditor.modules',
                   dict(BuildId='fixture', Modules={}))
        self.write(self.root / 'Soul.uproject', dict(
            Modules=[dict(Name='Soul', Type='Runtime')],
            Plugins=[dict(Name='RBTest', Enabled=True)]))
        self.write(self.root / 'Source/SoulEditor.Target.cs', 'target rules')
        self.write(self.root / 'Plugins/RBTest/RBTest.uplugin',
                   dict(Modules=[dict(Name='RBTest', Type='Runtime')]))
        self.products = []
        self.add_module(self.root, 'Soul', '"RBTest"')
        self.add_module(self.root / 'Plugins/RBTest', 'RBTest', '')
        self.receipt = dict(TargetName='SoulEditor', Platform='Win64',
                            Configuration='Development', TargetType='Editor',
                            Version=self.version, BuildProducts=self.products)
        self.receipt_path = self.root / 'Binaries/Win64/SoulEditor.target'
        self.write(self.receipt_path, self.receipt, 200)

    def write(self, path, value, offset=0):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value) if isinstance(value, dict) else value, encoding='utf-8')
        os.utime(path, (self.old + offset, self.old + offset))

    def add_module(self, owner, name, rules):
        self.write(owner / f'Source/{name}/{name}.Build.cs', rules)
        self.write(owner / f'Source/{name}/Private/{name}.cpp', 'native input')
        self.write(owner / f'Source/{name}/Public/{name}.h', 'header')
        manifest = owner / 'Binaries/Win64/UnrealEditor.modules'
        dll = owner / f'Binaries/Win64/UnrealEditor-{name}.dll'
        self.write(manifest, dict(BuildId='fixture', Modules={name: dll.name}), 100)
        self.write(dll, 'synthetic DLL, never loaded', 100)
        for path, kind in ((manifest, 'RequiredResource'), (dll, 'DynamicLibrary')):
            self.products.append(dict(Path='$(ProjectDir)/' + path.relative_to(self.root).as_posix(), Type=kind))

    def run_gate(self, error=None):
        command = ("Set-StrictMode -Version Latest; $ErrorActionPreference='Stop'; "
                   f". {quote(HELPER)}; try {{ Assert-SoulEditorFreshness "
                   f"-ProjectRoot {quote(self.root)} -EngineRoot {quote(self.engine)} | ConvertTo-Json; "
                   "exit 0 } catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }")
        result = subprocess.run(['powershell', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-Command', command],
                                capture_output=True, text=True)
        if error:
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn(error, result.stderr)
        else:
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['status'], 'NO_PROVABLE_STALENESS')
            self.assertEqual(report['modules_checked'], 2)
            self.assertEqual(len(report['receipt_sha256']), 64)
        return result

    def test_current_project_and_plugin_pass(self):
        self.run_gate()

    def test_missing_receipt_fails(self):
        self.receipt_path.unlink()
        self.run_gate('missing')

    def test_source_newer_than_receipt_fails(self):
        self.write(self.root / 'Source/Soul/Private/Soul.cpp', 'changed', 250)
        self.run_gate('source newer than SoulEditor receipt')

    def test_fresh_receipt_cannot_mask_stale_project_dll(self):
        self.write(self.root / 'Source/Soul/Private/Soul.cpp', 'changed', 150)
        self.run_gate('stale Soul DLL')

    def test_fresh_receipt_cannot_mask_stale_plugin_dll(self):
        self.write(self.root / 'Plugins/RBTest/Source/RBTest/Private/RBTest.cpp', 'changed', 150)
        self.run_gate('stale RBTest DLL')

    def test_dependency_header_invalidates_consumer(self):
        self.write(self.root / 'Plugins/RBTest/Source/RBTest/Public/RBTest.h', 'changed', 150)
        self.write(self.root / 'Plugins/RBTest/Binaries/Win64/UnrealEditor-RBTest.dll', 'rebuilt', 170)
        self.run_gate('stale Soul DLL')

    def test_dependency_cpp_does_not_require_consumer_rebuild(self):
        self.write(self.root / 'Plugins/RBTest/Source/RBTest/Private/RBTest.cpp', 'changed', 150)
        self.write(self.root / 'Plugins/RBTest/Binaries/Win64/UnrealEditor-RBTest.dll', 'rebuilt', 170)
        self.run_gate()

    def test_descriptor_newer_than_receipt_fails(self):
        path = self.root / 'Soul.uproject'
        os.utime(path, (self.old + 250, self.old + 250))
        self.run_gate('source newer than SoulEditor receipt')

    def test_engine_version_mismatch_fails(self):
        self.write(self.engine / 'Engine/Build/Build.version', dict(self.version, Changelist=1))
        self.run_gate('engine version mismatch')

    def test_plugin_build_id_mismatch_fails(self):
        self.write(self.root / 'Plugins/RBTest/Binaries/Win64/UnrealEditor.modules',
                   dict(BuildId='other', Modules={'RBTest': 'UnrealEditor-RBTest.dll'}), 100)
        self.run_gate('module BuildId mismatch')

    def test_missing_plugin_dll_fails(self):
        (self.root / 'Plugins/RBTest/Binaries/Win64/UnrealEditor-RBTest.dll').unlink()
        self.run_gate('DLL missing from disk/receipt')

    def test_new_module_missing_from_manifest_fails(self):
        self.write(self.root / 'Plugins/RBTest/RBTest.uplugin', dict(Modules=[
            dict(Name='RBTest', Type='Runtime'), dict(Name='NewModule', Type='Editor')]))
        self.run_gate('declared module missing from manifest')

    def test_manifest_omitted_from_receipt_fails(self):
        self.receipt['BuildProducts'] = [p for p in self.products if 'Plugins/RBTest/' not in p['Path']]
        self.write(self.receipt_path, self.receipt, 200)
        self.run_gate('manifest absent from receipt')

    def test_dll_newer_than_receipt_fails(self):
        self.write(self.root / 'Binaries/Win64/UnrealEditor-Soul.dll', 'changed', 250)
        self.run_gate('DLL newer than receipt')

    def test_package_gate_precedes_validate_only_and_uat(self):
        wrapper = (ROOT / 'Tools/package_soul_weekend.ps1').read_text(encoding='utf-8-sig')
        gate = wrapper.index('$editorPreflight = Assert-SoulEditorFreshness')
        self.assertLess(gate, wrapper.index('if ($ValidateOnly)'))
        self.assertLess(gate, wrapper.index('& $uat @uatArgs'))
        self.assertIn('editor_preflight = $editorPreflight', wrapper)
        self.assertNotIn('SkipFreshness', wrapper)


if __name__ == '__main__':
    unittest.main(verbosity=2)
