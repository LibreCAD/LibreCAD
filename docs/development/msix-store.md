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

# Microsoft Store MSIX

`build-all.yml` packages an MSIX in each of its Windows jobs after the NSIS
installer has been built and uploaded, from the payload staged for it, so no
extra compile is needed and the installer is built exactly as before. On
pushes to `master` its `PackageWindowsMsix` job combines the x64 and ARM64
packages into one `.msixbundle`, wrapped in a `.msixupload` archive; the
`winMsixBundle` artifact holds the `.msixupload`, so the bundle is stored
once. `msix-store-submit.yml` submits that archive to the Store once the
product is live.

These artifacts are unsigned. **An unsigned MSIX cannot be installed**, so they
are for Store submission and validation only, never direct-download
installers, and they are not attached to the `continuous` release. Microsoft
signs a package after it passes Store certification, so the Store route needs
no certificate and no private key in GitHub Actions.

Only the **LibreCAD-beta** channel is wired up: every package is built with
the package name `LibreCAD-beta` and the beta identity. Nothing builds a
`LibreCAD` (stable) package yet, and no workflow reads the stable variables
listed below.

## What the package contains

`scripts/build-msix.ps1` packages the staged folder with these changes:

- The packaged `LibreCAD.exe` is a Windows GUI program (PE subsystem 2). A
  console build, which would open a console window beside LibreCAD on every
  start, gets its copy in the package marked as a GUI program, as
  `editbin /SUBSYSTEM:WINDOWS` would; a GUI build is packaged as it is. The
  installers keep the program as built.
- It is the only program in the package: test and developer programs, linker
  files (`.lib`, `.exp`, `.ilk`), debug symbols, the app-local Universal CRT,
  the software OpenGL renderer and the Direct3D shader compilers
  (`d3dcompiler_47.dll`, `dxcompiler.dll`, `dxil.dll`) are left out.
  LibreCAD paints with QPainter and loads none of them.
- LibreCAD's own icon engine, `iconengines\lc_svgiconengine.dll`, which
  draws the icons in the colours set in the options, must be in the package.
  If the staged folder lacks it, as it does when the CMake build writes the
  DLL outside the Release folder that is staged, it is copied from the build
  folder.
