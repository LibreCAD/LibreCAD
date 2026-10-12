# Phase 1: CAD Familiar workspace

Date: October 11, 2026. Based on the native arm64 engineering baseline published as `45c23fead4997456b2dd875eb402d3280ffe8cd3`. This checkpoint implements the first workspace milestone from the [master plan](../00-master-plan.md); it does not establish full AutoCAD command or file compatibility.

## Implemented behavior

- **Top quick access:** New, Open, Save, Undo and Redo, with a Workspace menu.
- **Home ribbon:** Draw, Modify, Layers, Properties and Annotation. Controls invoke the existing actions, preserving their actual prompts and dialogs. Move / Copy is accurately labeled as the existing combined operation. Lengthen invokes LibreCAD's existing trim-amount operation.
- **View ribbon:** Properties, Layers and Command Line recovery, grid, zoom, pan, workspace switching and Reset Current Workspace.
- **Output ribbon:** Existing Print Preview, Print and Export PDF actions. Real sheet publishing remains a later implementation stage.
- **Properties / Layers:** Right-side sibling palettes; Properties is the initial active tab. The current-layer ribbon selector follows the active document and layer list. Color, lineweight and linetype reuse the single existing pen toolbar, hosted vertically in Home. Pen utility actions remain available from the Properties menu.
- **Command Line:** Bottom dock, with existing parsing, prompts, history, completion and aliases. Cursor information remains disabled by default, including the previous one-time settings migration. A deliberate later opt-in remains persistent.
- **Compact windows:** The ribbon workspace permits a smaller canvas before the dock fitter hides command input. The command prompt uses the theme's normal text/link colors instead of hard-coded black/blue.
- **Presets:** CAD Familiar and Classic, with independent arrangements during switching and across restarts. The earlier arrangement is preserved as Classic during the first migration. Versioned preset identifiers are included in startup settings and named workspace files; older named workspaces default to Classic.
- **Reset:** Restores the current preset's arrangement while retaining drawings, alias files, shortcut files and named workspaces. Existing View actions recover closed palettes.
- **Selection refresh:** Properties defers suppressed selection changes and refreshes when the drawing returns to selection mode. Its entity-type/count selector reads the document selection directly; mixed selections default to All while an explicit valid type filter is retained.
- **Shutdown:** Removing the first of multiple drawings during main-window teardown no longer activates its sibling and starts a new default action against the window being destroyed. The native edit/reopen test exposed this lifetime issue, and its two-drawing cleanup now passes.

The ribbon uses original LibreCAD icons and the same QAction instances as the native menus. macOS retains its system menu bar and shortcut handling. Annotation collapses to a menu below 960 logical pixels. No second command parser or independent pen-state implementation was introduced.

## Verification

The source builds with Apple Clang, Qt 6.8.2 and the macOS 26.5 SDK on an arm64 Mac running macOS 27.0.1. The main executable's `LC_BUILD_VERSION` records **minimum macOS 26.5**; this checkpoint does not establish a lower OS floor.

| Check | Result |
| --- | --- |
| Native Cocoa workspace/dock suite, including capture and restart | 22 cases, 611 assertions passed |
| Properties selection/lifetime suite | 8 cases, 46 assertions passed |
| Core fast lane | 1,611 cases passed; 37 skipped for absent fixtures; all 40,837 assertions passed |
| Registered DWG/DXF CTest checks | 5/5 passed |
| 1280 × 800 native workspace capture | MDI area 776 × 420 with right palette; all three pen controls visible; bottom command visible |
| 1024 × 640 native mixed-selection capture | MDI area 520 × 260; bottom command remains visible |
| Mixed selection | Four editable entities; Properties shows All (4), with locked content excluded by the existing selection behavior |
| Final app verification | All 67 Mach-O files are arm64; bundled linkage and ad-hoc signatures verified; disposable-copy `--help` smoke passed |
| Development DMG | 131 MB; mounted app matches the finalized app manifest; Applications shortcut/layout verified |

