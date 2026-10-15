<!-- SPDX-License-Identifier: GPL-2.0-or-later; Copyright (C) 2026 LibreCAD contributors -->

# Phase 2 checkpoint: drawing cursor and bottom command input

Date: October 14, 2026. Starting source checkpoint: `a407db8e876ba574a3d100beb76ee181a3ed6337`, following Phase 1 `da72935e2`. This report records the first input checkpoint, included with the later [grip/lasso source checkpoint](03-grip-lasso-results.md). This completes the first [Phase 2 input checkpoint](00-input-experience.md), not the broader Phase 2 command/property/layer program or full AutoCAD compatibility.

## Implemented

### Drawing cursor

- Reuses `LC_Crosshair` and the existing overlay renderer. A separately owned drawing-cursor overlay sits at the pointer in logical screen coordinates; snap markers continue to show the actual snapped point.
- Idle: crosshair plus central pickbox. Point entry: crosshair. Object selection: pickbox. The default action's selection-window corners and entity/grip movement now use point-entry cursor states. Pan/zoom and other special native cursors retain their existing roles.
- **Application Preferences → Snap → Snap Behavior** exposes the CAD cursor toggle, crosshair percentage and pickbox side length. Defaults are enabled, **12%**, and **10 logical pixels**. Bounds are 1–100% and 4–80 pixels. At 100%, arms extend to the canvas edges. These are appearance settings, not picking-tolerance changes.
- Existing snapper crosshair lines are suppressed when the drawing cursor owns them; snap shapes remain independent. Cleanup follows leave, hide, action changes, viewport changes and document teardown. Print preview retains its original cursor/rendering path.
- Short arms and near-edge arms cannot draw backward through the central pickbox/gap. Isometric/adaptive rendering and existing UCS coordinate conversions are retained.
- The following coordinate/information tooltip remains disabled by default. The existing opt-in and migration behavior are retained.

### Bottom command row

- Prompt, input and options share one compact row below the drawing, with vertically resizable/collapsible history above it. The input row cannot be collapsed; the history splitter restores across widget recreation/restarts.
- Long prompts do not widen the dock or squeeze away input. They use middle elision; the complete prompt remains in the label tooltip/accessibility name and the existing status prompt. Dock/widget identities remain stable for workspace persistence.
- Uses the existing `QG_CommandEdit::processInput()` and dispatcher. No second parser was introduced. Empty Enter with no document now avoids the previous empty-string indexing path.
- Active prompts still come from the actual actions, including commands started from the ribbon. Idle prompt refreshes show `Command:` and do not dismiss a live suggestion list.

### Suggestions and autocomplete

- Live prefix filtering uses the effective `RS_Commands` registry, including built-in and accepted user aliases. Each entry shows its accepted spelling, canonical LibreCAD command identity and a short description.
- Exact-case matches rank first, preserving aliases that differ only by case; then exact case-insensitive matches and alphabetical prefix matches. Canonical identities are registry names such as `line2p`, not invented AutoCAD command adapters.
- The non-focus-taking list is anchored above the bottom input when space permits, clamped to the current screen, and shows up to eight rows with scrolling.
- **Up/Down** navigate the open list. **Tab** inserts the selected completion. **Enter** accepts and submits through the existing parser exactly once. **Mouse selection** inserts without executing; Enter executes it. **Escape** dismisses the list and preserves input, then subsequent Escape follows the existing clear/cancel behavior.
- Closed-list Up/Down history, Ctrl-arrow relative drafting, coordinate input, calculator/keycode modes, command files, variables, native menu shortcuts and shared ribbon actions retain their existing paths.
- Global suggestions are suppressed during active point/selection/text/argument input and when no drawing can accept a command. Numeric, absolute/relative/polar coordinate, expression and sequence input are not rewritten. Reserved parser syntax in alias names is excluded from the popup. Existing action-option Tab completion remains available without a global fallback.
- Document/action context changes and focus loss dismiss stale suggestions. A native regression exposed an idle mouse-enter prompt refresh hiding the list; state-based prompt handling now preserves it.

## Validation

Test machine: native Apple Silicon Mac, macOS **27.0.1**, Apple Clang **21.0.0**, Qt **6.8.2**, macOS **27.0 SDK**. The build uses the existing locked Pixi dependencies and arm64 baseline profile. A clean build and fresh plugin install were performed after the host SDK changed.

