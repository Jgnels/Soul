param([string]$VCVars = 'D:\DevTools\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$adapter = Join-Path $PSScriptRoot 'NativeReinforcement'
$productionHeader = Join-Path $projectRoot 'Plugins/RefinedBadgerCombat/Source/RefinedBadgerCombatCore/Public/RBCombatCommands.h'
$actual = [regex]::Match((Get-Content $productionHeader -Raw), 'MaximumMembers\s*=\s*(\d+)').Groups[1].Value
$adapted = [regex]::Match((Get-Content (Join-Path $adapter 'RBCombatCommands.h') -Raw), 'MaximumMembers\s*=\s*(\d+)').Groups[1].Value
if (!$actual -or $actual -ne $adapted) { throw 'Native adapter no longer matches installed RB Combat group limit.' }
if (!(Test-Path -LiteralPath $VCVars)) { throw 'Pass -VCVars with the installed MSVC vcvars64.bat path.' }
$vcRoot = Split-Path (Split-Path (Split-Path $VCVars -Parent) -Parent) -Parent
$toolVersion = (Get-Content (Join-Path $vcRoot 'Auxiliary/Build/Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
foreach ($header in @('algorithm', 'vector')) {
    $headerPath = Join-Path $vcRoot "Tools/MSVC/$toolVersion/include/$header"
    $bytes = [System.IO.File]::ReadAllBytes($headerPath)
    if ($bytes.Length -eq 0 -or $bytes[0] -eq 0) { throw "NATIVE_BLOCKED: installed MSVC header is empty/zero-filled: $headerPath" }
}
$outputDir = Join-Path $projectRoot 'Saved/NativeReinforcement'
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
$publicDir = Join-Path $projectRoot 'Source/SoulRealtimeBattle/Public'
$rules = Join-Path $projectRoot 'Source/SoulRealtimeBattle/Private/SoulRealtimeBattleRules.cpp'
$test = Join-Path $adapter 'main.cpp'
$commandShell = if ($env:ComSpec) { $env:ComSpec } else { 'C:\Windows\System32\cmd.exe' }
# Compile the production translation unit against test-only containers; never UBT/UE.
Push-Location $outputDir
try {
    & $commandShell /d /s /c "`"call `"$VCVars`" >nul && cl /nologo /EHsc /std:c++17 /W4 /WX /I`"$adapter`" /I`"$publicDir`" `"$rules`" `"$test`" /Fe:reinforcement_tests.exe && reinforcement_tests.exe`""
    if ($LASTEXITCODE -ne 0) { throw "Native reinforcement tests failed ($LASTEXITCODE)." }
} finally { Pop-Location }
