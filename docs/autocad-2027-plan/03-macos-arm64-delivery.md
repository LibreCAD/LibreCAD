<!--
********************************************************************************
This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2026 LibreCAD.org

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

# macOS arm64 Delivery Plan

## Research baseline

| Field | Value |
| --- | --- |
| Repository revision inspected | `628a1ba845931a37b8d745a8ad08d4731615812c` (2026-10-10) |
| Research date | 2026-10-11 |
| Product target | A native Apple Silicon LibreCAD first release with a Windows-style AutoCAD 2027-inspired 2D drafting interface adapted to macOS, supporting DWG and DXF |
| Evidence status | Source and official documentation reviewed; no new build was run and no successful build is claimed here |

This plan is for native macOS delivery and runtime validation. The first release requires arm64; preserve the current universal pipeline as a separate optional future artifact. The requested UI direction is AutoCAD 2027's Windows-style 2D drafting experience adapted to macOS conventions, with DWG and DXF in scope. The AutoCAD 2027 requirements page names Tahoe 26, Sequoia 15, and Sonoma 14, supports Intel and M-series CPUs, and lists Retina/high-resolution display support. Use that as a comparison point for the requested user experience, not as proof of compatibility or as permission to copy Autodesk product assets. [Autodesk AutoCAD 2027 for Mac requirements](https://www.autodesk.com/support/technical/article/caas/sfdcarticles/sfdcarticles/System-requirements-for-AutoCAD-2027-including-Specialized-Toolsets.html)

### What the checked-in source actually establishes

- The application is a C++17 Qt Widgets application. The primary CMake project requires CMake 3.28, finds Qt 6 Gui/Core/Widgets/PrintSupport/Svg/Network, and creates the `librecad` target. The qmake project (`librecad.pro`, `librecad/src/src.pro`) remains the macOS release entry point. See `CMakeLists.txt:1-4,55-56,2310-2343`, `librecad.pro`, and `librecad/src/src.pro:60-83`.
- `pixi.toml` includes `osx-arm64`, requests Qt 6 `>=6.8.1,<7`, Boost headers, FreeType, compilers, Ninja, and CMake, and provides a CMake/Ninja install task. The checked-in `pixi.lock` resolves its macOS arm64 environment to Qt 6.8.2. This is a dependency-resolution record, not a successful app build or a deployment recipe. See `pixi.toml:1-34` and `pixi.lock` entries for `osx-arm64/qt6-main-6.8.2`.
- The macOS qmake path makes a `.app`, sets its icon/Info.plist, and has an Apple Silicon Qt 6 compatibility prefix header (`librecad/src/mac_arm_acle_prefix.h`). The CMake path also makes `LibreCAD.app` and uses the shared plist template. These are real app-bundle starting points; CMake and qmake packaging still need to stay aligned.
- `.github/workflows/build-all.yml:BuildMacOS` uses `macos-14`, Qt `6.9.*` `clang_64`, a universal FreeType helper, and `QMAKE_APPLE_DEVICE_ARCHS=arm64 x86_64`. It publishes a universal DMG and calls `test-osx-package.sh`. This is an existing universal route, not an arm64-native release lane and not a record of an arm64-only GUI run.
- `scripts/build-osx.sh`, `package-osx-app.sh`, `create-osx-dmg.sh`, and `verify-osx-package.sh` implement the current packaging contract. The helpers deploy Qt, move built-in Mach-O plug-ins from legacy `Contents/Resources/plugins` into `Contents/PlugIns/LibreCAD`, prune unusable Qt plugins, remove external RPATHs, sign nested code inside-out, create the DMG, and re-check the mounted image. The verifier checks bundle plist/executable, every Mach-O's architecture membership, dependency resolution, RPATHs, signatures, and image contents. `--architectures arm64` means every Mach-O must include arm64; the current verifier does not assert that no extra x86_64 slice exists. See `scripts/package-osx-app.sh:242-255,445-491`, `scripts/verify-osx-package.sh:557-700,705-819`, and `scripts/test-osx-package.sh:96-166`.
- `docs/plan_macos_dmg_integrity.md` describes an implemented packaging design, but its listed verification evidence is not re-established by reading it. For this plan, packaging behavior and verifier acceptance above were checked against the scripts at the repository revision in this document. At this revision the workflow remains universal and its macOS runner invokes package tests, but no macOS arm64-only end-to-end artifact or interactive GUI acceptance result is established here.

## Recommendation