| Check | Result |
| --- | --- |
| Native Cocoa workspace/input suite, including both-size captures | **26 cases, 719 assertions passed** |
| Native preferences acceptance, isolated process | **1 case, 9 assertions passed** |
| Focused command input/registry/alias tests | **7 cases, 84 assertions passed** |
| Rendered cursor tests, including 1×/2× sizing and short/near-edge arms | **2 cases, 32 assertions passed** |
| Properties selection/lifetime suite | **8 cases, 46 assertions passed** |
| Core fast lane | **1,611 cases passed; 37 skipped; 40,837 assertions passed** |
| Registered DWG/DXF CTest checks | **5/5 passed** |
| Final app architecture/linkage/signature verification | **67 Mach-O files, arm64 present throughout, bundled linkage and ad-hoc signatures verified** |
| Disposable-copy packaged executable `--help` smoke | Passed |
| Package regression suite | Passed: spaces in paths, crash-report/signature preservation, mutation rejection, missing architecture and external dependency rejection |
| Development DMG mounted verification | Passed; mounted application matches finalized source manifest |
| Diff whitespace, build-wrapper shell syntax and inspection-tool Python syntax | Passed |

The native workflows cover registered and user-alias keyboard/mouse acceptance, exact-case alias precedence, Enter/Tab/Escape, history and focus, active ribbon-command suppression, document switching and multiple-drawing shutdown. Precise drafting includes absolute and relative Cartesian entry, endpoint snapping and relative polar entry under a rotated UCS with an explicitly configured counterclockwise angular basis. Properties/layer edits are undone, redone, undone again, saved to DXF, reopened and checked for the original 100-unit line on GEOMETRY. Preset restart/reset and alias/custom-shortcut file preservation retain the Phase 1 acceptance checks.

Saving the complete Preferences dialog also migrates legacy startup options and may display notices owned by the main window. Its acceptance case runs in a separate process, handles notices only in isolated test windows, and reloads the saved cursor controls. Native typing tests explicitly activate their Qt owner before synthetic key delivery so another application owning macOS focus does not turn a background test into a false completion failure. Test settings/documents are isolated from user data.

The 37 core skips are missing/external DWG fixtures. They are not passes or new interoperability evidence.

### Native captures

The captures use the Cocoa backend and a **2× Retina** backing store. Window sizes are logical dimensions. The drawing-view measurements below exclude the MDI title/layout tabs and scrollbars; they are not directly comparable to the Phase 1 outer-MDI-area measurements.

| Window | Drawing view | Command input width |
| --- | --- | --- |
| 1280 × 800 | 973 × 349 | 883 logical pixels |
| 1024 × 640 | 717 × 189 | 627 logical pixels |

Screenshots are native Qt-rendered widget captures with assertions for actual cursor/popup visibility, dimensions, focus and screen bounds. Since `QWidget::grab()` excludes top-level tool windows, suggestion captures combine the actual native main-window and suggestion-window surfaces. They are not an AutoCAD reference comparison or a manual input recording.

- 1280 × 800: [idle](../../../build/phase0-evidence/phase2-ui/input-1280x800-idle.png), [point entry](../../../build/phase0-evidence/phase2-ui/input-1280x800-point.png), [selection](../../../build/phase0-evidence/phase2-ui/input-1280x800-selection.png), [suggestions](../../../build/phase0-evidence/phase2-ui/input-1280x800-suggestions.png).
- 1024 × 640: [idle](../../../build/phase0-evidence/phase2-ui/input-1024x640-idle.png), [point entry](../../../build/phase0-evidence/phase2-ui/input-1024x640-point.png), [selection](../../../build/phase0-evidence/phase2-ui/input-1024x640-selection.png), [suggestions](../../../build/phase0-evidence/phase2-ui/input-1024x640-suggestions.png).
- Measurements: [input-captures.json](../../../build/phase0-evidence/phase2-ui/input-captures.json).

## Development artifacts and actual requirements

**Historical artifact record:** the app/DMG and hashes below describe the input checkpoint. The UI/selection checkpoint replaced that package, and the old input DMG and Phase 1 build directories were removed at the user's request. See [current UI results and artifacts](02-ui-selection-results.md).

