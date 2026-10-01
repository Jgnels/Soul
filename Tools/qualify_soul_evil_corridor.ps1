param([ValidateSet('Input','Load','Roundtrip','Defeat')][string]$Mode='Input',[Parameter(Mandatory=$true)][string]$Run)
$ErrorActionPreference='Stop'
if($Run -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Use a plain run name'}
$root=Split-Path $PSScriptRoot -Parent
$output=Join-Path $root ('Evidence/EvilCorridor-20261001/Local/'+$Run)
$user=Join-Path $root 'Evidence/EvilCorridor-20261001/Local/User'
$stage='G0';$units=0;$marker='SOUL_WORLD_VISUAL_INPUT_PASS'
$flags=@('-ForceRes','-RenderOffscreen','-unattended','-nosound','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-DDC=InstalledNoZenLocalFallback','-SoulMesaTerrain','-SoulEvilCorridor',('-UserDir='+$user),('-SoulCampaignCapturePrefix='+$Run))
switch($Mode){
 'Input' {$flags+='-SoulCampaignVisualProof'}
 'Load' {$flags+='-SoulCampaignLoadProof';$marker='SOUL_CAMPAIGN_COLD_LOAD_PASS'}
 'Roundtrip' {$stage='G6';$units=30;$marker='SOUL_CAMPAIGN_ROUNDTRIP_PASS';$flags+=@('-SoulCampaignMouseRoundtrip','-SoulRealtimeVisualUnits','-SoulRealtimeMagicProof')}
 'Defeat' {$stage='G6';$units=27;$marker='SOUL_CAMPAIGN_ROUNDTRIP_PASS';$flags+=@('-SoulCampaignMouseRoundtrip','-SoulCampaignDefeatProof','-SoulRealtimeVisualUnits')}
}
$arguments=@((Join-Path $PSScriptRoot 'qualify_soul_vertical.py'),'--ue-exe','C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe','--project',(Join-Path $root 'Soul.uproject'),'--stage',$stage,'--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode','--resolution','1280x720','--max-fps','30','--expected-active-units',"$units",'--duration','60','--completion-marker',$marker,'--completion-timeout','600','--output',$output)
foreach($flag in $flags){$arguments+=('--ue-arg='+$flag)}
$lease=New-Object System.Threading.Mutex($false,('Global\RefinedBadger_Unreal_'+$env:COMPUTERNAME));$held=$false
try {
 try {$held=$lease.WaitOne(0)}catch [System.Threading.AbandonedMutexException]{$held=$true}
 if(!$held){throw 'Existing Studio Control Unreal lease is occupied'}
 & 'C:/Users/Jeff/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' @arguments
 $result=$LASTEXITCODE
}finally{if($held){$lease.ReleaseMutex()};$lease.Dispose()}
exit $result