- Fonts, hatch patterns, the block library, plugins and translations go under
  `resources\`, where LibreCAD looks for them. The translations, LibreCAD's
  and its plugins', are packaged as files even though the program embeds
  them, because the language list in the options only shows the `.qm` files
  it finds on disk.
- `resources.pri` indexes every logo variant: the script removes the
  packaging section MakePri's default config has, which would split the
  index by scale into files Windows never reads.
- `LICENSE` and the files of `licenses/` go under `licenses\`: the GPLv2 of
  LibreCAD and of the Qt libraries it ships, the GPLv3 of WenQuanYi Micro
  Hei, the licences of the other bundled fonts, and `qt-third-party.txt`
  with the copyright and licence text of each third-party component inside
  the Qt DLLs and plugins. `readme.md` names the copyright holders of the
  CJK fonts and says that Qt is used under the GPLv2. `qt-third-party.txt`
  is for one Qt release: when
  `build-all.yml` installs another, `build-msix.ps1` warns, and
  `scripts/msix/update-qt-third-party.py --tag v<version>` regenerates it
  from Qt's own attribution data. `licenses\SOURCE.txt`
  names the commit the package was built from, the Qt version it contains
  and the Boost version compiled into `LibreCAD.exe`, links to the source of
  all three, and carries the project's written offer of that source for
  three years, as GPLv2 section 3(b) asks: a link alone would not do for a
  Store distribution, neither for LibreCAD nor for the Qt libraries the
  package ships under the GPLv2, nor for Boost, which is not in the
  repository. Requests for the source arrive on the issue tracker; answer
  them with LibreCAD's commit and the Boost and Qt releases named in the
  file.
- The input folder must hold the Visual C++ runtime DLLs, which
  `scripts/copy-vc-runtime.ps1` stages; windeployqt only brings
  `vc_redist.<arch>.exe`, which is left out.
- The script prints the size of the package it made.
- The tiles and icons are scaled from `scripts/msix/librecad-logo.png`, a
  1080 x 1080 rendering of `CI/librecad.svg` without its white background
  square. `scripts/msix/librecad-tile-300.png` is the same rendering at the
  size of the Store's app tile icon.

## One-time setup

### Partner Center product

Use the LibreCAD Microsoft Partner Center developer account. The
`LibreCAD-beta` product is reserved; a stable `LibreCAD` product would be a
separate product with its own identity, so that Windows can install both side
by side.

The identity values come from **Product management > Product identity** of the
product. They are case, space and punctuation sensitive; copy them verbatim:

| Manifest slot | Partner Center field |
| --- | --- |
| `Package/Identity/@Name` | Package/Identity/Name |
| `Package/Identity/@Publisher` | Package/Identity/Publisher (`CN=<GUID>`) |
| `Package/Properties/PublisherDisplayName` | Package/Properties/PublisherDisplayName |
| `Package/Properties/DisplayName` | A name reserved for the product, exactly |

`build-msix.ps1` sets `DisplayName` to the package name, so the product must
have **`LibreCAD-beta`** among its reserved names, spelled exactly so. Check
the list under **Product management > Product identity** (or **Manage app
names**) before the first upload; a package whose `DisplayName` is not a
reserved name is rejected at upload.

### Repository variables

These are non-secret **repository variables**. They must be repository-scoped,
not environment-scoped, because the x64 and ARM64 build jobs need the real
identity before any protected job starts. LibreCAD/LibreCAD has the four beta
values set; a fork that wants a submittable package needs its own.

| Variable | Used by | Value |
| --- | --- | --- |
| `MICROSOFT_STORE_BETA_IDENTITY_NAME` | `build-all.yml` | Package/Identity/Name of LibreCAD-beta |
| `MICROSOFT_STORE_PUBLISHER` | `build-all.yml` | Package/Identity/Publisher, `CN=<GUID>` |
| `MICROSOFT_STORE_PUBLISHER_DISPLAY_NAME` | `build-all.yml` | Package/Properties/PublisherDisplayName |
| `MICROSOFT_STORE_BETA_PRODUCT_ID` | `msix-store-submit.yml` | Store ID of LibreCAD-beta: 12 letters and digits, such as `9NBLGGH4NNS1` |
| `MICROSOFT_STORE_IDENTITY_NAME` | nothing yet | Package/Identity/Name of a future stable product |
| `MICROSOFT_STORE_PRODUCT_ID` | nothing yet | Store ID of a future stable product |

`msstore --appId` takes the 12-character Store ID, not the Partner Center
product GUID; the submit job refuses any other format.

Without the identity variables a build uses the placeholder identity
`LibreCAD.Beta.Development` and publisher `CN=LibreCAD Development`, so CI
still exercises the packaging, and the submit job refuses such a bundle.

### The `microsoft-store` environment

`msix-store-submit.yml` runs in the `microsoft-store` environment, which is
the release gate. In **Settings > Environments > microsoft-store**, set up
the gate before adding any secret: with secrets and no gate, anyone with
write access could publish from any branch.

1. Under **Deployment protection rules**, turn on **Required reviewers** and
   add the maintainers who may publish.
2. Under **Deployment branches and tags**, choose **Selected branches and
   tags** and allow `master` only. A manual dispatch runs on the branch
   picked in the dialog, so this keeps other branches from submitting.
3. Add these **environment secrets** (Microsoft Entra ID application for the
   Microsoft Store Developer CLI):

   | Secret | Value |
   | --- | --- |
   | `AZURE_AD_TENANT_ID` | Entra tenant ID, from Identity > Overview |
   | `AZURE_AD_APPLICATION_CLIENT_ID` | Application (client) ID of the app registration |
   | `AZURE_AD_APPLICATION_SECRET` | Client secret value; it expires, so plan its rotation |
   | `SELLER_ID` | Partner Center seller ID, from Account settings > Identifiers |

The credentials only work once the account side is wired up as well:

- The Entra tenant must be associated with the Partner Center account.
- The app registration must be added under **Account settings > User
  management > Microsoft Entra applications** with the **Manager** role.
- The product must already have one live Store submission: `msstore` updates
  published products only.
- Store updates through the CLI are supported for free products only.

None of this is needed for the first submission, which is made by hand.

## First submission

The first submission of a product is always made by hand in Partner Center.
Certification takes up to three business days, and the review of the
`runFullTrust` capability can add to that.

Before you start:

- This work must be on `master`: the privacy policy URL below points at
  `PRIVACY.md` there, and the package must come from a `master` run.
- A reserved app name lapses three months after it was reserved unless a
  submission uses it. Check the reservation date of `LibreCAD-beta` in
  Partner Center and submit before then, or reserve the name again.

### 1. Get the package

1. Open the latest **Continuous build** run of `master` in the Actions tab
   whose **PackageWindowsMsix** job succeeded. The run must have started
   after the repository variables were set; otherwise its package carries
   the placeholder identity and Partner Center refuses it.
2. Download the `winMsixBundle` artifact (kept for 30 days) and unzip it. It
   holds `LibreCAD-beta-<version>-windows.msixupload`, the file to upload.
   The `.msixupload` is a ZIP archive with the `.msixbundle` inside it.

Every run numbers its package `<major>.<minor>.<run number>.0`. The Store
gives each device the highest version available to it, so a bundle reaches
existing users only when its version is higher than the one they have:
always submit the newest run. To roll back new installs after a bad
release, submit the previous good run's bundle instead; users who already
have the bad version keep it until a higher one ships.

### 2. Start the submission

In Partner Center open **Apps and games > LibreCAD-beta**. The product must be
an **MSIX or PWA app** (an EXE or MSI product asks for a download URL instead
of a package), and a reserved name lapses after three months without a
submission. Choose **Start your submission** and fill in the pages below in
any order; **Submit to the Store** becomes available once each one is
complete.

### 3. Pricing and availability

- **Markets**: all markets (the default).
- **Audience**: **Private audience**, with a group of testers' Microsoft
  accounts, to try the Store install before anyone else sees it; **Public
  audience** to list it. Once a submission has gone out public, the product
  can never be made private again.
- **Discoverability**: make it available and discoverable.
- **Pricing**: **Free**, no free trial. The Store CLI only updates free
  products.

### 4. Properties

- **Category**: **Multimedia design**, subcategory **Illustration + graphic
  design**, with **Productivity** as the secondary category.
- **Privacy policy**: answer **Yes** to "Does this product access, collect,
  or transmit personal information?" and give
  `https://github.com/LibreCAD/LibreCAD/blob/master/PRIVACY.md`. Store policy
  10.5.1 counts every Win32 desktop product as having access to personal
  information, so it always needs a privacy policy; this one says that
  LibreCAD collects nothing and names the anonymous update check.
