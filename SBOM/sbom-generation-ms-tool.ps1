<#
.SYNOPSIS
Generates SPDX 2.2 and CycloneDX 1.6 SBOMs for LibreCAD for ProNest.

.DESCRIPTION
Generates an SPDX SBOM from the staged Windows build using Syft, merges
repository-maintained SPDX manifests from SBOM/AdditionalAspects, runs structural
validation checks on the aggregate document, and converts it to CycloneDX 1.6.
When Syft is not on PATH, the script installs it using winget. When CycloneDX CLI is
not on PATH, the script downloads its official Windows x64 release binary into
SBOM/tools.
#>

[CmdletBinding()]
param(
    [string]$BuildPath,
    [string]$ProductVersion = $env:VERSION_FULL,
    [string]$ProductName = 'LibreCAD-for-ProNest',
    [string]$ProductSupplier = 'Hypertherm'
)

$ErrorActionPreference = 'Stop'

if ($PSVersionTable.PSVersion.Major -lt 7) {
    throw "This script requires PowerShell 7 or later. Current version: $($PSVersionTable.PSVersion)."
}

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($BuildPath)) {
    $BuildPath = Join-Path $repositoryRoot 'windows'
}
elseif (-not [System.IO.Path]::IsPathRooted($BuildPath)) {
    $BuildPath = Join-Path $repositoryRoot $BuildPath
}

if ([string]::IsNullOrWhiteSpace($ProductVersion)) {
    $ProductVersion = (& git -C $repositoryRoot describe --tags --always --dirty 2>$null)
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($ProductVersion)) {
        $versionProject = Get-Content (Join-Path $repositoryRoot 'librecad/src/src.pro')
        $versionLine = $versionProject | Where-Object { $_ -match '^LC_VERSION=' } | Select-Object -First 1
        $ProductVersion = if ($versionLine -match '"([^"]+)"') { $Matches[1] } else { '0.0.0-unknown' }
    }
}
$ProductVersion = $ProductVersion.Trim()

$reportsPath = Join-Path $PSScriptRoot 'reports'
$primaryManifestPath = Join-Path $reportsPath '_manifest/spdx_2.2/manifest.spdx.json'
$aggregateSpdxPath = Join-Path $reportsPath 'aggregate-output/spdx_2.2'
$aggregateManifestPath = Join-Path $aggregateSpdxPath 'manifest.spdx.json'
$additionalAspectsPath = Join-Path $PSScriptRoot 'AdditionalAspects'
$validationOutputPath = Join-Path $reportsPath 'aggregate-validation.json'
$toolCachePath = Join-Path $PSScriptRoot 'tools'

function Resolve-SbomToolCommand {
    param(
        [Parameter(Mandatory = $true)][string[]]$CommandNames,
        [Parameter(Mandatory = $true)][string]$DownloadUrl,
        [Parameter(Mandatory = $true)][string]$FileName
    )

    foreach ($commandName in $CommandNames) {
        $command = Get-Command $commandName -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -ne $command) {
            return $command.Source
        }
    }

    New-Item $toolCachePath -ItemType Directory -Force | Out-Null
    $toolPath = Join-Path $toolCachePath $FileName
    if (-not (Test-Path $toolPath -PathType Leaf)) {
        Write-Host "Downloading $FileName from its official release." -ForegroundColor Yellow
        Invoke-WebRequest -Uri $DownloadUrl -OutFile $toolPath
    }

    if (-not (Test-Path $toolPath -PathType Leaf)) {
        throw "Unable to download '$FileName' from '$DownloadUrl'."
    }

    return $toolPath
}

