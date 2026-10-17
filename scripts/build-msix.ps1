# ********************************************************************************
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li (github.com/dxli)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.
# ********************************************************************************

<#
.SYNOPSIS
Creates one unsigned LibreCAD MSIX package for a single architecture.

.DESCRIPTION
The package is left unsigned.  The Microsoft Store re-signs submissions after
certification, so nothing more is needed for that route; for direct download the
bundle has to be signed afterwards, because Windows refuses to install an MSIX
whose certificate does not chain to a root trusted on the machine.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('x64', 'arm64')]
    [string]$Architecture,

    [Parameter(Mandatory = $true)]
    [string]$InputDirectory,

    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,

    [Parameter(Mandatory = $true)]
    [string]$PackageVersion,

    [Parameter(Mandatory = $true)]
    [string]$IdentityName,

    [Parameter(Mandatory = $true)]
    [string]$Publisher,

    [Parameter(Mandatory = $true)]
    [string]$PublisherDisplayName,

    [Parameter(Mandatory = $true)]
    [ValidateSet('LibreCAD', 'LibreCAD-beta')]
    [string]$PackageName,

    [string]$ResourceDirectory = (Join-Path $PSScriptRoot '..\librecad\support'),

    [string]$IconPath = (Join-Path $PSScriptRoot '..\librecad\res\images\librecad.png')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ExistingDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "$Description directory does not exist: $Path"
    }

    return (Resolve-Path -LiteralPath $Path).Path
}

function Find-SdkTool {
    param([Parameter(Mandatory = $true)][string]$Name)

    # Sorting every hit by path puts the x86 build first, and there is no arm64
    # build of makepri.exe at all, so ask for the x64 tools by name.  Package on
    # an x64 runner.
    $sdkBin = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    if (Test-Path -LiteralPath $sdkBin -PathType Container) {
        $candidate = Get-ChildItem -LiteralPath $sdkBin -Directory -Filter '10.*' |
            Sort-Object -Property Name -Descending |
            ForEach-Object { Join-Path $_.FullName "x64\$Name" } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate
        }
    }

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Path
    }

    throw "$Name was not found under $sdkBin or on PATH. Install the Windows SDK."
}

function Test-MsixVersion {
    param([Parameter(Mandatory = $true)][string]$Version)

    if ($Version -notmatch '^\d{1,5}\.\d{1,5}\.\d{1,5}\.\d{1,5}$') {
        throw "PackageVersion must have four numeric components: $Version"
    }

    $components = $Version.Split('.')
    foreach ($component in $components) {
        if ([int]$component -gt 65535) {
            throw "Each PackageVersion component must be at most 65535: $Version"
        }
    }
    if ([int]$components[0] -eq 0) {
        throw "The first PackageVersion component must not be 0: $Version"
    }
    # The Microsoft Store reserves the revision component and rejects any package
    # that does not build it as 0.
    if ([int]$components[3] -ne 0) {
        throw "The PackageVersion revision component must be 0 for Store submission: $Version"
    }
}

function Escape-XmlAttribute {
    param([Parameter(Mandatory = $true)][string]$Value)

    return [System.Security.SecurityElement]::Escape($Value)
}

