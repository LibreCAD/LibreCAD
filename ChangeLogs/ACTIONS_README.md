
This pull request includes several handy actions for entities editing and modification (see below).

Actions are:
1) Draw Cross (https://youtu.be/H9Lul0J02a0)
2) Draw Star (https://youtu.be/bAD7kkAR5wA)
3) Draw Rectangle - 1 Point (https://youtu.be/eOscpTiWJMY)
4) Draw Rectangle - 2 Points (https://youtu.be/9bjBZAcEmVc)
4) Draw Rectangle - 3 Points (https://youtu.be/IJhG3Rz9U5o)
5) Draw Line of Points (https://youtu.be/8kDSSKg-vqU)
6) Draw Snake Line (https://youtu.be/bXom0IWw4R4)
7) Draw Snake (X) Line (https://youtu.be/bXom0IWw4R4)
8) Draw Snake (X) Line (https://youtu.be/bXom0IWw4R4)
9) Draw Angle From Line (https://youtu.be/ZcnIUXZdSBI)
10) Draw Orthogonal From Line (https://youtu.be/sfO2aUeQ_2E)
11) Draw Line from Point To Line (https://youtu.be/3bEWIryP6jQ)
12) Slice/Divide Line (https://youtu.be/z0I8N73aWFM)
13) Slice/Divide Circle/Arc (https://youtu.be/ILACvThv6I4)
14) Draw Circle by Arc (https://youtu.be/xSJnC-Mvczw)
15) Duplicate entity (https://youtu.be/xSJnC-Mvczw)
16) Lines Join (https://youtu.be/NCqodd0i_gE)
    17) Break/Divide (line, arc, circle) https://youtu.be/_UHawUJXZe0
18) Line Gap (https://youtu.be/0Y0YsNSZIJA)

### Action Details

The following actions are included:

### Cross Action

This action draws cross for selected circle, arc, ellipse or ellipse arc. Intersection of cross lines is located in the center point of entity.

Cross is drawn using active pen and layer.

#### Example Video: https://youtu.be/H9Lul0J02a0

**Options:**

1) **X** - horizontal size value

2) **Y** - vertical size value

3) **Angle** - angle used to rotate the cross (between x-axis and horizontal line)  

4) **Type** - defines type of cross size calculation. There are the following size types:
    - **Extension**  - cross extends selected entity on specified x and y values (so the lengths of lines is radius + specified x or y)
    - **Total Length** - x and y values are considered as absolute length value
    - **Percentage** - x and y values are considered as percentage of entity's radius
 
#### Existing Pre-Selection:

If there are selected entities during action's invocation, cross for each applicable entity type is created based previously on saved settings.

#### Command widget
Action is not fully scriptable, as requires mouse selection of entity.

#### Invocation Commands:

```cross``` or ```cx```

##### Action Commands:

No additional commands

#### Mouse + SHIFT mode:
No support

---
### Star Action
This action draws a star with given amount of rays. It is possible to create symmetric or non-symmetric star, and specify rounding radiuses for outer and inner vertexes.

Star is drawn using active pen and layer.

#### Example Video: https://youtu.be/bAD7kkAR5wA

#### Options:

1) **Symmetric** - flag that indicates whether star is symmetric

2) **Number** - number of rays (min is 3)

3) **Radius Outer** - Rounding radius for outer vertexes of rays (if any)

3) **Radius Inner**
Rounding radius for inner vertexes of rays (if any)

4) **Polyline** Flag indicates whether all elements of start should be drawn as individual entities or as single polyline.

#### Existing Selection:

No support

#### Command widget

Action is fully scriptable and operations may be performed via command widget only. 

##### Invocation Command: 

```star``` or ```st```

##### Action Commands:

Positions for center of star, outer ray vertex and inner ray vertex may be entered as coordinates.
Also, the following commands are supported:

```number``` - specified amount of rays

```radius``` - allows to specify rounding radiuses

```sym``` - specified that star is symmetric

```nosym``` - specifies that start is not symmetric

```nopoly``` - do not create polyline

```usepoly``` - draw star as single polyline

#### Mouse + SHIFT mode: 
For setting center point of star state of the action, allows snapping center to relative zero point. 

----
###  Draw Rectangle (1 Point) Action
This action draws a rectangle of specified size and places it into specified insertion point.

Rectangle is drawn using active pen and layer.

#### Example Video: https://youtu.be/eOscpTiWJMY

#### Options:

1) **Width** - width of rectangle

2) **Height** - height of rectangle