Local evidence is kept under `build/phase0-evidence/phase1-*`; build artifacts are ignored and are not present in a fresh clone. The core skip reasons concern external/missing DWG fixtures and do not establish support for those files.

The focused acceptance workflow creates a 100-unit line using precise command input, edits its endpoint to 120, changes its layer, undoes both edits, exports DXF, reopens it, verifies the reopened line is 100 units on GEOMETRY, and switches documents. The current default DXF writer produced R2007 for this workflow. Other checks cover original dock recovery, preset switching and restart, requested palette visibility, alias/shortcut preservation, Properties lifetime and mixed selection.

Native captures: [default workspace](../../../build/phase0-evidence/phase1-ui/workspace-baseline.png), [line Properties](../../../build/phase0-evidence/phase1-ui/workspace-line-properties.png), [running line command](../../../build/phase0-evidence/phase1-ui/workspace-line-command.png), [Layers](../../../build/phase0-evidence/phase1-ui/workspace-layers.png), [compact workspace](../../../build/phase0-evidence/phase1-ui/workspace-1024.png), and [Classic](../../../build/phase0-evidence/phase1-ui/workspace-classic.png). These are native Qt harness captures using isolated settings and the committed fixture, not a Windows AutoCAD comparison.

## Review artifacts

- [Native arm64 app](../../../build/macos-arm64-phase1-final/LibreCAD.app)
- [Development DMG](../../../build/macos-arm64-phase1-final/LibreCAD-phase1-arm64.dmg)
- [Package verification](../../../build/phase0-evidence/phase1-package-verification.txt)
- [Mounted DMG verification](../../../build/phase0-evidence/phase1-dmg-verification.txt)

The package is ad-hoc signed and **not notarized**. Gatekeeper rejects that development signature as expected; this is not a public distribution package. Minimum macOS is 26.5. The existing installed application in `/Applications` was not replaced. The Phase 1 implementation and this report form the source checkpoint; built applications, DMGs and detailed local evidence remain ignored build artifacts.

Reproduce the focused tests after configuring the baseline build:

```sh
.pixi/envs/default/bin/cmake --build build/macos-arm64-baseline \
  --target librecad librecad_dock_layout_tests librecad_property_sheet_tests librecad_fast_tests -j 6
QT_QPA_PLATFORM=offscreen build/macos-arm64-baseline/librecad_property_sheet_tests '[properties]' --reporter compact
QT_QPA_PLATFORM=offscreen build/macos-arm64-baseline/librecad_fast_tests --reporter compact
QT_PLUGIN_PATH="$PWD/build/macos-arm64-install/bin/LibreCAD.app/Contents/PlugIns" \
LIBRECAD_BASELINE_OUTPUT="$PWD/build/phase0-evidence/phase1-ui" QT_QPA_PLATFORM=cocoa \
  build/macos-arm64-baseline/librecad_dock_layout_tests '[.baseline],[dock-window]' --reporter compact
```

Use the Cocoa backend for the 1280 × 800 canvas check: the offscreen backend advertises a smaller virtual screen, and the application's monitor-boundary correction intentionally clamps the window to it. Test settings and fixture drawings are isolated from user preferences/documents.

## Limits and next checkpoint

This remains a development build. Native screenshots and Qt-driven interaction checks do not substitute for manual keyboard/trackpad, accessibility, light-theme and monitor-change acceptance, or comparison against a real Windows AutoCAD 2027 installation.

DWG export failures, real paper-space/layout ownership, multiple scaled viewports, ordered sheet publishing and PDF plot-style/color fidelity remain the documented later gates. Existing Model/Layout tabs are retained with their previous behavior, including the duplicate Model label observed in the baseline. Layer visibility/freeze semantics have not been relabeled or expanded.

The next stage is Phase 2. Following the user's review, begin with the [AutoCAD-style crosshair, command line and live command prediction](../phase2/00-input-experience.md), followed by command contracts and property/layer workflows.
