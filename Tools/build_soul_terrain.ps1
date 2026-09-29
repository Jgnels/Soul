param([ValidateSet('SoulEditor','Soul')][string]$Target='SoulEditor', [string]$Log)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if($Log -and (Test-Path -LiteralPath $Log)) { throw 'Build log already exists; use a fresh run path.' }
$started=[DateTime]::UtcNow.ToString('o')
$previousCompat=$env:SOUL_NO_PCH_COMPAT
try {
    $env:SOUL_NO_PCH_COMPAT=if($Target -eq 'SoulEditor'){'1'}else{$null}
    $arguments=@($Target,'Win64','Development',('-Project='+(Join-Path $root 'Soul.uproject')),'-WaitMutex','-NoPCH','-NoUBA','-MaxParallelActions=2','-ForceRulesCompile')
    if($Log) {
        & 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' @arguments > $Log 2>&1
    } else {
        & 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' @arguments
    }
    $buildExit=$LASTEXITCODE
    if($Log) {
        [ordered]@{target=$Target;configuration='Win64 Development';started_utc=$started;finished_utc=[DateTime]::UtcNow.ToString('o');exit_code=$buildExit;arguments=$arguments;compatibility_header_sha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'SoulNoPchCompatibility.h')).Hash} |
            ConvertTo-Json -Depth 4 | Set-Content -LiteralPath ($Log+'.result.json')
    }
} finally { $env:SOUL_NO_PCH_COMPAT=$previousCompat }
exit $buildExit