3) **Snap** - defines which part of rectangle will be used as as snap point (and so it will be placed into insertion point)  
there are the following options for snap: 
    * **Top Left** - top left corner of rect
    * **Top** - middle of top edge 
    * **Top-Right** - top-right corner
    * **Left** - middle point of left edge
    * **Middle** - central point of rectangle
    * **Right** - middle point of right edge
    * **Bottom-left** - bottom-left corner
    * **Bottom** - middle of bottom edge
    * **Bottom-Right** - bottom-right corner
4) **Base Angle** - rotation angle of rectangle (betwen x-axis and bottom edge)
5) **Polyline** - Flag indicates whether all elements of rectangle should be drawn as individual entities or as single polyline.
6) **Corners** - Defines now corners of rectangle should be drawn. The following options are available:
   * **Straight** - normal 90 degrees corners
   * **Round** - rounded (arc) corners with specified radius
   * **Bevel** - bevels with specified size 
7) **Edges**  - for _Straight_ corner mode, allows to specify which edges of rectangle should be shown. If not all edges are set, it is possible to draw 2 parallel lines. Options are:
   * **Both** - all edges are draws, so result is rect
   * **Vertical** - only vertical edges are drawn, 2 parallel lines
   * **Horizontal** - only horizontal edges are drawn, 2 parallel lines
9) **Radius** - for _Round_ corners mode, radius of rounding corners arcs
10) **Snap Shift** - for _Round_ corners mode, flag indicates that normal snap point should be adjusted and be located, say, not at the corner of rect, but in the center point of rounding arc
11) **Size inner** - for _Round_ corners mode, if set this flag indicates that size of rectangle should be applied not to outer size of rectangle, but for size between centers of rounding arcs.
12) **Length X** - for _Bevel_ corners mode, specifies bevel size by x-axis
13) **Length Y** - for _Bevel_ corners mode, specifies bevel size by y-axis

#### Existing Selection:

No support

#### Command widget

Action is fully scriptable and operations may be performed via command widget only.

##### Invocation Command:

```rect1``` or ```re1```

##### Action Commands:

Position for insertion point may be entered as coordinates.

Also, the following commands are supported:

```width``` - initiates entering width

```height``` - initiates entering height

```size``` - initiates entering size of rect (as width, height pair, similar to coordinates)

```pos``` -  switches to entering insertion point coordinates

```snap1``` - initiates entering snap mode for insertion point. Commands for snap point type are:
```topl, top, topr, left, middle, right, bottoml, bottom, bottomr```

```snapcorner``` - switches to snap to corner mode (opposite to ```snapshift```)

```snapshift``` - switches to snap to center of rounded corner arc mode (opposite to ```snapshift```)

```sizeout``` -  switches sizes calculation relating to corners (opposite to ```sizein```)

```sizein``` -   switches sizes calculation relating to centers of rounding arcs (opposite to ```sizeout```)

```angle``` -   initiates entering of base angle of rect (angle from corner1 to corner2)

```radius``` - initiates entering rounding radius for corners

```bevels``` - initiates entering of bevels or setting bevels corners mode

```corners``` - initiates entering corners mode. Command values for corners are: ```str, round, bevels```

```edges``` - initiates entering corners mode. Command values for corners are: ```both, hor, vert``` 

```usepoly``` - draw rectangle as single polyline

```usepoly``` - disables drawing rect as polyline (so all elements are individual entities)

#### Mouse + SHIFT mode
If relative zero point is valid, uses it as rectangle insertion point.

----
###  Draw Rectangle (2 Points) Action
This action draws a rectangle by specifying positions of two snap points (specified by settings) of rectangle.
Such customizable snapping mode allows better positioning of created rectangle.

Rectangle is drawn using active pen and layer.

#### Example Video: https://youtu.be/9bjBZAcEmVc

#### Options:

1) **Start Snap** - defines which part of rectangle will be used as a first snap point (and so it will be placed into insertion point)  
   there are the following options for snap:
    * **Corner** - corner of rectangle
    * **Mid-Vertical** - middle of vertical edge
    * **Mid-Horizontal** - middle point of horizontal edge    
    * **Middle** - central point of rectangle
2) **End Snap** - defines which part of rectangle will be used as a first snap point (and so it will be placed into insertion point)  
   there are the following options for snap:
    * **Corner** - corner of rectangle
    * **Mid-Vertical** - middle of vertical edge
    * **Mid-Horizontal** - middle point of horizontal edge
    * **Middle** - central point of rectangle 
