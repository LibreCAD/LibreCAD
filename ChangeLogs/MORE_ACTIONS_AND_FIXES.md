This pull requests is focused mostly on adding drawing modification actions and fixing bugs. 

### Modification actions

There are several nice actions, which are quite simple but which are very helpful and convenient for positioning entities.

#### Align 

A really handy action that allows aligning selection vertically and horizontally to entity, coordinate or drawing. 

#### Align Single 

Another variation of entities align that allows to align individual entities one by one to designed base entity or point.  

#### Align Reference Points

Powerful transformation action, allows to move\rotate\scale selected entities transforming them based mapping 2 source points to 2 target points. 

### Drawing Actions

Several utility drawing actions were added: 

#### Middle Point
Creates point in the middle of line between start and end point. Might be considered as a shortcut for Snap Middle Manual, except that it a bit simpler to invoke and allows to specify the amount of middle points. 
The major purpose for such action - use points as corner snaps for further drawings.  

#### Middle Line
Simple action that draws line that connects middle points of lines between endpoints of two lines (with specified offset of endpoints). The major purpose for this action is to simplify drawing axis for parallel lines (say, for bi-section of the hole or cylinder), however, it may be also used for other geometrical drawings. 

#### Bounding Box
Simple action that allows to draw either bonding box around entity or selection (with specified offset), or just draw a points in the corners of the box.

### Polygon Actions
All actions that draws polygons now supports:
1) Rounded corners of created polygon;
2) Entire polygon drawn by polyline. 

Also, the action that allows creation of polygon by 2 points (either Side/Side of edge Or by Vertex/Vertex) was added. 

### Creation of entities COPIES in default action

If CTRL is pressed during mouse button release after dragging selected entities in default action, instead of moving original entities (which is normal behavior), a copies of them will be created in new location.

### More natural positioning for Text/MText
Now it's possible to move these entities using insertion points/second point which now are used as first-class handles. This allows to position texts that use different text alignment easily. 

### Other changes

1) Draw method is added for construction line entity, reference construction line for preview is supported.
2) Layout of some source files was changed. 
3) All entities modification actions that allows the user to select entities first, now supports bulk window selection mode in addition to individual entity selection mode.
4) Setting was added to the Application Preferences that controls whether entities that were modified should stay selected after modification or not.
5) Preview mode for snap middle manual operation was improved. 

### Issues fixed
#1937
#1944
#1949
#1948
#1952
#1957
#1959
#1968
#1969
