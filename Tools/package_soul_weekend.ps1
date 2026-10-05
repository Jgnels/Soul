# Integration captain only. This script builds/cooks; -ValidateOnly never starts UE/UAT/UBT.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Project,
    [Parameter(Mandatory = $true)][string]$Output,
    [switch]$ValidateOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$engineRoot = 'C:\Program Files\Epic Games\UE_5.8'

function Assert-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Required file missing: $Path" }
}

try {
    $projectFile = (Resolve-Path -LiteralPath $Project).ProviderPath
    if ([IO.Path]::GetFileName($projectFile) -ne 'Soul.uproject') { throw 'Project must be Soul.uproject.' }
    $projectRoot = Split-Path -Parent $projectFile
    $scriptRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).ProviderPath
    if ($projectRoot -ne $scriptRoot) { throw 'Integrate this script into the RC project first; it only builds its own checkout.' }
    $branch = & git -C $projectRoot symbolic-ref --quiet --short HEAD
    if ($LASTEXITCODE -ne 0 -or $branch -in @('main', 'master')) { throw 'Use a named RC/worker branch, never main/master or detached HEAD.' }

    $outputRoot = [IO.Path]::GetFullPath($Output).TrimEnd('\', '/')
    if (-not [IO.Path]::IsPathRooted($Output) -or $outputRoot -match '["%&|<>^!\r\n]') {
        throw 'Output must be an absolute path without shell metacharacters.'
    }
    if ($projectFile -match '["%&|<>^!\r\n]') { throw 'Project path contains unsupported shell metacharacters.' }
    if (Test-Path -LiteralPath $outputRoot) { throw 'Output must be a NEW directory; previous diagnostics/packages are never overwritten.' }
    if ($outputRoot.StartsWith($projectRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
        $projectRoot.StartsWith($outputRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Output must be outside the project tree and must not contain the project.'
    }
    # Output ancestors may not redirect writes into source/donor junctions.
    $ancestor = Split-Path -Parent $outputRoot
    while ($ancestor) {
        if (Test-Path -LiteralPath $ancestor) {
            if ((Get-Item -LiteralPath $ancestor).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Output ancestor is a junction/symlink: $ancestor"
            }
        }
        $ancestor = Split-Path -Parent $ancestor
    }

    . (Join-Path $PSScriptRoot 'Test-SoulEditorFreshness.ps1')
    $editorPreflight = Assert-SoulEditorFreshness -ProjectRoot $projectRoot -EngineRoot $engineRoot

    $uat = Join-Path $engineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
    Assert-File $uat
    $gameConfig = Get-Content -LiteralPath (Join-Path $projectRoot 'Config\DefaultGame.ini') -Raw
    $engineConfig = Get-Content -LiteralPath (Join-Path $projectRoot 'Config\DefaultEngine.ini') -Raw
    if ($engineConfig -notmatch '(?m)^GameDefaultMap=/Engine/Maps/Entry\s*$' -or
        $engineConfig -notmatch '(?m)^GlobalDefaultGameMode=/Script/Soul.SoulFounderPlaytestGameMode\s*$') {
        throw 'Normal campaign startup settings have changed; review before packaging.'
    }
    if ($gameConfig -notmatch '(?m)^bShareMaterialShaderCode=True\s*$' -or
        $gameConfig -match '(?m)^\+IniSectionDenylist=/Script/UnrealEd.ProjectPackagingSettings\s*$') {
        throw 'Runtime shader-library config must be enabled and preserved during staging.'
    }
    $packages = @([regex]::Matches($gameConfig, '(?m)^\+MapsToCook=\(FilePath="([^"]+)"\)') |
        ForEach-Object { $_.Groups[1].Value })
    if ($packages.Count -ne 181) { throw 'Expected the reviewed 181 exact cook roots; review any breadth change.' }
    foreach ($package in $packages) {
        $parts = $package.TrimStart('/').Split('/')
        switch ($parts[0]) {
            'Game' { $contentRoot = Join-Path $projectRoot 'Content' }
            'Engine' { $contentRoot = Join-Path $engineRoot 'Engine\Content' }
            'RBWeather' { $contentRoot = Join-Path $projectRoot 'Plugins\RBWeather\Content' }
            default { throw "Unreviewed content mount: $package" }
        }
        $assetBase = Join-Path $contentRoot ($parts[1..($parts.Length - 1)] -join '\')
        if (-not ((Test-Path -LiteralPath ($assetBase + '.uasset') -PathType Leaf) -or
                  (Test-Path -LiteralPath ($assetBase + '.umap') -PathType Leaf))) {
            throw "Missing cook root: $package. Captain must wire the existing donor junctions in RC."
        }
    }
    # Firebolt requires its Niagara system. The other spells use bounded Paragon
    # fallback effects, explicitly included in the reviewed cook roots.
    foreach ($fx in @('Fire/FX/NS_Fireball')) {
        Assert-File (Join-Path $projectRoot ('Content/MagicSpells/' + $fx + '.uasset'))
    }
    foreach ($icon in @('InfluenceComponentIcon.png', 'InfluenceComponentIcon_64.png')) {
        Assert-File (Join-Path $projectRoot ('Plugins/TCAT/Resources/' + $icon))
    }
    $rules = Get-Content -LiteralPath (Join-Path $projectRoot 'Source\Soul\Soul.Build.cs') -Raw
    $dataFiles = @([regex]::Matches($rules, '"((?:Data/|Plugins/RBFoundation/)[^"]+\.json)"') |
        ForEach-Object { $_.Groups[1].Value })
    if ($dataFiles.Count -ne 8) { throw 'Expected eight exact JSON runtime dependencies.' }
    if (!(Test-Path -LiteralPath (Join-Path $projectRoot 'Data/CampaignTerrainV2/FounderHeight.r16'))) { throw 'Missing V2 heightfield runtime payload.' }
    if (!(Test-Path -LiteralPath (Join-Path $projectRoot 'Data/CampaignMesaLocal/MesaHeight.r16'))) { throw 'Missing external Mesa heightfield; run Tools/Setup_Soul_Mesa.ps1.' }
    foreach ($relative in $dataFiles) {
        $file = Join-Path $projectRoot $relative
        Assert-File $file
        $null = Get-Content -LiteralPath $file -Raw | ConvertFrom-Json
    }

    $stage = Join-Path $outputRoot 'Stage'
    $archive = Join-Path $outputRoot 'Archive'
    $diagnostics = Join-Path $outputRoot 'Diagnostics'
    # Do not add -map, -allmaps, -cookall, -skipcook, -skipbuild, -run or proof flags.
    # UE 5.8 ProjectParams.SkipBuildEditor excludes only the editor build agenda.
    # Cook still needs current SoulEditor binaries; captain prepares those separately.
    $uatArgs = @('BuildCookRun', '-nocompileuat', '-noturnkeyvariables', '-nop4', '-unattended', '-utf8output',
        "-project=$projectFile", '-target=Soul', '-platform=Win64', '-clientconfig=Development',
        '-ubtargs=-MaxParallelActions=2 -NoUBA', '-AdditionalCookerOptions=-DDC=InstalledNoZenLocalFallback -DisablePlugins=AndroidFileServer',
        '-build', '-skipbuildeditor', '-cook', '-stage', '-pak', '-iostore', '-package', '-archive', '-prereqs', '-nocleanstage',
        "-stagingdirectory=$stage", "-archivedirectory=$archive")
    if ($ValidateOnly) {
        Write-Output 'Static preflight passed. No UE/UAT/UBT process started; package/runtime acceptance UNKNOWN.'
        Write-Output ($uatArgs -join "`n")
        exit 0
    }

    $null = New-Item -ItemType Directory -Path $diagnostics
    @{
        project = $projectFile; branch = $branch; engine = $engineRoot
        executable = $uat; arguments = $uatArgs; cook_roots = $packages; runtime_json = $dataFiles
        editor_preflight = $editorPreflight
        runtime_acceptance = 'UNKNOWN'
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $diagnostics 'invocation.json') -Encoding UTF8
    $oldLogFolder = [Environment]::GetEnvironmentVariable('uebp_LogFolder', 'Process')
    $oldFinalLogFolder = [Environment]::GetEnvironmentVariable('uebp_FinalLogFolder', 'Process')
    $oldTmp = [Environment]::GetEnvironmentVariable('TMP', 'Process')
    $exitCode = 1
    try {
        # Build.bat writes its lock below %TMP%. Some launchers only set TEMP;
        # an empty TMP otherwise becomes an unwritable drive-root lock forever.
        if ([string]::IsNullOrWhiteSpace($oldTmp)) {
            $env:TMP = Join-Path $diagnostics 'Temp'
            $null = New-Item -ItemType Directory -Path $env:TMP
        }
        # UAT clears this log directory on entry. It is NEW and confined to this output.
        $env:uebp_LogFolder = Join-Path $diagnostics 'UAT'
        $env:uebp_FinalLogFolder = $env:uebp_LogFolder
        # PS 5.1 treats native stderr as error records; retain it without aborting the pipe.
        $ErrorActionPreference = 'Continue'
        $PSNativeCommandUseErrorActionPreference = $false
        & $uat @uatArgs 2>&1 | Tee-Object -FilePath (Join-Path $diagnostics 'console.log')
        $pipelineSucceeded = $?
        $exitCode = $LASTEXITCODE
        if ($exitCode -eq 0 -and -not $pipelineSucceeded) { $exitCode = 1 }
    }
    finally {
        $ErrorActionPreference = 'Stop'
        [Environment]::SetEnvironmentVariable('TMP', $oldTmp, 'Process')
        [Environment]::SetEnvironmentVariable('uebp_LogFolder', $oldLogFolder, 'Process')
        [Environment]::SetEnvironmentVariable('uebp_FinalLogFolder', $oldFinalLogFolder, 'Process')
        $exitCode | Set-Content -LiteralPath (Join-Path $diagnostics 'exit-code.txt')
    }
    if ($exitCode -ne 0) {
        [Console]::Error.WriteLine("UAT failed (exit $exitCode). Diagnostics retained: $diagnostics")
        exit $exitCode
    }
    & (Join-Path $PSScriptRoot 'Stage-SoulTcatResources.ps1') -ProjectRoot $projectRoot -WindowsPackageRoots @(
        (Join-Path $stage 'Windows'), (Join-Path $archive 'Windows')) |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $diagnostics 'supplemental-runtime-resources.json') -Encoding UTF8
    Write-Output "UAT completed. Archive: $archive. Runtime/package acceptance remains UNKNOWN until captain verification."
    exit 0
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
