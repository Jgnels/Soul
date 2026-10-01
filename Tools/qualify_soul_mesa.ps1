param([ValidateSet('Input','Load','Roundtrip','Defeat')][string]$Mode='Input', [string]$Run)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(!$Run){$Run=$Mode.ToLowerInvariant()+'-'+[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')}
if($Run -notmatch '^[a-zA-Z0-9_-]+$'){throw 'Run must be a plain evidence name.'}
$output=Join-Path $root ('Evidence/CampaignIntegration-20260930/Local/'+$Run)
$stage='G0';$units=0;$marker='SOUL_WORLD_VISUAL_INPUT_PASS'
$flags=@('-ForceRes','-RenderOffscreen','-unattended','-nosound',
    '-DisablePlugins=AndroidFileServer,NwiroIntegrationKit','-DDC=InstalledNoZenLocalFallback',
    '-SoulMesaTerrain',('-SoulCampaignCapturePrefix='+$Run))
switch($Mode){
    'Input' {$flags+='-SoulCampaignVisualProof'}
    'Load' {$flags+='-SoulCampaignLoadProof';$marker='SOUL_CAMPAIGN_COLD_LOAD_PASS'}
    'Roundtrip' {
        $stage='G6';$units=30;$marker='SOUL_CAMPAIGN_ROUNDTRIP_PASS'
        $flags+=@('-SoulCampaignMouseRoundtrip','-SoulRealtimeVisualUnits','-SoulRealtimeMagicProof')
    }
    'Defeat' {
        $stage='G6';$units=27;$marker='SOUL_CAMPAIGN_ROUNDTRIP_PASS'
        $flags+=@('-SoulCampaignMouseRoundtrip','-SoulCampaignDefeatProof','-SoulRealtimeVisualUnits')
    }
}
$arguments=@((Join-Path $PSScriptRoot 'qualify_soul_vertical.py'),
    '--ue-exe','C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe',
    '--project',(Join-Path $root 'Soul.uproject'),'--stage',$stage,
    '--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode',
    '--resolution','1920x1080','--max-fps','30','--expected-active-units',"$units",
    '--duration','60','--completion-marker',$marker,'--completion-timeout','600',
    '--output',$output)
foreach($flag in $flags){$arguments+=('--ue-arg='+$flag)}
& python @arguments
exit $LASTEXITCODE
