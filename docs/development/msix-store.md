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

# Microsoft Store MSIX CI

`build-all.yml` packages an MSIX in each of its existing Windows jobs -- right
after the payload is staged for the NSIS installer, so no extra compile is
needed -- and its `PackageWindowsMsix` job combines the two into one
`.msixbundle` plus a `.msixupload` archive. `msix-store-submit.yml` submits that
bundle.

These artifacts are intentionally unsigned. **An unsigned MSIX cannot be
installed**, so they are Store-submission and validation artifacts only, never
direct-download installers, and they are deliberately not attached to the
`continuous` release. Microsoft re-signs an MSIX after Store certification, so
the Store path needs no paid certificate and no private key in GitHub Actions.
Publishing a signed bundle for direct download is a separate decision that
requires its own certificate and a different package identity.
## One-time Store setup

1. Create or use the LibreCAD Microsoft Partner Center developer account and
   reserve separate `LibreCAD` and `LibreCAD-beta` products. The two products
   must have different package identity names so Windows can install them
   side by side.
2. Create the `microsoft-store` GitHub Actions environment. Require approval
   and restrict deployments to release branches or tags; this is the release gate
   for the Store submission job.
3. Add these non-secret **repository variables**, using the values assigned by
   Partner Center. They must be repository-scoped because the x64 and ARM64
   package-build jobs need the real identity before the protected publish job
   starts:

   All identity values come from **Product management > Product identity** of the
   matching Partner Center product. They are case, space, and punctuation
   sensitive; copy them verbatim.

   | Variable | Manifest slot | Value |
   | --- | --- | --- |
   | `MICROSOFT_STORE_IDENTITY_NAME` | `Package/Identity/@Name` | Reserved LibreCAD package identity name, such as `12345LibreCAD.LibreCAD` |
   | `MICROSOFT_STORE_BETA_IDENTITY_NAME` | `Package/Identity/@Name` | Reserved LibreCAD-beta package identity name |
   | `MICROSOFT_STORE_PUBLISHER` | `Package/Identity/@Publisher` | Publisher subject, normally `CN=<GUID>`, not a readable name |
   | `MICROSOFT_STORE_PUBLISHER_DISPLAY_NAME` | `Package/Properties/PublisherDisplayName` | Publisher display name shown in Windows |
   | `MICROSOFT_STORE_PRODUCT_ID` | `msstore --appId` | LibreCAD **Store ID**: 12 alphanumeric characters, such as `9NBLGGH4NNS1` |
   | `MICROSOFT_STORE_BETA_PRODUCT_ID` | `msstore --appId` | LibreCAD-beta Store ID |

   `msstore --appId` takes the 12-character Store ID, not the Partner Center
   product GUID. The publish job rejects any other format.

   The reserved app name also matters: `Package/Properties/DisplayName` is set
   from the selected package name, so **both `LibreCAD` and `LibreCAD-beta` must
   be reserved app names** in Partner Center, each on its own product.

4. Register a Microsoft Entra ID (Azure AD) application for Microsoft Store
   Developer CLI access and add these secrets to the protected `microsoft-store`
   environment:

   | Secret | Value |
   | --- | --- |
   | `AZURE_AD_TENANT_ID` | Entra tenant ID, from Identity > Overview |
   | `AZURE_AD_APPLICATION_CLIENT_ID` | Application (client) ID of the app registration |
   | `AZURE_AD_APPLICATION_SECRET` | Client secret value; these expire, so plan rotation |
   | `SELLER_ID` | Partner Center publisher/seller ID, from Account settings > Identifiers |

5. The credentials only work once the account side is wired up as well:

   - The Entra tenant must be associated with the Partner Center account.
   - The app registration must be added under **Account settings > User
     management > Microsoft Entra applications** and assigned the **Manager**
     role.
   - The product must already have one live Store submission before CLI-driven
     updates work.
   - Store app updates through GitHub Actions are currently supported for free
     products only.

Each package identity name and publisher must be fixed before its first Store
submission. `LibreCAD` and `LibreCAD-beta` deliberately use different Partner
Center identities and product IDs. Their package family names are therefore
different, allowing Windows to install, update, and uninstall the two channels
independently. Both packages may use the same publisher.

## CI behavior

- Pushes and pull requests that touch the Windows package surface build and
  validate both architecture packages and the bundle.
