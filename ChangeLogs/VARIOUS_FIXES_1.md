### Generic refactoring

Some generic refactorings were performed: 

1) initialization of the application (QC_ApplicationWindow) was reviewed and initialization/assembling logic was extracted to specialized classes. 
2) documents reading/writing logic was encapsulated and moved outside RS_Document (and inherited) entities;
3) the logic of working with drawings in QC_ApplicationWindow was reviewed and refined (events for opening, activation, closing etc.)
4) several invalid/bad dependencies from model entities to view/ui, static references were removed
5) connections were modernized to modern reference-based form of signals and slots
6) not used classes were removed (old CAD Toolbars/widgets) 
7) bugfixes
8) code cleanup, fields renaming (WIP)


### Widget Options
Options dialog was expanded so now it's possible to control specifics of docking better - set nested docking mode and allow vertical title bars

### Block/Library widgets
Inserting block and drawing from library was corrected so now they properly supports rotation angle if UCS is active.

### Command widget
Command widget may be activated by focus action even if the widget is not visible

### Application Preference
1) added settings for naming of auto-save file (Librecad#2061)

### Layers export
Generic refactoring, more export options in the save dialog, ability to save also UCSs and Named Views as part of export. 

### Fixed Issues
* LibreCAD#2103
* LibreCAD#2090
* LibreCAD#2086 (?)
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
