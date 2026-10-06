param()
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Split-Path (Split-Path $PSScriptRoot -Parent) -Parent))
$content = Join-Path $root 'Content'
$source = 'D:\RefinedBadger\AssetLibraries\MedievalKingdom-42d4a792\Content'
$evidence = Join-Path $root 'Evidence\SettlementEnvironmentPlan-20261005'
$receipt = Join-Path $evidence 'Continuation-20261006\human-complete-source-mount.json'
$preserved = Join-Path $evidence 'Local\PreservedHumanMounts'
if (Test-Path -LiteralPath $receipt) { throw 'Mount receipt already exists; inspect current mounts instead.' }
if (Get-Process UnrealEditor,UnrealEditor-Cmd,Soul -ErrorAction SilentlyContinue) { throw 'Close the owned Unreal session before changing mounts.' }
$rows = @()
foreach ($relative in @('CastleTown','__ExternalActors__\CastleTown','__ExternalObjects__\CastleTown')) {
    $target = Join-Path $source $relative
    if (!(Test-Path -LiteralPath $target -PathType Container)) { throw "Complete source missing: $relative" }
    $mount = [IO.Path]::GetFullPath((Join-Path $content $relative))
    $backup = [IO.Path]::GetFullPath((Join-Path $preserved $relative))
    if (!$mount.StartsWith($content + '\') -or !$backup.StartsWith($preserved + '\')) { throw 'Unexpected mount path' }
    if (Test-Path -LiteralPath $backup) { throw 'Preserved mount already exists' }
    $old = $null
    if (Test-Path -LiteralPath $mount) {
        $item = Get-Item -LiteralPath $mount
        if ($item.LinkType -ne 'Junction') { throw "Will not move actual content: $mount" }
        $old = [string]$item.Target
    }
    $rows += [ordered]@{ relative=$relative; mount=$mount; previous_target=$old; preserved_mount=$backup; new_target=$target }
}
# Preserve the junction objects outside Content so Unreal sees only one family.
# Move-Item is nonrecursive here. No donor files are moved, edited or deleted.
foreach ($row in $rows) {
    if ($row.previous_target) {
        New-Item -ItemType Directory -Path (Split-Path $row.preserved_mount -Parent) -Force | Out-Null
        Move-Item -LiteralPath $row.mount -Destination $row.preserved_mount
    }
    New-Item -ItemType Directory -Path (Split-Path $row.mount -Parent) -Force | Out-Null
    New-Item -ItemType Junction -Path $row.mount -Target $row.new_target | Out-Null
    if ((Get-Item -LiteralPath $row.mount).Target -ne $row.new_target) { throw 'Mount verification failed' }
}
[ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); source_authority='Jeff supplied complete verified Human capital; no recovery or download'; mounts=$rows; donor_mutation='none' } |
    ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $receipt
Write-Output $receipt
