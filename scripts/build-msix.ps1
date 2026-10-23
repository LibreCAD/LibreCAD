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

    [string]$LicenseDirectory = (Join-Path $PSScriptRoot '..\licenses'),

    [string]$IconPath = (Join-Path $PSScriptRoot 'msix\librecad-logo.png'),

    # The commit the package is built from; licenses\SOURCE.txt names it.
    [string]$SourceRevision = $env:GITHUB_SHA
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
    # build of makepri.exe at all, so ask for the x64 tools by name.  An arm64
    # runner runs them under emulation.
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

function Get-PeSubsystem {
    param([Parameter(Mandatory = $true)][string]$Path)

    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) {
            throw "$Path is not a Windows executable."
        }
        $stream.Position = 0x3C
        $peHeader = $reader.ReadInt32()
        $stream.Position = $peHeader
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "$Path has no PE header."
        }
        # Subsystem sits 68 bytes into the optional header, which follows the
        # 4-byte signature and the 20-byte COFF header, in PE32 and PE32+ alike.
        $stream.Position = $peHeader + 4 + 20 + 68
        return $reader.ReadUInt16()
    }
    finally {
        $stream.Dispose()
    }
}

function Set-PeSubsystem {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][uint16]$Subsystem
    )

    # Call Get-PeSubsystem first: it checks that the file is a PE image.
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::ReadWrite)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        $stream.Position = 0x3C
        $peHeader = $reader.ReadInt32()
        $stream.Position = $peHeader + 4 + 20 + 68
        $writer = [System.IO.BinaryWriter]::new($stream)
        $writer.Write($Subsystem)
        $writer.Flush()
    }
    finally {
        $stream.Dispose()
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
        [Parameter(Mandatory = $true)][string]$Operation,
        [Parameter(Mandatory = $true)][string]$LogPath
    )

    # MakeAppx lists every file it packs, which would double the length of
    # the installer job's log, so its output goes to a file that is shown
    # when it fails.  The file outlives a run stopped by a time limit.
    Write-Host "MakeAppx $Operation, output in $LogPath"
    & $MakeAppx @Arguments > $LogPath
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $LogPath | ForEach-Object { Write-Host $_ }
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
$licenseDirectory = Resolve-ExistingDirectory -Path $LicenseDirectory -Description 'License'
$licenseFile = Join-Path $PSScriptRoot '..\LICENSE'
if (-not (Test-Path -LiteralPath $licenseFile -PathType Leaf)) {
    throw "LICENSE does not exist: $licenseFile"
}
if (-not (Test-Path -LiteralPath $IconPath -PathType Leaf)) {
    throw "MSIX icon source does not exist: $IconPath"
}
# System.Drawing resolves a relative path against the process directory,
# which Set-Location does not change.
$IconPath = (Resolve-Path -LiteralPath $IconPath).Path

$libreCadExecutable = Join-Path $inputDirectory 'LibreCAD.exe'
if (-not (Test-Path -LiteralPath $libreCadExecutable -PathType Leaf)) {
    throw "Input directory does not contain LibreCAD.exe: $inputDirectory"
}

# windeployqt --compiler-runtime brings only vc_redist.<arch>.exe, which is left
# out below; scripts\copy-vc-runtime.ps1 stages the runtime DLLs themselves.
foreach ($runtimeDll in @('vcruntime140.dll', 'msvcp140.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $inputDirectory $runtimeDll) -PathType Leaf)) {
        throw "Input directory is missing $runtimeDll; run scripts\copy-vc-runtime.ps1 on it first."
    }
}

