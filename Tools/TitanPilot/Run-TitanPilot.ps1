param(
    [Parameter(Mandatory=$true)][ValidateSet('BuildEditor','Audit','CreatePilot','Runtime','CookWindows')][string]$Stage
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
    if ($Stage -eq 'BuildEditor') {
        $exe = Join-Path $engine 'Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
        $arguments = @((Join-Path $engine 'Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'),
            'SoulEditor','Win64','Development',("-Project="+$project),'-WaitMutex',
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
            '-NoSourceControl',("-abslog="+$log))
    } else {
        $exe = Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe'
        $arguments = @($project,'/Game/Soul/Maps/Soul_TitanPilot','-game','-windowed','-ResX=1280','-ResY=720',
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
    & $exe @arguments
    $receipt.exit_code = $LASTEXITCODE
    $receipt.status = if ($LASTEXITCODE -eq 0) { 'EXIT_ZERO_REQUIRES_EVIDENCE_REVIEW' } else { 'FAILED' }
    if ($Stage -in @('Runtime','CreatePilot') -and $receipt.exit_code -eq 0) {
        $text = Get-Content -LiteralPath $log -Raw
        $receipt.load_error_count = ([regex]::Matches($text, '(?m)LoadErrors:|LogLinker: Warning:.*(?:Failed|Can.t find)|Fatal error:')).Count
        if ($receipt.load_error_count -gt 0 -or ($Stage -eq 'Runtime' -and $text -notmatch 'SOUL_TITAN_PASS')) {
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
