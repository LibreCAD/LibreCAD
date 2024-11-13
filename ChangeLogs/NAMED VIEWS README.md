This pull request is focused on adding initial support of persistent named views as well as some minor improvements.
They ara listed below:

### Named Views
The following functionality was added: 

1) Support of VIEW table on DXF level (read/write)
2) Dockable widget "Named Views" for views management
3) "Named Views" toolbar for saving/restoring views
4) Actions for quick restoring of 5 top views via shortcuts (default shortcuts were not set for them, however)

Of course, only 2D views are supported (no 3D). 

So far, named views for paper space are supported partially - if they are created outside LibreCAD, they are parsed, shown in the widget and saved back to DXF. 
However, so far they can't be created within LibreCAD and there is no navigation to paper space view. 

Implementation of named views on DXF level is interoperable at least with QCAD (so QCAD and LibreCAD can properly open files with named views). 

Interoperability with other CAD software (AutoCAD, Draftsight etc.) for named views should be tested. 

### Action Icon in Options Widget
 
Now it's possible to specify via application preferences that the icon for active action should be shown in options widget too.
This allows to hide Mouse Widget (if status bar is not in "classic" mode) if needed, as its functionality is duplicated by bottom status bar and options widget.

### Variables Viewer

Drawing Preferences dialog was extended and "Variables" tab was added. On that tab there is a table that lists all variables of the drawing as well as their values.

### Localization

Localization files were regenerated to reflect latest changes in the codebase. 

### Source Code layout
The additional sub-directories were added for source code in /lib/engine in order to have more specific and focused source code files hierarchy.