# A console-subsystem LibreCAD.exe, which is what master builds, opens a console
# window beside the GUI each time Start, a file association or certification
# starts it.  The package gets its own copy marked as a Windows GUI program
# (subsystem 2) below, as editbin /SUBSYSTEM:WINDOWS would mark it; the entry
# point stays the same, and the installers keep the program as it was built.
$subsystem = Get-PeSubsystem -Path $libreCadExecutable
if ($subsystem -ne 2 -and $subsystem -ne 3) {
    throw "LibreCAD.exe has PE subsystem $subsystem, neither Windows GUI (2) nor console (3)."
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
$layoutExecutable = Join-Path $layoutDirectory 'LibreCAD.exe'
if ($subsystem -eq 3) {
    Set-PeSubsystem -Path $layoutExecutable -Subsystem 2
    Write-Host 'Marked the packaged LibreCAD.exe as a Windows GUI program.'
}
if ((Get-PeSubsystem -Path $layoutExecutable) -ne 2) {
    throw "The packaged LibreCAD.exe is not a Windows GUI program."
}

# windeployqt's translations\ is re-staged under resources\qm below, which is
# where rs_system looks; .pdb are debug symbols; .lib, .exp and .ilk are linker
# output of the build directory (its static librecad_lib.lib alone is close to
# 300 MB); the Universal CRT in the system directory is always used on Windows 10
# and later, so an app-local copy is dead weight; and LibreCAD paints with
# QPainter, never OpenGL or Direct3D, so the software OpenGL renderer and the
# Direct3D shader compilers windeployqt may deploy are dead weight too.
$layoutExclusions = @(
    (Join-Path $layoutDirectory 'translations')
)
foreach ($exclusion in $layoutExclusions) {
    if (Test-Path -LiteralPath $exclusion) {
        Remove-Item -LiteralPath $exclusion -Recurse -Force
    }
}
$excludedExtensions = @('.pdb', '.lib', '.exp', '.ilk')
$excludedNames = @('ucrtbase.dll', 'opengl32sw.dll', 'd3dcompiler_47.dll', 'dxcompiler.dll', 'dxil.dll')
Get-ChildItem -LiteralPath $layoutDirectory -Recurse -File |
    Where-Object { $_.Extension -in $excludedExtensions -or $_.Name -in $excludedNames -or $_.Name -like 'api-ms-win-*.dll' } |
    Remove-Item -Force

# The build directory also holds test and developer programs such as
# dwg_tarch_test.exe and libdxfrw_json_dump.exe.  LibreCAD.exe is the only
# program the package runs.
Get-ChildItem -LiteralPath $layoutDirectory -Recurse -File |
    Where-Object { $_.Extension -eq '.exe' -and $_.Name -ne 'LibreCAD.exe' } |
    ForEach-Object {
        Write-Host "Leaving $($_.Name) out of the package."
        Remove-Item -LiteralPath $_.FullName -Force
    }

# Only the payload the NSIS installer ships, which also keeps repository stray
# files such as .DS_Store out of the package.
$layoutResources = Join-Path $layoutDirectory 'resources'
Copy-SupportTree -Source (Join-Path $resourceDirectory 'fonts') -Destination (Join-Path $layoutResources 'fonts') -Filter '*.lff'
Copy-SupportTree -Source (Join-Path $resourceDirectory 'patterns') -Destination (Join-Path $layoutResources 'patterns') -Filter '*.dxf'
Copy-SupportTree -Source (Join-Path $resourceDirectory 'library') -Destination (Join-Path $layoutResources 'library') -Filter '*.dxf'

$pluginsDirectory = Join-Path $buildDirectory 'plugins'
Copy-PluginModules -Source $pluginsDirectory -Destination (Join-Path $layoutResources 'plugins')

# LibreCAD's own icon engine, which draws the icons in the colours set in the
# options, builds outside the staged Release folder, so the staged folder
# lacks it; main.cpp looks for it in iconengines\ beside LibreCAD.exe.
$layoutIconEngine = Join-Path $layoutDirectory 'iconengines\lc_svgiconengine.dll'
if (-not (Test-Path -LiteralPath $layoutIconEngine -PathType Leaf)) {
    # The Release build of a multi-config build folder, or a single-config
    # one; never a Debug copy, which needs Qt's debug DLLs.
    $builtIconEngine = @(
        (Join-Path $buildDirectory 'libraries\lciconengine\Release\lc_svgiconengine.dll'),
        (Join-Path $buildDirectory 'libraries\lciconengine\lc_svgiconengine.dll')
    ) | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
    if (-not $builtIconEngine) {
        throw "No Release lc_svgiconengine.dll under $buildDirectory\libraries\lciconengine."
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $layoutIconEngine) | Out-Null
    Copy-Item -LiteralPath $builtIconEngine -Destination $layoutIconEngine -Force
}

$translationDirectory = Join-Path $layoutResources 'qm'
Copy-MatchingFiles -Source (Join-Path $inputDirectory 'translations') -Filter '*.qm' -Destination $translationDirectory
Copy-MatchingFiles -Source $buildDirectory -Filter 'librecad_*.qm' -Destination $translationDirectory -Recurse -Required
Copy-MatchingFiles -Source $buildDirectory -Filter 'plugins_*.qm' -Destination $translationDirectory -Recurse -Required

# GPLv2 asks for a copy of the licence with every copy of the program, and the
# fonts and libraries LibreCAD bundles carry their own licences.
$layoutLicenses = Join-Path $layoutDirectory 'licenses'
Copy-MatchingFiles -Source $licenseDirectory -Filter '*.txt' -Destination $layoutLicenses -Required
Copy-MatchingFiles -Source $licenseDirectory -Filter 'readme.md' -Destination $layoutLicenses -Required
Copy-Item -LiteralPath $licenseFile -Destination (Join-Path $layoutLicenses 'LICENSE') -Force
# The notices of the fonts and Qt libraries that only this package names;
# readme.md is compiled into every build and stays as it is.
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'msix\third-party-notices.md') -Destination $layoutLicenses -Force