- **Website**: `https://librecad.org`
- **Support contact info**: `https://github.com/LibreCAD/LibreCAD/issues`
- **System requirements**: keyboard and mouse recommended; under additional
  requirements, Windows 10 version 1809 or later.
- **Product declarations**: keep the two that are on by default (installing
  to alternate drives, OneDrive backups) and leave the rest clear. LibreCAD
  sells nothing; Help > Donate only opens the project's donation page in the
  browser, which the certification notes below say. If certification still
  cites policy 10.8.2 for it, tick **This product allows users to make
  purchases, but does not use the Microsoft Store commerce system** and
  resubmit.

### 5. Age ratings

The IARC questionnaire asks first what kind of product it is (an app, not a
game) and then questions that depend on that answer. Answer them as they are:
LibreCAD has no communication between users, shares no location, sells
nothing and is not a web browser; it only opens its fixed help pages in the
default browser. Choose **Save and generate**.

### 6. Packages

Upload the `.msixupload`; Partner Center checks each package as it arrives and
lists the x64 and ARM64 packages. The identity and publisher must match the
**Product identity** page exactly. A `DisplayName` that is not reserved fails
with "The name found in the package is not one of your reserved app names".
A warning that `runFullTrust` needs approval is expected: the justification
under **Submission options** answers it. Under **Device family availability**
keep **Windows 10/11 Desktop** only.

