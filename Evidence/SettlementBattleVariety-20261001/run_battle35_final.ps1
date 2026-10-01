$ErrorActionPreference='Stop'
$w='D:/RefinedBadger/Worktrees/Soul-bannerlord-campaign-map-20260929'
$lease=New-Object System.Threading.Mutex($false,('Global\RefinedBadger_Unreal_'+$env:COMPUTERNAME));$held=$false
try {
 try {$held=$lease.WaitOne(0)}catch [System.Threading.AbandonedMutexException]{$held=$true}
 if(!$held){throw 'Unreal lease occupied'}
 if(Get-Process UnrealEditor*,ShaderCompileWorker -ErrorAction SilentlyContinue){throw 'Unreal already running'}
 $flags=@('-ForceRes','-RenderOffscreen','-unattended','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-DDC=InstalledNoZenLocalFallback','-SoulVerticalQualification','-SoulRealtimeVisualUnits','-SoulRealtimeMagicProof','-SoulEvilVisualRoster','-SoulActivePerSide=35','-SoulPlayerPool=100','-SoulEnemyPool=100',('-UserDir='+$w+'/Evidence/SettlementBattleVariety-20261001/Battle35FinalUser'))
 $args=@(($w+'/Tools/qualify_soul_vertical.py'),'--ue-exe','C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe','--project',($w+'/Soul.uproject'),'--stage','G6','--map-url','/Game/Dragon_graveyard/Level/L_showcase_level?game=/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode','--resolution','1280x720','--max-fps','30','--expected-active-units','70','--duration','60','--completion-marker','SOUL_RT_ARENA_PASS','--completion-timeout','600','--output',($w+'/Evidence/SettlementBattleVariety-20261001/battle35-final'))
 foreach($f in $flags){$args+=('--ue-arg='+$f)}
 & 'C:/Users/Jeff/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' @args
 $result=$LASTEXITCODE
}finally{if($held){$lease.ReleaseMutex()};$lease.Dispose()}
exit $result
