This is just interim PR, that does not bring some notable new functionality - it is rather focused on internal code cleanup and bugfixing. 

### Generic refactoring

Some generic refactorings were performed: 

1) initialization of the application (QC_ApplicationWindow) was reviewed and initialization/assembling logic was extracted to specialized classes. 
2) documents reading/writing logic was encapsulated and moved outside RS_Document (and inherited) entities (partially so far);
3) the logic of working with drawings in QC_ApplicationWindow was reviewed and refined (events for opening, activation, closing etc.)
4) several invalid/bad dependencies from model entities to view/ui, static references were removed
5) all connections were modernized to modern reference-based form of signals and slots (SIGNAL()/SLOT() were almost fully eliminated)
6) not used classes were removed (old CAD Toolbars/widgets)
7) refactoring for actions invocations,  QG_ActionHandler refactoring, separate factory for actions creation, actions adjustments 
8) bugfixes
9) generic code cleanup, fields renaming to conform m_ naming pattern (WIP)
10) Includes optimization, using forward declarations where possible 
11) Entity's layer actions


### Widget Options
The Widget Options dialog was expanded so now it's possible to control specifics of docking better - set nested docking mode and allow vertical title bars

### Block/Library widgets
Inserting block and drawing from library was corrected so now they properly supports rotation angle if UCS is active.

### Command widget
Command widget may be activated by focus action even if the widget is not visible (it will be shown)

### Application Preference
1) added settings for naming of auto-save file (Librecad#2061)
2) added setting for font files rendering if they are open as file - controls how many columns should be used for rendering letters in the opened font's file.

### Layers export
Generic refactoring, more export options in the save dialog, ability to save also UCSs and Named Views as part of export. 

### 2092 - commands for layers
Two simple commands were added to let management of layers via script file. 

1) cslayer - activates layer with specified name
2) cnlayer - creates and activates layer with given name

### Entity's layer actions
Several small actions (and toolbar for them) where added for layer-related operations based on the entity. 

Using them the user is able to select some entity, and for the layer of this selected entity:
* activate layer
* make layer invisible
* lock layer
* toggle layer's printing and construction flag. 

Thus, these actions allows the user to perform quick invocation of some common operations without switching to the Layers List or Layers Tree widgets.  

### Other 

* Support of creation of sharing library for LC_IconEngine in cmake build. So now the dll/shared lib is created.
* Build of plugins for WIN is updated to include plugin-specific RC file
* *.llf and *.cxf files may be also opened by drag&drop them to main window
* copy&paste and generic entities selection functionality improvements

### Fixed Issues
* LibreCAD#2111
* LibreCAD#2110
* LibreCAD#2108
* LibreCAD#2103
* LibreCAD#2098
* LibreCAD#2092
* LibreCAD#2090
* LibreCAD#2086 (?)* 
* LibreCAD#2082
* LibreCAD#2075
* LibreCAD#2074
* LibreCAD#2071
* LibreCAD#2062
* LibreCAD#2061
* LibreCAD#2043
* LibreCAD#2041
* LibreCAD#2013
* LibreCAD#2004
* LibreCAD#1986
