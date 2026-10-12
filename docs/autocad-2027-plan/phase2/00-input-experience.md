# Phase 2 priority: AutoCAD-style drawing input

Status: requirements confirmed by the user after the Phase 1 review; implementation is the next checkpoint. These requests take priority at the start of Phase 2, before the broader property/layer work. This note does not claim these changes are already implemented.

## Confirmed requirements

1. **Drawing cursor:** Use the familiar crosshair and central pickbox, with configurable crosshair length and pickbox size. Match the active input state: crosshair for point entry, pickbox for object selection, combined cursor when idle. Preserve snapping, picking tolerance, UCS behavior and normal pointer/text cursors outside the drawing. Keep the information tooltip following the cursor disabled, as previously requested. A crosshair does not require enabling that tooltip.
2. **Command line appearance and behavior:** Keep it docked below the drawing. Replace the current separated prompt/entry presentation with a compact, readable prompt-and-input row and resizable history above it. Make command prompts, available options, focus, cancellation and errors clear, including commands started from the ribbon. Preserve the existing parser, aliases, precise coordinate input and history.
3. **Command prediction while typing:** Add live filtered suggestions and completion for available commands and user aliases, anchored to the bottom command input. Show the command name, alias and a short description. Support keyboard selection, Tab completion, Enter acceptance and Escape dismissal; history keys retain their current role when the popup is closed. Keep candidates grounded in the actual registry and active prompt. Do not reinterpret coordinates/numbers as commands or suggest unsupported operations. Expand ranking/contextual options only when the underlying command contract supports them.

The user requested closer resemblance to Windows AutoCAD 2027. Use the confirmed workflow as the target while retaining macOS shortcuts, native menus and Retina rendering. Crosshair dimensions and command-history density remain tunable implementation choices for visual review.

## Starting points in this fork

- Crosshair rendering already exists in `librecad/src/lib/engine/overlays/crosshair/lc_crosshair.cpp`; inspect cursor state/selection handling in `librecad/src/ui/view/qg_graphicview.cpp` before adding another drawing path.
- Reuse `QG_CommandWidget` and `QG_CommandEdit` in `librecad/src/ui/dock_widgets/command_line/`. The current Tab path calls `RS_Commands::complete()`; it is not the requested live suggestion interface.
- Reuse command definitions, alias resolution and prefix matching in `librecad/src/cmd/rs_commands.cpp` and `lc_commandItems.h`. Retain shared actions and the current command dispatcher.
- Preserve Phase 1 preset/reset behavior and the quiet-cursor preference in both workspaces. Verify the command dock remains usable at 1280 × 800 and 1024 × 640.

## Acceptance checks

- Compare idle, point-entry and object-selection cursors; test snap accuracy, pan/zoom, crosshair repaint cleanup and Retina sizing. No coordinate/data tooltip should follow the cursor unless explicitly enabled.
- Type a command prefix, choose a registered command/alias by keyboard and mouse, execute it, cancel it and recall history. Popup focus must not steal coordinate input or accidentally execute a drawing command.
- Draw using absolute/relative coordinates, edit and undo geometry, switch documents, close/reopen and restart. Alias files, custom shortcuts and workspace arrangements must survive.
- Capture the cursor states, compact command row and suggestion popup on native macOS at both window sizes. Actual Windows AutoCAD visual/task comparison remains an acceptance item when a reference installation is available.

## Verified reference behavior

Autodesk describes point-entry crosshairs, an object-selection pickbox and their combined idle cursor, with configurable sizes. [AutoCAD 2027: cursors in the drawing area](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-566B8C0C-57BC-4986-942D-BC683C4F90A4.htm).

Autodesk describes command prompts, automatic completion, a suggestion list, aliases and command history. These guide the interaction target; LibreCAD's actual command capabilities determine the available entries. [AutoCAD 2027: entering commands](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-BB0C3E79-66AF-4557-9140-D31B4CF3C9CF.htm).
