param([string]$Preview='D:/RefinedBadger/AssetLibraries/SoulTerrainPreview')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
# External runtime mounts: this script never opens or saves any Unreal package.
# Junctions are not an ACL write barrier; never save source assets through them.
$links=@{
    'Content/SoulCampaignMountain'='Content/SoulCampaignMountain'
    'Content/LandscapePackOne'='Content/LandscapePackOne'
    'Content/LandscapePackTwo'='Content/LandscapePackTwo'
    'Data/CampaignMesaLocal'='SoulIntegration'
}
foreach($relative in $links.Keys) {
    $target=(Resolve-Path -LiteralPath (Join-Path $Preview $links[$relative])).Path
    $link=Join-Path $root $relative
    if(Test-Path -LiteralPath $link) {
        $existing=Get-Item -LiteralPath $link
        if($existing.LinkType -ne 'Junction' -or $existing.Target[0] -ne $target) {throw "Existing path is not the expected mount: $link"}
    } else {New-Item -ItemType Junction -Path $link -Target $target | Out-Null}
    git -C $root check-ignore --quiet -- $relative
    if($LASTEXITCODE -ne 0){throw "External content is not ignored: $relative"}
}
$profile=Get-Content -Raw -LiteralPath (Join-Path $root 'Data/CampaignMesa/presentation.json') | ConvertFrom-Json
$height=Join-Path $root 'Data/CampaignMesaLocal/MesaHeight.r16'
if((Get-FileHash -LiteralPath $height).Hash.ToLowerInvariant() -ne $profile.height_sha256){throw 'Mesa height/profile mismatch; run integrate_soul_mesa.py'}
Write-Output 'Soul mesa external runtime mounts and height hash verified.'
