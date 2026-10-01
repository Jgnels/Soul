$ErrorActionPreference='Stop'
$w='D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929'
$out=Join-Path $w 'Evidence/SettlementBattleVariety-20261001/AutomationFinal4'
if(Test-Path -LiteralPath $out){throw 'Use a fresh automation output'}
$lease=New-Object System.Threading.Mutex($false,('Global\RefinedBadger_Unreal_'+$env:COMPUTERNAME));$held=$false
try {
 try {$held=$lease.WaitOne(0)}catch [System.Threading.AbandonedMutexException]{$held=$true}
 if(!$held){throw 'Unreal lease occupied'}
 if(Get-Process UnrealEditor* -ErrorAction SilentlyContinue){throw 'Another Unreal process is running'}
 New-Item -ItemType Directory -Path $out | Out-Null
 & 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$w/Soul.uproject" -unattended -nop4 -nosplash -nosound -nullrhi '-DisablePlugins=AndroidFileServer,NwiroIntegrationKit' -DDC=InstalledNoZenLocalFallback '-ExecCmds=Automation RunTests Soul.' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$out" "-abslog=$out/native.log" "-UserDir=$out/User"
 $result=$LASTEXITCODE
}finally{if($held){$lease.ReleaseMutex()};$lease.Dispose()}
exit $result
