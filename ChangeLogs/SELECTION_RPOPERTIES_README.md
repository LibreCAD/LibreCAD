### Other UI Improvements

Several small improvements for UI are included. 

#### "All Tools" CAD panel (or tools matrix). 

This panel is enabled Application Properties dialog, and if enabled, is replaces all CAD panels by one that includes all tools. 

The main reason for this is better support for CAD panels on small screen resolutions (it simply tookl less screen space). 

Widget Options dialog was also adjusted to support settings for this panel. 

#### Separators in Custom Toolbars and Custom Menus 

Custom Toolbar Creator and Custom Menu Creator now supports adding separators.  

#### Hideable Main menu. 

Top level menu of the application now may be hidden/shown - with appropriate action (default shortcut is **F12**).

### Support of Ortho Angle Snap when Grid Snap is enabled

If "Snap to Grid" is enabled, the Angle Snap (invoked with pressed SHIFT key) now supports ortho mode, so vertical or horizontal restriction is applied based on the mouse position. 
If "Snap to Grid" mode is not enalbed, the Angle Snap allows to snap to specified angle intervals.

### Entities selection mode

Selecting entities was expanded, and now selecting entities my be performed in one of two modes, Additive and Exclusive.

1) In **Additive** mode, when the user selects specific entity(s), they are added to the set of selected entities.
This mode corresponds to historical way of selecting objects in LibreCAD.
2) In **Exclusive** mode, selected entity(s) replaced existing selection, so previously selected entities become unselected.

Deselecting entitiy removes the entity from the selection (as it was before).

#### Quick entities selection

No it is possible to select entities based on values of their specific attributes and specified conditional operator. 

### Properties Widget

New widget was added, that allows to inspect properties of selected entities as well as perform mass modification of them.

#### Workspace minor improvements
Workspace state now includes the state of main menu and fullscreen. 

#### Refactorings 
1) Unified workflow for document modification via LC_UndoSection. Consistent modification of the document and undo state, simplification of actions.
2) Selection changes - via RS_Document as primary interface for this, selected objects stored in container so no need to iterate over the entire document content. 
3) Selection modification listener
4) Selection mode (additive or exclusive, like AutoCAD PICKADD System variable)
5) Copy-paste fixes
6) Massive code cleanup for the entire codebase using several static analyzers - added constness (vars, parameters, methods), making naming (fields, parameters) more consistent, removing redundant/dead code as well as performing generic code cleanup. 
7) Enabled using QStringBuilder for string concantenations (via QT_USE_QSTRINGBUILDER macros), added minor adaptation in code for smooth support of it.

### Minor changes
- Undoable changes of entities in layers via operations performed in Layers Tree widget.
- Ability to perform entities selection without pressed mouse button (similar to AutoCAD rect selection)
- LibreCAD#2375 - added property to specify selection overlay linetype
- Improved angle snap mode. If grid is enabled, now with shift it acts like hor/vert restriction

### Changes in Actions: 

1) New Action - Line Radiant
2) Parallel through point - added "within" mode for multiple copies, that allows to specify that parallels are between point and entity 
3) Modify Rotate - added ability to specify which point should be selected first (reference or rotation center) plus added support of relative angle selection (instead of absolute one)
4) Line Angle (and Horizontal and Vertical) - added new option that defines how to handle length of line. Options are "Line", "By X", "By Y", "Free"


### Visual snap

Clear visual snap

ESC - clears solution
ESC+Shit - removes last addition

Optionally - either RMB click or CTRL+MMB click 
Last solution - CTRL+RMB

Remove document entity from VS - CTRL+ mouse cursor over it

TAB - add mouse position to VS

SHIFT+P  - adds point
SHIFT+L - adds line
SHIFT+C - adds guiding circle
