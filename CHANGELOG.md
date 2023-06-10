# Change Log

## [2.2.0] - 2022-12-17

### Added
- librecad.appdata.xml for AppImage creation

### Changed
- mitigated DWG open warning and allow opening with errors
- Help/Online menu, use links direct, instead of dialog with double click
- sync libdxfrw (d73a25c)

### Fixed
- missing resources in unix folder (#1570)
- flood of RS_Modification warnings when deleting (#1567)
- clang regression with std::initializer_list<> (#1568)
- missing plugins on Redhat/Fedora and their branches (#1563)
- spline issue on Windows (32-bit) from previous fix
- image mirroring (#1525)

## [2.2.0-rc4] - 2022-06-07

### Added
- symbols to unicode.lff
- CHANGELOG.md - history of changes back to version 2.1.0
- new error code from libdxfrw update
- build Windows, MacOS and Linux packages in one action

### Changed
- sync libdxfrw (8378f39)
- added tool tips to Categories tool widget (#1519)
- added @ symbol to relative coordinates in status bar (#1452)
- changed misleading unit label in curent drawing preferences (#1453)
- changed tab order in text dialog (#1432)
- maximum size of status bar for hidpi displays
- sync libdxfrw (072aecd)
- copy/rename media files in desktop folder to remove whitespaces

### Fixed
- fixed renaming of nested blocks (#1527)
- preserve list position in block and layer list (#1515)
- fixed polyline issue when line type was changed (#1496)
- add minutes label to auto save time in application preferences (#1412)
- add files saved as with new name to recent files (#1364)
- snap on middle point failed for spline through points (#1395)
- spline issue with tolerance on ascii to double conversion
- DXF viewport reading issue by updating libdxfrw
- status bar height toggling on auto save
- solid fill hatch issues
- Ukrainian and symbol characters in unicode.lff
- possible out of bounds read with MText entities
- AppImage was broken by fix for (#1488)
- wrong translations folder for additionals paths from settings
- getDirectoryList() failed on Linux when librecad is in $PATH (#1488)
- bounds check in LWPolyline
- NULL check for hatch code 93
- vulnerabilities in JWW parser

## [2.2.0-rc3] - 2021-11-29

### Added
- more expressive error messages on file open for DXF/DWG files
- deploy AppImage for Linux users

### Changed
- Move to new IRC network
- libdxfrw update
- List entities plugin: Add text insertion point and text content to its output (#1325)

### Fixed
- read failed with binary DXF (#1371)
- small drawing windows in tabbed mode, when open failed
- LFF font issue (#1252)
- issue with detecting LFF font files
- tools/ttf2lff - add more font info and FreeType version to output
- angular dimensions persistently pull single click selection with arrow size 0
- libdxfrw failed to open non LibreCAD DXF files (sf611)

## [2.2.0-rc2] - 2020-12-31

### Added
- multiple blocks selection, toggling, deletion
- multiple layers selection and deletion
- Tiled printing
- Buttons to refresh Library Browser contents
- Console dxf2pdf tool
- filter field to Block List widget
- new plugin Divide
- new pattern millstone.dxf (#964)

### Changed
- Combined Letter / ANSI A, Tabloid / ANSI B
- Changed Ledger to Tabloid
- Block may contain inserts of other blocks except itself
- Print Preview: Line width scaling
- holding the "Shift" key during placing the line endpoint with mouse snaps to 15° step angles (0°,15°,30°,45°,...)
- Optional inversion of zoom direction
- Changing attributes of a block does not change attributes of sub-entities (#755,#1080)
- Optional inversion of mouse wheel scrolling directions
- Respect both tolerance and negative factor values on block import
- bundle all plugin modifications into one undoCycle
- uncheck 'Print to file' checkbox in QPrinterDialog
- Lock/unlock all layers
- SVG export renamed to Export as CAM/plain SVG
- Improve appearance of inside-horizontal dimensions

### Fixed
- libdxfrw - reading DWG hatch entities
- angle not consistent between move and rotate option and dialog (#1137)
- missing subclass marker in PLOTSETTINGS.
- Enhance visibility of black lines on dark backgrounds
- save/restore issue of snap modes (#1147)
- new undo/redo buffer for active line drawing (#1101)
- export black/white with black background used black pen too (#962)
- some more issues with paper size, NPageSize is no paper size, localize paper size names
- equidistant polylines had the correct layer, but not the layers properties (#1201)
- Unhighlight entity after exiting Orthogonal Line tool
- libdxfrw: fix spline parsing
- Page sizes cleaned up, added ANSI
- Print Preview: fix auto zoom on opening
- Print Preview: Line Scaling respects General Scale.
- added auto-zoom after inserting an image
- added more digits to extension lines lenth input box (#1203)
- Print Preview behaves as other windows
- Sync options visibility with window focus
- Print Preview not closing
- Zoom panning can only be used once and must then be reactivated (#1085)
- allow Save As for unmodified files
- Save As could not create new files
- endless loop occurred when trying to save a write-locked file
- using shift key to select has deselected layers/blocks between after mouse button release
- Toggle actions adjustments for Layer/Block Lists multiselection feature
- Respect Layer List Filter on layers removal
- 'move to bottom' command with existing selection
- Unhighlight limiting entity after exiting Trim tool
- Create equidistant polylines in currently selected layer
- "hold shift" to mirroring entities doesn't snap on 15° angles
- Save Block: do not remove blocks while saving
- preview on inserting block with inserts
- Undo block removing
- Window for block editing
- Update window title on block renaming
- Deactivate removed block
- Block cannot contain blocks
- Drawing opts: paper size to orientation relationship
- custom landscape paper size resetting
- pasting from clipboard to block
- Fit to whole page
- Do not highlight selection on Print Preview
- persistence of the angle in a MoveRotate operation
- Improve User Experience while working with multiple drawings (#515,#1108)
- Create a preference to toggle Free Snap mode when SPACE BAR is pressed (#1107)
- modfiy image refresh (#1102)
- Use sentence case for all buttons in popup menus
- add new checkable button and preference to allow whitespace in the command interface (#1098)
- Negative numbers are not accepted for Copy/Move input points (#981)
- Print Preview: Limit paper movement
- Printing respect paper margins
- Print Preview shows paper margins
- Page margins and page preview
- Apply scroll direction inversion to Trackpad
- Exclude only INSERTs from Undo on Block-deep attributes modification
- Change layer of inserts by layer list click should not modify the layers of block entities (#1065)
- Skip re-saving already saved settings values
- disappearance of entities on modification after undo (#1049)
- single dimension tolerance labels made false lines on zoom out (#1060)
- Modification status mark in window title
- segfault (#1048)
- Respect paper scale with 'fixed' checked (#592,#605,#705)
- Exploded entities issues fix (#819)
- libdxfrw violation (#1038,#1045)
- write access violation when a malicious dxf is fed to libdxfrw (#1038)
- random LayerList icons issues on multi layer block imports (#1024)
- 2 segfaults
- Set factor to 1.0 if user inputs 0 on block import
- import block to active layer
- Factor value was ignored on block import
- RowSpacing and ColSpacing values were swapped in Insert properties dialog
- polyline was exploded, when selected while pasting (#932)
- improved Undo/Redo
- asking to save drawing, when closing unmodified drawing
- rotating multiple entities needed multiple undos, for each entity (#1002)
- crashed after cut, paste, undo (#993)
- oversized layer list icons for print and construction (#1021)
- '@' key press didn't activate command box (#1016)
- angular dimensions failed to move, rotate, mirror or scale (#1019)
- missing mouse button hint to clarify properties tool handling (#1008)
- column width in layer list too big since Ubuntu 18.04 (#980)
- Deselect entities on locking all layers
- Spline regression, where the startpoint was at 0,0 by mistake (#940,sf599)
- protect 4th vector when triangle solid is modified (Move,Rotate,Scale,Mirror) (#998)
- SVG export crashed on windows with Image Export
- length extension lines should scale according to the General Scale
- outside offset result of counterclockwise polyline wasn't connected (#972)
- Angular dimensions now respect user's custom label text

### Removed
- ISO B series paper size
- too big and too small printing scales

## [2.2.0-rc1] - 2018-02-16

### Added
- support to draw auxiliary lines
- a new method to draw a polygon
- superscript 5 for all fonts
- architectural units for metric dimensions (#861)
- macOS Sierra support
- gear creation plugin
- font folder selection button
- 'Paste Multiple Commands' to the command line menu (#782)
- Pen Wizard with favorite colors list (#793,#795)
- F11 as the fullscreen hotkey for Linux (#569)
- support for individual coordinate variables (#780,#781)
- ability to load command files (#256,#689,#780,#782)
- user option for loading a variable file on startup (#689,#780,#782)
- alternative relative point syntax (#781)
- multi-command support with variable replacement (#689,#780,#782)
- command line variables (#780)
- easier way to create relative orthogonal points (#779)
- Space/Enter toggle free snap when cmd line is empty or in key code mode (#745)
- Space as alternative to Enter for cmd line & changed cal (#764)
- Ctrl+D for toggling draft mode (#750)

### Changes
- Use a more suitable tabbar for the document list when in tab mode
- layer auto numbering improved
- Make the layerwidget filter field a bit more informative
- Change rotate to allow neg angles
- extend list plugin for more info about inserts
- improved modifier filtering for the command widget (#790,#817)
- improved keycode mode (#802,#804)
- modified the command line's ctrl+v to allow for multi-line input (#782)
- drag and drop for the favorite colors list (#793)
- improved circle anti-aliasing (#778)
- made 5.2.1 the minimum Qt version (#742)

### Fixed
- refactoring of Angular Dimansions (#824,#896,#911)
- 'open with' didn't work on MacOS (#877)
- trackpad scroll when scrollbars are disabled
- Check viewport intersection only through points along the line
- adjusted Angular Dimension to provide inside/outside angles rather than inside/supplementary angles.
- selection of last color in the list for any layer
- Document Surveyor's Units in Current Drawing Preferences AngleFormat now has case specifier for Surveyors units (#883)
- custom page size - incorrect unit handling (#864)
- Improve Current Drawing Preferences - Units tab Angle Precision referenced wrong comboBox (#884)
- gcc 6 builds failed (#880)
- entities painted by plugins become invisible at certain positions on drawing
- Google Code in - fixed vaious spelling mistakes, submitted by participants
- some more undo related exceptions (#830)
- plot equation plugin missed out end value (#838)
- allowed for more widgets to be added to the penwizard (#795)
- possibly eliminated crash on tool activation (#814)
- file open dialog filter is case sensitive on Linux (#791)
- DXF files with comments in SECTIONs failed to load (#808)
- Save after Undo does nothing (#803)
- fixed crash on undo after ctrl+c / ctrl + v (#794)
- ensured - is considered in the new relative syntax (#781)
- ensured command line doesn't hijack hotkeys (#790)
- ensured accurate line previews (#787)
- ensured all objects are shown when a layer is toggled (#784)
- ensured layer 0 is activated when creating a new file (#775)
- ensured proper layer highlighting (#775)
- eliminated crash when pasting before a successful copy
- copied objects are not hidden with other layer objects (#774)
- Right mouse button click did not select block in Block List (#759)
- limit scrolling to drawing + perimeter
- Fix for Trackpads without smooth/hi-res scrolling
- Pasted blocks in Block List widget did not contain proper hatch data (#760)
- changing attributes of a block sub-entities (#755)
- explode action changed container entities attributes to wrong values (#754)
- ensured that closing inactive win doesn't affect the active (#753)

### Removed
- removed wqy-unicode.lff

## [2.1.3] - 2016-09-23

### Fixed
- Shift would not activate the command line (#790,#817)
- Command+Tab didn't always activate the current drawing on OS X (#814,#821)

## [2.1.2] - 2016-09-17

### Fixed
- wouldn’t build with gcc 5.4 and 6
- mouse cursor was missing for ‘Arc Tangential’
- right-click with plugins could cause a crash (#786)
- construction lines were not drawn when the line segment was out of view
- DXF files with comments were not properly loaded (#808)
- drawings were not marked as modified after an ‘undo’
- the command line didn’t accept numpad input
- the command widget didn’t activate properly when floating (#790)

## [2.1.1] - 2016-06-20

### Fixed
- draft mode was not set for new/opened drawings (#749)
- performance did not improve when zooming into an area (#727)
- certain polylines were not rendered properly (#763)
- opening a file didn't restore the layer's construction state (#758)
- the cursor was hidden for 'polyline from existing'
- closing an inactive tab cleared the layer list
- wouldn't build with Qt4

## [2.1.0] - 2016-06-04