- `build-all.yml` runs on pushes to `master` and uses the `LibreCAD-beta`
  package name.
- A normal run uses distinct harmless development identities when Store
  variables are not configured: `LibreCAD.Development` and
  `LibreCAD.Beta.Development`. That keeps side-by-side behavior covered by CI
  but does not create a submit-ready Store package. The publish job refuses such
  a package rather than uploading one the Store would reject for a mismatched
  identity.
- To submit, manually dispatch **Submit MSIX to Microsoft Store** with the run
  ID of the "Continuous build" run whose bundle you want to ship. The protected
  `microsoft-store` environment must approve the job. Submission is manual by
  design: `msstore` can only update a product that is **already published and
  live**, so the very first submission has to be made by hand in Partner Center.
- The submit job uses `microsoft/microsoft-store-apppublisher` and `msstore` to
  upload the `.msixupload` archive and create the Partner Center submission.
  Microsoft handles signing after the package passes certification.

## Package version

The generated MSIX version is `major.minor.GitHub-run-number.0`.

The Microsoft Store reserves the fourth component and rejects any package whose
revision is not built as 0, so the monotonically increasing build counter goes in
the third component instead. The Store also requires every new submission to
carry a strictly higher version than the previous one, and the run number is the
only always-increasing value available to the workflow; the tag patch level is
not, because several submissions can be cut from the same tag. Both the workflow
and `build-msix.ps1` enforce the revision rule, so a package that would be
rejected at upload fails during packaging instead.

## Certification testing

`PackageWindowsMsix` runs two checks that need no certificate:

- It registers the unpacked x64 layout with `Add-AppxPackage -Register`, which
  exercises the manifest, identity and entry point. Developer Mode is already
  enabled on every GitHub-hosted Windows image. This does not validate the
  signature, the block map, or the real container.
- It runs the Windows App Certification Kit against the bundle when
  `appcert.exe` is present. WACK does not require a signed package -- its only
  signature test is optional and inspects payload PEs. The kit is not
  contractually part of the runner image, so the step skips when it is missing
  and never fails the build.

Both `build-msix.ps1` and `build-msix-bundle.ps1` additionally round-trip every
artifact through `MakeAppx unpack` / `unbundle` and re-assert the manifest,
because MakeAppx's own semantic validation is incomplete and packages it
produces are not guaranteed to be installable.

## Registry and legacy-installer behavior

MSIX file associations are declared in `AppxManifest.xml`; the package does not
write LibreCAD keys directly into the machine registry. Windows owns those
registrations and removes package-private registry state when the package is
uninstalled. Each manifest lists only its matching NSIS ProgIDs as migration
sources: `LibreCAD.DXF/DWG` for stable and `LibreCAD-beta.DXF/DWG` for beta.
This prevents one channel from inheriting the other channel's associations.
Migration does not grant the MSIX permission to delete machine-wide NSIS keys.

This follows Microsoft's documented
[packaged-desktop registry model](https://learn.microsoft.com/windows/msix/desktop/desktop-to-uwp-behind-the-scenes)
and its
[unpackaged-to-Store transition guidance](https://learn.microsoft.com/windows/apps/distribute-through-store/how-to-transition-users-from-your-web-unpackaged-app-to-store-packaged-app).

The Store package intentionally does not use `windows.customInstall`. That
extension requires restricted Microsoft approval and is intended for narrowly
approved package types. A normal Store MSIX therefore cannot run an elevated
registry-cleanup executable during installation. Legacy NSIS registrations are
cleaned by the NSIS installers. Installing one architecture replaces obsolete
architecture registrations for the same package name because those builds share
an install directory. A separately named stable or beta sibling is preserved
while its recorded uninstaller exists; abandoned sibling registrations are
removed. Per-user LibreCAD preferences under `HKCU` are intentionally retained;
they are user data rather than installer ownership records.
Microsoft documents `windows.customInstall` as a
[restricted facility intended for specially approved packages](https://learn.microsoft.com/uwp/schemas/appxpackage/uapmanifestschema/element-desktop6-custominstall).

## Local package creation

On a Windows machine with the Windows SDK, build and deploy LibreCAD first, then
run `scripts/build-msix.ps1`. The package must be signed with a locally trusted
development certificate before it can be installed outside the Store. Do not add
a production certificate to this repository or GitHub Actions secrets for the
Store workflow.