- [Development application](../../../build/macos-arm64-phase2-final/LibreCAD.app).
- [Development DMG](../../../build/macos-arm64-phase2-final/LibreCAD-phase2-input-arm64.dmg).
- [App verification](../../../build/phase0-evidence/phase2-package-verification.txt), [packaged smoke](../../../build/phase0-evidence/phase2-package-smoke.txt), [package regressions](../../../build/phase0-evidence/phase2-package-regression.log), [DMG verification](../../../build/phase0-evidence/phase2-dmg-verification.txt).
- [Per-binary deployment requirements](../../../build/phase0-evidence/phase2-package-requirements.json), generated by `scripts/inspect-osx-requirements.py`.
- Detailed test logs and package manifest: `build/phase0-evidence/phase2-*`.

Final SHA-256:

| Artifact | SHA-256 |
| --- | --- |
| Development DMG | `95fd654546fd311c0561396ef25882c11720a8984964b2356b1d55568f6a01e2` |
| Packaged main executable | `bd170830e146c5bff20d25805d550a34d0d472c8ec8367be8e1c94a6392ffc33` |

**Actual arm64 load-command floor: macOS 26.5.** The maximum deployment minimum across all 67 bundled Mach-O files is 26.5; the development copy's `Info.plist` explicitly declares 26.5. The main executable records SDK 27.0. Runtime validation was on macOS 27.0.1; this does not certify execution on the minimum OS or establish a lower public-support floor.

The app and DMG are development artifacts: the application is **ad-hoc signed, not notarized**. Gatekeeper rejection of that signature is expected. `/Applications/LibreCAD.app` and the separate Phase 1 final artifacts were not replaced. Build outputs/evidence are ignored and are not available in a fresh clone until regenerated.

## Reproduce

The wrapper now clears stale SDK-derived CUPS/OpenGL cache paths and selects the complete SDK libc++ headers when the Command Line Tools installation lacks them beside clang. To retain this checkpoint's deployment target explicitly:

```sh
LIBRECAD_MACOS_DEPLOYMENT_TARGET=26.5 scripts/build-macos-arm64-baseline.sh
QT_QPA_PLATFORM=offscreen build/macos-arm64-baseline/librecad_command_input_tests --reporter compact
QT_QPA_PLATFORM=offscreen build/macos-arm64-baseline/librecad_drawing_cursor_tests --reporter compact
QT_PLUGIN_PATH="$PWD/build/macos-arm64-install/bin/LibreCAD.app/Contents/PlugIns" \
LIBRECAD_BASELINE_OUTPUT="$PWD/build/phase0-evidence/phase2-ui" QT_QPA_PLATFORM=cocoa \
  build/macos-arm64-baseline/librecad_dock_layout_tests '[.baseline],[dock-window]' --reporter compact
QT_PLUGIN_PATH="$PWD/build/macos-arm64-install/bin/LibreCAD.app/Contents/PlugIns" QT_QPA_PLATFORM=cocoa \
  build/macos-arm64-baseline/librecad_dock_layout_tests '[phase2-preferences]' --reporter compact
```

Use a fresh copy of the installed app for packaging. Before final signing, set its `LSMinimumSystemVersion` to the inspected deployment floor; this checkpoint uses 26.5. Deploy/sign with `scripts/package-osx-app.sh`, then run `scripts/inspect-osx-requirements.py --app PATH --output REPORT.json` and `scripts/verify-osx-package.sh --app PATH --mode adhoc --architectures arm64 --smoke`. Construct a DMG from that finalized app using `scripts/create-osx-dmg.sh`.

## Remaining limits and next review

- Manual keyboard/trackpad, multiple-monitor changes, light-theme and accessibility acceptance and a real Windows AutoCAD 2027 comparison remain unperformed. Automated Cocoa interaction and rendered 1×/2× evidence do not replace them.
- Descriptions reuse registry annotations; many are English, and entries without an annotation use a generic `Run <canonical>` description. Description/localization enrichment and broader ranking are later polish.
- Live prediction is intentionally command-name-only: letter/underscore-led names with digits/hyphens. Other accepted alias spellings remain available through the original parser, but are not live-predicted. Active options retain existing Tab behavior rather than a new parameter-aware prediction engine.
- Long prompts can be elided in the compact row. Full prompt text remains accessible without enabling the drawing cursor's coordinate/information tooltip.
- Imported duplicate Model labels, real paper-space/layout ownership, viewport publishing, PDF fidelity and the previously documented DWG-writing/interoperability gates remain later work.

Stop at this input checkpoint for user review. Broader Phase 2 command contracts and property/layer semantics have not been started by this checkpoint.