### 7. Store listing (English (United States))

The package declares `en-us` only, so one English (United States) listing is
enough. The description may not contain URLs or HTML.

- **Description**:

  > LibreCAD is a free, open-source 2D CAD program for technical drawings,
  > plans, mechanical parts and diagrams. It opens and saves DXF and DWG
  > drawings and exports PDF, SVG and images.
  >
  > - Lines, arcs, circles, ellipses, splines, polylines, hatches and text
  > - Dimensions, leaders, blocks and a block library
  > - Layers, snapping and a command line for keyboard input
  > - Move, rotate, scale, mirror, trim, extend, offset, fillet, chamfer and
  >   more
  > - Printing and plotting with page layouts and scale
  > - Plugins and customizable toolbars, menus and shortcuts
  > - Translated into many languages by volunteers
  >
  > LibreCAD is free software under the GNU General Public License version 2;
  > its source code is on GitHub.
  >
  > This is the beta channel, built from the development branch: it gets new
  > features and fixes first, and may also have new bugs.

- **What's new in this version**: leave it blank for the first submission.
  Later submissions made by CI copy it unchanged, so edit it in Partner
  Center when it should say something.
- **Product features** (up to 20, 200 characters each): the bullet points of
  the description, one per line.
- **Screenshots**: PNG, at least 1366 x 768, up to 10; four are recommended.
  Take them on Windows from the build being submitted, for example a drawing
  with the layer and block docks, a dimensioned part and the print preview.
- **Store logos**: upload `scripts/msix/librecad-tile-300.png` as the **1:1
  App tile icon (300 x 300)**. The Store shows it on its pages instead of the
  package's own logo. Box art and poster art are for games and Xbox only.
- **Short description**: "Free, open-source 2D CAD for DXF and DWG drawings."
- **Keywords** (up to 7): CAD, 2D CAD, DXF, DWG, drafting, technical drawing,
  floor plan.
- **Copyright and trademark info**: `Copyright (C) LibreCAD contributors`.
- **Additional license terms**: the URL
  `https://github.com/LibreCAD/LibreCAD/blob/master/LICENSE`, which the Store
  shows as a link. Left blank, Microsoft's Standard Application License Terms
  would apply, and they forbid the copying and changing that the GPL allows.
- **Developed by**: `LibreCAD contributors`.

Keep other companies' product names out of the listing and the keywords
(policy 10.1.3); describe the file formats as DXF and DWG.

### 8. Submission options

- **Publishing hold options**: choose **Publish this submission as soon as it
  passes certification**. Every later submission made by CI copies this
  choice, so a hold here would hold each CI update until someone presses
  Publish in Partner Center.
- **Notes for certification** (testers only, customers never see them):

  > LibreCAD is a free, open-source (GPLv2) 2D CAD desktop program built
  > with Qt. It needs no account and no sign-in, and depends on no driver or
  > service. On first start it asks for the language and units; then draw
  > a line (Tools > Line > 2 Points) or open a .dxf or .dwg file. At start
  > it asks api.github.com anonymously whether a newer LibreCAD release
  > exists; Options > Application Preferences > Startup turns this off.
  > Help > Donate only opens https://librecad.org/donate.html in the default
  > browser, where voluntary donations go through Open Collective or GitHub
  > Sponsors; the app takes no payment, and donating unlocks or changes
  > nothing in it. Source code: https://github.com/LibreCAD/LibreCAD

- **Restricted capabilities**, why the app needs `runFullTrust` and how it
  uses it:

  > LibreCAD is an existing Win32 desktop program (C++ and Qt) packaged as
  > MSIX. runFullTrust lets it run as a desktop program: it opens and saves
  > drawings the user picks anywhere on disk, loads its own plugin DLLs and
  > prints. It installs no services or drivers and never asks for
  > elevation.

### 9. Submit

Choose **Submit to the Store**. Certification scans the package, runs the
App Certification Kit's tests, and testers install and start the app and
compare it with the listing. Once the product is live, later versions go
through the **Submit MSIX to Microsoft Store** workflow below.

