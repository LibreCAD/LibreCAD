# LibreCAD Windows Build Script

## Overview

`build-windows.bat` is an enhanced build script for LibreCAD on Windows that provides:

1. **Automatic dependency detection and installation via Chocolatey**
2. **Complete local builds with resource synchronization**
3. **Optional NSIS installer packaging**

The script creates a `windows/` directory that mirrors the final installed application structure, making it suitable for both local testing and distribution.

## Features

### 1. Zero-Dependency Setup with Auto-Installation

The script provides a complete zero-dependency build experience:

**Automatic Chocolatey Installation:**
- If Chocolatey is not found, the script automatically installs it
- Requires Administrator privileges (will prompt UAC if needed)
- Uses multiple installation methods for maximum reliability:
  1. PowerShell direct execution (primary method)
  2. BITS download fallback
  3. CertUtil direct download fallback
- Verifies installation and displays version

**Automatic Dependency Installation via Chocolatey:**
Once Chocolatey is installed, the script auto-installs:
- ✅ **CMake** - Required build system (project specification)
- ✅ **Qt 6.x** - Complete Qt development environment
- ✅ **Boost C++ Libraries** - Required C++ libraries
- ✅ **C++ Compiler** - Visual Studio Build Tools 2022
- ✅ **NSIS** - Windows installer packaging tool

This enables true **one-command builds** on fresh Windows installations!

### 2. Resource File Synchronization

After building, the script copies all necessary resources to match the NSIS package structure:

```
windows/
├── LibreCAD.exe
├── platforms/          (Qt platform plugins)
├── imageformats/       (Qt image format plugins)
├── styles/            (Qt style plugins)
└── resources/
    ├── qm/            (Translation files)
    │   ├── qt_*.qm
    │   ├── qtbase_*.qm
    │   ├── librecad_*.qm
    │   └── plugins_*.qm
    ├── fonts/         (LFF font files)
    │   └── *.lff
    ├── patterns/      (Hatch patterns)
    │   └── *.dxf
    └── library/       (Library parts)
        └── *.dxf (with subfolder structure)
```

This ensures the portable build works identically to the installed version.

### 3. Version Extraction

Automatically extracts version information from:
1. Git repository (`git describe --always`)
2. Source code (`librecad/src/src.pro` LC_VERSION)
3. Fallback default: `2.2.2-alpha`

## Usage

### Basic Build

```batch
cd scripts
build-windows.bat
```

This will:
1. Check dependencies
2. Build LibreCAD
3. Deploy Qt dependencies
4. Copy resource files
5. Create NSIS installer

### Skip Clean Build

To skip the clean step (faster rebuilds):

```batch
build-windows.bat NoClean
```

### Skip NSIS Installer

For local testing without creating an installer:

```batch
set LC_SKIP_NSIS=1
build-windows.bat
```

Output: `windows\LibreCAD.exe` (portable version)

### Force Architecture

Override automatic architecture detection:

```batch
set LC_ARCH=AMD64
build-windows.bat

rem or for ARM64
set LC_ARCH=ARM64
build-windows.bat
```

### Custom NSIS Script

Use a different NSIS script:

```batch
set LC_NSIS_FILE=custom-installer.nsi
build-windows.bat
```

## Dependencies

### Required (Auto-installed via Chocolatey if missing)

- **CMake** (>= 3.16) - **Required build system per project specification**
  - Auto-installed as: `choco install cmake -y --installargs 'ADD_CMAKE_TO_PATH=System'`
  - Note: LibreCAD must use CMake, not QMake or direct MSBuild
  
- **Qt 6.x** with MSVC or MinGW support
  - Modules: qt5compat, qtimageformats, qtshadertools (recommended)
  - Tools: qmake, windeployqt6, lrelease
  - Auto-installed as: `choco install qt6`
  
- **Boost C++ Libraries** (>= 1.55.0)
  - Auto-installed as: `choco install boost-msvc-14.3`
  
- **C++ Compiler**
  - MSVC 2022 Build Tools (recommended), OR
  - MinGW-w64
  - Auto-installed as: `choco install visualstudio2022buildtools`

### Optional

- **Git** - For automatic version extraction
- **NSIS 3.x** - For creating Windows installer
  - Auto-installed as: `choco install nsis`
  - Download from: https://nsis.sourceforge.io/Download

### Installing Chocolatey

**The build script now auto-installs Chocolatey if missing!** Simply run the script as Administrator.

