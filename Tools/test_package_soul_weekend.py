"""Static packaging regression checks. Never launches UE, UBT, UAT or the package."""
import json
import os
from pathlib import Path
import re
import subprocess
import unittest
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = (ROOT / "Config/DefaultGame.ini").read_text(encoding="utf-8-sig")
COOK_ROOTS = re.findall(r'^\+MapsToCook=\(FilePath="([^"]+)"\)', GAME, re.M)
RULES = (ROOT / "Source/Soul/Soul.Build.cs").read_text(encoding="utf-8-sig")


class WeekendPackagingTests(unittest.TestCase):
    def test_exact_roots_cover_runtime_string_loads_and_scenario_maps(self):
        required = set()
        for directory in ("Source", "Plugins"):
            for source in (ROOT / directory).rglob("*"):
                if source.suffix not in (".cpp", ".h") or "Tests" in source.parts:
                    continue
                text = source.read_text(encoding="utf-8-sig")
                # Also handle C++ adjacent string literals.
                text = re.sub(r'"\s*"', '', text)
                required.update(p.split(".")[0] for p in re.findall(
                    r'"(/(?:Game|Engine|RBWeather)/[^"\s]+)"', text))
        scenario = json.loads((ROOT / "Data/soul_vertical_scenario_20260925.json").read_text())
        required.update((scenario["campaign_map"], scenario["battle_map"]))
        self.assertEqual(required, set(COOK_ROOTS), "Missing runtime load or unneeded explicit cook root")
        self.assertEqual(len(COOK_ROOTS), len(set(COOK_ROOTS)))
        self.assertEqual(len(COOK_ROOTS), 48)

    def test_exact_disk_dependencies_match_actual_readers(self):
        source = (ROOT / "Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp").read_text()
        required = {"Data/" + name for name in re.findall(r'ReadData\(TEXT\("([^"]+)"\)', source)}
        required.add("Plugins/RBFoundation/StackManifest.json")
        required.add("Data/CampaignTerrainV2/presentation.json")
        required.add("Data/CampaignMesa/presentation.json")
        required.add("Data/CampaignEvilCorridor/presentation.json")
        staged = set(re.findall(r'"((?:Data/|Plugins/RBFoundation/)[^"]+\.json)"', RULES))
        self.assertEqual(staged, required)
        self.assertEqual(len(staged), 8)
        self.assertIn('"Data/CampaignMesaLocal/MesaHeight.r16"', RULES)
        self.assertIn('"Data/CampaignTerrainV2/FounderHeight.r16"', RULES)
        self.assertIn('RuntimeDependencies.Add("$(ProjectDir)/" + File, StagedFileType.NonUFS)', RULES)
        for relative in staged:
            json.loads((ROOT / relative).read_text(encoding="utf-8-sig"))

    def test_magic_soft_references_are_bounded_existing_profiles(self):
        expected = {
            "Firebolt": "/Game/MagicSpells/Fire/FX/NS_Fireball",
            "ChainLightning": "/Game/MagicSpells/Electric/FX/NS_ChainLightning",
            "Blizzard": "/Game/MagicSpells/Ice/FX/NS_Ice_Hailstorm",
        }
        for spell, dependency in expected.items():
            profile = ROOT / f"Content/Soul/Magic/Presentation/DA_SoulPresentation_{spell}.uasset"
            references = {s.decode("ascii") for s in re.findall(
                rb'/Game/MagicSpells/[A-Za-z0-9_/]+', profile.read_bytes())}
            self.assertEqual(references, {dependency})
        shipped = {p for p in COOK_ROOTS if p.startswith("/Game/Soul/Magic/")}
        self.assertEqual(shipped, {
            "/Game/Soul/Magic/Spells/DA_Soul_Firebolt",
            "/Game/Soul/Magic/Presentation/DA_SoulPresentation_Firebolt",
        })
        # Inspect both selected serialized assets: neither pulls in unshipped profiles.
        selected_effects = set()
        for package in shipped:
            asset = ROOT / ("Content/" + package.removeprefix("/Game/") + ".uasset")
            data = asset.read_bytes()
            selected_effects.update(s.decode("ascii") for s in re.findall(
                rb'/Game/MagicSpells/[A-Za-z0-9_/]+', data))
            for unshipped in (b"ChainLightning", b"Blizzard", b"SoulProof"):
                self.assertNotIn(unshipped, data)
        self.assertEqual(selected_effects, {expected["Firebolt"]})
        preflight = (ROOT / "Tools/package_soul_weekend.ps1").read_text()
        effects = re.search(r"foreach \(\$fx in @\(([^)]*)\)\)", preflight).group(1)
        checked_effects = {"/Game/MagicSpells/" + p for p in re.findall(r"'([^']+)'", effects)}
        self.assertEqual(checked_effects, selected_effects)
        self.assertIn("Assert-File (Join-Path $projectRoot ('Content/MagicSpells/' + $fx + '.uasset'))", preflight)
        self.assertIn(f"$packages.Count -ne {len(COOK_ROOTS)}", preflight)
        # This inspects serialized names only; it does not prove cooked dependency closure.

    def test_weekend_inputs_retain_firebolt_without_unshipped_loads(self):
        arena = (ROOT / "Source/SoulRealtimeBattle/Private/SoulRealtimeBattleArena.cpp").read_text()
        for key in ("Two", "Three"):
            self.assertNotIn(f"EKeys::{key}", arena)
        for spell in ("ChainLightning", "Blizzard"):
            self.assertNotIn(spell, arena)
        # Both legacy player control and the normal formation loop retain key 1.
        for start, end in (("void ASoulRealtimeArenaGameMode::PlayerTick(",
                            "void ASoulRealtimeArenaGameMode::Tick("),
                           ("void ASoulRealtimeArenaGameMode::Tick(", "if (bMapOnly)")):
            block = arena.split(start, 1)[1].split(end, 1)[0]
            self.assertRegex(block, r"(?s)WasInputKeyJustPressed\(EKeys::One\).*?CastPlayerSpell\(")
            self.assertIn("/Game/Soul/Magic/Spells/DA_Soul_Firebolt.DA_Soul_Firebolt", block)
            self.assertIn("/Game/Soul/Magic/Presentation/DA_SoulPresentation_Firebolt.DA_SoulPresentation_Firebolt", block)
        state = (ROOT / "Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp").read_text()
        self.assertIn('Hero.KnownSpells.Add(TEXT("Magic.Spell.Fire.Firebolt"))', state)

    def test_campaign_startup_and_packaging_breadth(self):
        engine = (ROOT / "Config/DefaultEngine.ini").read_text()
        self.assertRegex(engine, r'(?m)^GameDefaultMap=/Engine/Maps/Entry$')
        self.assertRegex(engine, r'(?m)^GlobalDefaultGameMode=/Script/Soul.SoulFounderPlaytestGameMode$')
        for setting in ("bCookAll=False", "bUseZenStore=False", "BuildConfiguration=PPBC_Development"):
            self.assertIn(setting, GAME)
        self.assertNotRegex(GAME, r'(?m)^\+DirectoriesToAlways(?:Cook|StageAsUFS|StageAsNonUFS)=')
        self.assertNotIn("!IniKeyDenylist", GAME)
        self.assertNotIn("!IniSectionDenylist", GAME)
        self.assertIn("bShareMaterialShaderCode=True", GAME)
        self.assertNotIn("+IniSectionDenylist=/Script/UnrealEd.ProjectPackagingSettings", GAME)
        self.assertIn("+IniSectionDenylist=/Script/AndroidRuntimeSettings.AndroidRuntimeSettings", GAME)
        # The save is created at runtime, never included as input.
        domains = (ROOT / "Plugins/RBSave/Source/RBSave/Private/RBSaveDomains.cpp").read_text()
        self.assertIn('FPaths::ProjectSavedDir(), TEXT("RBSave"), TEXT("Domains")', domains)
        self.assertNotIn("Saved/", RULES)

    def test_uat_temp_fallback_and_restore_without_ue(self):
        script = (ROOT / "Tools/package_soul_weekend.ps1").read_text(encoding="utf-8-sig")
        block = script[script.index("    $oldLogFolder ="):script.index("    if ($exitCode -ne 0)")]
        for missing in (True, False):
            with self.subTest(missing_tmp=missing), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                diagnostics = root / "diagnostics"
                diagnostics.mkdir()
                existing = root / "existing-temp"
                existing.mkdir()
                probe = root / "probe.cmd"
                probe.write_text('@echo off\nif not defined TMP exit /b 22\n'
                                 '> "%TMP%\\soul-test-lock.txt" echo temp-ready\n'
                                 'if errorlevel 1 exit /b 23\nexit /b 0\n')
                env = os.environ.copy()
                if missing:
                    env.pop("TMP", None)
                else:
                    env["TMP"] = str(existing)
                quote = lambda value: "'" + str(value).replace("'", "''") + "'"
                command = ("$ErrorActionPreference='Stop'; $diagnostics=" + quote(diagnostics)
                           + "; $uat=" + quote(probe) + "; $uatArgs=@();\n" + block
                           + "\nif ($exitCode -ne 0) { exit $exitCode }; "
                           + "[Console]::WriteLine('RESTORED_TMP=' + [Environment]::GetEnvironmentVariable('TMP', 'Process'))")
                result = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", command],
                                        env=env, capture_output=True, text=True, timeout=20)
                self.assertEqual(result.returncode, 0, result.stderr)
                expected = diagnostics / "Temp" if missing else existing
                self.assertTrue((expected / "soul-test-lock.txt").is_file())
                self.assertEqual((diagnostics / "exit-code.txt").read_text(encoding="utf-8-sig").strip(), "0")
                restored = next(line for line in result.stdout.splitlines() if line.startswith("RESTORED_TMP="))
                self.assertEqual(restored, "RESTORED_TMP=" + ("" if missing else str(existing)))

    def test_powershell_parser_and_uat_arguments_without_execution(self):
        script = ROOT / "Tools/package_soul_weekend.ps1"
        # Parse AST only. No dot-sourcing or invocation of the packaging script.
        command = r"""
$tokens = $null; $parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile(
    (Join-Path (Get-Location) 'Tools/package_soul_weekend.ps1'), [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { $parseErrors | ForEach-Object { Write-Error $_ }; exit 1 }
$assignments = @($ast.FindAll({ param($node)
    $node -is [System.Management.Automation.Language.AssignmentStatementAst] -and
    $node.Left.Extent.Text -eq '$uatArgs'
}, $true))
if ($assignments.Count -ne 1) { throw 'Expected one UAT argument assignment.' }
$argumentNodes = $assignments[0].Right.FindAll({ param($node)
    $node -is [System.Management.Automation.Language.StringConstantExpressionAst] -or
    $node -is [System.Management.Automation.Language.ExpandableStringExpressionAst]
}, $true)
ConvertTo-Json -InputObject @($argumentNodes | ForEach-Object { $_.Value })
"""
        result = subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", command],
                                cwd=ROOT, capture_output=True, text=True, check=True)
        arguments = json.loads(result.stdout)
        # Exact tokens distinguish editor-only skipping from -skipbuild/-skipbuildclient.
        # Keep a real Soul Win64 Development build and every packaging phase enabled.
        self.assertEqual(arguments, [
            "BuildCookRun", "-nocompileuat", "-noturnkeyvariables", "-nop4", "-unattended", "-utf8output",
            "-project=$projectFile", "-target=Soul", "-platform=Win64", "-clientconfig=Development",
            "-ubtargs=-MaxParallelActions=2 -NoUBA", "-AdditionalCookerOptions=-DDC=InstalledNoZenLocalFallback -DisablePlugins=AndroidFileServer",
            "-build", "-skipbuildeditor", "-cook", "-stage", "-pak", "-iostore", "-package", "-archive",
            "-prereqs", "-nocleanstage", "-stagingdirectory=$stage", "-archivedirectory=$archive",
        ])
        text = script.read_text()
        self.assertIn("& $uat @uatArgs", text)
        self.assertIn("$exitCode = $LASTEXITCODE", text)
        self.assertIn("exit $exitCode", text)


if __name__ == "__main__":
    unittest.main(verbosity=2)