function Test-PackagedManifest {
    param(
        [Parameter(Mandatory = $true)][string]$ManifestPath,
        [Parameter(Mandatory = $true)][string]$ExpectedPackageName,
        [Parameter(Mandatory = $true)][string]$ExpectedIdentityName
    )

    [xml]$manifestXml = Get-Content -LiteralPath $ManifestPath -Raw
    $namespaceManager = [System.Xml.XmlNamespaceManager]::new($manifestXml.NameTable)
    $namespaceManager.AddNamespace('f', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10')
    $namespaceManager.AddNamespace('uap', 'http://schemas.microsoft.com/appx/manifest/uap/windows10')
    $namespaceManager.AddNamespace('uap3', 'http://schemas.microsoft.com/appx/manifest/uap/windows10/3')
    $namespaceManager.AddNamespace('rescap3', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities/3')

    $packageDisplayName = $manifestXml.SelectSingleNode('/f:Package/f:Properties/f:DisplayName', $namespaceManager)
    $identity = $manifestXml.SelectSingleNode('/f:Package/f:Identity', $namespaceManager)
    $visualElements = $manifestXml.SelectSingleNode(
        '/f:Package/f:Applications/f:Application/uap:VisualElements',
        $namespaceManager
    )
    if (-not $packageDisplayName -or $packageDisplayName.InnerText -ne $ExpectedPackageName) {
        throw "MSIX package DisplayName does not match PackageName '$ExpectedPackageName'."
    }
    if (-not $identity -or $identity.GetAttribute('Name') -ne $ExpectedIdentityName) {
        throw "MSIX identity does not match IdentityName '$ExpectedIdentityName'."
    }
    if (-not $visualElements -or $visualElements.GetAttribute('DisplayName') -ne $ExpectedPackageName) {
        throw "MSIX application DisplayName does not match PackageName '$ExpectedPackageName'."
    }

    $fileTypes = @(
        $manifestXml.SelectNodes(
            '//*[local-name()="Extension" and @Category="windows.fileTypeAssociation"]/*[local-name()="FileTypeAssociation"]/*[local-name()="SupportedFileTypes"]/*[local-name()="FileType"]',
            $namespaceManager
        ) | ForEach-Object { $_.InnerText }
    )
    foreach ($requiredFileType in @('.dxf', '.dwg')) {
        if ($requiredFileType -notin $fileTypes) {
            throw "MSIX manifest is missing the $requiredFileType file association."
        }
    }

    $migrationProgIds = @(
        $manifestXml.SelectNodes('//rescap3:MigrationProgId', $namespaceManager) |
            ForEach-Object { $_.InnerText }
    )
    $requiredProgIds = @('LibreCAD.DXF', 'LibreCAD.DWG')
    foreach ($requiredProgId in $requiredProgIds) {
        if ($requiredProgId -notin $migrationProgIds) {
            throw "MSIX manifest is missing migration ProgID $requiredProgId."
        }
    }
    if ($migrationProgIds.Count -ne $requiredProgIds.Count) {
        throw 'MSIX manifest contains unexpected migration ProgIDs.'
    }

    $customInstall = $manifestXml.SelectSingleNode(
        '//*[local-name()="Extension" and @Category="windows.customInstall"]',
        $namespaceManager
    )
    if ($customInstall) {
        throw 'Store MSIX packages must not use restricted windows.customInstall actions.'
    }
}

function Copy-DirectoryContents {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $Destination -Recurse -Force
    }
}

function Copy-MatchingFiles {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Filter,
        [Parameter(Mandatory = $true)][string]$Destination,
        [switch]$Recurse,
        [switch]$Required
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        if ($Required) {
            throw "Required source directory for $Filter does not exist: $Source"
        }
        return
    }

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    $found = @(Get-ChildItem -LiteralPath $Source -Filter $Filter -File -Recurse:$Recurse)
    foreach ($item in $found) {
        Copy-Item -LiteralPath $item.FullName -Destination $Destination -Force
    }

    # A silent no-op here used to ship a package with no translations at all.
    if ($Required -and $found.Count -eq 0) {
        throw "No files matching $Filter were found under $Source."
    }
}

function Copy-SupportTree {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination,
        [Parameter(Mandatory = $true)][string]$Filter
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        throw "Support directory does not exist: $Source"
    }

    $items = @(Get-ChildItem -LiteralPath $Source -Filter $Filter -File -Recurse)
    if ($items.Count -eq 0) {
        throw "No $Filter files found under $Source."
    }

    $prefix = (Resolve-Path -LiteralPath $Source).Path.TrimEnd('\') + '\'
    foreach ($item in $items) {
        $relative = $item.FullName.Substring($prefix.Length)
        $target = Join-Path $Destination $relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Copy-Item -LiteralPath $item.FullName -Destination $target -Force
    }
}

