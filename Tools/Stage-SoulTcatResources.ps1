# TCAT's runtime StartupModule registers these two Slate brushes even outside the editor.
# UAT omits plugin Resources by default. Supplement only these exact runtime reads.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ProjectRoot,
    [Parameter(Mandatory=$true)][string[]]$WindowsPackageRoots
)
$ErrorActionPreference='Stop'
foreach($package in $WindowsPackageRoots){
    $package=(Resolve-Path -LiteralPath $package).ProviderPath
    if(!(Test-Path -LiteralPath (Join-Path $package 'Soul/Binaries/Win64/Soul.exe') -PathType Leaf)){
        throw "Expected a completed Development Windows stage/archive: $package"
    }
    foreach($name in @('InfluenceComponentIcon.png','InfluenceComponentIcon_64.png')){
        $relative='Plugins/TCAT/Resources/'+$name
        $source=Join-Path $ProjectRoot $relative
        if(!(Test-Path -LiteralPath $source -PathType Leaf)){throw "Missing TCAT runtime resource: $source"}
        $destination=Join-Path $package ('Soul/'+$relative)
        if(Test-Path -LiteralPath $destination){throw "Runtime resource already staged; preserve it: $destination"}
        $null=New-Item -ItemType Directory -Force -Path (Split-Path $destination -Parent)
        Copy-Item -LiteralPath $source -Destination $destination
        [PSCustomObject]@{source=$source;destination=$destination;bytes=(Get-Item -LiteralPath $destination).Length}
    }
}