If you prefer manual installation or encounter issues:

```
# Run in PowerShell as Administrator
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

Or visit: https://chocolatey.org/install

**Note:** The automatic installation requires:
- Administrator privileges (UAC prompt will appear)
- Internet connection
- PowerShell execution policy allowing scripts

## Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `Qt6_DIR` | Qt installation path | `C:\Qt\6.9.0\msvc2022_64` |
| `LC_ARCH` | Force architecture | `AMD64` or `ARM64` |
| `LC_SKIP_NSIS` | Skip installer creation | `1` to skip |
| `LC_NSIS_FILE` | Custom NSIS script | `custom.nsi` |
| `TRANSLATIONS_DIR` | Qt translations path | Auto-detected from Qt6_DIR |

## Build Process

The script executes these steps in order:

### Step 1: Dependency Check
- Verifies Qt, compiler, and tools are available
- Auto-detects Qt installation if not in PATH
- Reports missing dependencies with helpful messages

### Step 2: Project Build
- Runs `qmake` with appropriate spec for detected compiler
- Executes `nmake` (MSVC) or `mingw32-make` (MinGW)
- Verifies `LibreCAD.exe` was created

### Step 3: Qt Deployment
- Runs `windeployqt6.exe` to copy Qt DLLs and plugins
- Creates complete standalone application directory

### Step 4: Resource Synchronization
- Compiles translation files (.ts → .qm)
- Copies Qt translations to `resources/qm/`
- Copies LibreCAD translations to `resources/qm/`
- Copies LFF fonts to `resources/fonts/`
- Copies hatch patterns to `resources/patterns/`
- Copies library parts to `resources/library/` (preserves structure)

### Step 5: Version Extraction & NSIS Packaging
- Extracts version from Git or source
- Generates `VIProductVersion` (X.X.X.X format)
- Calls NSIS compiler to create installer
- Output: `generated\LibreCAD-{version}-Windows-{arch}.exe`

## Troubleshooting

### "Administrator privileges required to install Chocolatey"

**Solution**: Run the script as Administrator:
1. Right-click Command Prompt or PowerShell
2. Select "Run as administrator"
3. Navigate to scripts directory and run `build-windows.bat`

Or manually install Chocolatey first (see Dependencies section).

### "Chocolatey installation failed"

This can happen due to:
- **No internet connection**: Ensure you have network access
- **Firewall blocking**: Allow PowerShell/BITS through firewall
- **Execution policy restrictions**: The script uses `-ExecutionPolicy Bypass` to override this

**Manual installation:**
```powershell
# Open PowerShell as Administrator
Set-ExecutionPolicy Bypass -Scope Process -Force
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

Then re-run `build-windows.bat`.

### "Chocolatey not found" and auto-install skipped

If running without admin privileges, the script cannot auto-install Chocolatey.

**Solutions:**
1. **Recommended**: Re-run as Administrator
2. Install Chocolatey manually (see above)
3. Manually install all dependencies and set environment variables

### "CMake not found"

**This is a critical error** - CMake is required by project specification (QMake and direct MSBuild are not allowed).

**Solution 1**: The script will auto-install CMake via Chocolatey if available.

**Solution 2**: Install manually:
```batch
choco install cmake -y --installargs 'ADD_CMAKE_TO_PATH=System'
```

**Solution 3**: Download from https://cmake.org/download/ and ensure it's in PATH.

**Note**: During installation, make sure to select "Add CMake to system PATH" option.

### "Qt not found" and Chocolatey unavailable

**Solution 1**: Set Qt6_DIR environment variable
```batch
set Qt6_DIR=C:\Qt\6.9.0\msvc2022_64
set PATH=%Qt6_DIR%\bin;%PATH%
```

**Solution 2**: Add Qt to system PATH permanently
```batch
setx PATH "%PATH%;C:\Qt\6.9.0\msvc2022_64\bin"
```

**Solution 3**: Install Chocolatey and re-run the script (recommended)

### "Boost not found"

The script will automatically install Boost via Chocolatey if available. Otherwise:

**Solution 1**: Set BOOST_ROOT environment variable
```batch
set BOOST_ROOT=C:\local\boost_1_87_0
```

**Solution 2**: Install via Chocolatey manually
```batch
choco install boost-msvc-14.3 -y
```

**Solution 3**: Download from https://www.boost.org/ and extract to C:\local\

### "No C++ compiler found"