function Copy-PluginModules {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        return
    }

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    $pluginModules = @(Get-ChildItem -LiteralPath $Source -Recurse -Filter '*.dll' -File)
    foreach ($pluginModule in $pluginModules) {
        Copy-Item -LiteralPath $pluginModule.FullName -Destination $Destination -Force
    }

    if ($pluginModules.Count -eq 0) {
        throw "No plugin DLLs found under $Source. The MSIX layout would be incomplete."
    }
}

function New-MsixImageAsset {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination,
        [Parameter(Mandatory = $true)][int]$Width,
        [Parameter(Mandatory = $true)][int]$Height
    )

    $image = $null
    $bitmap = $null
    $graphics = $null
    try {
        $image = [System.Drawing.Image]::FromFile($Source)
        $bitmap = [System.Drawing.Bitmap]::new($Width, $Height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality

        $scale = [Math]::Min($Width / $image.Width, $Height / $image.Height)
        $targetWidth = [Math]::Max(1, [int][Math]::Round($image.Width * $scale))
        $targetHeight = [Math]::Max(1, [int][Math]::Round($image.Height * $scale))
        $x = [int][Math]::Floor(($Width - $targetWidth) / 2)
        $y = [int][Math]::Floor(($Height - $targetHeight) / 2)

        $graphics.DrawImage($image, [System.Drawing.Rectangle]::new($x, $y, $targetWidth, $targetHeight))
        $bitmap.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        if ($graphics) { $graphics.Dispose() }
        if ($bitmap) { $bitmap.Dispose() }
        if ($image) { $image.Dispose() }
    }
}

function Invoke-MakeAppx {
    param(
        [Parameter(Mandatory = $true)][string]$MakeAppx,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$Operation
    )

    & $MakeAppx @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "MakeAppx $Operation failed with exit code $LASTEXITCODE."
    }
}

Test-MsixVersion -Version $PackageVersion
if ($IdentityName -notmatch '^[A-Za-z0-9.-]+$') {
    throw "IdentityName may contain only letters, numbers, periods, and hyphens: $IdentityName"
}

$inputDirectory = Resolve-ExistingDirectory -Path $InputDirectory -Description 'Input'
$buildDirectory = Resolve-ExistingDirectory -Path $BuildDirectory -Description 'Build'
$resourceDirectory = Resolve-ExistingDirectory -Path $ResourceDirectory -Description 'Resource'
if (-not (Test-Path -LiteralPath $IconPath -PathType Leaf)) {
    throw "MSIX icon source does not exist: $IconPath"
}

