<!-- SPDX-License-Identifier: GPL-2.0-or-later; Copyright (C) 2026 LibreCAD contributors -->

# Phase 2 UI and selection checkpoint

Date: October 14, 2026. This extends the [input checkpoint](01-results.md) after explicit authorization to implement the supplied AutoCAD reference. These changes are included with the later [grip/lasso source checkpoint](03-grip-lasso-results.md). This is still Phase 2; paper-space/layout implementation remains Phase 3.

## Implemented behavior

- Home ribbon: large Line and Circle tools, compact secondary drafting tools, nine Modify tools including Mirror, Scale and Stretch, and shared Layers/Properties controls. Original LibreCAD glyphs use the current palette for contrast. Panel separators and captions are smaller, and ribbon height drops from 156 to 140 logical pixels.
- Insert and Annotate tabs expose block creation/insertion/editing, clipboard commands, text, hatch and five dimension commands. Home, Insert, Annotate, View and Output are the available tabs.
- More tools menu: annotation, block insertion, clipboard and distance measurement. Annotation moves into this menu below 1120 pixels; the menu remains available at every width. All controls invoke existing QActions and retain their enabled/check/shortcut state.
- Bottom drafting strip: current UCS coordinates, selection count, Grid, grid Snap, Ortho, line Draft and an Object snaps menu. These are shared actions rather than a second snapping model. Coordinates reuse the existing document/unit/UCS formatter; selection count follows the existing selected-set listener. The legacy snap toolbar is hidden in CAD Familiar. Command prompts/history/input remain in the bottom Command Line dock.
- Selected geometry: solid 1.5-logical-pixel outlines, blue by default, with outlined square grips (8-pixel default). Stored color and grip-size preferences remain honored. Hovered grips are pale blue; the moving grip is red. Hover feedback appears for a single selected entity, matching the existing endpoint editing contract.
- Grip feedback uses the action-owned preview overlay. It clears on movement away, action transitions, leave/suspend, drawing changes and Escape. Entity pens and geometry are not modified by styling. Existing endpoint dragging, snapping, UCS, undo and cancellation behavior is reused.
- Preferences → Appearance → Entity Handle: CAD style selection and square grips checkbox. Disable it to restore dashed selection and distinct start/intermediate/end handle colors. Grip size and selection color remain editable.
- Classic arrangement and native macOS menus remain available. This checkpoint adds neither a 3D workspace nor unsupported command placeholders.

## Validation

Native Apple Silicon build and all required checks passed on this host. Evidence is under `build/phase0-evidence/ui-polish-*`.

| Check | Result |
| --- | --- |
| Native Cocoa workspace, command input, rotated UCS, grip editing, undo/cancel and restart suite | 25 cases, 399 assertions passed |
| Preferences save/reload (cursor, selection style and grip size) | 1 case, 12 assertions passed |
| Cursor and selection rendering at 1x/2x device ratios and two zoom levels | 3 cases, 74 assertions passed; entity pens unchanged |
| Command registry and input tests | 7 cases, 84 assertions passed |
| Core fast lane | 1599 passed, 4 fixture skips, 40133 assertions passed |
| Registered DWG/DXF checks | 5/5 passed |
| Native captures and grip interaction capture run | 3 cases, 463 assertions passed (includes the grip case above) |
| App deployment/signatures/dependencies and disposable-copy smoke | 67 arm64 Mach-O files; passed |
| DMG mounted layout, signatures, content manifest | Passed |
| Final promoted app verification and deployment requirements | Passed; declared and actual minimum macOS 26.5 |
| Whitespace check | `git diff --check` passed |

The native test exposed an existing Escape dispatch problem: Qt key events could arrive accepted, preventing the default action's cancellation branch from running. Escape now resets the grip/window preview before inherited handling. The rerun proves preview cleanup and unchanged geometry after cancellation.

Actual drawing viewport height increased by **47 logical pixels** at each tested size, with command input widths retained:

| Window | Drawing viewport | Command input width |
| --- | --- | --- |
| 1280 × 800 | 973 × 396 (previously 973 × 349) | 883 |
| 1024 × 640 | 717 × 236 (previously 717 × 189) | 627 |