function Resolve-SyftCommand {
    $syftCommand = Get-Command syft -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $syftCommand) {
        return $syftCommand.Source
    }

    $wingetCommand = Get-Command winget -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $wingetCommand) {
        throw "Syft is not installed and winget was not found. Install Syft from https://github.com/anchore/syft."
    }

    Write-Host 'Installing Syft using winget package Anchore.Syft.' -ForegroundColor Yellow
    & $wingetCommand.Source install --id Anchore.Syft --exact --accept-package-agreements --accept-source-agreements --disable-interactivity
    if ($LASTEXITCODE -ne 0) {
        throw "winget failed to install Syft (exit code $LASTEXITCODE)."
    }

    $candidatePaths = @(
        (Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Links\syft.exe'),
        (Join-Path $env:ProgramFiles 'syft\syft.exe'),
        (Join-Path $env:ProgramFiles 'Anchore\Syft\syft.exe')
    )
    foreach ($candidatePath in $candidatePaths) {
        if (-not [string]::IsNullOrWhiteSpace($candidatePath) -and (Test-Path $candidatePath -PathType Leaf)) {
            return $candidatePath
        }
    }

    $syftCommand = Get-Command syft -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -eq $syftCommand) {
        throw 'Syft installation completed, but syft.exe was not found on PATH in this session.'
    }
    return $syftCommand.Source
}

function Invoke-SyftGenerate {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $attempts = @(
        @('scan', $SourcePath, '-o', "spdx-json=$OutputPath"),
        @($SourcePath, '-o', "spdx-json=$OutputPath")
    )

    $lastExitCode = 1
    foreach ($arguments in $attempts) {
        & $Command @arguments
        $lastExitCode = $LASTEXITCODE
        if ($lastExitCode -eq 0 -and (Test-Path $OutputPath -PathType Leaf)) {
            return
        }
    }

    throw "Syft failed to generate an SPDX manifest (exit code $lastExitCode)."
}

function Invoke-SbomCommand {
    param(
        [Parameter(Mandatory = $true)][string]$Command,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Command failed with exit code $LASTEXITCODE."
    }
}

function Get-SbomManifest {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path $Path -PathType Leaf)) {
        throw "SBOM manifest '$Path' does not exist."
    }
    return Get-Content $Path -Raw | ConvertFrom-Json -AsHashtable
}

function Copy-SbomObject {
    param([Parameter(Mandatory = $true)][hashtable]$Value)
    return $Value | ConvertTo-Json -Depth 100 | ConvertFrom-Json -AsHashtable
}

function Initialize-SbomCollections {
    param([Parameter(Mandatory = $true)][hashtable]$Manifest)

    foreach ($name in @('packages', 'files', 'relationships')) {
        if (-not $Manifest.ContainsKey($name)) {
            $Manifest[$name] = @()
        }
    }
}

function Add-SbomRelationship {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Manifest,
        [Parameter(Mandatory = $true)][hashtable]$Relationship,
        [Parameter(Mandatory = $true)][System.Collections.Generic.HashSet[string]]$Keys
    )

    $key = '{0}|{1}|{2}' -f $Relationship.spdxElementId, $Relationship.relationshipType, $Relationship.relatedSpdxElement
    if ($Keys.Add($key)) {
        $Manifest.relationships += $Relationship
    }
}

