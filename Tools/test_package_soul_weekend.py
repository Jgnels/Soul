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
                # Comparisons classify existing packages; their literal prefixes are not load requests.
                text = re.sub(r'\.StartsWith\(TEXT\("/(?:Game|Engine|RBWeather)/[^"]+"\)\)', '', text)
                # Also handle C++ adjacent string literals.
                text = re.sub(r'"\s*"', '', text)
                for package in re.findall(r'"(/(?:Game|Engine|RBWeather)/[^"\s]+)"', text):
                    if package.endswith("/"):  # A directory guard is not a loadable package.
                        continue
                    package = package.split(".")[0]
                    if "%s" in package:
                        required.update(package.replace("%s", spell) for spell in
                            ("Firebolt", "ChainLightning", "Blizzard", "TidalWard", "Tailwind"))
                    else:
                        required.add(package)
        for relative in ("Data/soul_vertical_scenario_20260925.json", "Data/SettlementEnvironments/DwarfHoldRuntimeProof.json", "Data/SettlementEnvironments/HumanCapitalRuntimeProof.json"):
            scenario = json.loads((ROOT / relative).read_text())
            required.update((scenario["campaign_map"], scenario["battle_map"]))
        self.assertEqual(required, set(COOK_ROOTS), "Missing runtime load or unneeded explicit cook root")
        self.assertEqual(len(COOK_ROOTS), len(set(COOK_ROOTS)))
        self.assertFalse(any("%" in package for package in COOK_ROOTS))
        for package in COOK_ROOTS:
            if package.startswith("/Game/"):
                asset = ROOT / "Content" / package.removeprefix("/Game/")
                self.assertTrue(asset.with_suffix(".uasset").is_file() or
                                asset.with_suffix(".umap").is_file(), package)

    def test_exact_disk_dependencies_match_actual_readers(self):
        source = (ROOT / "Source/Soul/Private/SoulFounderPlaytestStateSubsystem.cpp").read_text()
        required = {"Data/" + name for name in re.findall(r'ReadData\(TEXT\("([^"]+)"\)', source)}
        required.add("Plugins/RBFoundation/StackManifest.json")
        required.add("Data/CampaignTerrainV2/presentation.json")
        required.add("Data/CampaignMesa/presentation.json")
        required.add("Data/CampaignEvilCorridor/presentation.json")
        staged = set(re.findall(r'"((?:Data/|Plugins/RBFoundation/)[^"]+\.json)"', RULES))
        self.assertEqual(staged, required)
        self.assertEqual(len(staged), 10)
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
        spells = ("Firebolt", "ChainLightning", "Blizzard", "TidalWard", "Tailwind")
        self.assertEqual(shipped, {
            f"/Game/Soul/Magic/{folder}/{prefix}{spell}"
            for spell in spells
            for folder, prefix in (("Spells", "DA_Soul_"),
                                   ("Presentation", "DA_SoulPresentation_"))
        })
        for package in shipped:
            asset = ROOT / ("Content/" + package.removeprefix("/Game/") + ".uasset")
            self.assertTrue(asset.is_file(), package)
            self.assertNotIn(b"SoulProof", asset.read_bytes())
        # Missing optional Niagara effects must have explicit cooked fallback roots.
        fallbacks = {
            "P_Aurora_Melee_SucessfulImpact",
            "P_Aurora_Freeze_Whrilwind",
            "P_Aurora_Freeze_Rooted",
            "P_Aurora_JumpPad_Swirl",
        }
        self.assertTrue(fallbacks.issubset({p.rsplit("/", 1)[-1] for p in COOK_ROOTS}))
        preflight = (ROOT / "Tools/package_soul_weekend.ps1").read_text()
        self.assertIn("Assert-File (Join-Path $projectRoot ('Content/MagicSpells/' + $fx + '.uasset'))", preflight)
        self.assertIn(f"$packages.Count -ne {len(COOK_ROOTS)}", preflight)
        self.assertIn("$dataFiles.Count -ne 10", preflight)

    def test_battle_spell_bar_uses_bounded_cooked_spells(self):
        actions = (ROOT / "Source/SoulRealtimeBattle/Private/SoulBattlePlayerActions.cpp").read_text()
        names = re.search(r"Names\[\]=\{([^}]+)", actions).group(1)
        selected = re.findall(r'TEXT\("([^"]+)"\)', names)
        self.assertEqual(selected, ["Firebolt", "ChainLightning", "Blizzard", "TidalWard", "Tailwind"])
        for spell in selected:
            self.assertIn(f"/Game/Soul/Magic/Spells/DA_Soul_{spell}", COOK_ROOTS)
            self.assertIn(f"/Game/Soul/Magic/Presentation/DA_SoulPresentation_{spell}", COOK_ROOTS)
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