## CI behavior

- On pushes to `master`, both Windows jobs package an MSIX and
  `PackageWindowsMsix` bundles them. A packaging failure there does not stop
  the installers from being published: the MSIX steps continue on error,
  nothing is uploaded, and `PackageWindowsMsix` fails instead.
- The packaging and upload steps stop after 10 minutes, so a hang counts as
  an MSIX failure instead of holding back the installers. MakeAppx writes
  its file list to `msix-output\makeappx-*.log`, which the job prints when
  packaging fails or times out.
- A pull request that changes `build-all.yml`, `scripts/build-msix*.ps1` or
  `scripts/msix/` starts `build-all.yml`, which builds macOS and ARM64 as
  for any pull request, and also packages the ARM64 MSIX as a check: a
  packaging failure fails the job after its NSIS installer has been
  uploaded, and nothing is uploaded for the MSIX, so the pull request's
  artifacts are the ones it had before. Pull requests that start the
  workflow for other files (the macOS scripts, `nsis-msvc.nsi`) skip the
  check unless they also change `licenses/` or `LICENSE`, which the
  package copies; those two alone do not start the workflow and are first
  packaged on `master`. Bundling runs on pushes only.
- To submit an update, manually dispatch **Submit MSIX to Microsoft Store**
  on `master` with the run ID of the **Continuous build** run whose bundle
  should ship. A reviewer of the `microsoft-store` environment must approve
  the job. The job:
  1. accepts only a push run of `build-all.yml` on `master`, and refuses a
     bundle with the placeholder identity;
  2. refuses the run if its App Certification Kit report has a failing
     required test (`ignore_wack` overrides);
  3. refuses to start while another submission is in progress, whatever its
     state, and links to it (`replace_pending` deletes it instead);
  4. uploads the `.msixupload` to a new draft with a pinned msstore CLI;
     `no_commit` stops here and leaves the draft, which
     `msstore submission publish <product id>` sends on later (Partner
     Center shows the uploaded package only once it is sent);
  5. sends the draft to certification, waits up to an hour for the Store to
     process the commit, and writes the submission's state and a Partner
     Center link to the job summary.

  A green job means the commit went through, not that the submission passed
  certification, which can take up to three business days. If the commit was
  still being processed when the job stopped waiting, the job warns, and the
  commit can still fail; Partner Center shows the outcome.

### What a dispatch does to the product

- Each run creates a new submission as a copy of the last published one and
  changes only its packages: the new `.msixupload` replaces the previous
  `.msixupload`. The CLI replaces a package of the same file type only, so
  the first submission must upload the `.msixupload` too, as above; a bare
  `.msixbundle` uploaded there would stay in every later submission.
  Everything else carries over: the listing fields of step 7 including its
  "What's new" text, markets, the audience, the publishing hold option and
  the price (free). Change those in a Partner Center submission, not through
  CI. The pinned msstore CLI has no field for screenshot captions or the sort
  title, and a submission update replaces the whole submission, so a CI run
  most likely clears both; leave them empty.
- The product must stay free: the Store CLI updates free products only.
- After a submission with gradual rollout, the next run fails until the
  rollout is finalized or halted in Partner Center
  (`msstore submission rollout finalize|halt <product id>`).
- Once a submission has shipped a `.msixbundle`, every later one must too,
  which the `.msixupload` always contains.

## Package version

The package version is `major.minor.<GitHub run number>.0`.

The Microsoft Store reserves the fourth component and rejects any package whose
revision is not 0, so the monotonically increasing build counter goes in the
third component instead. The Store always serves the highest version a device
can use, so an update reaches existing users only when its version is higher,
and the run number is the only always-increasing value available to the
workflow; the tag patch level is
not, because several submissions can be cut from the same tag. Both the workflow
and `build-msix.ps1` enforce the revision rule, so a package that would be
rejected at upload fails during packaging instead.

## Certification testing

`PackageWindowsMsix` runs two checks that need no certificate:

