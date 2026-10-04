param([string]$Mode='runtime',[int]$Active=15,[int]$PlayerPool=0,[int]$EnemyPool=0,[switch]$OffscreenProof,[switch]$DefeatRetry,[switch]$Spellbook,[switch]$Profile,[switch]$Audio,[switch]$VisibleProof,[int]$Width=1280,[int]$Height=720)
$ErrorActionPreference='Stop'
if($PlayerPool -le 0){$PlayerPool=$Active}
if($EnemyPool -le 0){$EnemyPool=$Active}
$root='D:\RefinedBadger\Worktrees\Soul-bannerlord-campaign-map-20260929'
$evidence=Join-Path $root 'Evidence\Spellbook-20261004'
$report=Join-Path $evidence ('Automation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
$common=@('-NoSound','-NoSplash','-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-DDC=InstalledNoZenLocalFallback')
if($Audio){$common=@($common|Where-Object {$_ -ne '-NoSound'})}
if($Mode -eq 'automation'){
    $runArgs=$common+@('-unattended','-NullRHI','-ExecCmds="Automation RunTests Soul."','-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$report+'"'),('-abslog="'+$evidence+'\Automation-readability.log"'))
} elseif($Mode -eq 'small'){
    $runArgs=$common+@('/Game/Dragon_graveyard/Level/L_showcase_level?game=/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode','-game','-windowed',('-ResX='+$Width),('-ResY='+$Height),'-ForceRes','-d3d11','-SoulRealtimeVisualUnits','-SoulEvilVisualRoster','-SoulControlDiagnostics',("-SoulActivePerSide="+$Active),("-SoulPlayerPool="+$PlayerPool),("-SoulEnemyPool="+$EnemyPool),'-SoulArenaOriginX=14000','-SoulArenaOriginY=-10000',('-UserDir="'+$evidence+'\ReadabilityUser"'),('-abslog="'+$evidence+'\SmallBattleRuntime.log"'))
} else{
    $runArgs=$common+@('/Engine/Maps/Entry','-game','-windowed',('-ResX='+$Width),('-ResY='+$Height),'-ForceRes','-d3d11','-SoulRealtimeVisualUnits','-SoulEvilVisualRoster','-SoulMesaTerrain','-SoulEvilCorridor','-SoulCampaignVisualProof','-SoulCampaignMouseRoundtrip','-SoulControlDiagnostics',('-UserDir="'+$evidence+'\ReadabilityCampaignUser"'),('-abslog="'+$evidence+'\ReadabilityCampaignRuntime.log"'))
}
if($DefeatRetry){
    $runArgs=$runArgs|ForEach-Object { $_.Replace('ReadabilityCampaignUser','DefeatRecoveryUser').Replace('ReadabilityCampaignRuntime.log','DefeatRecoveryRuntime.log') }
    $runArgs+=@('-RenderOffscreen','-unattended','-SoulCampaignDefeatProof','-SoulCampaignRetryQualification','-SoulRealtimeMagicProof')
}
if($Spellbook){$runArgs+=@('-SoulSpellbookProof')}
if($Profile){$runArgs+=@('-SoulBattleProfile')}
if($VisibleProof){$runArgs+=@('-unattended','-SoulBattleReadabilityProof')}
if($OffscreenProof){$runArgs+=@('-RenderOffscreen','-unattended','-SoulBattleReadabilityProof')}
& 'D:\RefinedBadger\Studio-Control\tools\RB_UNREAL.ps1' -Lane Soul -Project (Join-Path $root 'Soul.uproject') -ExtraArgs $runArgs