Native screenshots: [wide selection and hover](../../../build/phase0-evidence/phase2-ui-polish/selection-1280x800-hover.png), [narrow selection and hover](../../../build/phase0-evidence/phase2-ui-polish/selection-1024x640-hover.png), [active grip](../../../build/phase0-evidence/phase2-ui-polish/selection-active-grip.png), [command suggestions](../../../build/phase0-evidence/phase2-ui-polish/input-1280x800-suggestions.png). Measurements: [input-captures.json](../../../build/phase0-evidence/phase2-ui-polish/input-captures.json). Captures are rendered native Qt surfaces, not a manual recording or a pixel comparison to AutoCAD.

## Development artifacts

**Historical artifact record:** the paths and hashes below record the UI checkpoint. The [grip/lasso checkpoint](03-grip-lasso-results.md) replaces that packaged version with the current app and DMG; see its results for current hashes.

- [LibreCAD.app](../../../build/macos-arm64-phase2-final/LibreCAD.app)
- [LibreCAD-phase2-ui-arm64.dmg](../../../build/macos-arm64-phase2-final/LibreCAD-phase2-ui-arm64.dmg)
- [Final app verification](../../../build/phase0-evidence/ui-polish-final-verification.txt), [smoke verification](../../../build/phase0-evidence/ui-polish-package-smoke.txt), [DMG verification](../../../build/phase0-evidence/ui-polish-dmg-verification.txt), [deployment requirements](../../../build/phase0-evidence/ui-polish-package-requirements.json).

SHA-256: DMG `c842f3945549996701df902fe5a68bad95dc2000a8e71ec8c07a2f6ae47afcf2`; main executable `3d940250cef25ab5c4bb667aa2f87ae748cd8058c709445ffc64dbfe3c82584b`.

This is an **ad-hoc signed development build, not notarized**. Its actual load-command floor is macOS 26.5; runtime checks were on macOS 27.0.1. This does not certify execution on the minimum OS. The existing installation in `/Applications/LibreCAD.app` was not replaced. Ignored build artifacts are regenerated locally, not included in a Git clone.

To reproduce the build and regression checks:

```sh
LIBRECAD_MACOS_DEPLOYMENT_TARGET=26.5 scripts/build-macos-arm64-baseline.sh
LIBRECAD_BASELINE_OUTPUT="$PWD/build/phase0-evidence/phase2-ui-polish" QT_QPA_PLATFORM=cocoa \
  build/macos-arm64-baseline/librecad_dock_layout_tests '[.baseline],[phase2-selection]' --reporter compact
```

## Build retention

The user requested one build version. Obsolete package directories (package, package-final, command-workspace, phase1, phase1-final) and the redundant install output were removed after checking the retained Phase 2 app/DMG hashes. This initially recovered **2.31 GiB** of allocated disk space. The active compiler cache is retained for incremental builds. The verified app and DMG now replace the prior input package in `build/macos-arm64-phase2-final`. Temporary install/staging copies and the old input DMG were removed. The folder contains one current version: its app and installer. Compiler objects, dependencies and evidence remain separate from the distributable.

## Reference and scope limits

The supplied screenshot guides grouped top tools, bottom drafting controls and the dark 2D workspace. This is an original Qt implementation using LibreCAD artwork and existing commands. It is not a pixel-identical AutoCAD clone. Autodesk documents [ribbon panels](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-D20EF1D7-4135-48A7-B68E-65BF3BFF3D70.htm), [status controls](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-C5C9380F-5469-4858-B306-B1BFFC19C0A9.htm) and [separate grip color states](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-D973A3F3-6123-4105-B37B-39C4E7C7F26E-htm.html). The exact colors here are design choices.

Broader Phase 2 command/property/layer audits remain open. DWG fidelity, full layout ownership/viewports and ordered PDF publishing retain their original acceptance gates. Minimum-OS execution, physical multi-monitor behavior and manual accessibility acceptance are separate from native automated checks on this host.
