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

# macOS DMG Integrity and Distribution Plan

## Plan Status

| Field | Value |
| --- | --- |
| Issue | [LibreCAD #2574](https://github.com/LibreCAD/LibreCAD/issues/2574) |
| Trigger report | [ARM64 DMG failure comment](https://github.com/LibreCAD/LibreCAD/issues/2574#issuecomment-5652451639) |
| Target | `origin/master` only |
| State | Implemented and locally validated; credentialed and interactive checks remain |
| Primary mode | Account-free, ad-hoc-signed application in a verified DMG |
| Optional mode | Developer ID signing and Apple notarization |
| Last code review | 2026-09-14 |

This plan permits replacing every current DMG construction path. Compatibility
is required at the command-entry and artifact levels, not for obsolete internal
implementation details.

## Progress

| Phase | Status | Commit | Required evidence |
| --- | --- | --- | --- |
| 0. Baseline | Complete | `fcea8a45e` | Reusable app snapshot plus positive and generated negative checks |
| 1. Independent verifier | Complete | `fcea8a45e` | App, mounted DMG, tamper, architecture, dependency, path, and cleanup checks |
| 2. Deployment and signing | Complete | `fcea8a45e` | Account-free inside-out signing with no post-sign mutation |
| 3. Plug-in layout | Implemented; GUI smoke pending | `97b976260` | qmake/CMake place all ten plug-ins in `Contents/PlugIns/LibreCAD` |
| 4. DMG construction | Complete | `fcea8a45e` | Direct and conversion backends both produce verified read-only images |
| 5. Caller migration | Complete | `101f7ae7f` | CI, nightly, and legacy wrappers use the shared helpers |
| 6. Optional notarization | Implemented; credentialed run pending | `fcea8a45e` | Fail-closed Developer ID, timestamp, team, runtime, stapling, and Gatekeeper checks |
| 7. Documentation and cleanup | Complete | `101f7ae7f` | User guidance and search proving one owned DMG creation path |

The commit column records the implementation commits after rebasing onto the
current target branch. A phase is not marked complete when its required
environment is unavailable; those limits are called out explicitly.

## Implementation Evidence

- A finalized arm64 application with 75 Mach-O files passes strict ad-hoc
  signature, architecture, dependency, RPATH, plist, and bundle-layout checks.
- Direct UDZO and UDRW-to-UDZO fallback images pass `hdiutil verify`, read-only
  mounting, exact root-layout checks, strict mounted-app verification, and
  source-versus-mounted content-manifest comparison.
- Generated negative checks reject post-sign mutation, a missing architecture,
  an external load dependency, a truncated image, an invalid Applications
  symlink, stale output, path collisions, and evidence paths inside the app.
- qmake6 recursive generation and CMake generation both place all ten built-in
  plug-ins in `Contents/PlugIns/LibreCAD` without changing non-Apple locations.
- ShellCheck, `bash -n`, Actionlint, YAML parsing, plist validation, and
  `git diff --check` pass for the implementation.
- A credentialed Developer ID/notarization run was not possible without project
  credentials. The optional path is implemented but remains open validation.
- The opt-in application `--help` smoke test uses a disposable bundle copy, but
  a successful interactive GUI and Plug-ins dialog check remains open because
  the current execution environment cannot run that UI session reliably.

## Executive Decision

LibreCAD's default macOS package must remain buildable and publishable without a
paid Apple Developer account. The default output will therefore be:

1. A fully deployed application bundle.
2. Explicitly ad-hoc signed after every bundle mutation.
3. Verified structurally before image creation.
4. Copied into a compressed, read-only DMG by a packaging-only helper.
5. Remounted from the completed DMG and verified again before publication.

This can prevent the invalid-code-signature failure reported as an application
being "damaged." It cannot make an Internet-downloaded application trusted by
Gatekeeper. Apple-issued Developer ID signing and notarization require Apple
credentials and remain an optional distribution mode. The account-free release
must clearly document the first-launch right-click **Open** workflow and the
quarantine-removal fallback without claiming notarization.

Self-signed certificates are not the default. They do not establish public
Gatekeeper trust unless every user manually installs and trusts the certificate,
which is worse than a transparent ad-hoc package for this project.

## Verified Root Cause

The reported ARM64 release image was not a corrupt disk image:

- Its downloaded SHA-256 matched the published release asset.
- `hdiutil verify` accepted the DMG.
- The embedded application failed `codesign --verify --deep --strict` with
  `code has no resources but signature indicates they must be present`.
- The crash report recorded `SIGKILL`, `Namespace CODESIGNING`, and an invalid
  executable page.
- Reapplying an ad-hoc signature to a copy of the same deployed bundle made
  strict code-signature verification pass.

The direct failure mechanism is therefore a stale or partial application seal,
usually caused by changing the bundle after some code has already been signed.
The DMG format and compression are not the primary cause, but the current image
pipeline does not independently detect the bad embedded application.

## Current Gaps

### Build and deployment

- `scripts/build-osx.sh` combines compilation, Qt deployment, pruning, signing,
  image construction, optional notarization, and publication preparation.
- It uses `#!/bin/bash -xe`, which makes optional credential handling harder to
  audit and can expose command arguments in logs.
- `macdeployqt` is allowed to perform its default signing behavior before
  LibreCAD finishes pruning the bundle.
- Extra executables and LibreCAD plug-ins are not all passed through
  `macdeployqt -executable`, so dependency rewriting is implicit.
- Obsolete Qt 5.4 checks and comments still describe `macdeployqt -codesign`
  even though the current script signs later itself.
- A signing identity inherited through the environment is overwritten with an
  empty value unless it is supplied through the legacy `-cert` option.

### Bundle structure and signing

- Ten built-in LibreCAD plug-ins are installed under
  `Contents/Resources/plugins`. Apple reserves `Resources` for non-code data;
  loadable code belongs below `Contents/PlugIns`.
- `RS_System::getDirectoryList("plugins")` knows the legacy Resources location
  but not a dedicated macOS built-in plug-in location.
- The identity path explicitly signs some Resources dylibs and then uses
  `codesign --deep` to sign the outer application. Apple recommends signing
  nested code explicitly from the inside out and using `--deep` for diagnosis
  and verification, not as the signing algorithm.
- The ad-hoc path relies entirely on `codesign --deep`, which can miss code in a
  nonstandard nesting location.
- The script proves only that the source application verifies. It does not prove
  that the copy inside the finished DMG verifies.

### DMG construction

- Image creation is embedded in the build script and cannot be repeated safely
  against one finalized application without rerunning unrelated build steps.
- The default flow creates a writable UDRW image and then converts it in place
  to UDZO with a shadow file. This adds state and failure points that are not
  needed when `hdiutil` can create UDZO directly.
- `cp -R` is used for staging instead of the macOS-native `ditto`, which is more
  explicit about preserving bundle metadata, resource forks, and extended
  attributes.
- A failed run can leave an old output DMG that a caller mistakes for the new
  artifact.
- The image is not mounted after creation to validate the `Applications`
  symlink, top-level contents, signatures, architectures, or load paths.

### Multiple entry points

The following paths can currently produce, rename, or publish a DMG and must be
migrated together:

| Entry point | Current role | Required disposition |
| --- | --- | --- |
| `scripts/build-osx.sh` | Build, deploy, sign, and make `LibreCAD.dmg` | Retain as the compatible orchestrator; delegate packaging |
| `scripts/build-dmg.sh` | Legacy SDK/deployment-target matrix | Replace duplicated matrix logic with calls to the orchestrator and shared verifier |
| `scripts/build-dmg-externalqt-clang.sh` | Legacy external-Qt matrix | Reduce to a compatibility wrapper using the same pipeline |
| `scripts/nightly-build-osx.sh` | Build, rename, checksum, and upload | Consume only a verified artifact and abort on packaging failure |
| `.github/workflows/build-all.yml` | Universal CI build and release upload | Run account-free packaging and verification explicitly |
| `macdeployqt -dmg` | Historical alternative | Prohibit in the release path because it redeploys and mutates the bundle |
| Native UDRW to UDZO conversion | Current image backend | Keep only as a selectable fallback during migration |

There is no active CPack DragNDrop backend in the tree. Adding one would create a
second packaging implementation without solving a present requirement, so this
plan does not add it. If CPack packaging is introduced later, its output must
pass the same mounted-DMG verifier before publication.

## Goals

1. Make the no-account artifact structurally valid on both Apple Silicon and
   Intel macOS.
2. Ensure no file in the application changes after its final signature.
3. Detect signature, dependency, architecture, or image-integrity regressions in
   CI before upload.
4. Give local developers a packaging-only loop that reuses one compiled app.
5. Route all DMG creation and publication entry points through one contract.
6. Preserve existing output naming and drag-to-Applications behavior by default.
7. Keep Developer ID and notarization support optional, secure, and fail-closed.
8. Use only tools included with macOS and Qt for the account-free path.

## Non-Goals

- Obtaining Gatekeeper trust or notarization without Apple-issued credentials.
- Suppressing legitimate unidentified-developer warnings through unsupported
  metadata, quarantine manipulation, or user security-setting changes.
- Making DMG bytes reproducible across hosts. Filesystem UUIDs and timestamps can
  legitimately differ; content and signature invariants are what matter.
- Reworking Windows, Linux, or the 2.2.1 branch.
- Adding a third-party DMG layout dependency solely for Finder cosmetics.
- Shipping old release DMGs or downloaded binaries as test fixtures.

## Distribution Modes

The implementation will expose an explicit mode instead of inferring security
properties from whichever environment variables happen to exist.

| Mode | Account required | App signature | Hardened runtime | DMG signature | Notarization | Publication |
| --- | --- | --- | --- | --- | --- | --- |
| `adhoc` | No | Identity `-` | No | None | No | Default continuous and developer artifact |
| `developer-id` | Yes | Developer ID | Yes | Developer ID | No | Manual/internal only; must be labeled non-notarized |
| `notarized` | Yes | Developer ID | Yes | Developer ID | Required and stapled | Optional trusted release path |
| `unsigned` | No | None | No | None | No | Local diagnosis only; publication forbidden |

Rules:

- `adhoc` is the default when no mode is specified.
- `developer-id` and `notarized` require an explicit identity.
- `notarized` also requires a `notarytool` keychain profile or the complete set
  of supported credentials. Missing credentials are a hard error.
- `unsigned` must be explicitly requested and cannot produce a publishable DMG.
- The outer DMG is not ad-hoc signed. Such a signature adds no public trust and
  obscures the meaningful distinction between structural and identity trust.
- `spctl` rejection is expected in `adhoc` mode and is not treated as structural
  corruption. Strict `codesign` verification is still mandatory.

### Account-free method evaluation

| Method | Gatekeeper result | Structural result | Decision |
| --- | --- | --- | --- |
| Explicit ad-hoc signing (`codesign -s -`) | Untrusted identity; user override required | Valid code seal when performed last | Selected default |
| Completely unsigned ARM64 app | Untrusted and may be killed for invalid/missing code pages | Does not meet issue acceptance criteria | Reject for publication |
| Locally self-signed certificate | Untrusted until manually installed by each user | Valid only as a local identity | Reject as public default |
| Free Apple ID / Personal Team development certificate | Not a Developer ID distribution identity | Intended for personal development, with unsuitable lifetime and distribution semantics | Reject for releases |
| Third-party free signing service | Cannot independently mint Apple Developer ID trust | Adds custody and availability dependencies | Do not require |
| Instruct users to disable Gatekeeper globally | Avoids protection rather than fixing packaging | Hides invalid-signature defects | Explicitly reject |
| Apple Developer Program fee waiver | Can provide normal Apple credentials to an eligible organization | Same trusted path as credentialed mode | Organizational option, not a technical dependency |

Ad-hoc signing is selected because it is the only zero-account option that gives
the ARM64 code a consistent, locally verifiable seal without pretending to add
an Apple-trusted publisher identity.

## Packaging Invariants

Every implementation and caller must preserve these invariants:

1. The build phase does not create a DMG.
2. `macdeployqt` runs with `-no-codesign` and never with `-dmg`.
3. Qt deployment, install-name rewriting, translation generation, plug-in moves,
   pruning, and all other mutations complete before signing begins.
4. Built-in LibreCAD code resides below `Contents/PlugIns/LibreCAD`, Qt plug-ins
   remain in their normal `Contents/PlugIns` subdirectories, and data remains in
   `Contents/Resources`.
5. Every bundled Mach-O dependency resolves to the bundle, `/usr/lib`, or
   `/System/Library`; Homebrew, MacPorts, build-directory, and developer-machine
   absolute paths are forbidden.
6. Every bundled Mach-O contains the architecture set required by the main
   executable. CI currently requires `arm64` and `x86_64`.
7. Nested code is signed explicitly in deterministic inside-out order. The main
   application bundle is signed last.
8. No bundle mutation is allowed after the main application signature.
9. The application passes strict verification before DMG creation.
10. DMG construction treats the finalized application as read-only input and
    performs no deployment or signing.
11. A completed DMG is verified, mounted read-only, and its embedded application
    is subjected to the same strict checks.
12. Output creation is atomic. A failed run removes temporary files and cannot
    leave a stale path that appears successful.
13. Publication consumes the exact path and SHA-256 produced by the verifier.
14. Account-free and credentialed artifacts exercise the same deployment,
    staging, image, and verification code.

## Target Pipeline

```text
qmake/CMake build
    -> assemble raw LibreCAD.app
    -> macdeployqt -no-codesign (+ every extra executable)
    -> prune unused Qt components
    -> normalize built-in plug-in placement
    -> inspect architectures and Mach-O dependencies
    -> explicitly sign nested code inside-out
    -> sign LibreCAD.app last
    -> verify finalized app
    -> stage with ditto + Applications symlink
    -> create compressed DMG atomically
    -> hdiutil verify
    -> mount DMG read-only
    -> verify embedded app and layout
    -> optional Developer ID DMG sign/notarize/staple on candidate path
    -> final verification + SHA-256
    -> atomic rename to public output
    -> publish
```

The optional Developer ID operation signs the DMG before notarization. The
orchestrator keeps credentialed output under a private candidate name until all
post-staple checks pass; a failed notary operation therefore cannot leave a
release-looking output. No step after final app signing may reopen or alter the
embedded app.

## Shared Script Design

### 1. `scripts/package-osx-app.sh`

Add a focused helper that turns an already-built application into a finalized,
signed application. Proposed interface:

```text
scripts/package-osx-app.sh \
    --app LibreCAD.app \
    --qt-bin /path/to/qt/bin \
    --sign-mode adhoc|developer-id|notarized|unsigned \
    [--identity "Developer ID Application: ..."] \
    [--entitlements scripts/librecad-macos.entitlements]
```

Responsibilities:

- Validate all required tools and arguments before modifying the app.
- Run `macdeployqt` with `-no-codesign`, `-always-overwrite`, and
  `-executable=<path>` for `ttf2lff` and every built-in LibreCAD plug-in.
- Remove the known unused virtual-keyboard plug-in and only those frameworks
  proven orphaned after a dependency scan.
- Reject an app containing unresolved or external non-system load paths.
- Reject a thin nested Mach-O when the main executable is universal.
- Sign nested code and then the application according to the selected mode.
- Run application-level verification and emit a concise machine-readable report.
- Never create or alter a DMG.

The helper must use `set -euo pipefail`, targeted command logging, quoted arrays,
and a cleanup trap. It must not run globally with `set -x`.

### 2. `scripts/create-osx-dmg.sh`

Add a packaging-only helper. Proposed interface:

```text
scripts/create-osx-dmg.sh \
    --app LibreCAD.app \
    --output LibreCAD.dmg \
    [--volume-name LibreCAD] \
    [--backend direct|convert]
```

Responsibilities:

- Require the input application to pass app verification before staging.
- Create a private temporary staging directory and clean it on every exit path.
- Copy the app with `ditto` and create an `Applications -> /Applications`
  symlink.
- Refuse unexpected pre-existing entries in the staging directory.
- Write to a temporary output beside the requested destination and rename it
  only after complete verification.
- Never call `macdeployqt`, `codesign` on the app, or any build tool.

Backends:

- `direct` is the production default and uses `hdiutil create` with `HFS+` and
  `UDZO` directly.
- `convert` is a migration fallback. It creates a temporary UDRW image and
  converts it to a distinct temporary UDZO path. It must not use `-shadow` and
  must not convert onto the same path.
- Both backends produce the same logical volume layout and run the same final
  verifier.
- `macdeployqt -dmg` is deliberately unsupported because it combines deployment
  with image creation and can invalidate a finalized signature.

The direct backend is published after it passes CI. The convert backend remains
available for one release cycle as a diagnostic fallback, then can be removed if
no supported host needs it.

### DMG method evaluation

| Method | Bundle mutation risk | Extra dependency | Decision |
| --- | --- | --- | --- |
| `hdiutil create -format UDZO -srcfolder` | None when given finalized app | macOS only | Selected production backend |
| `hdiutil` UDRW then separate UDZO conversion | None when temporary paths are distinct | macOS only | Temporary fallback |
| `macdeployqt -dmg` | High because deployment and image creation are coupled | Qt | Prohibited for finalized release app |
| CPack DragNDrop | Separate staging semantics must be maintained | CMake/CPack | Defer until CMake owns the complete release pipeline |
| Third-party `create-dmg`, `appdmg`, or GUI automation | Varies; usually adds Finder-layout mutations | External tool/runtime | Reject unless a future visual-layout requirement justifies it |
| Manual Finder image editing | Unscripted post-sign and metadata changes | Interactive desktop | Reject for release artifacts |

All future image methods are acceptable only as backends of the packaging-only
contract: consume a verifier-approved app, do not deploy or sign the app, and
pass the same mounted-image verifier. The backend choice must never change the
security claims attached to the artifact.

### 3. `scripts/verify-osx-package.sh`

Add one verifier that accepts `--app` or `--dmg`, plus expected architectures and
distribution mode. It must be callable independently by developers, wrappers,
and CI.

Application checks:

- `plutil -lint` accepts `Contents/Info.plist`.
- The main executable and bundle identifier named by the plist exist.
- No built-in dylib remains under `Contents/Resources/plugins` after migration.
- Every regular Mach-O is inventoried without following external symlinks.
- `lipo -archs` contains each expected architecture for every required Mach-O.
- `otool -L` and LC_RPATH inspection reveal no unresolved, Homebrew, MacPorts,
  build-tree, or user-home dependency.
- Every nested code object passes strict verification.
- `codesign --verify --deep --strict --verbose=2` accepts the outer application
  in every publishable mode.
- The designated signing mode matches the observed signature. An ad-hoc app must
  not be reported as Developer ID signed.
- An opt-in command-line `--help` smoke run uses a disposable copy of the
  finalized bundle, so any application-side mutation cannot invalidate the
  source artifact. It is not part of the default noninteractive verifier path.

DMG checks:

- `hdiutil verify` accepts the image.
- The image attaches read-only at a private, explicit mount point.
- The root contains one expected `.app` and the `Applications` symlink, with no
  temporary or packaging-control files.
- The symlink resolves textually to `/Applications`.
- The mounted application passes all application checks again.
- A content manifest of regular files and symlink targets matches the finalized
  source application. Volatile filesystem metadata is excluded deliberately.
- Detachment occurs from a trap even when a later check fails.
- `notarized` mode additionally requires successful `stapler validate` and
  Gatekeeper assessment. `adhoc` mode records but does not require `spctl`
  acceptance.
- The verifier writes SHA-256 only after all checks pass.

Exit status is the public contract: zero means publishable for the requested
mode; nonzero means no caller may upload or rename the artifact as successful.

### 4. `scripts/build-osx.sh`

Retain the familiar entry point and output `LibreCAD.dmg`, but reduce it to:

1. Locate Qt and parse build options.
2. Build the application.
3. Call `package-osx-app.sh`.
4. Call `create-osx-dmg.sh`.
5. Call `verify-osx-package.sh` on the final image.
6. Perform optional DMG signing/notarization only through the explicit mode.

Compatibility requirements:

- Keep `-p`/`--qtpath`, `-q`/`-qmake_opts`, `--no-qtpath`, and `-cert` for one
  transition cycle.
- Map legacy `-cert` to `--sign-mode developer-id` and print one deprecation
  notice.
- Honor `MACOS_CODESIGN_IDENTITY`; accept `CODESIGN_IDENTITY` as a compatibility
  fallback rather than unconditionally erasing it.
- Add `--skip-build` for local packaging iterations against an existing app.
- Add `--dmg-backend direct|convert` for backend validation.
- Remove the obsolete Qt-version gate and stale `macdeployqt -codesign` comments.
- Do not run `make distclean` in `--skip-build` mode.

## Bundle Layout and Plug-In Loading

Move built-in LibreCAD plug-ins to the standard nested-code location:

```text
LibreCAD.app/
  Contents/
    MacOS/LibreCAD
    MacOS/ttf2lff
    Frameworks/...
    PlugIns/
      LibreCAD/libalign.dylib
      LibreCAD/libasciifile.dylib
      ...
      platforms/...
      imageformats/...
    Resources/...
```

Required source changes:

- Update all ten plug-in `.pro` files from
  `Contents/Resources/plugins` to `Contents/PlugIns/LibreCAD` on macOS.
- Update all ten plug-in CMake install rules with an Apple-specific bundle
  destination while preserving the current non-Apple destination.
- Add `Contents/PlugIns/LibreCAD` to the macOS plug-in search paths in
  `RS_System::getDirectoryList("plugins")`.
- Retain the legacy `Contents/Resources/plugins` lookup for one transition cycle
  so developer-created old bundles remain usable, but do not package new plug-ins
  there.
- Keep Documents and `~/.librecad/plugins` support unchanged.
- Pass each built-in plug-in to `macdeployqt -executable` so its Qt linkage is
  rewritten and its framework dependencies are copied before signing.

For notarized builds, LibreCAD remains a third-party plug-in host. The main
executable may need the narrowly scoped
`com.apple.security.cs.disable-library-validation` entitlement to load existing
user-installed plug-ins that are unsigned or signed by another team. Before
enabling the entitlement, add a notarized-build smoke test with one external
sample plug-in. Apply the entitlement only to the host application, document the
security tradeoff, and do not give it to bundled plug-ins.

## Explicit Signing Algorithm

The signing helper will build an inventory of nested code and sign in descending
path depth, with stable path ordering inside a depth:

1. Nested dylibs or helpers inside frameworks.
2. Framework bundles.
3. Qt plug-in dylibs and LibreCAD built-in plug-ins.
4. Standalone helper executables such as `ttf2lff`.
5. The outer `LibreCAD.app` bundle.

The exact inventory must come from Mach-O inspection plus known bundle
containers, not only filename extensions. Symlinks are never signed as files.

Ad-hoc mode uses identity `-`, no secure timestamp, and no hardened-runtime flag.
Developer ID modes use `--options runtime`, a secure timestamp, and the selected
identity for every nested object. `--deep` is forbidden on signing commands and
retained only for final verification.

After the outer app is signed, the script records a manifest. Any subsequent app
verification with different file contents is a hard failure. Image staging may
copy the app but may not modify it.

## Optional Notarized Path

The account-free path is sufficient to fix structural corruption, but the shared
pipeline must preserve a clean route to trusted distribution:

1. Require `--sign-mode notarized` explicitly.
2. Require a Developer ID Application identity before deployment starts.
3. Sign all nested code and the app with hardened runtime and secure timestamps.
4. Sign the finalized DMG with the same identity.
5. Submit the DMG using `xcrun notarytool --wait`.
6. Prefer `NOTARIZE_KEYCHAIN_PROFILE` so secrets are not passed on command lines.
7. Support individual credential variables only in a tracing-disabled subshell.
8. Staple the accepted ticket to the DMG.
9. Repeat `hdiutil verify` and mounted-app verification after the DMG has changed.
10. Run `stapler validate`, strict signature verification, and Gatekeeper
    assessment before publication.
11. Rename the private candidate to the public output only after all checks pass.

`developer-id` mode follows the same candidate-file rule, signs the outer DMG,
and repeats image verification, but deliberately skips submission and stapling.

No release mode may silently degrade from `notarized` to `developer-id` or
`adhoc`. A missing credential, rejected submission, failed staple, or failed
assessment terminates the build and removes the candidate output.

## Caller Migration

### GitHub Actions

Update `.github/workflows/build-all.yml` to:

- Build the universal application once.
- Invoke account-free `adhoc` mode explicitly.
- Pass expected architectures `arm64,x86_64` to the verifier.
- Verify every bundled Mach-O, not only the main executable.
- Package the direct backend for publication.
- In an initial non-publishing validation step, reuse a copy of the same finalized
  app to exercise the convert backend without rebuilding.
- Upload the DMG, SHA-256 file, and concise verification report together.
- Name and move the artifact only after verification succeeds.
- Keep signing/notary secrets entirely optional; absence of a paid account must
  not fail the default job.

### Legacy matrix scripts

Replace the hard-coded Lion-through-Sierra arrays in `scripts/build-dmg.sh` and
`scripts/build-dmg-externalqt-clang.sh`. Those SDK names are not an appropriate
source of truth on current Xcode installations.

The retained wrappers should accept explicit deployment target values, call
`build-osx.sh` once per requested target, and verify the resulting named image.
With no target list, they should build one package using the project default.
The external-Qt wrapper should differ only in Qt discovery options.

### Nightly build

Update `scripts/nightly-build-osx.sh` to consume the verifier-generated checksum,
refuse an unverified image, and avoid generating checksums from whatever stale
file happens to occupy `LibreCAD.dmg`. Upload remains outside the shared packager.

### Documentation

Update `README.md` and script help text to distinguish these outcomes:

- The application and DMG have passed structural integrity checks.
- The default build is ad-hoc signed and is not notarized.
- First launch may require right-click **Open**.
- If quarantine override still fails, the documented fallback is
  `xattr -dr com.apple.quarantine /Applications/LibreCAD.app` after the user has
  verified the published SHA-256.
- A "damaged" error accompanied by a failing strict signature check is a package
  defect and should be reported; users should not be told to bypass a truly
  invalid signature.

## Implementation Phases and Commits

Each phase should be a reviewable commit. The plan file is updated after every
commit by deleting completed work from the active checklist and recording the
commit hash in the progress table.

### Phase 0: Baseline and reproducible checks

Changes:

- Capture current app and DMG verification commands in a temporary developer
  test script or CI log, without committing external binary fixtures.
- Confirm the existing universal build's executable, plug-in, framework, and
  helper architecture inventory.
- Record current `otool -L` exceptions produced by Qt itself.

Acceptance:

- One locally built app can be copied and tested repeatedly without recompiling.
- Deliberately changing one sealed resource makes the verification command fail.
- The baseline evidence distinguishes DMG verification from app verification.

### Phase 1: Add the independent verifier

Changes:

- Add `scripts/verify-osx-package.sh` with app and mounted-DMG modes.
- Add shell syntax and negative tamper tests.
- Run it against the current output before changing packaging behavior.

Acceptance:

- It accepts a correctly ad-hoc-signed local app.
- It rejects a stale signature, thin nested code, forbidden load path, invalid
  plist, malformed DMG, wrong `Applications` symlink, or failed mount cleanup.
- It explains the failing invariant in one concise diagnostic.

### Phase 2: Separate deployment and sign inside-out

Changes:

- Add `scripts/package-osx-app.sh`.
- Make `macdeployqt -no-codesign` and `-executable` coverage explicit.
- Move pruning ahead of all signing.
- Replace signing-time `--deep` with explicit nested signing.
- Add explicit distribution modes and secure credential handling.

Acceptance:

- Ad-hoc packaging needs no account, certificate, keychain setup, or network.
- Strict app verification passes after deployment and pruning.
- A filesystem manifest confirms nothing changes after app signing.
- Developer ID mode cannot accidentally fall back to ad-hoc.

### Phase 3: Normalize plug-in layout

Changes:

- Update qmake and CMake destinations for all ten built-in plug-ins.
- Update macOS runtime discovery with a temporary legacy fallback.
- Verify all plug-in install names through `macdeployqt -executable`.

Acceptance:

- All built-in plug-ins are found and loaded from `Contents/PlugIns/LibreCAD`.
- No packaged executable code remains under `Contents/Resources/plugins`.
- A bundle moved to another directory has no package-manager Qt dependencies.
- User plug-in directories behave as before in ad-hoc mode.

### Phase 4: Replace DMG creation

Changes:

- Add `scripts/create-osx-dmg.sh` with direct and convert backends.
- Use `ditto`, atomic output, private staging, and cleanup traps.
- Remove the embedded UDRW/shadow conversion block from `build-osx.sh`.

Acceptance:

- Direct and convert images have equivalent root layouts.
- Both pass `hdiutil verify` and mounted application verification.
- Failure cannot leave a newly timestamped but unverified output at the requested
  path.
- Paths containing spaces work without word splitting.

### Phase 5: Migrate every caller

Changes:

- Reduce `build-osx.sh` to orchestration and add `--skip-build`.
- Update both legacy DMG matrix wrappers.
- Update nightly packaging and checksum handling.
- Update GitHub Actions to publish only verifier-approved artifacts.

Acceptance:

- `scripts/build-osx.sh` still creates `LibreCAD.dmg` by default.
- A packaging-only rerun does not invoke qmake or make.
- CI compiles once while testing both DMG backends.
- No caller contains its own `hdiutil create`, `macdeployqt -dmg`, or app-signing
  sequence.

### Phase 6: Validate optional notarization

Changes:

- Add the minimal host entitlement only if the external plug-in test proves it
  necessary.
- Make notarization and stapling a strict explicit mode.
- Add post-staple validation and mode-aware Gatekeeper checks.

Acceptance:

- This phase can be skipped with no effect on account-free CI.
- When credentials are available, the mounted app loads bundled plug-ins and the
  DMG passes notarization, stapling, and Gatekeeper assessment.
- Missing or rejected credentials never produce a mislabeled release artifact.

### Phase 7: Documentation and cleanup

Changes:

- Update user-facing installation guidance and developer packaging help.
- Remove obsolete Qt signing checks, stale comments, duplicate matrix logic, and
  the convert backend after its agreed migration window if no longer needed.
- Document how to run a packaging-only verification loop.

Acceptance:

- Documentation states both what the free method guarantees and what it cannot
  guarantee.
- Every script has one current usage description and a GPL v2-or-later header.
- Repository search finds no undocumented DMG creation path.

## Test Strategy

### Fast local loop

Build only once. Preserve an unsigned or pre-deployment app copy in a temporary
directory, and use `ditto` to make a fresh working copy for each deployment/sign
test. Once one finalized app passes verification, run both image backends from
that same app. This makes script iterations seconds-to-minutes instead of full
rebuilds.

Fast checks after each script edit:

```text
bash -n scripts/build-osx.sh
bash -n scripts/package-osx-app.sh
bash -n scripts/create-osx-dmg.sh
bash -n scripts/verify-osx-package.sh
scripts/verify-osx-package.sh --app /tmp/LibreCAD-final.app --mode adhoc
scripts/create-osx-dmg.sh --app /tmp/LibreCAD-final.app --output /tmp/direct.dmg
scripts/verify-osx-package.sh --dmg /tmp/direct.dmg --mode adhoc
```

Run `shellcheck` when installed, but do not make an unavailable optional tool a
developer-build blocker.

### Negative tests

Use generated temporary copies, never committed binary fixtures:

- Modify one resource after signing and require strict verification to fail.
- Modify one plug-in after signing and require nested verification to fail.
- Inject a thin test Mach-O into a universal app and require architecture failure.
- Rewrite a copied load command to a temporary absolute path and require
  dependency failure.
- Replace the `Applications` symlink with a regular file or wrong target.
- Truncate a copied DMG and require `hdiutil verify` to fail.
- Pre-create a stale destination and force image creation to fail; confirm the
  stale file is not presented as a new verified artifact.
- Exercise app, staging, mount, and output paths containing spaces.
- Interrupt image creation and confirm traps detach mounts and remove staging.

### Functional tests

- Launch the copied application on Apple Silicon after the documented no-account
  first-launch override.
- Launch the x86_64 slice under Rosetta where CI or a maintainer host supports it.
- Open the Plug-ins dialog and confirm all ten built-in plug-ins load without
  invalid-metadata or library-validation errors.
- Run `ttf2lff --help` or another non-mutating command to prove its bundled Qt
  dependencies resolve.
- Open and save a small locally generated DXF to verify that the packaged app is
  functionally usable, without adding a binary fixture to git.
- Mount, copy the app to `/Applications` or a temporary Applications-equivalent
  directory, detach the image, and launch the copied app.

### CI matrix

| Dimension | Required coverage |
| --- | --- |
| Build system | qmake release path; CMake bundle/install layout validation |
| Architecture | Universal `arm64` + `x86_64`; every nested Mach-O checked |
| Signing | `adhoc` on every run; `notarized` only when protected credentials exist |
| DMG backend | `direct` required and published; `convert` temporary validation |
| Source path | Normal checkout and a temporary path containing spaces |
| Failure behavior | Tamper test and stale-output test |

Do not rebuild LibreCAD separately for each row. Use copies of one build product
for packaging variations, and keep slow GUI launch tests out of the default fast
script test unless a macOS runner is available.

## File-Level Change Map

| File or group | Planned change |
| --- | --- |
| `scripts/package-osx-app.sh` | New deployment, pruning, signing, and app-verification helper |
| `scripts/create-osx-dmg.sh` | New atomic direct/fallback DMG builder |
| `scripts/verify-osx-package.sh` | New reusable app and mounted-DMG verifier |
| `scripts/librecad-macos.entitlements` | New only if external plug-in validation requires it |
| `scripts/build-osx.sh` | Convert to compatible orchestrator; add explicit modes and `--skip-build` |
| `scripts/build-dmg.sh` | Remove stale SDK matrix assumptions and delegate |
| `scripts/build-dmg-externalqt-clang.sh` | Make a thin Qt-discovery compatibility wrapper |
| `scripts/nightly-build-osx.sh` | Upload only verified artifact and verifier checksum |
| `.github/workflows/build-all.yml` | Account-free publish path, nested-arch checks, reports, backend reuse |
| Ten `plugins/*/*.pro` files | Install built-in plug-ins below `Contents/PlugIns/LibreCAD` |
| Ten `plugins/*/CMakeLists.txt` files | Match the Apple bundle destination without changing other platforms |
| `librecad/src/lib/engine/rs_system.cpp` | Search new built-in macOS plug-in path, retain temporary legacy fallback |
| `scripts/postprocess-osx.sh` | Ensure all resource generation remains before signing; no signing or DMG logic |
| `README.md` | Explain structural validity, ad-hoc trust limits, checksum, and first launch |

Every newly added source, script, or documentation file must carry the LibreCAD
GPL v2-or-later notice and copyright lines following the style of
`lc_hyperbola.h`.

## Review Gates

### Gate A: Account-free correctness

- No certificate, Apple ID, app-specific password, paid service, or network is
  required.
- Strict verification passes for source and mounted applications.
- Fresh Apple Silicon launch no longer terminates with an invalid code-signature
  page after the documented Gatekeeper override.

### Gate B: Bundle completeness

- Every nested Mach-O has required architectures and closed dependencies.
- Built-in plug-ins load from their standard location.
- No post-sign mutation is possible in the scripted flow.

### Gate C: Image correctness

- The image verifies, mounts read-only, has the exact expected layout, and
  preserves the app content manifest.
- Both native backends agree logically; only one is published.
- Failure and interruption leave no misleading output or mounted volume.

### Gate D: Caller convergence

- Repository search identifies a single implementation of deployment, signing,
  DMG creation, and package verification.
- Every wrapper, nightly job, and GitHub job delegates to it.

### Gate E: Optional trust path

- Notarized mode is explicit and fail-closed.
- Secrets are not printed.
- Hardened runtime does not break supported external plug-ins, or the limitation
  is resolved and documented before that mode is published.

## Risks and Mitigations

| Risk | Mitigation |
| --- | --- |
| Users interpret ad-hoc signing as notarization | Make mode and first-launch behavior explicit in artifact notes and README |
| Plug-in relocation breaks discovery | Add new path before packaging changes, retain old lookup temporarily, and test all built-ins |
| Hardened runtime blocks user plug-ins | Test an external plug-in and use the host-only library-validation entitlement only when required |
| `macdeployqt` misses a helper dependency | Pass every helper/plug-in through `-executable` and reject external load paths afterward |
| Qt adds new optional frameworks | Prune only after dependency analysis and verify closure after pruning |
| Direct UDZO differs on an older macOS host | Keep the separate UDRW-to-UDZO fallback for one migration cycle |
| Mount remains after a failed test | Install detach and directory cleanup traps before attachment |
| Old DMG is uploaded after a failure | Build to a temporary name, remove candidate on failure, and publish verifier output only |
| Script logs expose notary credentials | Remove global tracing and prefer a keychain profile |
| Full CI becomes too slow | Compile once and copy the built/finalized app for backend and negative tests |

## Definition of Done

The issue is fixed when all of the following are true:

1. The default GitHub macOS job creates a universal DMG with no paid account.
2. The embedded app is ad-hoc signed only after all deployment mutations.
3. Source-app and mounted-app strict signature verification both pass.
4. Every bundled Mach-O has `arm64` and `x86_64` slices and no external package
   manager or build-tree dependencies.
5. All built-in plug-ins load from `Contents/PlugIns/LibreCAD`.
6. A clean Apple Silicon machine launches the app after the documented
   unidentified-developer override without a `Namespace CODESIGNING` crash.
7. The direct DMG backend passes image, layout, manifest, and cleanup checks.
8. Every DMG-producing or publishing script delegates to the shared helpers.
9. CI publishes the DMG only after verifier success and includes its SHA-256.
10. The README accurately explains that account-free artifacts are structurally
    valid but neither Developer ID signed nor notarized.
11. Optional notarized mode, when credentials are supplied, fails closed and
    passes signing, stapling, and Gatekeeper checks.
12. No external DMG/app fixture or paid third-party packaging dependency is added
    to the repository.

## Reference Oracles

- [Apple: Developer ID](https://developer.apple.com/developer-id/)
- [Apple: Notarizing macOS software before distribution](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
- [Apple: Creating distribution-signed code for the Mac](https://developer.apple.com/documentation/xcode/creating-distribution-signed-code-for-the-mac)
- [Apple: Placing content in a bundle](https://developer.apple.com/documentation/bundleresources/placing-content-in-a-bundle)
- [Apple: Disable Library Validation entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.cs.disable-library-validation)
- [Qt 6: macOS deployment](https://doc.qt.io/qt-6/macos-deployment.html)

These references define the signing, bundle-layout, hardened-runtime, and Qt
deployment constraints. LibreCAD's independent verifier remains the executable
oracle for every artifact produced by this plan.
