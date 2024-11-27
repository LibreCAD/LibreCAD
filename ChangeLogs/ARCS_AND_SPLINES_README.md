This merge request includes the following functionality: 

### Arc drawing actions

Several actions were added for drawing arcs. These arcs-related actions are: 

1) **2 points and Angle** - creates the arc using arc's start point, end point and angle of the arc
2) **2 points and Length** - creates the arc using arc's start point, end point and length of the arc
3) **2 points and Radius** - creates the arc using arc's start point, end point and radius of the arc
4) **2 points and Height** - creates the arc using arc's start point, end point and the height of the arc (the distance between chord and middle point of the arc)

Also, support of alternative drawing modes (drawing reversed arc) was added to 
1) **Draw Arc Center, radius** action
2) **Draw Arc by 3 points** action
3) **Arc Tangential** action

### Points-related actions
1) **Lattice of points** - creates XxY lattice of points
2) **Select Points** - selects/deselects point entities in specified window (actually, this is a shortcut for "Select Window" action)
3) **Insert to points** - a variant of "Paste" action, that uses selected Point objects as reference points for copied data.

### Selection of entities by type

**Select/De-select Window** action was expanded and now the user is able to specify which types of entities should be selected/de-selected using actions options widget. 

Selection of entities by selecting area in default action is not affected by this change and works as before.   

### Spline-related actions

Operations with splines were extended by the following actions: 

1) **Spline from Polyline** - allows to create spline (or spline by points) from selected polyline
2) **Append Point** - allows to append start/end point to existing spline/spline by points
3) **Insert Point** - allows to insert control/fit point to existing spline.
4) **Delete Point** - deleted control/fit point of the spline
5) **Delete between 2 points** - deletes control points between two points of the spline
6) **Lines from Spline** - replaces spline by polyline/set of lines. Each segment of spline is replaced by specified amount of line segments (either by one extracted from the document's settings, or by one set explicitly)

Also, fit points for Spline by Points now are properly restored from DXF (so they are not lost after save and re-open of the drawing).

### Polyline Append Point

The action was fixed (or rather completed), so now it supports appending all segment types supported by polyline, not only line as it was before.

### Default Action
Now it's possible to de-select entities if SHIFT is pressed at the end of the overlay box selection.

### UI changes
The structure of menu, toolbars and docking widgets with actions wase chaged to be more precise - so now there are separate categories for points, arcs and splines related actions. 


### Bug Fixes ###
Fixes for the following issues are included:
1) LibreCAD#1950
2) LibreCAD#1920
3) LibreCAD#1951 
4) Fix for finding intersection with construction lines.

### Sources Re-layout
The source code for acs/splines actions was re-layouted for better separation and consistency.