- It registers the unpacked x64 layout with `Add-AppxPackage -Register`, which
  exercises the manifest, identity and entry point. Developer Mode is already
  enabled on every GitHub-hosted Windows image. This does not validate the
  signature, the block map, or the real container.
- It runs the packaged `LibreCAD.exe` once, converting
  `resources\library\misc\screw.dxf` to PNG with stderr redirected to a
  file, and fails unless that exits with 0, writes the PNG and leaves Qt's
  messages in the file (`QT_FORCE_STDERR_LOGGING` is set, since a GUI program
  without a console window otherwise logs to the debugger). This catches a
  missing Qt DLL and a packaged program that does not start before
  certification does; the runner has the Visual
  C++ runtime installed, so a missing runtime DLL is caught by
  `build-msix.ps1` instead.
- It runs the Windows App Certification Kit against the bundle when
  `appcert.exe` is present. WACK does not require a signed package -- its only
  signature test is optional and inspects payload PEs. The kit is not
  contractually part of the runner image, so the step skips when it is missing
  and never fails the build; instead it prints the overall result, warns for
  each failed test, and uploads the report as the `winMsixWackReport`
  artifact. Read that report for the run you submit.

`build-msix.ps1` also unpacks every package it makes and checks the
manifest and the required files again, because MakeAppx's own semantic
validation is incomplete and the packages it produces are not guaranteed to be
installable; a package that fails is deleted. `build-msix-bundle.ps1`
unbundles the bundle and checks that it holds both architecture packages.

## Registry, settings and the NSIS installer

MSIX file associations are declared in `AppxManifest.xml`; the package does not
write LibreCAD keys into the machine registry. The manifest lists the ProgIDs
the NSIS installer registers, `LibreCAD.DXF` and `LibreCAD.DWG`, as migration
sources, so an existing installation's associations move to the package.
Migration does not let the MSIX delete the machine-wide NSIS keys.

Settings are another matter. A packaged app's writes to `HKCU\Software` and
new files under `AppData` go to a private per-package store that Windows
deletes when the package is uninstalled; reads still see the real registry.
The Store build therefore starts with the settings of an existing NSIS
installation. Its registry changes and the files it creates under `AppData`
stay inside the package, and uninstalling it removes them. A settings folder
set outside `AppData` (**Options > Application Preferences > Paths**) is not
virtualized at all. Since Windows 10 version 1903,
though, a file that already exists under `AppData` is changed in place: when
LibreCAD rewrites a file an earlier installation left in its settings folder
(`workspaces.lcws` and `penpalette.lcpp` on every exit, `shortcuts.lcsc` when
shortcuts are edited), the change reaches the real file, is shared with that
installation, and survives uninstalling the package.

See Microsoft's
[packaged-desktop registry model](https://learn.microsoft.com/windows/msix/desktop/desktop-to-uwp-behind-the-scenes)
and its
[unpackaged-to-Store transition guidance](https://learn.microsoft.com/windows/apps/distribute-through-store/how-to-transition-users-from-your-web-unpackaged-app-to-store-packaged-app).

The Store package does not use `windows.customInstall`, a
[restricted extension for specially approved packages](https://learn.microsoft.com/uwp/schemas/appxpackage/uapmanifestschema/element-desktop6-custominstall),
so it cannot run an elevated registry clean-up while installing. Legacy NSIS
registrations are cleaned by the NSIS installers.

## Local package creation

On a Windows machine with the Windows SDK, build LibreCAD and stage it as
`build-all.yml` does for the installer: `windeployqt --release
--compiler-runtime` on the built `LibreCAD.exe`, copy the build's `Release`
folder to a staging folder, and copy the Visual C++ runtime into it with
`scripts\copy-vc-runtime.ps1 -Architecture AMD64 -Destination <folder>`
(`ARM64` on Arm). Then run `scripts/build-msix.ps1` on that folder, with
the build folder as `-BuildDirectory`, from a Git checkout, so that it can
name the commit in `licenses\SOURCE.txt`. The package must be signed with a locally trusted
development certificate before it can be installed outside the Store. Do not
add a production certificate to this repository or to GitHub Actions secrets.