# Tell recipients which commit this very build comes from, where its source is,
# and offer them the source, as GPLv2 section 3(b) asks.
$repositoryUrl = 'https://github.com/LibreCAD/LibreCAD'
if ($env:GITHUB_SERVER_URL -and $env:GITHUB_REPOSITORY) {
    $repositoryUrl = "$env:GITHUB_SERVER_URL/$env:GITHUB_REPOSITORY"
}
if (-not $SourceRevision) {
    try {
        $SourceRevision = (& git -C $PSScriptRoot rev-parse HEAD 2>$null | Out-String).Trim()
    }
    catch {
        $SourceRevision = ''
    }
}
if ($SourceRevision -notmatch '^[0-9a-f]{40}$') {
    throw "Cannot tell which commit this package is built from; pass -SourceRevision: '$SourceRevision'"
}
# The Qt libraries ship under the GPLv2 too, so the offer below covers them.
$qtCoreVersion = (Get-Item -LiteralPath (Join-Path $inputDirectory 'Qt6Core.dll')).VersionInfo
if ($qtCoreVersion.FileMajorPart -lt 6) {
    throw "Cannot read the Qt version from Qt6Core.dll: '$($qtCoreVersion.FileVersion)'"
}
$qtVersion = "$($qtCoreVersion.FileMajorPart).$($qtCoreVersion.FileMinorPart).$($qtCoreVersion.FileBuildPart)"
$qtSourceUrl = "https://download.qt.io/archive/qt/$($qtCoreVersion.FileMajorPart).$($qtCoreVersion.FileMinorPart)/$qtVersion/"
# licenses\qt-third-party.txt carries the notices of the third-party code in
# one Qt release; a different Qt needs it regenerated.
$qtNotices = Join-Path $licenseDirectory 'qt-third-party.txt'
$qtNoticesHeader = Get-Content -LiteralPath $qtNotices -TotalCount 1
if ($qtNoticesHeader -notmatch "\(Qt $([regex]::Escape($qtVersion))\)$") {
    $message = "licenses/qt-third-party.txt is not for Qt $qtVersion ('$qtNoticesHeader'); run scripts/msix/update-qt-third-party.py --tag v$qtVersion"
    if ($env:GITHUB_ACTIONS) { Write-Host "::warning::$message" } else { Write-Warning $message }
}
# Boost is compiled into LibreCAD.exe (header-only) but is not in the
# repository, so the offer names the release this build used.
$cmakeCache = Join-Path $buildDirectory 'CMakeCache.txt'
$boostIncludeEntry = $null
if (Test-Path -LiteralPath $cmakeCache -PathType Leaf) {
    $boostIncludeEntry = Select-String -LiteralPath $cmakeCache -Pattern '^Boost_INCLUDE_DIR:[A-Z]+=(.+)$' |
        Select-Object -First 1
}
if (-not $boostIncludeEntry) {
    throw "Cannot find Boost_INCLUDE_DIR in $cmakeCache"
}
$boostVersionHeader = Join-Path $boostIncludeEntry.Matches[0].Groups[1].Value.Trim() 'boost\version.hpp'
$boostVersionDefine = $null
if (Test-Path -LiteralPath $boostVersionHeader -PathType Leaf) {
    $boostVersionDefine = Select-String -LiteralPath $boostVersionHeader -Pattern '^#define\s+BOOST_VERSION\s+(\d+)' |
        Select-Object -First 1
}
if (-not $boostVersionDefine) {
    throw "Cannot read the Boost version from $boostVersionHeader"
}
$boostNumber = [int]$boostVersionDefine.Matches[0].Groups[1].Value
$boostVersion = '{0}.{1}.{2}' -f [Math]::Floor($boostNumber / 100000), ([Math]::Floor($boostNumber / 100) % 1000), ($boostNumber % 100)
$boostSourceUrl = "https://archives.boost.io/release/$boostVersion/source/"
$sourceNotice = @"
LibreCAD is free software, licensed under the GNU General Public License
version 2; see LICENSE and gpl-2.0.txt in this folder. This package also
contains the libraries of Qt $qtVersion, which LibreCAD uses under the same
licence, and LibreCAD.exe contains code from Boost $boostVersion (Boost
Software License 1.0).

This package was built from commit $SourceRevision.
The source code of LibreCAD at that commit is at:
$repositoryUrl/tree/$SourceRevision
The source code of Qt $qtVersion is at:
$qtSourceUrl
The source code of Boost $boostVersion is at:
$boostSourceUrl