function Merge-AdditionalAspects {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Manifest,
        [Parameter(Mandatory = $true)][string]$RootPackageId
    )

    $existingIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($element in @($Manifest.packages) + @($Manifest.files)) {
        if (-not [string]::IsNullOrWhiteSpace($element.SPDXID)) {
            [void]$existingIds.Add($element.SPDXID)
        }
    }

    $relationshipKeys = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($relationship in @($Manifest.relationships)) {
        [void]$relationshipKeys.Add(('{0}|{1}|{2}' -f $relationship.spdxElementId, $relationship.relationshipType, $relationship.relatedSpdxElement))
    }

    if (-not (Test-Path $additionalAspectsPath -PathType Container)) {
        return @()
    }

    $aspectFiles = @(Get-ChildItem $additionalAspectsPath -Filter 'manifest.spdx.json' -Recurse -File |
        Where-Object { $_.FullName -like "*\_manifest\spdx_2.2\manifest.spdx.json" } |
        Sort-Object FullName)

    foreach ($aspectFile in $aspectFiles) {
        $aspect = Get-SbomManifest $aspectFile.FullName
        Initialize-SbomCollections $aspect
        $aspectName = $aspectFile.Directory.Parent.Parent.Name
        $prefix = [Regex]::Replace($aspectName, '[^A-Za-z0-9.-]', '-')
        $idMap = @{}

        foreach ($element in @($aspect.packages) + @($aspect.files)) {
            if ([string]::IsNullOrWhiteSpace($element.SPDXID)) {
                continue
            }
            $suffix = $element.SPDXID -replace '^SPDXRef-', ''
            $candidate = "SPDXRef-$prefix-$suffix"
            $baseCandidate = $candidate
            $index = 1
            while ($existingIds.Contains($candidate)) {
                $candidate = "$baseCandidate-$index"
                $index++
            }
            $idMap[$element.SPDXID] = $candidate
            [void]$existingIds.Add($candidate)
        }

        foreach ($package in @($aspect.packages)) {
            $copy = Copy-SbomObject $package
            $copy.SPDXID = $idMap[$package.SPDXID]
            if ($copy.ContainsKey('hasFiles')) {
                $copy.hasFiles = @($copy.hasFiles | ForEach-Object { if ($idMap.ContainsKey($_)) { $idMap[$_] } else { $_ } })
            }
            $Manifest.packages += $copy
        }
        foreach ($file in @($aspect.files)) {
            $copy = Copy-SbomObject $file
            $copy.SPDXID = $idMap[$file.SPDXID]
            $Manifest.files += $copy
        }

        $aspectRoots = @()
        foreach ($relationship in @($aspect.relationships)) {
            $copy = Copy-SbomObject $relationship
            if ($idMap.ContainsKey($copy.spdxElementId)) { $copy.spdxElementId = $idMap[$copy.spdxElementId] }
            if ($idMap.ContainsKey($copy.relatedSpdxElement)) { $copy.relatedSpdxElement = $idMap[$copy.relatedSpdxElement] }
            if ($relationship.spdxElementId -eq 'SPDXRef-DOCUMENT' -and $relationship.relationshipType -eq 'DESCRIBES') {
                $aspectRoots += $copy.relatedSpdxElement
            }
            else {
                Add-SbomRelationship $Manifest $copy $relationshipKeys
            }
        }
        foreach ($describedId in @($aspect.documentDescribes)) {
            if ($idMap.ContainsKey($describedId)) { $aspectRoots += $idMap[$describedId] }
        }
        foreach ($aspectRoot in @($aspectRoots | Select-Object -Unique)) {
            Add-SbomRelationship $Manifest @{
                spdxElementId = $RootPackageId
                relationshipType = 'DEPENDS_ON'
                relatedSpdxElement = $aspectRoot
            } $relationshipKeys
        }
    }

    return $aspectFiles
}

function Add-CycloneDxCpeEnrichment {
    param(
        [Parameter(Mandatory = $true)][string]$CycloneDxPath,
        [System.IO.FileInfo[]]$AspectFiles
    )

    if (@($AspectFiles).Count -eq 0) {
        return
    }

    $document = Get-Content $CycloneDxPath -Raw | ConvertFrom-Json -AsHashtable
    foreach ($aspectFile in $AspectFiles) {
        $aspect = Get-SbomManifest $aspectFile.FullName
        foreach ($package in @($aspect.packages)) {
            $cpe = @($package.externalRefs | Where-Object {
                $_.referenceCategory -eq 'SECURITY' -and $_.referenceType -eq 'cpe23Type'
            } | Select-Object -First 1)
            if ($cpe.Count -eq 0) { continue }

            $components = @($document.components | Where-Object {
                $_.name -eq $package.name -and $_.version -eq $package.versionInfo
            })
            if ($components.Count -ne 1) {
                throw "Expected one CycloneDX component for '$($package.name)' $($package.versionInfo), found $($components.Count)."
            }
            $components[0].cpe = $cpe[0].referenceLocator
        }
    }
    $document | ConvertTo-Json -Depth 100 | Set-Content $CycloneDxPath -Encoding utf8
}

