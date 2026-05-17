# copy-vc-runtime.ps1
# Copy Microsoft Visual C++ redistributable runtime DLLs (VCRUNTIME140*, MSVCP140*,
# concrt140, vcamp140, vcomp140) into the deployment directory so the produced NSIS
# installer is self-contained on machines without the VC++ redistributable.
#
# Why this exists:
#   windeployqt6.exe --compiler-runtime on Qt 6.x for MSVC only copies the
#   vc_redist.<arch>.exe installer (when present beside Qt's binaries), not the
#   individual DLLs. VCRUNTIME140_1.dll (VS 2019 16.5+, __CxxFrameHandler4) is
#   not part of stock Windows 10 / Server 2019, so without this step LibreCAD
#   fails to start on fresh systems.
#
# Usage:
#   .\copy-vc-runtime.ps1 -Architecture AMD64|ARM64|x86 -Destination <path>

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("AMD64", "ARM64", "x86")]
    [string]$Architecture,

    [Parameter(Mandatory=$true)]
    [string]$Destination
)

$ErrorActionPreference = 'Stop'

switch ($Architecture) {
    "AMD64" { $archDir = "x64" }
    "ARM64" { $archDir = "arm64" }
    "x86"   { $archDir = "x86" }
}

# 1. Locate Visual Studio install via vswhere (shipped under Program Files (x86))
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "[copy-vc-runtime] vswhere.exe not found at $vswhere"
    exit 1
}

$vsInstall = & $vswhere -latest -products * -property installationPath
if (-not $vsInstall -or -not (Test-Path $vsInstall)) {
    Write-Error "[copy-vc-runtime] vswhere returned no Visual Studio installation"
    exit 1
}
Write-Host "[copy-vc-runtime] VS install: $vsInstall"

# 2. Resolve the MSVC tools version (e.g. 14.40.33807) to pick the matching redist
$verFile = Join-Path $vsInstall "VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"
$redistRoot = Join-Path $vsInstall "VC\Redist\MSVC"
$crtDir = $null

if (Test-Path $verFile) {
    $toolsVer = (Get-Content $verFile -Raw).Trim()
    $candidate = Join-Path $redistRoot "$toolsVer\$archDir\Microsoft.VC143.CRT"
    if (Test-Path $candidate) {
        $crtDir = $candidate
        Write-Host "[copy-vc-runtime] Using tools-matched redist: $toolsVer"
    }
}

# 3. Fallback: pick the newest VC143.CRT folder under any redist version
if (-not $crtDir) {
    if (-not (Test-Path $redistRoot)) {
        Write-Error "[copy-vc-runtime] No redist root at $redistRoot"
        exit 1
    }
    $crtDir = Get-ChildItem $redistRoot -Directory |
              Sort-Object Name -Descending |
              ForEach-Object { Join-Path $_.FullName "$archDir\Microsoft.VC143.CRT" } |
              Where-Object { Test-Path $_ } |
              Select-Object -First 1
    if ($crtDir) {
        Write-Host "[copy-vc-runtime] Using fallback redist: $crtDir"
    }
}

if (-not $crtDir) {
    Write-Error "[copy-vc-runtime] No Microsoft.VC143.CRT folder found for $archDir under $redistRoot"
    exit 1
}

# 4. Copy every redistributable DLL into the destination
if (-not (Test-Path $Destination)) {
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
}

$dlls = Get-ChildItem -Path $crtDir -Filter *.dll -File
if ($dlls.Count -eq 0) {
    Write-Error "[copy-vc-runtime] No DLLs found in $crtDir"
    exit 1
}

foreach ($dll in $dlls) {
    Copy-Item -Force -Path $dll.FullName -Destination $Destination
    Write-Host "[copy-vc-runtime] $($dll.Name) -> $Destination"
}

Write-Host "[copy-vc-runtime] Copied $($dlls.Count) DLL(s) from $crtDir"