4) **Base Angle** - rotation angle of rectangle (between x-axis and bottom edge)
5) **Polyline** - Flag indicates whether all elements of rectangle should be drawn as individual entities or as single polyline.
6) **Corners** - Defines now corners of rectangle should be drawn. The following options are available:
    * **Straight** - normal 90 degrees corners
    * **Round** - rounded (arc) corners with specified radius
    * **Bevel** - bevels with specified size
7) **Edges**  - for _Straight_ corner mode, allows to specify which edges of rectangle should be shown. If not all edges are set, it is possible to draw 2 parallel lines. Options are:
    * **Both** - all edges are draws, so result is rect
    * **Vertical** - only vertical edges are drawn, 2 parallel lines
    * **Horizontal** - only horizontal edges are drawn, 2 parallel lines
9) **Radius** - for _Round_ corners mode, radius of rounding corners arcs
10) **Snap Shift** - for _Round_ corners mode, flag indicates that normal snap point should be adjusted and be located, say, not at the corner of rect, but in the center point of rounding arc
12) **Length X** - for _Bevel_ corners mode, specifies bevel size by x-axis
13) **Length Y** - for _Bevel_ corners mode, specifies bevel size by y-axis


#### Existing Selection:

No support

#### Command widget

Action is fully scriptable and operations may be performed via command widget only.

##### Invocation Command:

```rect2``` or ```re2```

##### Action Commands:

Position for start snap point and end snap point may be entered as coordinates.

Also, the following commands are supported:

```snap1``` - starts entering of snap mode for point 1. Options for snap point type are:
```corner, mid-vert, mid-hor, middle```

```snap2``` - starts entering of snap mode for point 1. Options for snap point type are:
```corner, mid-vert, mid-hor, middle```

```size``` - initiates entering size of rect (as width, height pair, similar to coordinates)

```pos``` -  switches to entering insertion point coordinates

```snapcorner``` - switches to snap to corner mode (opposite to ```snapshift```)

```snapshift``` - switches to snap to center of rounded corner arc mode (opposite to ```snapshift```)

```angle``` -   initiates entering of base angle of rect (angle from corner1 to corner2)

```radius``` - initiates entering rounding radius for corners

```bevels``` - initiates entering of bevels or setting bevels corners mode

```corners``` - initiates entering corners mode. Command values for corners are: ```str, round, bevels```

```edges``` - initiates entering corners mode. Command values for corners are: ```both, hor, vert```

```usepoly``` - draw rectangle as single polyline

```usepoly``` - disables drawing rect as polyline (so all elements are individual entities)

#### Mouse + SHIFT mode
Depends on state of the action.

1) For setting first snap point - uses relative zero point as first snap point.
2) For setting second snap point - draws SQUARE instead of rectangle.

---
###  Draw Rectangle (3 Points) Action
This action draws a rectangle or quadrangle by specifying positions of 3 corner points.

Rectangle is drawn using active pen and layer.

#### Example Video: https://youtu.be/IJhG3Rz9U5o

#### Options:

1) **Quadrangle** - flag that indicates that quadrangle (non 90 degree angle between edges) should be created instead of rectangle
2) **Fixed inner angle** - for _Quadrangle_ mode - defines (if any) inner angle of quadrangle (angle between edges)
3) **Fixed Base Angle** - if specified, base angle (between bottom edge and x-axis) is fixed to specified angle value. If not set, the angle is defined by points for first and second corners.
5) **Polyline** - Flag indicates whether all elements of rectangle should be drawn as individual entities or as single polyline.
6) **Corners** - for _Rectangle_ mode - Defines now corners of rectangle should be drawn. The following options are available:
    * **Straight** - normal 90 degrees corners
    * **Round** - rounded (arc) corners with specified radius
    * **Bevel** - bevels with specified size
7) **Edges**  - for _Straight_ corner mode, allows to specify which edges of rectangle should be shown. If not all edges are set, it is possible to draw 2 parallel lines. Options are:
    * **Both** - all edges are draws, so result is rect
    * **Vertical** - only vertical edges are drawn, 2 parallel lines
    * **Horizontal** - only horizontal edges are drawn, 2 parallel lines
9) **Radius** - for _Round_ corners mode, radius of rounding corners arcs
10) **Snap Shift** - for _Round_ corners mode, flag indicates that normal snap point should be adjusted and be located, say, not at the corner of rect, but in the center point of rounding arc
12) **Length X** - for _Bevel_ corners mode, specifies bevel size by x-axis
13) **Length Y** - for _Bevel_ corners mode, specifies bevel size by y-axis

