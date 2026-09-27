# Read-only checks only. Dot-sourcing defines functions; never starts a build.
function Assert-SoulEditorFreshness([string]$ProjectRoot, [string]$EngineRoot) {
    $ErrorActionPreference = 'Stop'
    $ProjectRoot = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\', '/')
    $EngineRoot = [IO.Path]::GetFullPath($EngineRoot).TrimEnd('\', '/')
    function Read-RequiredJson([string]$Path) {
        if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Editor preflight: missing $Path" }
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    }
    function Resolve-ReceiptPath([string]$Path) {
        return [IO.Path]::GetFullPath($Path.Replace('$(ProjectDir)', $ProjectRoot).Replace('$(EngineDir)', $EngineRoot))
    }
    $receiptPath = Join-Path $ProjectRoot 'Binaries/Win64/SoulEditor.target'
    $receipt = Read-RequiredJson $receiptPath
    if ($receipt.TargetName -ne 'SoulEditor' -or $receipt.Platform -ne 'Win64' -or
        $receipt.Configuration -ne 'Development' -or $receipt.TargetType -ne 'Editor') {
        throw 'Editor preflight: expected SoulEditor Win64 Development Editor receipt.'
    }
    $receiptTime = (Get-Item -LiteralPath $receiptPath).LastWriteTimeUtc
    $engineManifest = Read-RequiredJson (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.modules')
    $engineVersion = Read-RequiredJson (Join-Path $EngineRoot 'Engine/Build/Build.version')
    foreach ($key in @('MajorVersion', 'MinorVersion', 'PatchVersion', 'Changelist', 'CompatibleChangelist')) {
        if ($receipt.Version.$key -ne $engineVersion.$key) { throw "Editor preflight: engine version mismatch ($key)." }
    }
    if (-not $engineManifest.BuildId -or $receipt.Version.BuildId -ne $engineManifest.BuildId) {
        throw 'Editor preflight: engine/receipt BuildId mismatch.'
    }
    $project = Read-RequiredJson (Join-Path $ProjectRoot 'Soul.uproject')
    $owners = @(@{ Root = $ProjectRoot; Descriptor = (Join-Path $ProjectRoot 'Soul.uproject'); Modules = @($project.Modules) })
    # Include local plugins with editor products in the receipt (also covers transitive plugins).
    # Explicitly enabled local plugins must be present even if an old receipt omitted them.
    $products = @{}
    foreach ($product in $receipt.BuildProducts) { $products[(Resolve-ReceiptPath $product.Path)] = $product.Type }
    foreach ($descriptor in Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Plugins') -Filter '*.uplugin' -Recurse -File) {
        $pluginRoot = $descriptor.DirectoryName
        $manifestPath = Join-Path $pluginRoot 'Binaries/Win64/UnrealEditor.modules'
        $enabled = @($project.Plugins | Where-Object { $_.Name -eq $descriptor.BaseName -and $_.Enabled }).Count -gt 0
        if ($enabled -or $products.ContainsKey($manifestPath)) {
            $plugin = Read-RequiredJson $descriptor.FullName
            $owners += @{ Root = $pluginRoot; Descriptor = $descriptor.FullName; Modules = @($plugin.Modules) }
        }
    }
    $modules = @{}
    $inputs = @((Get-Item -LiteralPath (Join-Path $ProjectRoot 'Soul.uproject')))
    $inputs += @(Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Source') -Filter '*.Target.cs' -File)
    foreach ($owner in $owners) {
        $inputs += Get-Item -LiteralPath $owner.Descriptor
        $nativeModules = @($owner.Modules | Where-Object { $_.Type -in @('Runtime', 'RuntimeNoCommandlet', 'Editor', 'EditorNoCommandlet', 'Developer', 'DeveloperTool', 'UncookedOnly', 'CookedOnly') })
        # CookedOnly/RuntimeNoCommandlet modules may still be in editor receipts; use manifest for actual inventory.
        if (-not $nativeModules.Count) { continue }
        $manifestPath = Join-Path $owner.Root 'Binaries/Win64/UnrealEditor.modules'
        $manifest = Read-RequiredJson $manifestPath
        if (-not $products.ContainsKey($manifestPath)) { throw "Editor preflight: manifest absent from receipt: $manifestPath" }
        if ($manifest.BuildId -ne $engineManifest.BuildId) { throw "Editor preflight: module BuildId mismatch: $manifestPath" }
        foreach ($declared in $nativeModules) {
            if ($declared.Type -in @('Runtime', 'Editor', 'Developer', 'DeveloperTool', 'UncookedOnly') -and
                -not $manifest.Modules.PSObject.Properties[$declared.Name]) {
                throw "Editor preflight: declared module missing from manifest: $($declared.Name)"
            }
        }
        foreach ($entry in $manifest.Modules.PSObject.Properties) {
            $name = $entry.Name
            if ($entry.Value -ne "UnrealEditor-$name.dll") { throw "Editor preflight: nonstandard/hot-reload DLL for $name" }
            $dll = Join-Path (Split-Path -Parent $manifestPath) $entry.Value
            if (-not $products.ContainsKey($dll) -or $products[$dll] -ne 'DynamicLibrary' -or
                -not (Test-Path -LiteralPath $dll -PathType Leaf)) { throw "Editor preflight: DLL missing from disk/receipt: $dll" }
            $dllInfo = Get-Item -LiteralPath $dll
            if ($dllInfo.Length -eq 0 -or $dllInfo.LastWriteTimeUtc -gt $receiptTime) { throw "Editor preflight: invalid DLL or DLL newer than receipt: $dll" }
            $sourceRoot = Join-Path $owner.Root "Source/$name"
            $rules = Join-Path $sourceRoot "$name.Build.cs"
            if (-not (Test-Path -LiteralPath $rules -PathType Leaf)) { throw "Editor preflight: cannot verify local module source: $rules" }
            $sources = @(Get-ChildItem -LiteralPath $sourceRoot -Recurse -File | Where-Object { $_.Extension -in @('.cpp', '.c', '.cc', '.cxx', '.h', '.hpp', '.inl', '.ipp', '.cs', '.rc') })
            $inputs += $sources
            $dependencies = @([regex]::Matches([IO.File]::ReadAllText($rules), '"([A-Za-z_][A-Za-z0-9_]*)"') | ForEach-Object { $_.Groups[1].Value })
            if ($modules.ContainsKey($name)) { throw "Editor preflight: duplicate module $name" }
            $modules[$name] = @{ Dll = $dllInfo; Sources = $sources; Dependencies = $dependencies }
        }
    }
    # Receipt must postdate every native input, target rule and descriptor. Data/config are runtime inputs.
    foreach ($inputFile in $inputs) {
        if ($inputFile.LastWriteTimeUtc -gt $receiptTime) { throw "Editor preflight: source newer than SoulEditor receipt: $($inputFile.FullName). Rebuild SoulEditor before cook." }
    }
    foreach ($name in $modules.Keys) {
        $module = $modules[$name]
        $checks = @($module.Sources)
        $queue = New-Object 'System.Collections.Generic.Queue[string]'
        $seen = @{}
        foreach ($dependency in $module.Dependencies) { $queue.Enqueue($dependency) }
        while ($queue.Count) {
            $dependency = $queue.Dequeue()
            if ($seen.ContainsKey($dependency) -or -not $modules.ContainsKey($dependency)) { continue }
            $seen[$dependency] = $true
            # Conservative transitive header/rule check; C++ changes alone need not relink consumers.
            $checks += @($modules[$dependency].Sources | Where-Object { $_.Extension -in @('.h', '.hpp', '.inl', '.ipp', '.cs') })
            foreach ($next in $modules[$dependency].Dependencies) { $queue.Enqueue($next) }
        }
        foreach ($inputFile in $checks) {
            if ($inputFile.LastWriteTimeUtc -gt $module.Dll.LastWriteTimeUtc) {
                throw "Editor preflight: stale $name DLL; newer input $($inputFile.FullName). Rebuild SoulEditor before cook."
            }
        }
    }
    if ($modules.Count -eq 0) { throw 'Editor preflight: no verified modules.' }
    return [PSCustomObject]@{
        status = 'NO_PROVABLE_STALENESS'; modules_checked = $modules.Count
        receipt = $receiptPath; receipt_sha256 = (Get-FileHash -LiteralPath $receiptPath -Algorithm SHA256).Hash
        receipt_utc = $receiptTime.ToString('o'); build_id = $engineManifest.BuildId
        limitation = 'Timestamp/receipt checks do not prove compiler provenance or detect timestamp-preserving edits. No build/runtime acceptance.'
    }
}
