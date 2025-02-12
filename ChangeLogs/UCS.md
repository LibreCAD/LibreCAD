## User Coordinates System and Angles Basis

This pull request includes relatively small amount of new functionality (yet affects the codebase quite significantly). 

It introduced a couple of concepts, that are new for LibreCAD.

### Angles Basis

Not it is possible to specify positive direction for angles as well as where angle 0 is located. 

In terms of DXF variables, now LC supports both ___ANGDIR___  and  ___ANGBASE___ variables. These variables are stored in drawing.

It is possible to edit settings in Drawing Preferences on Units tab.  

By default, **ANGDIR** is counterclockwise and **ANGBASE** is 0 (so zero angle is at 3 pm) - standard basis of angles used by LibreCAD. 

Values of variables affects (all over the codebase, for all actions and ui): 

1) Handling values of angles, entered by the user for commands via command line or action options.
2) Handling values of angles that are parts of polar coordinates.
3) Displaying angles in UI (in informational cursor, informational actions, entity info widget etc.)
4) Displaying relative polar coordinates.  
5) Snapping via "Snap to Angle"

It is assumed that the user enters angles in currently active angles basis, and such angle values are translated to internal angles in LC default angles system (counterclockwise, 0 at 3pm).

#### Angles Basic Overlay 

For providing the user with information about currently active angles basis, an overlay that displays what is possible direction of angles as well as direction of basis zero angle. 

The appearance of that overall customizable via Application Preferences Dialog. 

#### Angles Basis Toolbar

Also, informational toolbar "Angles Basis" was added. That toolbar also includes information about direction and zero angle value. 

Click on that toolbar invokes Drawing Settings dialog that allows to edit angles basis. 

#### Settings 

Appropriate settings related to angles basis were added to Application Preferences and Drawing Options.

### Angles and Polar coordinates Input formats

Now it's possible to enter angles (and so, polar coordinates) in several formats:

1) Decimal degrees (as it was before)
2) **Surveyor** e.g. N21d33'19.9"E
3) **Radiant** - e.g. 2.7r
4) **Grad** - e.g. 231g
5) **Degrees** - e.g. 85d23'3
6) **Bearing** - e.g. 83b24'7"

Also, there is an option in Application Preferences that allows to disable input in all formats except decimal degrees.

### User Coordinates System (UCS) 

Previously LibreCAD supported only one fixed coordinates system, with origin located in 0,0 and angle of X axis equal to 0 in coordinates of model space of the drawing. 

This coordinate system is still supported and is considered as World Coordinates System (**WCS**).  

Now, a support of the user-defined coordinate systems (**UCS**) was added.  The user is able dynamically specify new zero point of the coordinate system (in WCS coordinates) and direction of X axis. 

As soon as such UCS will be applied, the content of drawing will be rotated on rendering (if necessary) and all coordinates that are entered by the user or be shown to the user will be considered as ones, specified in newly-defined UCS. 

Such approach simplifies creating of non-orthogonal parts of the drawing. 

#### Coordinates Mapping

Internally, all coordinates data are stored in WCS coordinates (as before). 

If custom UCS is active: 

1. All necessary translations of coordinates are performed either during processing of the user's input or as part of rendering process.

2. While the user performs drawing in local UCS coordinates, the drawing data is still in WCS.
   
3. Displaying of user's coordinates (in info actions, entity info widget, informational cursor, coordinates toolbars etc.) is performed in UCS coordinates.

If non-default Angles Basis is active, this is also properly handled, so it's absolutely fine to combine custom UCS and Angles Basis.  

Information about UCS is stored in DXF using standard tag of DXF (so parsing/generation of DXF was extended too).

#### Entering WCS coordinates

If UCS is active, it is possible to enter absolute coordinate in WCS using command line widget. 

WCS coordinates should start with exclamation mark which is __!__.

Format of WCS coordinate: 
1. For cartesian coordinates:  __!X,Y__  (like !200,100)
2. For polar coordinates : **!dist<angle** (like !100<45)

If WCS is active - it still possible to use this format, yet it is not clear what for... 

#### Coordinates System Marker Overlay

Overlay that draws position of UCS and WCS is shown. Which markers are shown as well as their visual appearance is controlled by options in Application Preferences. 

#### UCS Toolbar

A UCS toolbar was added. It provides the user with information about currently active coordinates system. 

#### UCS widget

A dock widget was added for managing the list of UCS of drawing.   

#### Named View widget

Named views widget was extended to provide the information about UCS related to named view. When view is restored, appropriate saved UCS is applied too. 

If UCS is active when VIEW is created, information about that view is stored as part of view.

#### Settings

Related settings were added to Application Preferences and settings of UCS list widget.

#### UCS Limitations

At the moment, there are several limitations in current implementation of UCS support. Mostly, they are related to determining the bounding box of entities within active UCS. 
So far, the entire WCS bounding box is rotated for UCS (instead of proper recalculation of the bounding box for new basis). 

1. If UCS is rotated to WCS, selection of entities by window may work not always correctly (and some additional entities outsider of selecting box may be selected).  
This is due to current selection logic that assumes that selection region represent rectangle parallel to x.
2. Bounding box action creates bounding box in WCS only. 
3. Align and Align Single actions does not support UCS.

### Internal Refactorings

Several internal refactorings were performed. 

#### Mouse event handling methods in actions.

More generic and uniform processing of mouse events was added to base actions. Due to this, implementation of mouse hanlding events in inherited actions now is more iniform and simpler.   
The major reason there was to ensure that mouse events are handled by all actions using the same generic flow.  

#### RS_GraphicView refactoring   

Historically, this class was over-complicated and has too many responsibilities. 

Now the code of it was separated to more focused notions:  

* UI-agnostic LC_GraphicViewport class, that may be considered as part of the model.  
* a set of LC_GraphicViewportRenderer classes, what are responsible for drawing the content of viewport
* RS_GraphicView, that represents a widget that controls viewport and uses renderer for painting. 

With such decomposition, lots of appropriate code now does not depend on objects of UI layer and widgets (like RS_Modification).
Where possible, dependencies to RS_GraphicView were replaced by LC_GraphicViewport. 

RS_Painter was also refactored, both for support of UCS/WCS coordinates translations, and for making rendering to be not dependent on RS_GraphicView. 

In general, the major goal for such refactoring (aside of code separation and simplification, of course) - is to achieve more independent rendering.

Later, this concept could be extended further - to support several viewports in paper space and layouts.     

Thus, this refactoring may be considered rather as a preparation step for later functionality.
 
### Other changes

* Option that allows to disable marking the drawing as "modified" if zoom or pan was performed only was added to the Application Preferences (inspired by johnfound)
* Layout of Application Preferences dialog was changed (again :( ), several pages were added to use more focused groupping of settings.
* Settings that allows to control rendering of arc/circles were added, and now it's possible to define whether arcs should be drawn natively by QT or be interpolated by line segments.
* Added option that allows to ignore "Snap To Grid" mode during snapping if the grid is not visible (inspired by johnfound)
* Added options for fine-tuning of snap constants (inspired by johnfound)
* Line Horizontal/Vertical actions - if there is non-standard UCS basis, options added that allows to create lines, orthogonal to angles basis axis
* Added ability to customize "Draft" mode marker
  
### Bugfixes
A couple of issues were fixed:

* LibreCAD#1978
* LibreCAD#1992
* LibreCAD#2002
* LibreCAD#2012