#### Existing Selection:

No support

#### Command widget

Action is fully scriptable and operations may be performed via command widget only.

##### Invocation Command:

```rect3``` or ```re3```

##### Action Commands:

Position for start snap point and end snap point may be entered as coordinates.

Also, the following commands are supported:

```pos``` -  switches to entering insertion point coordinates

```quad``` -  sets quadrangle mode

```noquad``` - sets rectangle mode

```angle_inner``` - starts entering of inner angle for quadrangle (and enables quadrangle mode)

```width``` -  starts entering width value

```height``` -  starts entering height value

```size``` - initiates entering size of rect (as width, height pair, similar to coordinates)

```snapcorner``` - switches to snap to corner mode (opposite to ```snapshift```)

```snapshift``` - switches to snap to center of rounded corner arc mode (opposite to ```snapshift```)

```angle``` -   initiates entering of base angle of rect (angle from corner1 to corner2)

```radius``` - initiates entering rounding radius for corners

```bevels``` - initiates entering of bevels or setting bevels corners mode

```corners``` - initiates entering corners mode. Command values for corners are: ```str, round, bevels```

```edges``` - initiates entering corners mode. Command values for corners are: ```both, hor, vert```

```usepoly``` - draw rectangle as single polyline

```usepoly``` - disables drawing rect as polyline (so all elements are individual entities)

#### Mouse + SHIFT mode
Depends on state of the action.

1) For setting first corner - uses relative zero point as first snap point.
2) For setting second corner - uses Angle snap (similar to line) for defining base angle of rectangle
2) For setting third corner - draws SQUARE instead of rectangle.

---
###  Draw Line of Points Action
This action draws a set of points located on the same line. Distance between points may be either fixed or calculated based on specified number of points. 

Rectangle is drawn using active pen and layer.

#### Example Video: https://youtu.be/8kDSSKg-vqU

#### Options:

3) **Angle** - if specified, defines direction angle for the line
1) **Number of Points** - amount of inner (non-edge) points to draw (not always applicable). 
2) **Fixed Distance** - flag that indicates whether distance between points is fixed (otherwise, it's calculated based on length of line and number of points) 
3) **Fit Line** - for _Fixed Distance_ mode. If enabled, amount of points will be determined dynamically based on the length of line and distance. If disabled - only specified number of points will be created.
6) **Edge Points** - defines how to draw points on edge points of line. 
    * **None** - no points are in start and end point of line
    * **Both** - points are placed in start and end points of line
    * **Start** - point is in start point, no point in end point
    * **End** - point in end point, no point in start point of line

#### Existing Selection:

No support

#### Command widget

Action is fully scriptable and operations may be performed via command widget only.

##### Invocation Command:

```linepoints``` or ```lpoints```

##### Action Commands:

Position for start point of line and end point line may be entered as coordinates.

The following commands are supported:

```angle``` -   initiates entering of line angle

```x``` -   fixes direction of line to horizontal and ask for entering line length

```y``` -   fixes direction of line to vertical and ask for entering line length

```p``` -   switches line to free direction (point) as it is supported by default line drawing action. Default directions mode

```number``` -  initiates entering number of points

```dist_fixed``` -  switches to fixed distance mode between points

```dist_flex``` -  switches to calculated distance mode between points (opposite to _dist_fixed_)

```distance``` -  for _Fixed Points Distance_ mode, initiates entering distance between points

```fit``` -  for fixed distance mode, ensures that all point are within the line defined by start and end point. Option for number of points is ignored.

```nofit``` -  for fixed distance mode, indicates creation of point according to distance between points and number of points starting from start point of line. Endpoint of line is used for specifying line direction only.  

```edges``` - initiates entering edge points mode. Command values for corners are: ```none, both, start, end```. There is also shortcuts for edge points mode, single commands as ```edge_none, edge_both, edge_start, edge_end```

#### Mouse + SHIFT mode
Depends on state of the action.

1) For setting first point state - uses relative zero point as first snap point.
2) For setting second point state 
   - if angle of line is not fixed - uses Angle snap (with 15 degrees step) for defining angle of line
   - if angle of line is fixed - uses angle that is mirrored to specified (180 degrees - angle)

---    
###  Draw Snake Line Action
This action draws a set of line segments (similar to standard draw line action). The major intent of this action - simpler usage via command widget, as well as support of fixed angle direction. 
Action is the most suitable for drawing the set of horizontal/vertical lines, as it automatically switch direction mode. 