function Set-SbomProductMetadata {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Manifest,
        [Parameter(Mandatory = $true)][string]$ProductName,
        [Parameter(Mandatory = $true)][string]$ProductVersion,
        [Parameter(Mandatory = $true)][string]$ProductSupplier
    )

    $rootPackage = $null
    $rootPackageId = $null

    if ($Manifest.ContainsKey('documentDescribes') -and @($Manifest.documentDescribes).Count -gt 0) {
        $rootPackageId = @($Manifest.documentDescribes)[0]
        $rootPackage = @($Manifest.packages | Where-Object { $_.SPDXID -eq $rootPackageId } | Select-Object -First 1)
    }

    if ($null -eq $rootPackage -and @($Manifest.packages).Count -gt 0) {
        $rootPackage = @($Manifest.packages)[0]
        $rootPackageId = $rootPackage.SPDXID
        if ([string]::IsNullOrWhiteSpace($rootPackageId)) {
            $rootPackageId = 'SPDXRef-RootPackage'
            $rootPackage.SPDXID = $rootPackageId
        }
        $Manifest.documentDescribes = @($rootPackageId)
    }

    if ($null -ne $rootPackage) {
        $rootPackage.name = $ProductName
        $rootPackage.versionInfo = $ProductVersion
        $rootPackage.supplier = "Organization: $ProductSupplier"
    }

    $Manifest.name = "$ProductName-$ProductVersion"
    return $rootPackageId
}