Keep the existing Qt Widgets/C++ application and improve it in place. Do not rewrite the application in a different UI framework as a prerequisite for Mac resemblance. LibreCAD already has a desktop CAD document/view model, dock widgets, command input, menus, printing, SVG, and mouse/keyboard interaction in Qt. A rewrite would replace the drawing, selection, undo, plug-in, settings, and export paths along with the UI, expanding schedule and file-compatibility risk without improving Apple Silicon delivery by itself.

Build the requested Windows-style AutoCAD 2027-inspired 2D drafting presentation around the shared CAD core, then adapt platform edges to macOS. Workspace organization, command and tool discovery, document lifecycle, properties/layers, and a ribbon/tool presentation belong to the product UI roadmap; native menu placement, Command-key conventions, Retina rendering, trackpad/mouse behavior, accessible focus, and window lifecycle need Mac-specific adaptation. Preserve Qt Widgets as the UI toolkit while testing whether Qt's macOS platform integration supplies the expected native behavior. Qt documents qmake app bundles and `macdeployqt` as supported deployment paths for frameworks, plugins, translations, and other resources. [Qt for macOS, 6.8](https://doc.qt.io/qt-6.8/macos.html) [Qt macOS deployment, 6.8](https://doc.qt.io/qt-6.8/macos-deployment.html)

Use AutoCAD 2027 as an interaction reference, then implement independently with LibreCAD's own code, original icons/art, and licensed resources. Do not bundle Autodesk marks, iconography, screenshots, fonts, or other proprietary content. The repository's `LICENSE` documents GPLv2 and separate notices for bundled icon/font assets; keep those notices and licenses in any redistributed app bundle and audit all newly added visual assets before inclusion.

## Build profiles and toolchain policy

Define two separately named release profiles:

| Profile | Compile slices | Purpose | Acceptance |
| --- | --- | --- | --- |
| `macos-arm64` | `arm64` | Required native Apple Silicon first-release artifact | Main app, plug-ins, Qt frameworks/plugins, helper executables, and all bundled Mach-O files contain arm64; test on a physical Apple Silicon Mac |
| `macos-universal` | `arm64` + `x86_64` | Preserve the existing universal release pipeline and artifacts as a separate product; it does not gate the native arm64 first-release artifact | Every bundled Mach-O contains both slices; test the native arm64 run and an x86_64 run under Rosetta on Apple Silicon, plus a real Intel Mac before claiming Intel support |

