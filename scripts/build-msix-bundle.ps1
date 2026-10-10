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
Bundles the x64 and ARM64 LibreCAD MSIX packages for Microsoft Store submission.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$X64Package,

    [Parameter(Mandatory = $true)]
    [string]$Arm64Package,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,

    [Parameter(Mandatory = $true)]
    [string]$PackageVersion,

    [Parameter(Mandatory = $true)]
    [ValidateSet('LibreCAD', 'LibreCAD-beta')]
    [string]$PackageName
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Find-MakeAppx {
    # Sorting every hit by path puts the x86 build first; ask for x64 by name.
    $sdkBin = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    if (Test-Path -LiteralPath $sdkBin -PathType Container) {
        $candidate = Get-ChildItem -LiteralPath $sdkBin -Directory -Filter '10.*' |
            Sort-Object -Property Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'x64\makeappx.exe' } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate
        }
    }

    $command = Get-Command MakeAppx.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Path
    }

    throw "MakeAppx.exe was not found under $sdkBin or on PATH. Install the Windows SDK."
}

function Resolve-ExistingFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description does not exist: $Path"
    }

    return (Resolve-Path -LiteralPath $Path).Path
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
    # The Microsoft Store reserves the revision component and rejects any bundle
    # that does not build it as 0.
    if ([int]$components[3] -ne 0) {
        throw "The PackageVersion revision component must be 0 for Store submission: $Version"
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
$x64Package = Resolve-ExistingFile -Path $X64Package -Description 'x64 MSIX package'
$arm64Package = Resolve-ExistingFile -Path $Arm64Package -Description 'ARM64 MSIX package'
$expectedPackages = @(
    @{
        Path = $x64Package
        Name = "$PackageName-$PackageVersion-windows-x64.msix"
    },
    @{
        Path = $arm64Package
        Name = "$PackageName-$PackageVersion-windows-arm64.msix"
    }
)
foreach ($package in $expectedPackages) {
    if ([System.IO.Path]::GetFileName($package.Path) -ne $package.Name) {
        throw "Unexpected MSIX package name. Expected '$($package.Name)', got '$([System.IO.Path]::GetFileName($package.Path))'."
    }
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$outputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path

$bundleInputDirectory = Join-Path $outputDirectory 'bundle-input'
$validationDirectory = Join-Path $outputDirectory 'bundle-validation'
foreach ($directory in @($bundleInputDirectory, $validationDirectory)) {
    if (Test-Path -LiteralPath $directory) {
        Remove-Item -LiteralPath $directory -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
}

Copy-Item -LiteralPath $x64Package -Destination $bundleInputDirectory -Force
Copy-Item -LiteralPath $arm64Package -Destination $bundleInputDirectory -Force

$makeAppx = Find-MakeAppx
$bundlePath = Join-Path $outputDirectory "$PackageName-$PackageVersion-windows.msixbundle"
Invoke-MakeAppx -MakeAppx $makeAppx -Operation 'bundle' -Arguments @(
    'bundle', '/o', '/bv', $PackageVersion, '/d', $bundleInputDirectory, '/p', $bundlePath
)
Invoke-MakeAppx -MakeAppx $makeAppx -Operation 'unbundle validation' -Arguments @(
    'unbundle', '/o', '/p', $bundlePath, '/d', $validationDirectory
)

# -Filter uses 8.3 wildcard matching, where '*.msix' also matches '.msixbundle'.
$bundledPackages = @(
    Get-ChildItem -LiteralPath $validationDirectory -Recurse -File |
        Where-Object { $_.Extension -eq '.msix' }
)
if ($bundledPackages.Count -ne 2) {
    throw "MSIX bundle validation expected 2 architecture packages, found $($bundledPackages.Count)."
}

$hash = (Get-FileHash -LiteralPath $bundlePath -Algorithm SHA256).Hash.ToLowerInvariant()
$hashPath = "$bundlePath.sha256"
[System.IO.File]::WriteAllText($hashPath, "$hash  $([System.IO.Path]::GetFileName($bundlePath))`n", [System.Text.Encoding]::ASCII)

$uploadPath = Join-Path $outputDirectory "$PackageName-$PackageVersion-windows.msixupload"
$uploadZipPath = "$uploadPath.zip"
if (Test-Path -LiteralPath $uploadZipPath) {
    Remove-Item -LiteralPath $uploadZipPath -Force
}
Compress-Archive -LiteralPath $bundlePath -DestinationPath $uploadZipPath -CompressionLevel Optimal
Move-Item -LiteralPath $uploadZipPath -Destination $uploadPath -Force

Add-Type -AssemblyName System.IO.Compression.FileSystem
$uploadArchive = $null
try {
    $uploadArchive = [System.IO.Compression.ZipFile]::OpenRead($uploadPath)
    $uploadEntries = @($uploadArchive.Entries | Where-Object { $_.FullName -eq [System.IO.Path]::GetFileName($bundlePath) })
    if ($uploadEntries.Count -ne 1) {
        throw 'MSIX upload archive validation expected exactly one MSIX bundle.'
    }
}
finally {
    if ($uploadArchive) { $uploadArchive.Dispose() }
}

$uploadHash = (Get-FileHash -LiteralPath $uploadPath -Algorithm SHA256).Hash.ToLowerInvariant()
$uploadHashPath = "$uploadPath.sha256"
[System.IO.File]::WriteAllText($uploadHashPath, "$uploadHash  $([System.IO.Path]::GetFileName($uploadPath))`n", [System.Text.Encoding]::ASCII)

if ($env:GITHUB_OUTPUT) {
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "bundle_path=$bundlePath"
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "bundle_sha256=$hash"
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "upload_path=$uploadPath"
    Add-Content -LiteralPath $env:GITHUB_OUTPUT -Value "upload_sha256=$uploadHash"
}

Write-Host "Created unsigned Store-submission bundle: $bundlePath"
Write-Host "SHA256: $hash"
Write-Host "Created Store upload archive: $uploadPath"
Write-Host "SHA256: $uploadHash"
