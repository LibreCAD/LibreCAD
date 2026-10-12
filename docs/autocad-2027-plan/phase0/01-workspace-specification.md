# Phase 1 implementation specification

Status: design specification produced during Phase 0. The implementation and its validation are recorded in [Phase 1 results](../phase1/00-results.md). Read alongside [UI research](../01-ui-and-commands.md) and [the source inventory](source-inventory.json).

## Default arrangement

Create a **CAD Familiar** preset with quick access and Home ribbon above the existing MDI drawing area, Properties active on the right with Layers as a sibling tab, Command Line below the canvas, and existing coordinates/snaps in the status area. Preserve the current arrangement as **Classic**. Keep the macOS menu bar native and shared actions reachable there.

Use logical Qt pixels: top quick access about 32 high; ribbon tabs 28 high; Home panels about 92 high; right palette initially 300 wide (minimum 240); command dock initially 110 high (minimum 72). These are prototype targets, to be adjusted after visual review. At 1280 × 800, reserve at least 650 × 420 for drafting. At 1024 × 640, collapse low-priority ribbon panels and allow palette hiding rather than forcing a larger window. Retina scaling must come from Qt rather than doubled hard-coded sizes.

## Shared controls

| Group | Initial visible controls | Existing IDs / owner |
| --- | --- | --- |
| Quick access | New, Open, Save, Undo, Redo | `FileNew`, `FileOpen`, `FileSave`, `EditUndo`, `EditRedo` |
| Home / Draw | Line, Polyline, Circle, Arc, Rectangle | Resolve from registry: `DrawLine`; the existing draw family IDs for remaining tools; preserve action option widgets |
| Home / Modify | Move, Copy, Rotate, Offset, Trim, Extend, Fillet | Existing Modify action family; show current LibreCAD prompts/dialogs accurately |
| Home / Layers | Current layer selector, Add, Edit, show Layers | Existing layer widget/list; `LayersAdd`, `LayersEdit`; dock `layer_dockwidget` |
| Home / Properties | Current color, linetype, lineweight; show Properties | Reuse existing pen controls and `property_sheet`; selection edits remain in Property Sheet |
| Home / Annotation | Text, Hatch, dimensions dropdown | Existing draw-other and dimension actions |
| View | Properties, Layers, Command Line, grid, zoom, workspace reset | Dock toggle actions; `ViewGrid`, `ZoomAuto`; curated reset added in Phase 1 |
| Output | Print preview, Print, Export PDF | Existing file actions, including `FilePrintPDF` |

Resolve actions by stable ID, validate every lookup, and show the same QAction in menus/ribbon/toolbars. Do not register duplicate shortcuts. The registry captured by the opt-in baseline test is authoritative for runtime availability; the static inventory also includes conditional source entries. Layout publishing stays out of the working toolbar until its implementation gate passes.

## Palette and input behavior

Properties must show drawing/default context with nothing selected and selection/type/count with selected entities. Keep existing provider/type-specific fields and mixed-value behavior. Enter commits validated input, Escape cancels an unaccepted edit; invalid input remains visible with an explanation. Numeric editing must not lose focus when the document selection refreshes. An accepted edit must have coherent undo behavior. Document switching must clear stale entity references.

Layers initially expose current/name/visibility/lock/color/linetype/lineweight/print according to existing semantics. Keep the current combined visibility/freeze control labeled accurately. Independent On and Freeze needs a later model change. Do not add separate switches that both mutate the same flag.

Command Line retains the existing parser, aliases, history, completion and action prompts. Preserve the focus-command shortcut and add clearly documented discovery from the ribbon. AutoCAD command names require a reviewed contract before adding aliases; an alias alone does not establish prompt/options parity.

User preference confirmed October 11, 2026: keep the information overlay beside the cursor disabled by default. Coordinates, action prompts and command entry remain available through their existing fixed controls. Users may explicitly re-enable the Info Cursor setting. The bottom command dock and this default are implemented as an initial correction before the ribbon/preset work, including a one-time update of existing startup settings.

## Appearance and persistence

Use original LibreCAD icons, consistent 20–24 pixel tool icons, readable text labels and visible keyboard focus. Respect light/dark palette contrast; selected and checked controls need more than color alone. Keep help/tooltips attached to their shared actions. Record screenshots at both window sizes with no selection, one line selected, mixed selections, Properties active, Layers active, and a running LINE command.

Workspace settings currently retain Qt widget state, geometry, area visibility and requested dock visibility. `Paths/OtherSettingsDir` controls `workspaces.lcws`, `shortcuts.lcsc`, and `librecad.alias`. Preserve these files and stable dock IDs: `property_sheet`, `layer_dockwidget`, `command_dockwidget`. Add a versioned preset identifier separately from Qt state. Reset changes the arrangement; it must preserve aliases, custom workspaces, shortcuts and drawing data. A hidden/floating palette must be recoverable from View.

## Acceptance tasks for the next phase

1. Launch at 1280 × 800 and 1024 × 640; all core controls and a usable canvas fit.
2. Create a 100 mm line with precise input; select it, edit a property, change its layer, undo, save DXF, reopen and inspect.
3. Open a second drawing; verify Properties follows the active drawing and command prompts follow focus.
4. Switch CAD Familiar ↔ Classic; relaunch; reset; recover hidden/floating palettes. Preserve a custom alias and shortcut throughout.
5. Compare these tasks in Windows AutoCAD 2027 when a reference installation is available. Capture screenshots and actual keystroke/prompt differences; avoid claiming visual parity from the schematic alone.

Tasks are defined for review but are not recorded as passed until executed. Native macOS trackpad/menu/focus validation and actual AutoCAD reference evidence remain separate from offscreen test images.
