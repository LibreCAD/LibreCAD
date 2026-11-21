<!--
 ********************************************************************************
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org
 Copyright (C) 2026 Dongxu Li (github.com/dxli)

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 USA.
 ********************************************************************************
-->

# Microsoft Store MSIX (2.2.1)

The 2.2.1 branch builds the **stable** LibreCAD package for the Microsoft
Store, as master builds the LibreCAD-beta package; the two are separate Store
products with their own identities, so Windows can install both side by side.
This page is master's `docs/development/msix-store.md` for what 2.2.1 does
differently.

## What CI builds

`build-all.yml` packages an MSIX in its x64 Windows job (`BuildWindows64`)
after the NSIS installer has been built and uploaded:

1. It copies the Visual C++ runtime DLLs into `windows\`, the folder the
   installer was built from (`scripts/copy-vc-runtime.ps1`). The installer has
   been made by then, so it stays as it was built.
2. `scripts/build-msix.ps1` packages that folder and uploads the package as
   the `win64Msix` artifact (2 days).

The `PackageWindowsMsix` job then:

1. bundles it into a `.msixbundle`, wrapped in a `.msixupload` archive
   (`scripts/build-msix-bundle.ps1`); the `winMsixBundle` artifact (30 days)
   holds the `.msixupload`;
2. checks the package as described under Certification testing below.

The bundle holds the x64 package only: 2.2.1 has no arm64 build, and the
32-bit build is not packaged. It is a bundle all the same, because a Store
product that has shipped a bundle must keep shipping one, which leaves room
for more architectures later.

These artifacts are unsigned. **An unsigned MSIX cannot be installed**, so they
are for Store submission and validation only, never direct-download
installers, and they are not attached to any release. Microsoft signs a
package after it passes Store certification.

Every package is built with the package name `LibreCAD`, the identity in the
repository variable `MICROSOFT_STORE_IDENTITY_NAME` and the publisher in
`MICROSOFT_STORE_PUBLISHER` and `MICROSOFT_STORE_PUBLISHER_DISPLAY_NAME`. The
publisher is the same as for LibreCAD-beta: it belongs to the Partner Center
account. Without these variables a build uses the placeholder identity
`LibreCAD.Development` and publisher `CN=LibreCAD Development`, so CI still
exercises the packaging; Partner Center refuses such a package.

Submitting a build to the Store is not wired up on 2.2.1 yet.

## What the package contains

`scripts/build-msix.ps1` is master's script adapted to the 2.2.1 build; each
change is marked `2.2.1:` in it. It packages the staged folder with these
changes:

- **The program.** The packaged `LibreCAD.exe` is a Windows GUI program (PE
  subsystem 2); a console build gets its copy in the package marked as a GUI
  program. It is the only program in the package.
- **Left out:** linker files (`.lib`, `.exp`, `.ilk`), debug symbols, the
  app-local Universal CRT, `vc_redist.x64.exe`, the software OpenGL renderer
  (`opengl32sw.dll`), Qt 5's ANGLE (`libEGL.dll`, `libGLESv2.dll`) and the
  Direct3D shader compiler (`d3dcompiler_47.dll`). LibreCAD paints with
  QPainter, and Qt loads these only for an OpenGL context, which 2.2.1 never
  creates.
- **Plugins.** LibreCAD's plugins are already in `windows\resources\plugins`,
  where their `.pro` files build them; the package must contain at least one.
- **Resources.** Fonts, hatch patterns and the block library come from
  `librecad/support` and go under `resources\`, as the installer puts them.
- **Translations.** `scripts\build-windows.bat` runs `lrelease`, which writes
  LibreCAD's and its plugins' `.qm` next to the `.ts` files (`librecad\ts`,
  `plugins\ts`). They go under `resources\qm`, with Qt's own from
  `windows\translations`.
- **Licences.** `LICENSE` and the files of `licenses/` go under `licenses\`:
  - the GPLv2, the GPLv3 of WenQuanYi Micro Hei and the licences of the other
    bundled fonts;
  - `qt-third-party.txt`, the copyright and licence text of each third-party
    component inside the Qt 5.15 DLLs and plugins.
- **`qt-third-party.txt` is for one Qt release.** When `build-all.yml`
  installs another, `build-msix.ps1` warns, and
  `python3 scripts/msix/update-qt-third-party.py --tag v<version>` regenerates
  the file from Qt's attribution data.
- **`licenses\SOURCE.txt`** names what the package was built from:
  - the commit;
  - the Qt version, read from `Qt5Core.dll`;
  - the Boost version compiled into `LibreCAD.exe`, read from
    `..\boost\boost\version.hpp`, the folder the qmake build uses.

  It links to the source of all three and carries the project's written
  offer of that source for three years, as GPLv2 section 3(b) asks.
- **Tiles and icons** are scaled from `scripts/msix/librecad-logo.png`, as on
  master.

## CI behavior

- **Pushes and tags of `2.2.1` package and bundle the MSIX.** A packaging
  failure there does not stop the installers from being published: the MSIX
  steps continue on error, nothing is uploaded, and `PackageWindowsMsix` fails
  instead.
- **The packaging and upload steps stop after 10 minutes**, so a hang counts as
  an MSIX failure. MakeAppx writes its file list to `msix-output\makeappx-*.log`,
  which the job prints when packaging fails or times out.
- **A pull request into `2.2.1` that changes a packaging file** starts
  `build-all.yml`: `build-all.yml`, `scripts/build-msix*.ps1`, `scripts/msix/`,
  `scripts/copy-vc-runtime.ps1`, `scripts/build-windows.bat`,
  `scripts/postprocess-windows/nsis-msvc.nsi`, `licenses/` or `LICENSE`.
  - It builds the two Windows jobs and `PackageWindowsMsix` and skips the
    others.
  - A packaging failure fails the pull request.
  - Nothing is published: `UpdateAssets` and `ReleaseAssets` run on pushes only.

## Package version

The package version is `2.2.<GitHub run number>.0`, with 2.2 read from
`LC_VERSION` in `librecad/src/src.pro`. About still shows `2.2.1.N`; Windows'
list of installed apps shows the package version.

The Microsoft Store reserves the fourth component and rejects any package whose
revision is not 0, so the increasing build counter goes in the third
component, as on master. The run number counts the runs of `build-all.yml` on
every branch, master's included, so it keeps increasing across 2.2.1 pushes
and tags as well. Both the workflow and `build-msix.ps1` enforce the revision
rule.

## Certification testing

`PackageWindowsMsix` runs the checks master's does:

- **Registration.** It registers the unpacked x64 layout with
  `Add-AppxPackage -Register`, which exercises the manifest, identity and entry
  point.
- **A packaged run.** It runs the packaged `LibreCAD.exe` once, converting
  `resources\library\misc\screw.dxf` to PNG with `QT_FORCE_STDERR_LOGGING`
  set. It fails unless that exits with 0, writes the PNG and leaves Qt's
  messages in the redirected stderr. This catches a missing Qt DLL.
- **App Certification Kit.** It runs the kit against the bundle when the
  runner has it, warns for each failed test, and uploads the report as
  `winMsixWackReport`.

`build-msix.ps1` also unpacks the package it made and checks the manifest
and the required files again; `build-msix-bundle.ps1` unbundles the bundle and
checks that it holds the package given.

## Registry, settings and the NSIS installer

As on master:
- **File associations** are declared in `AppxManifest.xml`, with the installer's
  ProgIDs `LibreCAD.DXF` and `LibreCAD.DWG` as migration sources.
- **Settings:** a packaged app's writes to `HKCU\Software` and new files under
  `AppData` go to a private per-package store that Windows deletes when the
  package is uninstalled. Reads see the settings of an existing NSIS
  installation.

## Local package creation

On a Windows machine with the Windows SDK:
1. Build LibreCAD as `build-all.yml` does, with `scripts\build-windows.bat`,
   which also runs `lrelease` and `windeployqt`.
2. Copy the Visual C++ runtime into the output folder with
   `scripts\copy-vc-runtime.ps1 -Architecture AMD64 -Destination windows`.
3. Run `scripts\build-msix.ps1` from a Git checkout:
   - `-InputDirectory windows`;
   - `-BoostDirectory` the Boost folder the build used;
   - an output folder, a version such as `2.2.1000.0`, an identity, a
     publisher, and `-PackageName LibreCAD`.

The package must be signed with a locally trusted development certificate
before it can be installed outside the Store.
