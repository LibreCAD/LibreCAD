<!--
Copyright (C) 2026 LibreCAD.org

This document is licensed under the GNU General Public License, version 2 or
any later version. This notice applies to this document only; it does not
change the license of the LibreCAD source tree or other project files.
-->

# UI, Properties, Layers, and Command Workflows: AutoCAD 2027-Inspired Plan

> **Plan only.** This document describes a proposed LibreCAD fork interface at repository commit `628a1ba845931a37b8d745a8ad08d4731615812c`. It does not claim AutoCAD compatibility, and it does not authorize source changes. The target is a Windows-style AutoCAD 2027 interface adapted to macOS, with 2D drafting first. AutoCAD for Mac documentation is supplementary guidance for platform conventions, not a limit on the intended feature set. DWG and DXF goals are tracked separately; this document covers the UI workflows around them.

## Scope and reference boundary

Use AutoCAD 2027 Windows as the primary reference for the requested visual hierarchy: document/title region, compact quick-access actions, task tabs and grouped controls, drawing area, Properties and Layers palettes, command line, and status controls. Adapt that hierarchy to macOS window and keyboard conventions. AutoCAD's 2027 Windows help describes a ribbon organized into tabs and panels, with panels docked or floating and workspaces controlling visible tabs and palettes. Autodesk's Mac customization help is useful supplementary context: it documents Mac menus, commands, tool sets, and toolbar, but those Mac-specific arrangements do not restrict this fork's target. [Autodesk Windows: About the Ribbon](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-D20EF1D7-4135-48A7-B68E-65BF3BFF3D70.htm), [Autodesk Mac: UI customization](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-MAC-Customization/files/GUID-8ACA5693-657B-460C-8A36-0A230CDE7A71.htm), [Autodesk Mac: Layers Palette](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-MAC-Core/files/GUID-8FBBD2EA-EE02-4D34-A82B-2F39AE34B40D.htm).

Use Autodesk help to inform behavior and presentation, then implement the requested workflows against LibreCAD's actual drawing model. Autodesk's product help is not a promise that this fork can read every DWG construct, share command semantics, or reproduce a proprietary subsystem. Expose commands and controls only when they dispatch to implemented LibreCAD actions.

## Existing implementation at the inspected commit

The application is already a Qt Widgets/QMainWindow application with a mature action and dock architecture. The work should compose and polish existing building blocks rather than replace them wholesale.

| Area | Present in this checkout | Gap relative to target |
|---|---|---|
| Top-level menus and actions | `LC_MenuFactoryMain` creates File, Settings, Edit, View, Plugins, Tools, Workspace, and Help menus from existing `QAction`s. Tool and entity actions are already grouped in factories. | No task-tabbed ribbon surface or consistent visual task grouping matching the Windows reference; commands currently appear through menus/toolbars. |
| Docking | `LC_WidgetFactory` creates named `QDockWidget`s for Layers Tree, Entity Info, Properties, and Command Line. `MainWindowX` supplies dock area visibility helpers. | The target's default palette grouping/relative widths and integrated top hierarchy need a deliberate default layout and workspace preset. |
| Properties | The Properties dock follows active graphic view and drawing selection, supports per-entity-type selection filtering, no-selection/general properties, and type-specific registered editors. Mixed values are represented by `LC_PropertyMulti`; user edits use `undoableModify` with cloned entities. | Audit the shown property set against desired 2D entity coverage and improve discoverability, selection count/context, grouped ordering, and editor consistency before adding duplicate infrastructure. |
| Layer management | `LC_LayerTreeModel`/`LC_LayerTreeWidget` provide a model/view tree, active layer, visibility/lock/print/construction flags, color sample, filter/highlight, hierarchy inferred from names, and many layer context actions. | The default visual column order/presentation is not the AutoCAD layer-list pattern; no compact Properties-palette-style layer table should be represented as Mac parity until designed. Existing grouping hierarchy is LibreCAD-specific and should remain an optional view. |
| Command line | `QG_CommandWidget` is dockable, has history/output, text entry, prompts, escape/focus behavior, multi-command paste/load, and keycode mode. `QG_CommandEdit` supports up/down history, Tab event, coordinates/relative-coordinate conveniences, and calculator input. | No evidence in inspected paths for a live ranked suggestion popup, contextual argument-choice list, or full completion semantics. The command system is LibreCAD's action/alias registry, not AutoCAD's grammar. |
| Commands and aliases | `RS_Commands` maps registered full and short forms to `RS2::ActionType`, reads a user alias file, guards full command meanings against reassignment, and offers sorted prefix completion over registered full names. The source comments identify forms such as `line2p` / `li` / `line` / `l`; layer actions include `cnlayer` / `cnly` and `cslayer` / `csly`. | AutoCAD command names/aliases are not generally registered, and matching strings alone does not establish compatible prompt flow, options, cancel behavior, or object semantics. |
| Workspace persistence | `LC_WorkspacesManager` captures and restores window geometry, dock layout/visibility, dock/tool areas, toolbar sizes, menu expansion, and appearance settings. Workspaces are named and persisted. | Define `CAD Familiar (2D Drafting)` around the new top surface and current dock widgets. Retain Classic and migrate existing workspace, alias, and user settings without resetting them. |