Rectangle is drawn using active pen and layer.

#### Example Video: https://youtu.be/bXom0IWw4R4

#### Options:

1) **Direction** - controls direction of line drawing. The following modes are supported: 
   * **X** - horizontal line segment will be drawn, and after setting endpoint of it direction will be switched to Y
   * **Y** - vertical line segment will be drawn, and after setting endpoint of it direction will be switched to X
   * **Point** - line segment will be drawn in arbitrary direction (as in default draw line action)
   * **Angle** - line segment will be drawn in specified angle
2) * **Rel** - if _Angle_ direction is set, flag indicates whether angle value is absolute or relative to previous segment

Also, it's possible to close line, create polyline and perform undo/redo operations via options buttons. 

#### Existing Selection:

No support

#### Command widget

Action is fully scriptable and operations may be performed via command widget only.

##### Invocation Command:

```sline``` or ```sli``` or ```sl```

##### Action Commands:

Position for start point of line and end point of line segment may be entered as coordinates.

The following commands are supported:

```x``` -   fixes direction of line to horizontal and ask for entering segment length

```y``` -   fixes direction of line to vertical and ask for entering segment length

```p``` -   switches line to free direction (point) as it is supported by default line drawing action. Default directions mode

```angle``` -   initiates entering of line angle

```anglerel``` -  specifies that relative angle should be used

```close``` -  closes the line if possible (by connecting enpoint of last segment and start point of first segment)

```undo``` -  perform undo operation and removes last segment

```redo``` - performs re-do operation if possible

```start``` - switches action to setting new start point of line segment

```polyline```or ```pl``` - creates polyline

#### Mouse + SHIFT mode
Depends on state of the action.

1) For setting start point state - uses relative zero point as first snap point.
2) For setting second point state
   - if angle of line is not fixed - uses Angle snap (with 15 degrees step) for defining angle of line
   - if angle of line is fixed - uses angle that is mirrored to specified (180 degrees - angle)

---
###  Draw Snake (X) Line Action
This action is actually the same as **Snake Line**, except that it starts new segment series with X-mode 
enabled by default, so it is convenient for drawing horizontal lines.

#### Example Video: https://youtu.be/bXom0IWw4R4

##### Invocation Command:

```slinex``` or ```slix``` or ```slx```

---
###  Draw Snake (Y) Line Action
This action is actually the same as **Snake Line**, except that it starts new segment series with Y-mode
enabled by default, so it is convenient for drawing vertical lines.

#### Example Video: https://youtu.be/bXom0IWw4R4

##### Invocation Command:

```sliney``` or ```sliy``` or ```sly```

---
###  Draw Angle From Line Action
This action draws a line that is directed by specific angle to selected line. In general, it is similar to **Relative Angle** action, however:   
   * it has fine-grained support of positioning angle line within base line
   * allows specifying length of angle line by value and by mouse
   * support both absolute and relative angles
   * can create angle line with some offset from base line
   * can divide base line to two segments based on lines intersection points

Line is drawn using active pen and layer.

#### Example Video: https://youtu.be/ZcnIUXZdSBI

#### Options:

1) **Length** - length of angle line
2) **Free** - if this flag is enabled, instead of fixed lenght of angle line, the user should specify end point of angle line
3) **Angle** - angle value
5) **Rel** - Flag that controls whether absolute (from x-axis) or relative (taking into consideration own angle of base line) angle should be used. 
6) **Line Snap** - defines which snap point on base line should be used. Values are:
   * **Free** - any point within base line, the user should select it
   * **Start** - start point of base line
   * **Middle** - middle point of base line
   * **End** - end point of base line
3) **Snap Distance** - distance from snap point on the base line to the point where angle line will intersect with base line. Not applicable for _Free_ Line Snap mode.    
7) **Tick Snap**  - defines which point on angle line should be used as snap (intersection) on base line. Values are:
   * **Start** - start point of angle line will be snapped to base line
   * **Middle** - middle point of angle line will be snapped to base line
   * **End** - end point of angle line will be snapped to base line
9) **Offset** - value of offset of the angle line snap point from base line (so it is possible to have a gap between line, if needed) 
10) **Divide** - if this flag is set, base line will be divided on two line segments, based on intersection point between angle line and base line.

#### Existing Selection:

No support

#### Command widget

Action is not scriptable, as it requires selection of base line by mouse. 

##### Invocation Command:

```angleline``` or ```aline```

##### Action Commands:

No additional commands are supportes. 

#### Mouse + SHIFT mode

Uses alternative mirrored angle (180-angle) instead of specified one.

---
###  Draw Orthogonal From Line Action
This action is actually the same as **Angle From Line**, except that angle is fixed and is 90 degrees
so it is convenient for creation of perpendicular lines.

#### Example Video: https://youtu.be/sfO2aUeQ_2E

##### Invocation Command:

```ortline``` or ```oline```


---
###  Draw line From Point To Line Action
This action draws a line from selected point to specified target line, with possibility to specify the angle of intersection of base line and created line. 

Line is drawn using active pen and layer.

#### Example Video: https://youtu.be/3bEWIryP6jQ

#### Options:

1) **Orthogonal** - if flag is set, the action will create a  line that is perpendicular to target line and that starts in selected point.  
2) **Angle** - if not perpendicular mode, specifies that angle between created line and target line.
3) **Size** - defines which size of created line should be used. Values are:
   * **To intersection** - line will be created from selected point to intersection with target line
   * **Fixed Length** - fixed length line will be created from selected point in direction to intersection with target line   
4) **Length** - applicable for _Fixed Length_ mode, represents a length of created line.
6) **Tick Snap**  - applicable for _Fixed Length_ mode, defines which point on created line should be used as snap point (located in initial point). Values are 
   * **Start** - start point of created line will be snapped to initial point
   * **Middle** - middle point of created line will be snapped to initial point
   * **End** - end point of created line will be snapped to initial point
9) **End Offset** - applicable for _To intersection_ mode, represent offset of end point of created line from target line.  

#### Existing Selection:

No support

#### Command widget

Action is not scriptable, as it requires selection of target line by mouse.

##### Invocation Command:

```point2line``` or ```p2l```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

Uses alternative mirrored angle (180-angle) instead of specified one (for non-orthogonal mode).


---
###  Slice/Divide Line Action
This is combined action, that allows to draw specified number of tick lines along selected target line using specified length and angle to target line, 
and also optionally divides target line by points of ticks intersections. If tick length is set to 0, may just divide target line to the set of line segments.

In general, this action may be also used as a replacement to "Divide" plugin. 

Tick lines ares drawn using active pen and layer, if entity is divided - original attributes are used for created segments.

#### Example Video: https://youtu.be/z0I8N73aWFM

#### Options:

1) **Fixed** - flag defines whether distance between tick lines is fixed or should be calculated.
2) **Distance** - for **Fixed** tick distance mode, defines distance between individual tick lines
3) **Count** - for non-**Fixed** tick distance mode, defines amount of inner (non-edge) ticks to be created. 
4) **Tick Length** - length of each tick line. If **Tick Length** is 0, it is possible just divide line by tick positions.
5) **Angle** - angle value used to draw tick lines
6) **Rel** - flag that indicates whether angle is absolute (to x-axis) or relative (to target line)
7) **Offset** - offset distance between target line and tick snap point
8) **Tick Snap**  - defines which point on tick lines should be used as snap point (located on intersection with base line). Values are
   * **Start** - start point of created tick line will be snapped to target line
   * **Middle** - middle point of created tick line will be snapped to target line
   * **End** - end point of created tick line will be snapped to target line
9) **Edge Tick** - defines how tick lines should be drawn at endpoints of target line. Values are: 
   * **None** - no tick lines at edges of target line
   * **Both** - tick lines are in both edges of target line
   * **Start** - tick line in start point of target line
   * **End** - tick line in end point of target line
10) **Divide** - if flag is set, in addition to creation of tick lines, the action will divide target line to the set of line segments. Division points are tick lines snap points. 

#### Existing Selection:

No support

#### Command widget

Action is not scriptable, as it requires selection of target entity by mouse.

##### Invocation Command:

```slicel``` or ```sll```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

Uses alternative mirrored angle (180-angle) for tick lines instead of specified one.

---
###  Slice/Divide Circle/Arc Action
The action is similar to Slice/Divide Line action, yet instead of line it operates with circles and arcs.
This is combined action, that allows to draw specified number of tick lines along selected target circle or arc using specified length and angle to entity,
and also optionally divides target line by points of ticks intersections. If tick length is set to 0, may just divide target sentity to the set of arc segments.

Line is drawn using active pen and layer.

#### Example Video: https://youtu.be/ILACvThv6I4

#### Options:

