# Packaged Development qualification through the existing bounded runner.
# Use Input then Load with the same UserDir. Never use a founder/player save directory.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackageRoot,
    [Parameter(Mandatory=$true)][string]$UserDir,
    [Parameter(Mandatory=$true)][string]$Output,
    [ValidateSet('Input','Load','Roundtrip','Defeat')][string]$Mode='Input',
    [ValidateSet('1280x720','1920x1080')][string]$Resolution='1920x1080'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$package=(Resolve-Path -LiteralPath $PackageRoot).ProviderPath
$exe=Join-Path $package 'Soul/Binaries/Win64/Soul.exe'
if(!(Test-Path -LiteralPath $exe -PathType Leaf)){throw 'Expected an archived Windows Development package with Soul/Binaries/Win64/Soul.exe.'}
if(![IO.Path]::IsPathRooted($UserDir) -or ![IO.Path]::IsPathRooted($Output)){throw 'UserDir and Output must be absolute qualification paths.'}
$userRoot=[IO.Path]::GetFullPath($UserDir).TrimEnd('\','/')
$markerFile=Join-Path $userRoot 'soul-rc2-qualification-only.txt'
if(Test-Path -LiteralPath $userRoot){
    if(!(Test-Path -LiteralPath $markerFile -PathType Leaf)){throw 'Existing UserDir is not an owned RC2 qualification directory.'}
}else{
    $null=New-Item -ItemType Directory -Path $userRoot
    'Isolated automated RC2 checkpoint; not the founder save.' | Set-Content -LiteralPath $markerFile
}
$stage='G0';$units=0;$marker='SOUL_WORLD_VISUAL_INPUT_PASS'
$flags=@('-ForceRes','-RenderOffscreen','-unattended','-nosound',('-UserDir='+$userRoot),('-SoulCampaignCapturePrefix=rc2-'+$Mode.ToLowerInvariant()))
switch($Mode){
    'Input' {$flags+='-SoulCampaignVisualProof'}
    'Load' {$flags+='-SoulCampaignLoadProof';$marker='SOUL_CAMPAIGN_COLD_LOAD_PASS'}
    'Roundtrip' {$stage='G6';$units=30;$marker='SOUL_CAMPAIGN_ROUNDTRIP_PASS';$flags+=@('-SoulCampaignMouseRoundtrip','-SoulRealtimeVisualUnits','-SoulRealtimeMagicProof')}
    'Defeat' {$stage='G6';$units=27;$marker='SOUL_CAMPAIGN_ROUNDTRIP_PASS';$flags+=@('-SoulCampaignMouseRoundtrip','-SoulCampaignDefeatProof','-SoulRealtimeVisualUnits')}
}
$arguments=@((Join-Path $PSScriptRoot 'qualify_soul_vertical.py'),'--ue-exe',$exe,
    '--project',(Join-Path $root 'Soul.uproject'),'--stage',$stage,
    '--map-url','/Engine/Maps/Entry?game=/Script/Soul.SoulFounderPlaytestGameMode',
    '--resolution',$Resolution,'--max-fps','30','--expected-active-units',"$units",
    '--duration','60','--completion-marker',$marker,'--completion-timeout','600','--output',$Output)
foreach($flag in $flags){$arguments+=('--ue-arg='+$flag)}
& python @arguments
exit $LASTEXITCODE