Make the arm64 first release independent of the universal CI job, and preserve the existing universal job as an optional separate artifact. Apple's universal-binary guidance explicitly requires universal slices for all compiled dependencies, including plug-ins, libraries, frameworks, and command-line tools. Qt's macOS architecture settings document the same arm64/x86_64 distinction. [Apple: Building a universal macOS binary](https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary) [Qt for macOS architectures, 6.8](https://doc.qt.io/qt-6.8/macos.html)

For the required arm64 lane, use a native Apple Silicon runner/machine. The app executable and LibreCAD-built plug-ins/helpers must contain arm64; third-party libraries/frameworks may be arm64-only or universal provided every packaged Mach-O contains arm64 and resolves inside the bundle. Keep the existing universal path and artifacts intact as a separately named profile. Prefer one build system per shipped artifact: the least disruptive initial release lane is the existing qmake macOS orchestrator because it already owns `macdeployqt` and DMG packaging; separately bring CMake parity up to a passing standard before making CMake the canonical macOS release route. The shipped arm64 profile must retain DWG and DXF support; CMake's application target currently defines `DWGSUPPORT`, so verify qmake parity rather than inferring it. Avoid mixing Qt installations, Homebrew dependencies, and pixi dependencies in one artifact. Record the provenance of Qt, compiler/Xcode, SDK, Boost, FreeType, and other linked libraries in build logs/manifests.

Pin an exact tested Qt patch, Xcode/Command Line Tools image, compiler, dependency lock, and `MACOSX_DEPLOYMENT_TARGET` in the arm64 release profile. Do not pin to an untested number in source or declare it supported until the arm64 build and runtime matrix pass. A reasonable initial product floor is macOS 14 Sonoma because it overlaps all three macOS versions listed for AutoCAD 2027 and is supported by Qt 6.8's documented macOS target range (Qt 6.8 lists macOS 12+ with Xcode 15/macOS 14 SDK); validate the exact chosen Qt release and all dependency deployment floors before adopting it. Keep the floor in one release-profile setting and pass it to qmake as `QMAKE_MACOSX_DEPLOYMENT_TARGET` and to CMake as `CMAKE_OSX_DEPLOYMENT_TARGET` if both build systems ship. Qt warns that lowering its default deployment floor can cause runtime failures. [Qt 6.8 supported platforms](https://doc.qt.io/qt-6.8/supported-platforms.html) [Qt macOS deployment targets, 6.8](https://doc.qt.io/qt-6.8/macos.html)

Required build checks before changing the profile from planned to supported:

1. Configure/compile/link arm64 from a clean checkout; record exact tools and package versions. This plan does not assert that it currently succeeds.
2. Inspect all app-bundle Mach-O files with `file`/`lipo -archs`; fail if any app, plug-in, helper, or dependency lacks arm64. The existing contains-requested-slices verifier semantics are appropriate: universal third-party binaries are valid in the arm64 product. An optional exact-slice audit may inform artifact-size optimization and labeling, but extra x86_64 slices are not an arm64 release failure.
3. Inspect each load dependency with `otool -L`, RPATHs with `otool -l`, and validate that Qt frameworks/plugins and LibreCAD plug-ins resolve from the app bundle rather than Homebrew, pixi, the build tree, or a developer machine.
4. Run the existing verifier on the `.app`, build and verify the DMG, mount and verify the app copied into the image, check SHA-256, and confirm a clean install/launch after quarantine on an Apple Silicon machine. Treat interactive runtime smoke for core 2D drafting plus DWG/DXF open/save as a release-support condition; a structurally valid bundle alone is not a supported release.
5. Keep CMake and qmake bundle metadata, resource install, built-in plug-in locations, and architecture acceptance aligned. A packaging fix in one path does not prove the other path.

## Native interaction and display work

Address these in the product UI roadmap, alongside—not as a condition for—the first arm64 packaging milestone:

- **Menus and shortcuts:** audit and extend the existing `LC_ShortcutsManager`/shortcut storage and `lc_actionfactory.cpp` default mappings; do not add a competing shortcut registry. Define macOS `Command` mappings for conventional app/file/edit actions without colliding with menu defaults; preserve the separate CAD command aliases handled by `RS_Commands`. Review menu placement, standard About/Preferences/Quit items, menu focus, keyboard-only completion, and customized shortcut migration. Current shortcut implementation is in `librecad/src/ui/actions/lc_shortcuts_manager.cpp:37-114,196`, `librecad/src/ui/main/init/lc_actionfactory.cpp:897-938`, and CAD command aliases in `librecad/src/cmd/rs_commands.cpp`.
- **Retina:** preserve Qt device-independent geometry, test 1x/2x and mixed-scale displays, ensure toolbar/cursor/icon assets stay crisp, check selection handles/crosshair/line weights at scale changes, and avoid multiplying logical dimensions by the device-pixel ratio twice. Existing renderer code obtains `devicePixelRatioF()` and sizes backing pixmaps in physical pixels; see `lc_widgetviewportrenderer.cpp:129-146,191-202` and `rs_painter.cpp:224`.
- **Mouse and trackpad:** exercise precise trackpad `pixelDelta`, wheel `angleDelta`, horizontal pan, pinch/zoom if supported, middle-button pan, two-button mice, Magic Mouse, and conventional wheel mice. `QG_GraphicView::wheelEvent` already handles pixel and angle deltas (`qg_graphicview.cpp:880-1033`); build interaction tests around the current semantics before adjusting them.
- **Accessibility and density:** test VoiceOver reachability of dialogs, meaningful accessible names for canvas controls and dock panels, full keyboard flow, high contrast/system appearance, larger text, and usable minimum toolbar/panel sizes. CAD canvas actions need keyboard alternatives and should not communicate state only by color.
- **Window/session behavior:** validate open/save dialogs, multiple documents, unsaved-close prompts, reopen behavior, file association/open-with, recent files, drag/drop, clipboard, print preview/export, and full-screen/Spaces. Distinguish ordinary document-window behavior from CAD interaction features.

Do not require pixel-identical screenshots to AutoCAD. Review common tasks on both products using the same drawing: create/edit/select geometry, layer operations, dimensions, command entry, pan/zoom, properties, and print/export. Judge familiarity through task completion, discoverability, shortcut consistency, and predictable Mac window behavior.

The interaction and accessibility pass is a release condition for claiming the Mac-native UI work is complete. It does not block the arm64 packaging milestone; each supported release still requires the core native runtime smoke for 2D drafting and DWG/DXF described above.

## Bundle, plug-ins, assets, and licensing

Continue to produce a self-contained `LibreCAD.app`; Qt's macOS guidance describes bundling Qt libraries as private frameworks and deploying required Qt plugins. The current package helper discovers extra executables for deployment, normalizes built-in plug-ins into `Contents/PlugIns/LibreCAD`, prunes unavailable Qt plugin dependencies, strips external RPATHs, and signs after these mutations. Preserve this ordering and verifier contract when adding or changing bundle contents. [Qt macOS deployment](https://doc.qt.io/qt-6.8/macos-deployment.html)

For each new bundled component, record origin, version, license, architecture, and dynamic dependencies. Maintain third-party license notices in the app's About/Notices location or a documented bundled notices file. Keep LibreCAD's GPLv2 notice and relevant component notices, including font licenses, with redistributed binaries. Use only original LibreCAD assets or assets with explicit compatible redistribution rights; do not reuse AutoCAD logos or artwork to imply affiliation.

## Signing and distribution state

The code already has four named modes: `adhoc`, `developer-id`, `notarized`, and `unsigned`. The default build script uses ad-hoc signing, and its help explicitly says that mode does not confer Developer ID trust or notarization. Developer ID/notarized modes require an available Developer ID Application identity; notarized mode requires credentials/profile and calls `xcrun notarytool submit`, then staples. The verifier distinguishes structural signature validity from Gatekeeper acceptance. These are implemented script paths, but the plan's documentary existence does not establish that credentialed notarization has succeeded on this revision.

Keep ad-hoc signing for account-free local/CI structural artifacts. For a public download intended to pass normal Gatekeeper checks, use a protected release job with Developer ID signing, hardened runtime, secure timestamp, Apple notarization, stapling, and post-staple Gatekeeper verification. Keep credentials out of pull-request jobs and logs. Apple's notary service requires Developer ID-signed code and provides a ticket; ad-hoc signing does not provide that trust. [Apple: Notarizing macOS software](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution) [Apple: Distribution](https://developer.apple.com/documentation/technologyoverviews/distribution)

Use only the existing DMG builder/verifier route until another backend is deliberately adopted. Do not add a second packaging path that skips `verify-osx-package.sh`. Make the release label explicit about architecture (`arm64` vs `universal`) and signing state (`adhoc` vs `notarized`); do not call an ad-hoc image notarized or Gatekeeper trusted.

## Roadmap, dependencies, and effort estimates

Ranges below are engineering estimates for an experienced maintainer, excluding Apple credential procurement, release waiting time, product-wide AutoCAD-parity work, and defects uncovered by user research. They are estimates, not commitments.

| Phase | Work and exit evidence | Dependencies | Estimate |
| --- | --- | --- | --- |
| 0. Profile and machine | Select supported macOS floor, native arm64 runner, exact Qt/Xcode/SDK/compiler and package source; preserve the universal profile | Access to Apple Silicon CI runner or maintainer Mac; dependency owners | 1–3 engineer-days |
| 1. Arm64 first-release lane | Add required named native arm64 build artifact; clean configure/build; record versions; inspect every Mach-O and dependency; prove every bundled component includes arm64 while permitting universal third-party binaries; ensure DWG and DXF remain enabled | Phase 0; qmake/CMake release entry point decision | 3–6 engineer-days |
| 2. Bundle parity | Verify Qt plugins, 10 built-in plug-ins, auxiliary executables, fonts/resources, install locations, Info.plist, licenses, signing and mounted DMG for arm64; align CMake/qmake where needed | Phase 1; current package helper contract | 3–7 engineer-days |
| 3. Native Mac smoke | Launch/install on Apple Silicon; exercise DWG and DXF open/save, command input, 2D drawing/editing, layers, dimensions, printing/export, plug-ins and device/display interactions | Phase 2; physical Mac and signed/quarantined artifact | 3–6 engineer-days |
| 4. Native UI pass | Implement and review macOS menus, shortcuts, display scaling, mouse/trackpad, keyboard focus and accessibility findings | Phase 3 plus design/workflow priorities from the main plan | 2–5 weeks, depending on UI scope |
| 5. Public release trust | Run protected Developer ID + notarize/staple + Gatekeeper verification, publish both artifact metadata and checksums; retain ad-hoc CI artifacts as distinct | Signing team identity, notary profile, secret-safe release runner, Phase 2 | 2–4 engineer-days plus external review latency |

## Runtime validation matrix

| Environment | Required checks |
| --- | --- |
| Apple Silicon, native arm64 artifact | Install from downloaded DMG with quarantine present; first launch; About reports correct version; verify Activity Monitor/`file`/`lipo` shows native arm64 execution and all packaged code includes arm64; open/save representative DWG and DXF; edit entities, layers, dimensions; command input and shortcuts; pan/zoom via trackpad and mice; print/PDF export; plugin load; relaunch and reopen; Retina and mixed-scale display; VoiceOver/keyboard pass |
| Apple Silicon, universal artifact | Repeat critical task flow once natively and once forced to run as Intel under Rosetta; confirm all bundled code contains both required slices |
| Intel Mac, only while supported | Install and run the universal artifact on real Intel hardware; confirm x86_64 execution and core workflows. A Rosetta run on Apple Silicon is supplemental, not a real Intel machine acceptance |
| macOS version floor and newest supported release | Install/launch and run file/render/printing smoke on the declared minimum version and latest supported macOS; the floor decision follows Qt and all dependency floors, not the AutoCAD comparison alone |
| Automated package checks | Existing app/DMG verifier, required arm64-slice coverage (universal third-party binaries accepted), unresolved-dependency/RPATH failure, strict signature checks, mounted image layout, checksum; tamper/missing-architecture negative checks |

Capture test machine model/chip, OS build, display scale, Qt version, compiler/Xcode/SDK versions, artifact SHA-256, and signing mode with each acceptance report. GUI/accessibility testing remains a separate requirement because the existing package verifier's optional `--help` smoke check only starts the executable and does not exercise an interactive window.

## Risk register

| Risk | Impact | Mitigation / evidence needed |
| --- | --- | --- |
| A universal dependency is missing an arm64 slice or links to a host path | Bundle fails at runtime or embeds a developer-specific dependency | Native arm64 build; inspect every Mach-O and dependency; verify on a clean Apple Silicon account/machine |
| Artifact label does not describe the slices it contains | Users or release maintainers may misunderstand the artifact's architecture/size tradeoff | Report observed slices in build metadata; optional exact-slice audit can guide size optimization, while arm64 presence remains the compatibility requirement |
| Qt/toolchain wildcard drift changes API, SDK floor, or bundle behavior | Unrepeatable builds or new deployment failures | Pin exact tested versions and deployment floor; upgrade by reviewed, tested change |
| qmake and CMake bundle/resource paths diverge | Local/CMake build differs from shipped qmake app, especially for plug-ins/resources | Define the release entry point; maintain parity checks and test both if both are supported |
| Qt plugin pruning removes a runtime capability | Print, image, input, accessibility, or platform-specific feature is absent | Inventory required plugins and exercise print/SVG/accessibility/input flows on clean systems |
| Retina/trackpad changes alter coordinate behavior | Mis-scaled hit testing, jumpy zoom/pan, fuzzy rendering | Separate logical/physical pixels carefully; run mixed-scale display and gesture checks |
| Ad-hoc artifact is mistaken for a trusted public release | Users encounter Gatekeeper warning and brand trust is harmed | Label modes in filenames/release notes; use protected Developer ID/notary job for trusted public distribution |
| Autodesk branding or third-party asset is copied without rights | Trademark/copyright/license exposure | Use original assets and reviewed compatible licenses; keep notices and attribution with the bundle |

## Official sources checked

Research links were checked on 2026-10-11. Qt macOS details here are pinned to the Qt 6.8 documentation matching the current pixi lock's Qt minor series; re-check the chosen Qt patch before release.

- [Qt 6.8 supported platforms](https://doc.qt.io/qt-6.8/supported-platforms.html) — supported macOS build/target floor and architectures.
- [Qt 6.8 macOS platform notes](https://doc.qt.io/qt-6.8/macos.html) — deployment target and `CMAKE_OSX_ARCHITECTURES` / `QMAKE_APPLE_DEVICE_ARCHS` configuration.
- [Qt 6.8 macOS deployment](https://doc.qt.io/qt-6.8/macos-deployment.html) — app bundles, `macdeployqt`, Qt frameworks, and Qt plugin deployment.
- [Apple universal binary guidance](https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary) — architecture slices for apps and all bundled code.
- [Apple notarization guidance](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution) — Developer ID, notary service, and ticket/stapling model.
- [Apple distribution overview](https://developer.apple.com/documentation/technologyoverviews/distribution) — Gatekeeper checks for direct distribution.
- [Autodesk AutoCAD 2027 for Mac system requirements](https://www.autodesk.com/support/technical/article/caas/sfdcarticles/sfdcarticles/System-requirements-for-AutoCAD-2027-including-Specialized-Toolsets.html) — target comparison OS versions, Apple silicon/Intel, and Retina display context.