1) **Count** - defines amount of inner (non-edge) ticks to be created.
2) **Tick Length** - length of each tick line. If **Tick Length** is 0, it is possible just perform divide 
3) **Angle** - angle value used to draw tick lines
4) **Rel** - flag that indicates whether angle is absolute (to x-axis) or relative (to target entity)
5) **Offset** - offset distance between target entity and tick snap point
6) **Tick Snap**  - defines which point on tick lines should be used as snap point (located on intersection with target entity). Values are
   * **Start** - start point of created tick line will be snapped to target entity
   * **Middle** - middle point of created tick line will be snapped to target entity
   * **End** - end point of created tick line will be snapped to target entity
7) **Edge Tick** - defines how tick lines should be drawn at endpoints of target arc (not applicable to circle). Values are:
   * **None** - no tick lines at edges of target arc
   * **Both** - tick lines are in both edges of target arc
   * **Start** - tick line in start point of target arc
   * **End** - tick line in end point of target arc   
8) **Start Circle Action** - if target entity is circle, defines start angle from which tick line positions will be calculated.    
9) **Divide** - if flag is set, in addition to creation of tick lines, the action will divide target entity to the set of arc segments. Division points are tick lines snap points.

#### Existing Selection:

No support

#### Command widget

Action is not scriptable, as it requires selection of target entity by mouse.

##### Invocation Command:

```slicec``` or ```slc```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

Uses alternative mirrored angle (180-angle) for tick lines instead of specified one.

---
###  Draw Circle by Arc Action

Simple action that creates circle based on selected arc entity (or ellipse by ellipse arc). Center of circle is the same as center of arc, radius of created circle may be either the same of with specified offset to arc's radius. 
Also, it is possible to specify:
 * in which layer (original or active) created circle should be placed to
 * which pen should be used for circle
 * should circle replace arc or not.

#### Example Video: https://youtu.be/xSJnC-Mvczw

#### Options:

1) **Replace arc** - flag defines whether original arc should be deleted or not
2) **Radius Shift** - if arc is not replaced, allows to specify offset to original radius of arc. Thus it is possible to create circle with offset relating to arc. 
3) **Pen**  - defines which pen should be applied to created circle. Options: 
   * **Active** - active pen
   * **Original** - original pen of that was used by arc
   * **Original Resolved** - resolved pen that was used by arc (so "By Layer" etc. attributes are not used and actual values are applied)
7) **Layer** - defines to which layer created circle will be placed
   * **Active** - active layer
   * **Original** - the same layer as one of arc's layer. 
   
#### Existing Selection:

If there are selected entities on action's invocation, for arcs creates circles based on previously saved settings. 

#### Command widget

Action is not scriptable, as it requires selection of target arc by mouse.

##### Invocation Command:

```circlebyarc``` or ```cba```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

No support

---
###  Duplicate Entity Action

Creates duplicate of entity with optional offset and possibility to specify which layer and pen should be applied. Action may be considered as a kind of shortcut for copying via "Move/Copy" action. 

#### Example Video: https://youtu.be/xSJnC-Mvczw

#### Options:

   1) **In Place** - flag defines whether entity's duplicate will be created in the same coordinates as original entity or whether it should be with offset. The major purpose of creation of in-place entity - make duplicate of entity in the same position as original, but in active layer  
2) **Offset X** - if duplicate is created not _In Place_, specifies coordinates offset by x-axis for duplicate entity from original entity. 
2) **Offset Y** - if duplicate is created not _In Place_, specifies coordinates offset by x-axis for duplicate entity from original entity. 
3) **Pen**  - defines which pen should be applied to created duplicate. Options:
   * **Active** - active pen
   * **Original** - original pen of that was used by original entity
   * **Original Resolved** - resolved pen that was used by original entity (so "By Layer" etc. attributes are not used and actual values are applied)
7) **Layer** - defines to which layer created duplicate will be placed
   * **Active** - active layer
   * **Original** - the same layer as layer of original entity.

#### Existing Selection:

If there are selected entities on actions' invocation, duplicates for them will be created based on previously specified settings. 

#### Command widget

Action is not scriptable, as it requires selection of source entities by mouse.

##### Invocation Command:

```duplicate``` or ```dup```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

No support

---
###  Line Join Action

Joins to selected line to their intersection point. Based on options, action may
   * extend lines
   * add additional segments to intersection point
   * trim line. 

For lines that lies on the same ray, it is possible to merge them into one line.  

#### Example Video: https://youtu.be/NCqodd0i_gE

#### Options:

1) **Line 1** - defines which operation should be performed with first selected line. Values:
   * **Extend/Trim** - the line will be extended or trimmed, thus so its edge will be in intersection point
   * **Add segment** - the line will not be affected, however, line segment will be created from the edge of line to intersection point 
   * **No Change** - line will be not affected.  
2) **Line 2** - defines which operation should be performed with second selected line. Values:
   * **Extend/Trim** - the line will be extended or trimmed, thus so its edge will be in intersection point
   * **Add segment** - the line will not be affected, however, line segment will be created from the edge of line to intersection point
   * **No Change** - line will be not affected.
3) **Attributes** - defines how pen and layer attributes should be applied during joining segments. Options are: 
   * **Active** - active pen and layer will be used
   * Line 1 - attributes of line 1 will be applied
   * Line 2 - attributes of line 2 will be applied
   * Both lines - attributes from each line will be applied to corresponding line segment.
4) **Create polyline** - defines whether polyline that joins two lines (or segments) should be created or whether individual lines should be used. 
4) **Remove originals** - defines whether original lines should be removed.  

#### Existing Selection:

No support

#### Command widget

Action is not scriptable, as it requires selection of source entities by mouse.

##### Invocation Command:

```linejoin``` or ```lj```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

No support

---
###  Break/Divide Action

Action allows to break or divide selected line, arc or circle to segments based on intersection points with another entities.

#### Example Video: https://youtu.be/_UHawUJXZe0

#### Options:

1) **Remove Segments** - if specified, segment of entity will be removed (so original entity will be split to remaining parts). Otherwise, entity will be divided.   
2) **Remove Selected** - if segments removal is set, this flag determines which segment should be removed. If set, segment of entity that was selected by mouse that that lies between intersection points will be selected. If not set, selected segment will survive, but other parts of entity will be removed. 

#### Existing Selection:

No support

#### Command widget

Action is not scriptable, as it requires selection of source entities by mouse.

##### Invocation Command:

```breakdivide``` or ```bd```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

No support

---
###  Line Gap

Action creates gap in specified line. 

#### Example Video: https://youtu.be/0Y0YsNSZIJA

#### Options:

1) **Gap Size** - length of the gap
2) **Free** - if enabled, length of gap is not fixed and the user should specify end point of gap
3) **Line Snap** - defines how gap start point should be snapped to the line
   * **Free** - any point within selected line, the user should select it
   * **Start** - start point of selected line
   * **Middle** - middle point of selected line
   * **End** - end point of selected line
4) **Snap Distance** - distance of snap point from line snap point
6) **Gap Snap**  - if not _Free_ size gap mode, defines how gap should be positioned to snap point.   
   * **Start** - start point of gap will be snapped
   * **Middle** - middle point of gap will be snapped
   * **End** - end point of gap will be snapped

#### Existing Selection:

No support

#### Command widget

Action is not scriptable, as it requires selection of line by mouse.

##### Invocation Command:

```gapline``` or ```gl```

##### Action Commands:

No additional commands are supported.

#### Mouse + SHIFT mode

If _Line Snap_ is not _Free_, mirrors edges of line (i.e setting for start edge of line are applied to end edge, and vise versa). 

---
### Other functionality

There is convenient base classes with predefined lifecycles and utility methods: 
1) Basic class for actions (LC_AbstractActionWithPreview)
2) Basic class for action options widget (LC_AbstractOptionsWidget)

Also, a couple of convenient utility functions were added: 

1) Automatic reopening files that were open during last close of application (controlled by the "Open last opened files" setting on "Defaults" tab in Application Preferences). This is good timesaver for development and debugging, yet it might be useful for users too.
2) Setting for defining amount of columns for left toolbar (located in Widget Options). Using this setting, it will be possible to address left toolbar size for small resolutions.  

**Video** for settings is there: https://youtu.be/_f-mgKy449c

### Ideas for generic improvements

There are several issues that might be discussed and implemented: 

1) Different way of handling action options widgets lifecycle

Such approach does not rely on static fields, as action creates and holds the reference to options widget. Examples - actions within pull request.   

2) Better support of relative zero with mouse-based operations.

In general, there is support for relative zero point in command (via shortcut and relative coordinates settings).

However, it is not possible to snap to relative zero point using mouse (say, use existing relative zero as starting point for line etc.)

New actions (where practical) lets the user start the action with relative zero point as initial point (if SHIFT is pressed during mouse move) event. 

Adding such functionality to existing actions may deliver better user experience and be convenient for the user. 