### Source map

Paths below are repository-relative and line numbers refer to the inspection commit named above. Lines are navigation anchors, not a promise that later revisions retain the same numbers.

| Concern | Source and useful lines |
|---|---|
| Qt main window docking helpers | [`librecad/src/ui/main/mainwindowx.cpp`](../../librecad/src/ui/main/mainwindowx.cpp#L66) (`QMainWindow`, dock/tool area toggles); [`mainwindowx.h`](../../librecad/src/ui/main/mainwindowx.h) |
| Menu creation and action reuse | [`librecad/src/ui/main/init/lc_menufactory_main.cpp`](../../librecad/src/ui/main/init/lc_menufactory_main.cpp#L66) (top-level menu construction; Tools menu starts at 163) |
| Toolbar factory | [`librecad/src/ui/main/init/lc_toolbarfactory.cpp`](../../librecad/src/ui/main/init/lc_toolbarfactory.cpp#L72) |
| Dock creation / dock object names | [`librecad/src/ui/main/init/lc_widgetfactory.cpp`](../../librecad/src/ui/main/init/lc_widgetfactory.cpp#L237) (Layers Tree), 250 (Entity Info), 262 (Properties), 308 (Command Line) |
| Properties behavior / active drawing attachment | [`librecad/src/ui/dock_widgets/property_sheet/lc_propertysheetwidget.cpp`](../../librecad/src/ui/dock_widgets/property_sheet/lc_propertysheetwidget.cpp#L151) (view and document selection listener); lines 546–593 (selection/entity-type property container); lines 480–526 (user edit and undoable application) |
| Mixed property aggregation | [`librecad/src/lib/properties/lc_property_multi.cpp`](../../librecad/src/lib/properties/lc_property_multi.cpp#L72) (detect differing selected values), lines 91–123 (edited values propagated to editable subproperties); [`librecad/src/ui/dock_widgets/property_sheet/lib/view/lc_property_view_multiple.cpp`](../../librecad/src/ui/dock_widgets/property_sheet/lib/view/lc_property_view_multiple.cpp#L86) |
| Layer widget/model | [`librecad/src/ui/dock_widgets/layers_tree/lc_layertreewidget.cpp`](../../librecad/src/ui/dock_widgets/layers_tree/lc_layertreewidget.cpp#L47) (filter and model/view setup; action bar starts 179); lines 300–378 (layer list / active layer); lines 631 onward (context menu); [`lc_layertreemodel.cpp`](../../librecad/src/ui/dock_widgets/layers_tree/lc_layertreemodel.cpp#L143) (model rebuild and hierarchy) |
| Layer model columns | [`librecad/src/ui/dock_widgets/layers_tree/lc_layertreemodel.h`](../../librecad/src/ui/dock_widgets/layers_tree/lc_layertreemodel.h#L46) (`empty`, visible, locked, print, construction, color, name) |
| Layer data fields | [`librecad/src/lib/engine/document/layers/rs_layer.h`](../../librecad/src/lib/engine/document/layers/rs_layer.h#L40) (`frozen`, `locked`, `print`, `construction`, list-only visibility/selection flags) |
| Layer operations / undo examples | [`librecad/src/ui/dock_widgets/layers_tree/lc_layertreewidget.cpp`](../../librecad/src/ui/dock_widgets/layers_tree/lc_layertreewidget.cpp#L1188) (add); lines 1634, 1724, 1774 show undoable layer/entity modifications; lines 1830 onward remove behavior and guards |
| Command UI and execution | [`librecad/src/ui/dock_widgets/command_line/qg_commandwidget.cpp`](../../librecad/src/ui/dock_widgets/command_line/qg_commandwidget.cpp#L41) (widget and signals); lines 198 onward (dispatch path); [`qg_commandedit.cpp`](../../librecad/src/ui/dock_widgets/command_line/qg_commandedit.cpp#L131) (history, Tab, keyboard handling; input parsing at 249) |
| Authoritative action-to-command table | [`librecad/src/cmd/lc_commandItems.h`](../../librecad/src/cmd/lc_commandItems.h#L53) (item shape and command design); lines 90 onward (registered command table) |
| Alias parsing and completion | [`librecad/src/cmd/rs_commands.cpp`](../../librecad/src/cmd/rs_commands.cpp#L182) (build from table); lines 275–356 (alias file load and validation); lines 359–390 (lookup and prefix completion) |
| Workspace capture / restore | [`librecad/src/ui/main/workspaces/lc_workspacesmanager.cpp`](../../librecad/src/ui/main/workspaces/lc_workspacesmanager.cpp#L56) (save/activate); lines 140–208 (state capture); lines 264–305 (restore) |
| Project license | [`LICENSE`](../../LICENSE) (GPLv2 project notice). |

## Proposed UI information architecture

Create the **CAD Familiar (2D Drafting) workspace** with a restrained CAD look and Mac-native window behavior. Keep the macOS application menu bar at the top edge. Inside the application window, use a narrow title/document strip with drawing name and save state, a small quick-access row for New/Open/Save/Undo/Redo, then a horizontally scrollable task-tab strip and compact grouped command band. The planned task tabs are **Home**, **Insert**, **Annotate**, **Layout**, **View**, **Manage**, and **Output**. Introduce Layout and new publishing controls when their document/rendering features are implemented; use existing print/PDF actions in Output first. Do not populate tabs with placeholder buttons. **Home** should lead with Draw, Modify, Layers, Annotation, and Properties groups. The menu bar remains complete for discoverability and keyboard navigation; a ribbon band supplements the menus and invokes the same named `QAction`s.

Keep the central drawing viewport visually dominant. Place **Layers** and **Properties** as adjacent/stacked tabs in a right `QDockWidget` area so either palette is one click away; allow normal Qt dock split/float/move behavior. A collapsible left tools surface can complement the ribbon where useful. Put the **Command Line** in a persistent bottom dock with a prompt/input row and scrollback above it; preserve direct command focus and Escape-to-canvas behavior. Keep the status row at the bottom for coordinates and drafting toggles. Show a clear current layer control in Home > Layers and the status row. Tooltips should identify action, command token where one exists, and shortcut; never imply unsupported AutoCAD behavior.

### Text wireframe

```text
┌ macOS menu bar: LibreCAD  File  Edit  View  Draw  Modify  Window  Help ─────┐
│ Drawing.dxf  ●       New Open Save | Undo Redo | CAD Familiar (2D Drafting) ▾│
│ Home   Insert   Annotate   Layout   View   Manage   Output                   │
│ Draw [Line ▾ Circle ▾ Arc ▾ Polyline ▾]  Modify [...]  Layer [0 ▾] ...      │
├──────────────────────────────────────────────────────────────┬───────────────┤
│                                                              │ Layers        │
│                                                              │ [filter...]   │
│                    model / paper drawing                     │ On Lock Plot  │
│                                                              │ Color Name    │
│                                                              ├───────────────┤
│                                                              │ Properties    │
│                                                              │ Selection: 2  │
│                                                              │ General       │
│                                                              │ Geometry      │
├──────────────────────────────────────────────────────────────┴───────────────┤
│ Command history / prompt  Specify first point:  [input................]      │
│ x: ...  y: ...  Model | Grid | Snap | Ortho | Polar | Osnap | Workspace      │
└──────────────────────────────────────────────────────────────────────────────┘
```

The tabs, panels, and group organization follow the Windows AutoCAD ribbon reference. The top band should collapse to tab-only mode and fit narrow MacBook widths; groups must not become an unscrollable dense icon wall. Use named `QAction`s from the existing action registry as the single source for menu and top-band invocation, so enable/checked/shortcut state is shared. Avoid duplicate command logic or bespoke drawing operations inside buttons.

## Properties Inspector plan

Retain `LC_PropertySheetWidget` and its property container/view factory. Reshape its presentation into an Inspector palette with a compact header showing **selection count**, selected entity type/filter, and a clear “No selection” state that exposes current defaults. Group rows under General, Geometry, and entity-specific sections; preserve collapse state, and add a text filter only after keyboard focus and property search behavior are specified. Autodesk documents common properties for multi-selection and current general settings when selection is empty. [Mac Properties Inspector (2024 reference)](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-MAC-Core/files/GUID-0AA644F1-1165-492F-A4D1-3EDA8598C644.htm), [Windows object properties tools](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-81585857-F1B1-44F4-B7D0-B707386CA721-htm.html).

Property semantics are a core correctness contract:

1. **Selection source.** Inspector always reflects the active document/view and the effective selected entity set. Label a selection count; make entity-type filtering explicit instead of silently mixing incompatible geometry fields. When a command owns selection, indicate that state or prevent stale edits.
2. **Mixed values.** For a field where all eligible selected entities share the same value, show that value. When values differ, show an em dash/“Varies” placeholder in a non-value style. Clicking/editing the field applies the entered value to every eligible entity. Do not show the first entity's value as if it represented the whole selection. Read-only or inapplicable fields should be hidden or explicitly disabled with an explanation.
3. **Edit transaction.** An editor change is one intentional user action. Capture original entities/values at edit start, validate and parse before mutation, then apply through the document's undoable modification batch. Preserve selection and current layer/pen context unless the specific property contract requires a change. Editing several fields separately may create separate undo steps; if a single editor changes a compound coordinate/color object, keep that compound edit atomic. Do not emit one undo entry per selected entity.
4. **Refresh.** After apply, update the Inspector, layer palette, viewport, selection count, and command history/status without rebuilding unrelated UI. If a layer edit makes selected geometry invisible, report the result and avoid retaining an invalid selection silently.
5. **Validation.** Use existing type-specific editors for doubles, vectors, layer references, line types and colors. Display drawing units/precision consistently and avoid converting visible text through rounded values on no-op edits.

## Layer palette plan

Build the default **Layers** view around a flat sortable table with clear per-layer columns. Preserve the current hierarchical/tree presentation as an optional grouping mode for LibreCAD layer naming conventions; it is useful functionality and differs from Autodesk's groups/filters. Model values should be in a Qt model, edits in delegates, and commands/actions in shared `QAction`s. Suggested default columns: status (current/used), Freeze, Lock, Plot/Print, color swatch, name, linetype, lineweight, and description if supported. The current `RS_LayerData` has one `frozen` state, not independent AutoCAD-style Off and Freeze states; the existing tree's visibility control toggles this frozen state, while `visibleInLayerList` only controls whether a row appears in the layer list. Do not present separate On/Off and Freeze controls as implemented today. If independent states are desired, first audit the entity/rendering model, persistence, DXF/DWG mapping, and undo semantics. Construction is a separate LibreCAD flag. Allow users to show/hide and reorder secondary columns.

Provide searchable/filterable layers, active-layer highlight, create, rename, duplicate, delete, edit attributes, set current, select entities on layer, move selection to layer, and freeze/thaw/lock/unlock operations where current code supports them. Preserve LibreCAD-only group/virtual layer behavior behind its grouping view/context menu. The current tree model infers hierarchy from layer-name separators and recognized suffixes; this must not silently rewrite names merely to look like an Autodesk grouping system. Autodesk's Mac Layers palette describes layer add/delete/rename/property edits, current-layer choice, live property changes, sortable/reorderable columns, and guarded deletion of referenced/current/system layers. Adopt those safe interaction patterns; do not promise fields absent in LibreCAD's layer model or file format path. [Autodesk Mac Layers Palette](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-MAC-Core/files/GUID-8FBBD2EA-EE02-4D34-A82B-2F39AE34B40D.htm).

### Layer data rules and persistence

- Drawing layer names, color/pen, frozen state, locked, printable, construction and other supported attributes belong to the drawing/document and serialize with that drawing. User table columns, widths/order, filter text, selected row, and grouping mode are UI preferences; workspace geometry and dock visibility belong to the workspace profile. Do not persist a per-drawing active row that disagrees with the actual current layer.
- Layer-list model operations must update the document layer list and notify listeners; the palette is a view over that model, not an independent source of truth. Preserve stable layer identity during rename so entities remain attached.
- New Layer should create a valid unique name and use a documented default/inheritance rule. Rename must validate empty names, illegal names and collisions. Delete must refuse or explicitly handle layer 0, the active layer, layers with dependent entities, and external/reference-dependent layers where known. Never show a success state after a rejected operation.
- Multi-layer property edits apply only to fields all selected rows can edit; display mixed values. Bulk visibility/lock/plot changes should be one operation with a clear undo result if the document model supports undo for that property. Current code's widget has direct layer-list operations alongside `undoableModify` for several entity/layer commands, so audit and normalize their undo semantics before presenting all manager edits as undoable.
- Saving/restoring workspace layouts should use the existing named workspace mechanism and stable dock object names. Add a default `2D Drafting` preset; save/restore its dock state via `saveState`/`restoreState` equivalents already wrapped by the application. Version the layout settings so future top-band controls can be added without discarding user dock arrangements.

## Command line and command compatibility plan

The command line is the main command workflow, not a text box that merely starts drawing actions. Preserve the current prompt/history model. Future improvements should use the registered action and prompt state as their source of truth: prefix suggestions over available commands, a separate hint for exact aliases, argument/choice suggestions only while a command prompt explicitly offers those choices, and inline syntax/help for coordinates where the parser accepts them. Tab may cycle deterministic matches; Up/Down should continue recalling history when the suggestion popup is closed and navigate suggestions only when it is open. Escape first dismisses suggestions, then follows the established command-cancel behavior. Enter chooses a visible suggestion or submits input. Keep command-line focus changes predictable and the command prompt visible for commands launched from menus/top controls.

Autodesk documents command-line prompts, Enter-to-accept defaults, keyword options, command aliases, prefix completion, and command history/cycling. Those are reference interaction patterns, not a claim that LibreCAD has the same command grammar or transparent-command support. [Windows command entry reference](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-BB0C3E79-66AF-4557-9140-D31B4CF3C9CF-htm.html). The project's official action table is `lc_commandItems.h`; `RS_Commands::complete()` currently prefix-matches registered full command strings only. Build suggestions from the real registry and parser, not from a scraped AutoCAD command list.

### Initial command name/alias policy

The table below is a planning shortlist. “Current” means present in the action/alias source observed at the pinned commit, subject to command translation and user alias-file override. “Proposed spelling” is allowed only if it maps to the same implemented LibreCAD action **and** its prompts, input grammar, cancellation, and effects are documented as LibreCAD semantics. AutoCAD-looking spellings are aliases, not conformance claims. Entries such as `TRIM` or `OFFSET` must not be advertised as compatible merely because similarly named LibreCAD tools exist.

| Workflow | Current registered LibreCAD token(s), examples | AutoCAD-inspired spelling decision |
|---|---|---|
| Draw line | `line2p`, `line`, `li`, `l` → `ActionDrawLine` | `LINE`/`L` can remain friendly spellings for this actual two-point action. Explain that command options are LibreCAD's. |
| Draw circle | `circle`, `ci`, `c` → circle action; variants include `circle2p` / `c2` / `c2p`, `circle3p` / `c3` / `c3p`, and `circlecr` / `cr` / `ccr`. | `CIRCLE` and `C` are already registered. Preserve the current action and show the supported construction method; do not imply every AutoCAD `CIRCLE` prompt option exists. |
| Draw arc | `arc`, `ar`, `a` → center/radius arc action; variants include `arc3p` / `a3`, `arctan` / `at`, `arc2pl` / `a2l`, `arc2ph` / `a2h`, and `arc2pa` / `a2a`. | `ARC` and `A` are already registered to a particular arc action. Surface other construction modes explicitly; do not imply Windows AutoCAD's prompt sequence. |
| Rectangle | `linerec`, `re`, `rect` → `ActionDrawLineRectangle` (verified in command table). | `RECTANG` may be added as a discoverable alias after collision and prompt review; keep `RE` as existing alias and make option parity explicit. |
| Draw polyline | `polyline`, `pl` → draw-polyline action. | `PLINE` is a candidate alias to the same action; preserve LibreCAD's prompt/finish behavior and avoid claiming AutoCAD width/arc/close options unless implemented. |
| Move | `modmove`, `mv` → modify move/copy action. | `MOVE` is a candidate alias only after confirming its selected-object and copy semantics; same name does not guarantee same prompts. |
| Trim | `trim`, `modtrim`, `tm` → trim/extend action; `modtrim2`, `t2`, `tm2` → multi-trim/extend. | `TRIM` is already registered. Document the implemented action behavior and don't promise AutoCAD's trimming modes. |
| Offset | `offset`, `modoffset`, `o`, `mo`, `moff` → offset action. | `OFFSET` is already registered. Explain actual supported entity/side selection; don't advertise unsupported distance/multiple/erase options. |
| Delete | `moddelete`, `er`, `del` → delete/erase action. | `ERASE` is a possible extra spelling after collision review; preserve current aliases. |
| Undo / redo | `undo`, `un`, `u` → undo cycle; `redo`, `rd`, `r` → redo cycle. | `UNDO`/`REDO` are already registered; retain their current implementation semantics. Review `R` carefully because familiar CAD users may expect a different command. |
| Layer manager / set current | `cnlayer`, `cnly` → `ActionLayersAddCmd`; `cslayer`, `csly` → `ActionLayersActivateCmd`. | `LAYER` must not be aliased to add-layer. Add it only as a new GUI command that opens the manager. `CLAYER`/`SETLAYER` are candidates only after verifying the current-layer action's exact prompt. |
| Properties palette | GUI action exists through the named Properties dock/menu/action path; no confirmed typed command token from the bounded table inspection. | Add `PROPERTIES`/`PR` to show the UI palette only after adding an explicit command action. Keep it separate from entity-property edit commands. |
| Unsupported AutoCAD-specific operations | No mapping unless the feature and action are implemented. | Omit entirely from completion and command documentation. A “not implemented” pseudo-command would mislead users into expecting compatibility. |

Before finalizing this matrix in implementation, audit all `g_commandList` records and action handlers; the examples above illustrate expected review, not a full inventory. Keep built-in English full names and aliases resolvable without blocking translated command forms or user aliases. Alias collisions need explicit resolution and display of the canonical action in completion. Store local user aliases in the existing alias format or a versioned compatible store; never overwrite a canonical command's meaning through a user alias. Completion should include full name, alias(es), short action description, category, and whether the command is valid in the current command state. Search both prefix and (optionally) infix terms only when the user explicitly invokes command search.

## Qt Widgets implementation direction

1. **Action catalog.** Introduce a presentation-neutral command catalog over the existing `QAction`/`RS_Commands` registry. Each entry records canonical LibreCAD ID, translated title, command tokens sourced from `g_commandList` and loaded aliases, tooltip/help, category/group, icon, shortcut, requirements, and implemented status. Make both menus and top-band groups consume this one catalog. Keep action enablement and check-state bound to the existing handler/document state.
2. **Top band.** Implement a custom `QWidget` composed from `QTabBar`/`QStackedWidget`, group widgets, and `QToolButton`s bound to catalog actions. Use `QWidgetAction::setDefaultWidget` only for controls that represent actual current layer/color/lineweight state. Use standard Qt accessibility names, focus traversal, menu keyboard mnemonics, and Mac shortcut mapping. Collapse groups into overflow menus at narrow width; support hide/collapse; persist visible tab and collapsed state in `LC_WorkspacesManager`.
3. **Docks.** Continue using `QDockWidget` and stable object names. Set useful minimum/initial sizes; group Layers and Properties in the right dock area using tabification; command area bottom. Store default layout state using the current workspace serialization mechanism. Don't hard-code pixel-only geometry for Retina scaling or assume Windows title-bar affordances.
4. **Inspector.** Extend current property descriptors and views rather than creating a second property system. Add selection count and mixed-value presentation using the established `LC_PropertyMulti` state. Editors need explicit commit/cancel and validation. Route mutations through document undo APIs and keep layer/pen context stable for selected entities.
5. **Layer manager.** Add a flat table as a second view backed by a `QAbstractTableModel` or adapt the existing `QAbstractItemModel` to flat mode. Use delegates for swatches and tri-state/status cells. Use layer-list listener updates and existing layer operations where sound; move the existing tree to an optional grouping mode. Funnel every document mutation through one layer command/service that makes undo behavior explicit.
6. **Command interaction.** Retain `QG_CommandWidget` and `QG_CommandEdit`; add a suggestion model/view (`QCompleter` or a custom popup) consuming registered full commands and aliases. Do not use `QCompleter` to invent arguments; the active action/prompt owns valid choices. Store command history per session as today unless a user preference explicitly opts into persistent history.
7. **Appearance.** Add semantic palette colors, density and icon sizing in the existing settings/style infrastructure; respect the OS appearance and readable contrast. Keep drawing canvas colors independently controlled by CAD background settings. Avoid copying Autodesk logos, product icons, branded labels, or trade dress assets; use LibreCAD assets and terminology.

## Phases and acceptance criteria

### Phase 0 — inventory and contract

- Produce an action inventory from `g_commandList`, action groups, and `QAction` metadata; mark implemented, partial, and absent workflows.
- Record macOS keyboard/menu and window considerations while using AutoCAD Windows 2027 as the primary design reference.
- Confirm current layer operations' undo/save semantics and property model mixed-value behavior before surface work.
- **Accept when:** every planned tab/group item has a source action, implemented-state label, shortcut policy, and command token policy. No placeholder AutoCAD claims.

### Phase 1 — workspace composition

- Add the Home-first compact task band, quick-access commands, native menu preservation, right tabified Layers/Properties docks, and bottom Command Line dock.
- Add a `CAD Familiar (2D Drafting)` workspace while preserving the current interface as a Classic preset and retaining existing user workspaces, arrangements, aliases, and shortcut preferences.
- **Accept when:** fresh profile opens to drawing-dominant 2D layout; existing profiles upgrade without losing workspace, alias, or user settings; all current menus/actions continue to work; dock show/hide, float, resize, tabify, reset layout, and restart restore are stable at narrow laptop and larger display sizes; VoiceOver/accessibility names and keyboard navigation work.

### Phase 2 — Inspector and layer workflows

Priority update after the Phase 1 user review: start the next master-plan Phase 2 with the crosshair/pickbox, compact command-line presentation and live command prediction described in the [input specification](phase2/00-input-experience.md). Live suggestions, previously listed in the UI workstream's Phase 3 below, are pulled forward into this checkpoint. The broader command-contract audit remains subsequent work.

- Improve selection count/context, mixed values, property section hierarchy, editor commit/cancel and refresh.
- Add flat layer-table mode, useful default columns, sorting/search, visible active layer, and safely guarded layer create/edit/rename/delete/set-current/move-selection operations.
- **Accept when:** selecting one entity shows its actual properties; selecting mixed entity types shows an explicit count/filter and only valid common properties; editing a mixed field updates every eligible selected entity in one undo step; undo/redo restores values and selection predictably; layer state, color and names persist through drawing save/reopen; the current single frozen state is labeled correctly, and the layer constraints supported by the document model are clear and cannot silently corrupt references.

### Phase 3 — command workflow

- Add sorted/ranked suggestions from actual registered full names and aliases, contextual prompts only for actual action choices, duplicate/collision diagnosis, and command catalog help.
- Implement only the high-frequency command aliases approved by the verified action inventory; label the prompts as LibreCAD behavior.
- **Accept when:** exact command and aliases resolve to the same registered action; Tab and history keys behave predictably; Escape cancels through existing action rules; invalid names/options give a useful error and leave drawing unchanged; no unimplemented Autodesk-only command appears in suggestions; menu and top-band invocations produce the same prompt/action state.

### Phase 4 — finish and compatibility audit

- Tune iconography, compact/expanded density, theme/contrast, command band overflow, and workspace preference migration.
- Review official source updates for 2027 Mac and Windows references if implementation starts after the referenced docs change.
- **Accept when:** the Windows-style target remains usable with macOS menus, windows, and keyboard conventions, and workflow demos cover a line/circle/rectangle, mixed selection property edit, layer lifecycle, undo/redo, save/reopen, and alias completion.

## Official references

The references below were checked **2026-10-11**. Autodesk's 2027 product-help pages include evergreen documentation. Windows references are primary for the requested UI model; Mac pages supplement platform-specific considerations.

### AutoCAD for Mac 2027 — supported interaction reference

- [What's New in AutoCAD for Mac 2027](https://help.autodesk.com/view/ACDMAC/2027/ENU/?contextId=WHATS_NEW_2027_ACDMAC) — official 2027 Mac release highlights.
- [Layers Palette](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-MAC-Core/files/GUID-8FBBD2EA-EE02-4D34-A82B-2F39AE34B40D.htm) — layer list, current layer, properties, sorting, column choices, and guarded deletion.
- [About User Interface Customization on Mac OS](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-MAC-Customization/files/GUID-8ACA5693-657B-460C-8A36-0A230CDE7A71.htm) — Mac commands, menus, tool sets, and toolbar.
- [About Setting the Behavior of Palettes (Mac)](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-MAC-Core/files/GUID-9EFE8CE8-0E26-4700-B5A4-C1A5B30B2804.htm) — Mac dock, float, tab, and auto-hide behavior.
- [PROPERTIES command (Mac)](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-MAC-Core/files/GUID-DC3674C5-A4C7-4CF6-9148-9B124DF29B78.htm) — Properties Inspector and common-property behavior for multi-selection/no selection; opened and verified in the 2027 help set.

### AutoCAD 2027 Windows — visual and behavior inspiration only

- [About the Ribbon](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-D20EF1D7-4135-48A7-B68E-65BF3BFF3D70.htm) — ribbon tabs/panels, floating panels, contextual tabs, and workspace relationship.
- [About Object Properties Tools](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-81585857-F1B1-44F4-B7D0-B707386CA721.htm) — common property behavior and ribbon Properties/Layers controls.
- [About Entering Commands in the Command Window](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-BB0C3E79-66AF-4557-9140-D31B4CF3C9CF-htm.html) — prompts, default/keyword input, autocomplete, aliases and history. Evergreen documentation published 2013-04-10 under current 2027 help.
- [About Controlling the Appearance of Palettes](https://help.autodesk.com/view/ACD/2027/ENU/?caas=caas%2Fdocumentation%2FACD%2F2014%2FENU%2Ffiles%2FGUID-CF105B57-A6CB-48B2-B129-6E73C4B74D11-htm.html) — Windows palette docking/anchoring and workspace display.
- [AutoCAD 2027 What's New Commands Reference](https://help.autodesk.com/view/acd/2027/enu/?guid=GUID-B93A458E-1A7F-4090-A8CF-87A31C24E404) — reminder that command inventory changes by release; do not copy its full list into LibreCAD completion.

---

This document's GPL-2.0-or-later notice applies to this document only. The repository's [`LICENSE`](../../LICENSE) describes project licensing; source-file license notices may vary.
