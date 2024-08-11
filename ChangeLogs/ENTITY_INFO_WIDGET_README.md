### Entity Info Widget

This is a small yet handy dockable widget. It is focused on two major functions: 
1) Displaying properties of selected entity in quick manner;
2) Collection of arbitrary coordinates picked from drawing.

Thus, the widget operates in two modes.

### Demo Video

Small demo clip that illustrates the widget functions is located there: https://youtu.be/H06BQGNLBQ4

#### Common operations
There are several common operations for both modes: 

1) Clearing displayed content;
2) Copying displayed content to clipboard.  
3) Setting relative zero for coordinate value
4) Highlighting points with specific displayed coordinate in drawing;
5) Support of various coordinates modes;
6) Support of context popup menu on view widget by right mouse button (additional menu items are shown if menu is invoked on link).

### Entity Info Mode

This mode of widget is used for displaying the set of properties for selected entity. 

#### Entity Selection

Properties are shown to one entity only. Entity may be selected either (according to setting):

1) If no specific action is in progress (so default action is executed) - just as entity under mouse cursor;
2) For entity under cursor in default action if CTRL/META key is pressed together with mouse move;
3) By explicit selection via action provided by the widget. 

Supported way of entity's selection is controlled by appropriate settings. 

#### Selected Entity
As soon as the entity is selected, the widget displays set of entity properties. 

The list of properties depends on type of selected entity.  

Using options, it is possible to define whether:
1) Entity boundaries (minV, MaxV) should be shown;
2) For polyline entity - whether detailed information about polyline segments (lines and arc) should be shown in addition to vertexes.

Property values that represent coordinates may be used for setting new position of relative zero. 

Also, it is possible to insert coordinates and single values of properties to input control of Cmd widget.

Additional operations: 
1) Selection of entity that is used for displaying properties in drawing.   
2) Invocation of Edit Properties dialog for selected entities. 

#### Coordinates View Mode
For the entity, all relevant coordinates may be shown either as:
* absolute ones
* relative to relative zero point.

#### NOTE: Outdated entity info
In some cases, displayed properties of entity may be outdated. The most possible reason for this - editing of entity via different operations. 
So far internal technical implementation does various operations on individual actions level, therefore it is technically challenged to insure proper realtime refreshing entity properties. 

Therefore, in order to see the actual properties of entities after editing operations, it better to select respecting entity again.

### Coordinates Mode

This mode of the widget allows to pick the set of coordinates from the drawing for later use.    

#### Coordinates selection

Coordinates selection is performed via appropriate action invoked from the widget interface. For picking the coordinate, the user should do mouse click in specific location.

Actually picked coordinate is extracted according to currently used snap mode.  

If *SHIFT* key is pressed, FREE snap is used. 

#### Coordinates list
This mode, the widget displays zero point (that depends on used coordinates mode) and the list of picked coordinates. 
Also, the total length of path between selected points is displayed.

Based on settings, the list of coordinates may also include distance and angle from zero point to each picked coordinate. 

#### Operations with coordinates
In addition to picking specific coordinate, the following operations are supported: 
1) Copying all displayed content to the clipboard;
2) Removing specific coordinate (available via popup content menu);
3) Inserting coordinate in specific position of the list (available via popup content menu);
4) Highlighting coordinate on drawing; 
5) Setting relative zero point in specific coordinate.

#### Coordinate View mode
For coordinates mode, collected coordinates may be displayed in several ways, as:
* Absolute coordinates;
* Relative coordinates to relative zero point;
* Relative coordinates to first picked point in the list; 
* Relative coordinate to previous picked point in the list. 

### Settings
Using settings, it is possible to control:
1) Integration of the widget with default action and entity selection way;
2) Entity display options (boundary coordinates, polyline details)
3) Displaying distance/angel for collected points; 
4) Preview for collected point;
5) Pen used for highlighting coordinate values. 

### Misc notes
At the moment, the content is copied to the clipboard in the same format as it is displayed by the widget. 
However, if it will be useful, support of additional formats (csv, xml etc.) may be added (as well as support of saving it to some file).







 

