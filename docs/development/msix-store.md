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

`msix-store-submit.yml` submits a release's bundle to the Store, as
described under Submitting a release below.

## One-time setup

### Partner Center product

Use the LibreCAD Microsoft Partner Center developer account. The stable
product is separate from LibreCAD-beta:
- **Type:** an **MSIX or PWA app**.
- **Reserved name:** **`LibreCAD`**, spelled exactly so. `build-msix.ps1`
  sets `DisplayName` to the package name, and a package whose `DisplayName`
  is not a reserved name of the product is rejected at upload.
- **Expiry:** a reserved name lapses three months after it was reserved
  unless a submission uses it.

The identity values come from **Product management > Product identity** of
the product; copy them verbatim. Package/Identity/Publisher and
Package/Properties/PublisherDisplayName belong to the account, so they are the
same as LibreCAD-beta's.

### Repository variables

These are non-secret **repository variables**, repository-scoped because the
build job needs them before any protected job starts.

| Variable | Used by | Value |
| --- | --- | --- |
| `MICROSOFT_STORE_IDENTITY_NAME` | `build-all.yml`, `msix-store-submit.yml` | Package/Identity/Name of the stable product |
| `MICROSOFT_STORE_PRODUCT_ID` | `msix-store-submit.yml` | Store ID of the stable product: 12 letters and digits |
| `MICROSOFT_STORE_PUBLISHER` | `build-all.yml`, `msix-store-submit.yml` | Package/Identity/Publisher, `CN=<GUID>`; already set for LibreCAD-beta |
| `MICROSOFT_STORE_PUBLISHER_DISPLAY_NAME` | `build-all.yml` | Package/Properties/PublisherDisplayName; already set for LibreCAD-beta |

`msstore --appId` takes the 12-character Store ID, not the Partner Center
product GUID; the submit job refuses any other format.

### The `microsoft-store-stable` environment

`msix-store-submit.yml` runs in the `microsoft-store-stable` environment,
which is the release gate. In **Settings > Environments >
microsoft-store-stable**, set up the gate before adding any secret:

1. Under **Deployment protection rules**, turn on **Required reviewers** and
   add the maintainers who may publish.
2. Under **Deployment branches and tags**, choose **Selected branches and
   tags** and allow `2.2.1` only. A dispatch runs on the branch picked in the
   dialog, so this keeps other branches from submitting.
3. Add the **environment secrets** of the `microsoft-store` environment, with
   the same values: `AZURE_AD_TENANT_ID`, `AZURE_AD_APPLICATION_CLIENT_ID`,
   `AZURE_AD_APPLICATION_SECRET`, `SELLER_ID`. The Entra application has the
   Manager role for the whole account, so it covers this product too. Its
   client secret expires; rotate it in both environments.

The first step of the submit job checks that all four secrets are set, that
the tenant and client IDs are GUIDs, that `MICROSOFT_STORE_PRODUCT_ID` is a
Store ID and that `MICROSOFT_STORE_IDENTITY_NAME` is set, and stops with a
list of what is wrong before it downloads anything.

The CLI updates published, free products only, so the first submission is
made by hand.

## First submission

Make it in Partner Center by hand, as master's `docs/development/msix-store.md`
describes for LibreCAD-beta (steps 2 to 9), with these differences:

- **Package.** Take the `winMsixBundle` artifact of a green push or tag run
  of `2.2.1` that started after the repository variables were set, and upload
  the `.msixupload` it holds.
- **Description.** Leave out master's last paragraph, which describes the beta
  channel.
- **Notes for certification.** Replace the sentence about Check for New
  Version with: "The Store build makes no network request of its own and
  shows no link to other downloads; the Store updates it."
- **Audience.** Choose **Private audience** first, and check the Store install
  before going public:
  - it starts, and opens a `.dxf` and a `.dwg` by double-click;
  - its plugins work, and Options lists the translations;
  - Help > License lists the GPLv3 and the Noto OFL;
  - Help > Online has no Release Information.

The privacy policy is master's `PRIVACY.md`, as for LibreCAD-beta.

## Submitting a release

After a `v2.2.1.N` tag has built green:
1. Open **Actions > Submit MSIX to Microsoft Store > Run workflow**.
2. Choose **Use workflow from: 2.2.1**.
3. Give the run ID of that tag's **Continuous build** run, the one whose
   branch is the tag.
4. A reviewer of `microsoft-store-stable` approves the job.

The job:
1. accepts only a push run of `build-all.yml` in this repository whose
   `PackageWindowsMsix` succeeded:
   - the run of a `v2.2.1.N` tag that still names the commit it built;
   - or, with `allow_branch_build`, a run of the current head of `2.2.1`;
2. refuses a bundle whose identity is not `MICROSOFT_STORE_IDENTITY_NAME` and
   `MICROSOFT_STORE_PUBLISHER`: a placeholder or a LibreCAD-beta bundle;
3. refuses the run if its App Certification Kit report has a failing required
   test (`ignore_wack` overrides);
4. refuses to start while another submission of the product is in progress,
   whatever its state (`replace_pending` deletes it instead);
5. uploads the `.msixupload` to a new draft with the msstore CLI pinned as on
   master; `no_commit` stops here and leaves the draft, which
   `msstore submission publish <product id>` sends on later;
6. sends the draft to certification, waits up to an hour for the Store to
   process the commit, and writes the submission's state and a Partner Center
   link to the job summary.

A green job means the commit went through, not that the submission passed
certification, which can take up to three business days.

Master's copy of `msix-store-submit.yml` handles LibreCAD-beta. GitHub starts
a `workflow_run` workflow only from the default branch's file, so 2.2.1's
copy is never started by a build; it runs when dispatched on 2.2.1.

## What each submission does to the product

As on master:
- **A submission copies the last published one** and changes only its
  packages: the new `.msixupload` replaces the previous one.
- **Everything else carries over:** the listing, markets, the audience, the
  publishing hold option and the price. Change those in Partner Center, not
  through CI.
- **The product must stay free:** the CLI updates free products only.
- **Gradual rollout:** after a submission with one, the next run fails until
  the rollout is finalized or halted
  (`msstore submission rollout finalize|halt <product id>`).

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