function Validate-AggregateSpdxManifest {
    param(
        [Parameter(Mandatory = $true)][hashtable]$Manifest,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $errors = [System.Collections.Generic.List[hashtable]]::new()

    if (-not $Manifest.ContainsKey('SPDXID') -or [string]::IsNullOrWhiteSpace($Manifest.SPDXID)) {
        $errors.Add(@{ code = 'MissingDocumentSpdxId'; message = 'Document SPDXID is missing.' })
    }
    if (-not $Manifest.ContainsKey('spdxVersion') -or [string]::IsNullOrWhiteSpace($Manifest.spdxVersion)) {
        $errors.Add(@{ code = 'MissingSpdxVersion'; message = 'spdxVersion is missing.' })
    }
    if (-not $Manifest.ContainsKey('documentDescribes') -or @($Manifest.documentDescribes).Count -eq 0) {
        $errors.Add(@{ code = 'MissingDocumentDescribes'; message = 'documentDescribes must contain at least one package SPDXID.' })
    }
    if (@($Manifest.packages).Count -eq 0) {
        $errors.Add(@{ code = 'MissingPackages'; message = 'At least one package is required.' })
    }

    $allIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($element in @($Manifest.packages) + @($Manifest.files)) {
        if (-not [string]::IsNullOrWhiteSpace($element.SPDXID)) {
            if (-not $allIds.Add($element.SPDXID)) {
                $errors.Add(@{ code = 'DuplicateSpdxId'; message = "Duplicate SPDXID '$($element.SPDXID)'." })
            }
        }
    }

    foreach ($describedId in @($Manifest.documentDescribes)) {
        if ([string]::IsNullOrWhiteSpace($describedId) -or -not $allIds.Contains($describedId)) {
            $errors.Add(@{ code = 'UnknownDocumentDescribes'; message = "documentDescribes references unknown SPDXID '$describedId'." })
        }
    }

    foreach ($relationship in @($Manifest.relationships)) {
        if ([string]::IsNullOrWhiteSpace($relationship.spdxElementId) -or
            [string]::IsNullOrWhiteSpace($relationship.relationshipType) -or
            [string]::IsNullOrWhiteSpace($relationship.relatedSpdxElement)) {
            $errors.Add(@{ code = 'InvalidRelationship'; message = 'Relationship is missing required fields.' })
            continue
        }

        if ($relationship.spdxElementId -ne 'SPDXRef-DOCUMENT' -and -not $allIds.Contains($relationship.spdxElementId)) {
            $errors.Add(@{ code = 'UnknownRelationshipSource'; message = "Relationship source '$($relationship.spdxElementId)' was not found." })
        }
        if ($relationship.relatedSpdxElement -ne 'SPDXRef-DOCUMENT' -and -not $allIds.Contains($relationship.relatedSpdxElement)) {
            $errors.Add(@{ code = 'UnknownRelationshipTarget'; message = "Relationship target '$($relationship.relatedSpdxElement)' was not found." })
        }
    }

    $result = if ($errors.Count -eq 0) { 'Success' } else { 'Failed' }
    $report = @{
        Result = $result
        ValidationErrors = @($errors)
        ValidationErrorCount = $errors.Count
    }
    $report | ConvertTo-Json -Depth 100 | Set-Content $OutputPath -Encoding utf8

    if ($errors.Count -gt 0) {
        throw "SBOM validation failed with $($errors.Count) validation error(s). See '$OutputPath'."
    }
}

try {
    $syftCommand = Resolve-SyftCommand
    $cycloneDxCommand = Resolve-SbomToolCommand -CommandNames @('cyclonedx') `
        -DownloadUrl 'https://github.com/CycloneDX/cyclonedx-cli/releases/latest/download/cyclonedx-win-x64.exe' `
        -FileName 'cyclonedx-win-x64.exe'

    if (-not (Test-Path $BuildPath -PathType Container)) {
        throw "Build path '$BuildPath' does not exist. Build LibreCAD before generating its SBOM."
    }
    if (-not (Get-ChildItem $BuildPath -File -Recurse | Select-Object -First 1)) {
        throw "Build path '$BuildPath' contains no files."
    }

    if (Test-Path $reportsPath) {
        Remove-Item $reportsPath -Recurse -Force
    }
    New-Item $reportsPath -ItemType Directory -Force | Out-Null
    New-Item (Split-Path $primaryManifestPath -Parent) -ItemType Directory -Force | Out-Null

    Invoke-SyftGenerate -Command $syftCommand -SourcePath $BuildPath -OutputPath $primaryManifestPath
    $generatedManifest = Get-SbomManifest $primaryManifestPath
    if (-not $generatedManifest.ContainsKey('SPDXID') -or -not $generatedManifest.ContainsKey('packages')) {
        throw 'Syft created an incomplete SPDX manifest.'
    }

    $aggregate = $generatedManifest
    Initialize-SbomCollections $aggregate
    $rootPackageId = Set-SbomProductMetadata -Manifest $aggregate -ProductName $ProductName -ProductVersion $ProductVersion -ProductSupplier $ProductSupplier
    if ([string]::IsNullOrWhiteSpace($rootPackageId)) {
        $rootPackageId = if (@($aggregate.documentDescribes).Count -gt 0) { @($aggregate.documentDescribes)[0] } else { 'SPDXRef-RootPackage' }
    }
    $aspectFiles = @(Merge-AdditionalAspects $aggregate $rootPackageId)

    Validate-AggregateSpdxManifest -Manifest $aggregate -OutputPath $validationOutputPath

    New-Item $aggregateSpdxPath -ItemType Directory -Force | Out-Null
    $aggregate | ConvertTo-Json -Depth 100 | Set-Content $aggregateManifestPath -Encoding utf8

    $safeName = $ProductName -replace '[^A-Za-z0-9._-]', '-'
    $safeVersion = $ProductVersion -replace '[^A-Za-z0-9._-]', '-'
    $spdxOutput = Join-Path $aggregateSpdxPath "${safeName}_v${safeVersion}.spdx.json"
    Move-Item $aggregateManifestPath $spdxOutput -Force

    $cycloneDxOutput = Join-Path $aggregateSpdxPath "${safeName}_v${safeVersion}.cdx.json"
    Invoke-SbomCommand $cycloneDxCommand @(
        'convert', '--input-file', $spdxOutput, '--input-format', 'spdxjson',
        '--output-file', $cycloneDxOutput, '--output-format', 'json', '--output-version', 'v1_6'
    )
    Add-CycloneDxCpeEnrichment $cycloneDxOutput $aspectFiles

    Write-Host "SBOM generation completed for $ProductName $ProductVersion." -ForegroundColor Green
    Write-Host "SPDX: $spdxOutput"
    Write-Host "CycloneDX: $cycloneDxOutput"
}
catch {
    Write-Error "SBOM generation failed: $($_.Exception.Message)"
    exit 1
}