Written offer, as section 3(b) of the GPLv2 asks: for at least three years
after this package was distributed, the LibreCAD project will give any third
party, for a charge no more than the cost of physically performing source
distribution, a complete machine-readable copy of the corresponding source
code of this package - LibreCAD at the commit above, Boost $boostVersion and
Qt $qtVersion - under the terms of the GPLv2, on a medium customarily used
for software interchange. Ask for it at https://github.com/LibreCAD/LibreCAD/issues
"@
[System.IO.File]::WriteAllText(
    (Join-Path $layoutLicenses 'SOURCE.txt'),
    ($sourceNotice -replace "`r?`n", "`r`n") + "`r`n",
    [System.Text.UTF8Encoding]::new($false)
)

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
# The default config splits the index by scale into resources.scale-*.pri for
# resource packs, which this package does not have; Windows reads only
# resources.pri, so every logo variant above 100% would be lost.  Without the
# packaging section MakePri writes one index with every variant.
$priConfig = [xml](Get-Content -LiteralPath $priConfigPath -Raw)
$priPackaging = $priConfig.DocumentElement.SelectSingleNode('packaging')
if ($priPackaging) {
    [void]$priConfig.DocumentElement.RemoveChild($priPackaging)
}
$priConfig.Save($priConfigPath)
& $makePri new /pr $layoutDirectory /mn (Join-Path $layoutDirectory 'AppxManifest.xml') /cf $priConfigPath /of (Join-Path $layoutDirectory 'resources.pri') /o
if ($LASTEXITCODE -ne 0) {
    throw "MakePri new failed with exit code $LASTEXITCODE."
}
$splitIndexes = @(Get-ChildItem -LiteralPath $layoutDirectory -Filter '*.pri' -File | Where-Object { $_.Name -ne 'resources.pri' })
if ($splitIndexes.Count -gt 0) {
    throw "MakePri split the resource index: $($splitIndexes.Name -join ', ')"
}
# The config lives outside the layout so it is never packaged.
Remove-Item -LiteralPath $priConfigPath -Force

$makeAppx = Find-SdkTool -Name 'makeappx.exe'
$packagePath = Join-Path $outputDirectory "$PackageName-$PackageVersion-windows-$Architecture.msix"
# A package that fails a check below is deleted, so that neither a later CI
# step nor a person can take it for a good one.
try {
    Invoke-MakeAppx -MakeAppx $makeAppx -Operation 'pack' `
        -LogPath (Join-Path $outputDirectory "makeappx-pack-$Architecture.log") -Arguments @(
        'pack', '/o', '/d', $layoutDirectory, '/p', $packagePath
    )
    Invoke-MakeAppx -MakeAppx $makeAppx -Operation 'unpack validation' `
        -LogPath (Join-Path $outputDirectory "makeappx-unpack-$Architecture.log") -Arguments @(
        'unpack', '/o', '/p', $packagePath, '/d', $validationDirectory
    )

    foreach ($requiredPath in @('AppxManifest.xml', 'LibreCAD.exe', 'resources.pri', 'Assets\Square150x150Logo.png', 'iconengines\lc_svgiconengine.dll', 'resources\fonts\standard.lff', 'licenses\gpl-2.0.txt', 'licenses\gpl-3.0.txt', 'licenses\qt-third-party.txt', 'licenses\OFL-noto-cjk.txt', 'licenses\third-party-notices.md', 'licenses\LICENSE', 'licenses\SOURCE.txt')) {
        if (-not (Test-Path -LiteralPath (Join-Path $validationDirectory $requiredPath) -PathType Leaf)) {
            throw "MSIX validation output is missing $requiredPath"
        }
    }
    Test-PackagedManifest `
        -ManifestPath (Join-Path $validationDirectory 'AppxManifest.xml') `
        -ExpectedPackageName $PackageName `
        -ExpectedIdentityName $IdentityName
}
catch {
    if (Test-Path -LiteralPath $packagePath) {
        Remove-Item -LiteralPath $packagePath -Force
    }
    throw
}

$hash = (Get-FileHash -LiteralPath $packagePath -Algorithm SHA256).Hash.ToLowerInvariant()
$hashPath = "$packagePath.sha256"
[System.IO.File]::WriteAllText($hashPath, "$hash  $([System.IO.Path]::GetFileName($packagePath))`n", [System.Text.Encoding]::ASCII)

if ($env:GITHUB_OUTPUT) {
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "package_path=$packagePath"
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "package_sha256=$hash"
}

Write-Host "Created unsigned package: $packagePath"
Write-Host "Size: $([Math]::Round((Get-Item -LiteralPath $packagePath).Length / 1MB, 1)) MB"
Write-Host "SHA256: $hash"
