param([string]$Preview='D:/RefinedBadger/AssetLibraries/SoulTerrainPreview')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$links=@{
    'Content/SoulCampaignWorld'='Content/SoulCampaignWorld'
    'Data/CampaignWorldLocal'='WorldTerrain'
}
foreach($relative in $links.Keys) {
    $target=(Resolve-Path -LiteralPath (Join-Path $Preview $links[$relative])).Path
    $link=Join-Path $root $relative
    if(Test-Path -LiteralPath $link) {
        $existing=Get-Item -LiteralPath $link
        $existingTarget=@($existing.Target)[0]
        if($existing.LinkType -ne 'Junction' -or $existingTarget -ne $target) {
            throw "Existing path is not the expected mount: $link"
        }
    } else { New-Item -ItemType Junction -Path $link -Target $target | Out-Null }
    git -C $root check-ignore --quiet -- $relative
    if($LASTEXITCODE -ne 0) { throw "External derivative payload is not ignored: $relative" }
}
$profile=Get-Content -Raw -LiteralPath (Join-Path $root 'Data/CampaignWorldTerrain/presentation.json') | ConvertFrom-Json
$height=Join-Path $root 'Data/CampaignWorldLocal/WorldHeight.r16'
if((Get-FileHash -LiteralPath $height).Hash.ToLowerInvariant() -ne $profile.height_sha256) {
    throw 'World height/profile mismatch; rebake and reimport the owned Landscape'
}
$qualification=Get-Content -Raw -LiteralPath (Join-Path $root 'Data/CampaignWorldLocal/qualification.json') | ConvertFrom-Json
if($qualification.status -ne 'pass') { throw 'Offline world terrain qualification is not passing' }
Write-Output 'Soul world local mounts and qualified height/profile hash verified. Runtime/import acceptance remains separate.'
