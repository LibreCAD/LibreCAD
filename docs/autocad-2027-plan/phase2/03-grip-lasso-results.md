<!-- SPDX-License-Identifier: GPL-2.0-or-later; Copyright (C) 2026 LibreCAD contributors -->

# Phase 2: lasso selection and dimensional grip editing

Date: October 14, 2026. Extends the [UI/selection checkpoint](02-ui-selection-results.md). This source checkpoint records the completed implementation and validation below.

## How to use it

- **Freehand selection:** press on empty space, hold the left mouse button and draw a boundary. Release to select. Initial movement to the right gives a blue window lasso (fully enclosed objects); initial movement to the left gives a green crossing lasso (enclosed or touched objects). Shift removes matching objects. Escape cancels the boundary. Click-release, move, click remains rectangular selection.
- **Grip placement:** select objects, then click a square grip. That click picks it up; releasing the mouse does not commit. Move the pointer and click again to place it, or press Enter from the dimensional editor. Fast second clicks delivered by Qt as double-click events also place the grip. Other selected objects retain their selection and geometry.
- **Dimensional input:** a temporary panel appears during the grip edit. Type a value to lock it; clear the field to unlock. Tab and Shift+Tab switch between the two fields. Both fields can be locked at once, with expressions such as `80+20` supported. Invalid, non-finite or non-positive dimensions remain uncommitted. Movement distance can be zero. Escape removes the panel and preview.
- **Length and angle meaning:** line endpoints show resulting length and the line's angle from start to end. Circle rim grips show radius and angle from the center; center grips show movement distance. Polyline vertices show the chosen adjacent segment's length and direction. Other grips show movement distance and angle. Lengths use drawing units; angles use degrees with the current UCS, angle basis and clockwise/counterclockwise setting. Typed 2D coordinates retain the grip's elevation metadata.
- **Command Space:** Space submits the command input through the Enter path, including accepting a visible suggestion once and empty-input Enter behavior. Shift+Space inserts a literal space for text or expressions. The existing Preferences keyboard option remains available; the new default is applied once to existing profiles.

The general information tooltip following the cursor remains disabled by default. The dimensional editors appear only while modifying a grip. Live previews remain visible when moving into those editors and clear on placement, cancellation, changing commands or invalidating the edited entity. Confirming the original dimensions finishes without replacing the entity or creating a geometry modification.

## Implementation and review

`LC_PolygonSelection` operates on world-coordinate boundaries, using exact line/arc/circle/ellipse intersections and interval containment for concave boundaries. Containers are evaluated through their children. Other entities fall back to their existing nearest-boundary/extent representation. Invisible, deleted and locked entities are excluded, with the existing selection notifications and additivity preference retained. Lasso points are sampled at three logical pixels and reduced at 2048 points; edges and bounds are prepared once for selection.

`LC_GripInputWidget` is owned by the drawing view and tracked through a guarded pointer by the default action. It contains temporary values, not model geometry. The existing `moveRef` and undoable modification paths remain responsible for geometry; entity pen and layer values are retained. Explicit numeric values take precedence after snapping/restriction calculation. Field focus, Tab and Escape stay within the grip edit rather than activating command completion.

## Validation

All native and headless regression checks passed. Evidence is under `build/phase0-evidence/grip-lasso-*`.

| Check | Result |
| --- | --- |
| Native Cocoa workspace, grip input, lasso, command popup, undo, cancellation and restart suite | 27 cases, 466 assertions passed |
| Preferences persistence | 1 case, 12 assertions passed |
| Exact lasso geometry, concave regions, curve tangency, locked layers and Retina cursor/selection rendering | 4 cases, 89 assertions passed |
| Command registry and command input (including Space and literal Shift+Space) | 8 cases, 92 assertions passed |
| Core fast lane | 1599 passed, 4 fixture skips, 40133 assertions passed |
| Registered DWG/DXF checks | 5/5 passed |
| Signed app, dependency/architecture checks and disposable-copy smoke | Passed; 67 arm64 Mach-O files |
| Deployment load commands and declared minimum | Both macOS 26.5 |
| Whitespace check | `git diff --check` passed |

The native cases exercise first-click release without a commit, fast second-click placement, simultaneous length/angle locks, Tab cycling, rotated UCS expression input, circle radius changes, polyline first-vertex changes, multi-object selection retention, invalid/zero length rejection, unchanged-dimension confirmation, leaving the canvas for an editor, changing commands, undo and Escape. Live preview remains visible inside the dimensional editors. Native viewport tests leave enough camera margin to test placement independently of automatic border panning.

Rendered native evidence: [grip dimensions](../../../build/phase0-evidence/phase2-grip-lasso/grip-length-angle.png), [freehand lasso](../../../build/phase0-evidence/phase2-grip-lasso/lasso-preview.png). These are real Qt widget captures, not mockups or a manual input recording.

Mounted-DMG layout, signatures and source-manifest comparison passed. Final promoted-app verification passed with 67 arm64 Mach-O files. The app is **ad-hoc signed, not notarized**; Gatekeeper assessment rejects this development signature as expected. Its declared and actual load-command floor is macOS 26.5. Runtime validation was on macOS 27.0.1; execution on the minimum OS and physical multi-monitor behavior are not certified by these checks.

## Current artifact and retention

The verified app and DMG now replace the previous version in `build/macos-arm64-phase2-final`. The folder contains only one version, as an app and its DMG. The obsolete package and temporary install/staging copies were removed. The active compiler cache, dependencies and evidence are retained. The installed `/Applications/LibreCAD.app` was not replaced.

- [Current application](../../../build/macos-arm64-phase2-final/LibreCAD.app)
- [Current development DMG](../../../build/macos-arm64-phase2-final/LibreCAD-phase2-ui-arm64.dmg)
- [Final app verification](../../../build/phase0-evidence/grip-lasso-final-verification.txt), [smoke check](../../../build/phase0-evidence/grip-lasso-smoke-verification.txt), [DMG verification](../../../build/phase0-evidence/grip-lasso-dmg-verification.txt), [deployment requirements](../../../build/phase0-evidence/grip-lasso-package-requirements.json).

Build outputs and evidence remain ignored and must be regenerated in a fresh clone.

SHA-256: DMG `3bf60502bcee5cfdc12bab292abfb78c1724ba5845813ef6e55b175f60545a3e`; main executable `7a1d17a9d64f8269814834ffe35cdfa5cd21c3586008604b8ee4c0ca6cbb621a`.

## Reference and remaining limits

Autodesk documents the [press-drag-release lasso and two-click rectangular selection](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-243E4DD0-8947-4905-AFE2-BE9B903A8C3F.htm) and [distance/angle fields during grip editing](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-1CAE0A1F-EE69-4EAF-AD67-7745D45C0CF7-htm.html). This implementation uses original LibreCAD code and Qt controls.

This does not implement every AutoCAD dynamic dimension, multiple hot-grip stretching, lasso Fence mode cycling or specialized dimensional semantics for all entity types. Text/image/filled-entity selection continues to depend on the existing boundary/extent representations; broad file/entity acceptance remains in the Phase 2 audit. Layout ownership, viewport scenes and ordered PDF publishing remain later plan gates. Manual hardware/monitor checks and minimum-OS execution remain separate from native automated validation on this host.