**For MSVC**: The script can auto-install Visual Studio Build Tools via Chocolatey. Or manually:
1. Download from: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
2. Select "Desktop development with C++" workload
3. After installation, open "Developer Command Prompt for VS 2022"

**For MinGW**: Ensure MinGW bin directory is in PATH:
```batch
set PATH=C:\msys64\mingw64\bin;%PATH%
```

### "NSIS packaging failed"

The script will attempt to auto-install NSIS via Chocolatey. If it fails:

**Solution 1**: Install NSIS manually
```batch
choco install nsis -y
```

**Solution 2**: Skip installer creation
```batch
set LC_SKIP_NSIS=1
build-windows.bat
```

**Solution 3**: Download from https://nsis.sourceforge.io/Download

### Translation files not copying

Ensure translation files are compiled first:
```batch
cd librecad\ts
lrelease *.ts
cd ..\..\plugins\ts
lrelease *.ts
```

Or the build script will compile them automatically.

### Qt installation via Chocolatey is slow

Qt installation is large (~2-3GB). To speed up future builds:
1. Let Chocolatey install Qt once
2. Subsequent builds will detect the installed Qt
3. Consider setting `Qt6_DIR` environment variable for faster detection

## Comparison with CI Build

| Feature | Local Build (`build-windows.bat`) | CI Build (GitHub Actions) |
|---------|-----------------------------------|---------------------------|
| Dependency check | ✅ Automatic | ✅ Pre-installed |
| Resource sync | ✅ Full sync | ✅ Full sync |
| NSIS packaging | ✅ Optional | ✅ Always |
| Version extraction | ✅ Git/pro fallback | ✅ Git only |
| Speed | Faster (cached deps) | Slower (fresh env) |
| Use case | Development/testing | Release/distribution |

## Tips

1. **Run as Administrator** (recommended): Enables full auto-installation of Chocolatey and all dependencies for a true zero-setup experience
2. **First-time setup on fresh Windows**:
   ```batch
   # Right-click Command Prompt → Run as Administrator
   cd scripts
   build-windows.bat
   # Script will: Install Chocolatey → Install Qt/Boost/Compiler/NSIS → Build LibreCAD
   ```
3. **Faster rebuilds**: Use `NoClean` parameter to skip cleaning
4. **Testing only**: Set `LC_SKIP_NSIS=1` to skip installer creation
5. **Custom resources**: Modify resource paths in the script if your project structure differs
6. **Debug builds**: Change `CONFIG+=release` to `CONFIG+=debug` in the qmake command
7. **Parallel builds**: For MinGW, adjust `-j4` to match your CPU cores

## Automated Installation Flow

Complete zero-dependency installation flow on fresh Windows:

```
Run build-windows.bat (as Administrator)
    ↓
Chocolatey not found?
    ↓ Yes
Auto-install Chocolatey via PowerShell
    ↓ Multiple fallback methods
    ↓ Verify installation
    ↓
┌──────────────────────┐
│ CMake missing?       │──Yes──→ choco install cmake
│ (Required by spec)   │         Add to system PATH
└─────────┬────────────┘
          ↓
┌──────────────────────┐
│ Qt 6.x missing?      │──Yes──→ choco install qt6
│                      │         Verify & Set Qt6_DIR
└─────────┬────────────┘
          ↓
┌──────────────────────┐
│ Boost missing?       │──Yes──→ choco install boost-msvc-14.3
│                      │         Verify & Set BOOST_ROOT
└─────────┬────────────┘
          ↓
┌──────────────────────┐
│ Compiler missing?    │──Yes──→ choco install visualstudio2022buildtools
│   (MSVC/MinGW)       │         (~8GB, may need restart)
└─────────┬────────────┘
          ↓
┌──────────────────────┐
│ NSIS missing?        │──Yes──→ choco install nsis
│   (Optional)         │         Continue or skip
└─────────┬────────────┘
          ↓
All dependencies satisfied
          ↓
Build LibreCAD → Create Installer
```

**Total time on fresh Windows:** ~45-60 minutes (mostly VS Build Tools download)  
**Subsequent builds:** ~5-10 minutes (dependencies cached)

**Note**: Visual Studio Build Tools installation (~8GB) is the longest step and may require a command prompt restart.

## See Also

- `build-nsis.bat` - Standalone NSIS packaging script
- `set-windows-env.bat` - Environment setup helper
- `generate-custom-nsh.ps1` - NSIS configuration generator
- `.github/workflows/build-all.yml` - CI build workflow
