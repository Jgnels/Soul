param(
    [Parameter(Mandatory=$true)][ValidateSet('BuildEditor','BuildGame','Audit','CreatePilot','Runtime','CookWindows','PackageWindows','PackagedRuntime')][string]$Stage
)
$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$expected = 'D:\RefinedBadger\Worktrees\Soul-titan-pilot-20261005'
if ($workspace -ne $expected) { throw 'This pilot is authorized only in its isolated worktree.' }
$project = Join-Path $workspace 'Soul.uproject'
$evidence = Join-Path $workspace 'Evidence\TitanPilot-20261005'
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$branch = & git -C $workspace branch --show-current
if ($branch -ne 'codex/soul-titan-pilot-20261005') { throw 'Unexpected branch.' }
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
$receipt = [ordered]@{ stage=$Stage; started_utc=[DateTime]::UtcNow.ToString('o'); project=$project; branch=$branch }
$mutex = $null
$acquired = $false
try {
    # Participate in the existing machine lease; do not create a second queue or
    # write Studio Control/Jeff OS operational state from the product worktree.
    $mutex = [Threading.Mutex]::new($false, ('Global\RefinedBadger_Unreal_' + $env:COMPUTERNAME))
    try { $acquired = $mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $acquired = $true }
    if (-not $acquired) { throw 'Existing Studio Unreal lease is occupied.' }
    $competitors = @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
        $_.ProcessName -match '^(UnrealEditor|UnrealEditor-Cmd|ShaderCompileWorker|UnrealBuildTool|AutomationTool|dotnet|Soul|Soul-Win64-Shipping)$'
    } | Select-Object Id,ProcessName,CPU,WorkingSet64)
    $receipt.competitors = $competitors
    if ($competitors.Count -gt 0) { throw 'Pre-existing Unreal workloads occupy the lane; no process was terminated.' }
    $receipt.free_bytes_before = [IO.DriveInfo]::new('D').AvailableFreeSpace
    if ($receipt.free_bytes_before -lt 15GB) { throw 'Less than 15 GiB free; refusing heavy work.' }
    $log = Join-Path $evidence ($Stage + '.log')
    if ($Stage -in @('BuildEditor','BuildGame')) {
        $exe = Join-Path $engine 'Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
        $target = if ($Stage -eq 'BuildEditor') { 'SoulEditor' } else { 'Soul' }
        $arguments = @((Join-Path $engine 'Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'),
            $target,'Win64','Development',("-Project="+$project),'-WaitMutex',
            '-NoHotReloadFromIDE','-NoUBA','-MaxParallelActions=2',("-Log="+$log))
    } elseif ($Stage -eq 'Audit') {
        $exe = Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe'
        $auditOutput = Join-Path $evidence ('asset_registry_closure-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '.json')
        $arguments = @($project,'-run=SoulTitanAudit','-unattended','-nosplash','-nullrhi','-nosound',
            '-NoAssetRegistryCache','-NoAutoSave','-NoSourceControl',
            '-Donor=D:\Unreal Projects\ProjectTitan',
            ("-Output="+$auditOutput),("-abslog="+$log))
    } elseif ($Stage -eq 'CreatePilot') {
        $exe = Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe'
        $arguments = @($project,'-run=SoulTitanPilotBuild','-unattended','-nosplash','-nullrhi','-nosound',
            '-NoSourceControl',("-abslog="+$log))
    } elseif ($Stage -eq 'CookWindows') {
        $exe = Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe'
        $arguments = @($project,'-run=Cook','-TargetPlatform=Windows','-Map=/Game/Soul/Maps/Soul_TitanPilot',
            '-CookMapsOnly','-SkipEditorContent','-unattended','-nosplash','-nullrhi','-nosound',
            '-CustomConfig=TitanPilot',
            '-NoSourceControl',("-abslog="+$log))
    } elseif ($Stage -eq 'PackageWindows') {
        # Reuse the existing Windows build and exact pilot cook. This is a local
        # qualification stage, not the separate weekend release/archive workflow.
        foreach ($requiredStage in @('BuildGame','CookWindows')) {
            $prior = Get-Content -LiteralPath (Join-Path $evidence ($requiredStage + '-attempt.json')) -Raw | ConvertFrom-Json
            if ($prior.status -ne 'EXIT_ZERO_REQUIRES_EVIDENCE_REVIEW' -or $prior.exit_code -ne 0) {
                throw ('Require successful ' + $requiredStage + ' before packaging.')
            }
        }
        $exe = Join-Path $engine 'Build\BatchFiles\RunUAT.bat'
        $stamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')
        $packageDir = Join-Path $workspace ('Saved\StagedBuilds\TitanPilot-' + $stamp)
        if (Test-Path -LiteralPath $packageDir) { throw 'Package output must be new.' }
        $receipt.package_directory = $packageDir
        $env:uebp_LogFolder = Join-Path $evidence ('UAT-' + $stamp)
        $env:uebp_FinalLogFolder = $env:uebp_LogFolder
        if ([string]::IsNullOrWhiteSpace($env:TMP)) {
            $env:TMP = Join-Path $workspace 'Saved\TitanPilotTemp'
            New-Item -ItemType Directory -Force -Path $env:TMP | Out-Null
        }
        $arguments = @('BuildCookRun','-nocompileuat','-noturnkeyvariables','-nop4','-unattended','-utf8output',
            ("-project="+$project),'-target=Soul','-platform=Win64','-clientconfig=Development',
            '-skipbuild','-skipcook','-stage','-pak','-iostore','-nodebuginfo','-nocleanstage',
            '-CustomConfig=TitanPilot',
            ("-stagingdirectory="+$packageDir))
    } else {
        $exe = Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe'
        $prefix = @($project)
        if ($Stage -eq 'PackagedRuntime') {
            $packageReceipt = Get-Content -LiteralPath (Join-Path $evidence 'PackageWindows-attempt.json') -Raw | ConvertFrom-Json
            if ($packageReceipt.exit_code -ne 0) { throw 'Require successful local package stage.' }
            $packageRoot = [IO.Path]::GetFullPath($packageReceipt.package_directory)
            if (-not $packageRoot.StartsWith($workspace + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Package outside isolated worktree.' }
            $exe = Join-Path $packageRoot 'Windows\Soul\Binaries\Win64\Soul.exe'
            if (-not (Test-Path -LiteralPath $exe)) { throw 'Staged Soul executable missing.' }
            $prefix = @('-NotInstalled','-CustomConfig=TitanPilot', ('-UserDir=' + (Join-Path $packageRoot 'PilotUser')))
            $receipt.package_directory = $packageRoot
        }
        $arguments = $prefix + @('/Game/Soul/Maps/Soul_TitanPilot','-game','-windowed','-ResX=1280','-ResY=720',
            '-d3d11','-nosplash','-unattended','-nosound','-SoulTitanPilotProof',
            '-ExecCmds=r.DynamicGlobalIlluminationMethod 0,r.ReflectionMethod 0,r.Shadow.Virtual.Enable 0,sg.ViewDistanceQuality 1,sg.ShadowQuality 1,sg.EffectsQuality 0,sg.PostProcessQuality 0,r.Streaming.PoolSize 2048,t.MaxFPS 60',
            ("-abslog="+$log))
    }
    $receipt.executable = $exe
    $receipt.arguments = $arguments
    $receipt.status = 'RUNNING'
    $receipt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 (Join-Path $evidence ($Stage + '-attempt.json'))
    # Call directly so the exact child exit is observed and there is no detached editor.
    $env:UE_SKIP_UBT_SDK_SETUP = '1'
    if ($Stage -eq 'PackagedRuntime') {
        # The GUI game target must be explicitly waited; PowerShell otherwise
        # reports the launcher return before the game has qualified anything.
        $quoted = ($arguments | ForEach-Object { '"' + $_ + '"' }) -join ' '
        $child = Start-Process -FilePath $exe -ArgumentList $quoted -Wait -PassThru -WindowStyle Hidden
        $receipt.exit_code = $child.ExitCode
    } else {
        & $exe @arguments
        $receipt.exit_code = $LASTEXITCODE
    }
    $receipt.status = if ($receipt.exit_code -eq 0) { 'EXIT_ZERO_REQUIRES_EVIDENCE_REVIEW' } else { 'FAILED' }
    if ($Stage -in @('Runtime','PackagedRuntime','CreatePilot') -and $receipt.exit_code -eq 0) {
        $reviewArgs = @((Join-Path $PSScriptRoot 'review_unreal_log.py'), $log)
        if ($Stage -in @('Runtime','PackagedRuntime')) { $reviewArgs += '--runtime' }
        & python @reviewArgs | Set-Content -Encoding UTF8 (Join-Path $evidence ($Stage + '-log-review.json'))
        if ($LASTEXITCODE -ne 0) {
            $receipt.status = 'FAILED_QUALIFICATION'
            $receipt.reason = 'Exit zero is insufficient: missing runtime pass marker or unresolved package load errors.'
        }
    }
    if ($Stage -eq 'Audit' -and $receipt.exit_code -eq 0) {
        Copy-Item -LiteralPath $auditOutput -Destination (Join-Path $evidence 'asset_registry_closure.json')
    }
} catch {
    $receipt.status = 'BLOCKED'
    $receipt.reason = $_.Exception.Message
    Write-Output $receipt.reason
} finally {
    $receipt.ended_utc = [DateTime]::UtcNow.ToString('o')
    $receipt.free_bytes_after = [IO.DriveInfo]::new('D').AvailableFreeSpace
    $receipt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 (Join-Path $evidence ($Stage + '-attempt.json'))
    if ($mutex) {
        if ($acquired) { $mutex.ReleaseMutex() }
        $mutex.Dispose()
    }
}
if ($receipt.status -ne 'EXIT_ZERO_REQUIRES_EVIDENCE_REVIEW') { exit 1 }