$libreCadExecutable = Join-Path $inputDirectory 'LibreCAD.exe'
if (-not (Test-Path -LiteralPath $libreCadExecutable -PathType Leaf)) {
    throw "Input directory does not contain LibreCAD.exe: $inputDirectory"
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$outputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$layoutDirectory = Join-Path $outputDirectory "layout-$Architecture"
$validationDirectory = Join-Path $outputDirectory "validation-$Architecture"
foreach ($directory in @($layoutDirectory, $validationDirectory)) {
    if (Test-Path -LiteralPath $directory) {
        Remove-Item -LiteralPath $directory -Recurse -Force
    }
}

New-Item -ItemType Directory -Force -Path $layoutDirectory | Out-Null
Copy-DirectoryContents -Source $inputDirectory -Destination $layoutDirectory

# windeployqt's translations\ is re-staged under resources\qm below, which is
# where rs_system looks; .pdb are debug symbols; the Universal CRT in the system
# directory is always used on Windows 10 and later, so an app-local copy is dead
# weight.
$layoutExclusions = @(
    (Join-Path $layoutDirectory 'translations')
)
foreach ($exclusion in $layoutExclusions) {
    if (Test-Path -LiteralPath $exclusion) {
        Remove-Item -LiteralPath $exclusion -Recurse -Force
    }
}
Get-ChildItem -LiteralPath $layoutDirectory -Recurse -File |
    Where-Object { $_.Extension -eq '.pdb' -or $_.Name -eq 'ucrtbase.dll' -or $_.Name -like 'api-ms-win-*.dll' } |
    Remove-Item -Force

# Only the payload the NSIS installer ships, which also keeps repository stray
# files such as .DS_Store out of the package.
$layoutResources = Join-Path $layoutDirectory 'resources'
Copy-SupportTree -Source (Join-Path $resourceDirectory 'fonts') -Destination (Join-Path $layoutResources 'fonts') -Filter '*.lff'
Copy-SupportTree -Source (Join-Path $resourceDirectory 'patterns') -Destination (Join-Path $layoutResources 'patterns') -Filter '*.dxf'
Copy-SupportTree -Source (Join-Path $resourceDirectory 'library') -Destination (Join-Path $layoutResources 'library') -Filter '*.dxf'

$pluginsDirectory = Join-Path $buildDirectory 'plugins'
Copy-PluginModules -Source $pluginsDirectory -Destination (Join-Path $layoutResources 'plugins')

$translationDirectory = Join-Path $layoutResources 'qm'
Copy-MatchingFiles -Source (Join-Path $inputDirectory 'translations') -Filter '*.qm' -Destination $translationDirectory
Copy-MatchingFiles -Source $buildDirectory -Filter 'librecad_*.qm' -Destination $translationDirectory -Recurse -Required
# plugins_*.qm are not produced today: every qt_add_translations in
# plugins/*/CMakeLists.txt is commented out.  Not required, so this stays quiet.
Copy-MatchingFiles -Source $pluginsDirectory -Filter 'plugins_*.qm' -Destination $translationDirectory -Recurse

Add-Type -AssemblyName System.Drawing
$assetDirectory = Join-Path $layoutDirectory 'Assets'
New-Item -ItemType Directory -Force -Path $assetDirectory | Out-Null

# Unqualified names are the fallback when no resource index resolves a variant.
$logoSizes = [ordered]@{
    'StoreLogo'         = @(50, 50)
    'Square44x44Logo'   = @(44, 44)
    'Square71x71Logo'   = @(71, 71)
    'Square150x150Logo' = @(150, 150)
    'Wide310x150Logo'   = @(310, 150)
    'Square310x310Logo' = @(310, 310)
}
foreach ($logo in $logoSizes.Keys) {
    $width, $height = $logoSizes[$logo]
    New-MsixImageAsset -Source $IconPath -Destination (Join-Path $assetDirectory "$logo.png") -Width $width -Height $height
    foreach ($scale in @(100, 125, 150, 200, 400)) {
        New-MsixImageAsset `
            -Source $IconPath `
            -Destination (Join-Path $assetDirectory "$logo.scale-$scale.png") `
            -Width ([int][Math]::Round($width * $scale / 100.0)) `
            -Height ([int][Math]::Round($height * $scale / 100.0))
    }
}

# Without the targetsize/altform-unplated variants Windows puts a backplate
# behind the icon on the taskbar and in Start.  They only resolve through the
# resource index built below.
foreach ($targetSize in @(16, 24, 32, 48, 256)) {
    foreach ($suffix in @('', '_altform-unplated')) {
        New-MsixImageAsset `
            -Source $IconPath `
            -Destination (Join-Path $assetDirectory "Square44x44Logo.targetsize-$targetSize$suffix.png") `
            -Width $targetSize -Height $targetSize
    }
}

$manifestTemplatePath = Join-Path $PSScriptRoot 'msix\AppxManifest.xml.in'
if (-not (Test-Path -LiteralPath $manifestTemplatePath -PathType Leaf)) {
    throw "MSIX manifest template does not exist: $manifestTemplatePath"
}

$manifest = Get-Content -LiteralPath $manifestTemplatePath -Raw
$replacements = @{
    '@IDENTITY_NAME@' = Escape-XmlAttribute -Value $IdentityName
    '@PUBLISHER@' = Escape-XmlAttribute -Value $Publisher
    '@PUBLISHER_DISPLAY_NAME@' = Escape-XmlAttribute -Value $PublisherDisplayName
    '@PACKAGE_NAME@' = Escape-XmlAttribute -Value $PackageName
    # The ProgIDs the NSIS installer registers in HKCR, so an existing
    # installation's associations migrate.  They are fixed, not derived from the
    # package name: a hyphen is not legal in a ProgID and 'LibreCAD-beta.DXF'
    # fails manifest schema validation.
    '@LEGACY_DXF_PROGID@' = 'LibreCAD.DXF'
    '@LEGACY_DWG_PROGID@' = 'LibreCAD.DWG'
    '@PACKAGE_VERSION@' = $PackageVersion
    '@ARCHITECTURE@' = $Architecture
}
foreach ($placeholder in $replacements.Keys) {
    $manifest = $manifest.Replace($placeholder, $replacements[$placeholder])
}
if ($manifest -match '@[A-Z][A-Z0-9_]*@') {
    throw "MSIX manifest contains an unresolved placeholder: $($Matches[0])"
}
[System.IO.File]::WriteAllText(
    (Join-Path $layoutDirectory 'AppxManifest.xml'),
    $manifest,
    [System.Text.UTF8Encoding]::new($false)
)
Test-PackagedManifest `
    -ManifestPath (Join-Path $layoutDirectory 'AppxManifest.xml') `
    -ExpectedPackageName $PackageName `
    -ExpectedIdentityName $IdentityName

$makePri = Find-SdkTool -Name 'makepri.exe'
$priConfigPath = Join-Path $outputDirectory "priconfig-$Architecture.xml"
if (Test-Path -LiteralPath $priConfigPath) {
    Remove-Item -LiteralPath $priConfigPath -Force
}
# /am (AutoMerge) makes the App Certification Kit fail, so it is deliberately absent.
& $makePri createconfig /cf $priConfigPath /dq en-US /o
if ($LASTEXITCODE -ne 0) {
    throw "MakePri createconfig failed with exit code $LASTEXITCODE."
}
& $makePri new /pr $layoutDirectory /mn (Join-Path $layoutDirectory 'AppxManifest.xml') /cf $priConfigPath /of (Join-Path $layoutDirectory 'resources.pri') /o
if ($LASTEXITCODE -ne 0) {
    throw "MakePri new failed with exit code $LASTEXITCODE."
}
# The config lives outside the layout so it is never packaged.
Remove-Item -LiteralPath $priConfigPath -Force

$makeAppx = Find-SdkTool -Name 'makeappx.exe'
$packagePath = Join-Path $outputDirectory "$PackageName-$PackageVersion-windows-$Architecture.msix"
Invoke-MakeAppx -MakeAppx $makeAppx -Operation 'pack' -Arguments @(
    'pack', '/o', '/d', $layoutDirectory, '/p', $packagePath
)
Invoke-MakeAppx -MakeAppx $makeAppx -Operation 'unpack validation' -Arguments @(
    'unpack', '/o', '/p', $packagePath, '/d', $validationDirectory
)

foreach ($requiredPath in @('AppxManifest.xml', 'LibreCAD.exe', 'resources.pri', 'Assets\Square150x150Logo.png', 'resources\fonts\standard.lff')) {
    if (-not (Test-Path -LiteralPath (Join-Path $validationDirectory $requiredPath) -PathType Leaf)) {
        throw "MSIX validation output is missing $requiredPath"
    }
}
Test-PackagedManifest `
    -ManifestPath (Join-Path $validationDirectory 'AppxManifest.xml') `
    -ExpectedPackageName $PackageName `
    -ExpectedIdentityName $IdentityName

$hash = (Get-FileHash -LiteralPath $packagePath -Algorithm SHA256).Hash.ToLowerInvariant()
$hashPath = "$packagePath.sha256"
[System.IO.File]::WriteAllText($hashPath, "$hash  $([System.IO.Path]::GetFileName($packagePath))`n", [System.Text.Encoding]::ASCII)

if ($env:GITHUB_OUTPUT) {
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "package_path=$packagePath"
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "package_sha256=$hash"
}

Write-Host "Created unsigned package: $packagePath"
Write-Host "SHA256: $hash"
