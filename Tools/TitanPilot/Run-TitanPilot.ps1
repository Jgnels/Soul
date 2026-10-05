param(
    [Parameter(Mandatory=$true)][ValidateSet('BuildEditor','Audit')][string]$Stage
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
    } else {
        $exe = Join-Path $engine 'Binaries\Win64\UnrealEditor-Cmd.exe'
        $arguments = @($project,'-run=SoulTitanAudit','-unattended','-nosplash','-nullrhi','-nosound',
            '-NoAssetRegistryCache','-NoAutoSave','-NoSourceControl',
            '-Donor=D:\Unreal Projects\ProjectTitan',
            ("-Output="+(Join-Path $evidence 'asset_registry_closure.json')),("-abslog="+$log))
    }
    $receipt.executable = $exe
    $receipt.arguments = $arguments
    $receipt.status = 'RUNNING'
    $receipt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 (Join-Path $evidence ($Stage + '-attempt.json'))
    # Call directly so the exact child exit is observed and there is no detached editor.
    & $exe @arguments
    $receipt.exit_code = $LASTEXITCODE
    $receipt.status = if ($LASTEXITCODE -eq 0) { 'EXIT_ZERO_REQUIRES_EVIDENCE_REVIEW' } else { 'FAILED' }
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
