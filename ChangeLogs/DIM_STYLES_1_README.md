## Dimension Styles and Context Actions

This pull request contains the following major functionality:

1.  Initial support of dimension styles.
2.  Changes in dimension-related actions; new actions for dimension styles.
3.  Support of the "context entity" by actions.
4.  Reworked default context popup menu for drawing area; support of entity-specific context menus.
5.  Reworked Custom Toolbar Creator and Custom Menu Creator; wide support of invocation shortcuts for custom menus.
6.  Added support of the drawing's metadata (author, title, subject, description, etc.).
7.  Added support of predefined and custom user data for the drawing document.
8.  Interactive input of action's parameters and entities properties - Measure Instead of Type. 
9.  Keyboard support improvements; moving selected entities by keys.
10. Various minor fixes and improvements.

### Dimension Styles

The major focus of this PR is to provide full support of dimension styles reading, editing, and storing by adding DXF parsing and writing functionality, as well as adding UI needed for dimension styles management and editing.

**NOTE: Support of rendering of dimensions according to dimension style attributes is STILL LIMITED and NOT ALL attributes of dimension style affect created dimensions!**

The logic of dimensions creation should be reworked deeper, and full support of dimension style attributes will be included in the next Pull Request.

#### Dimensions Styles Management

Dimension styles included in the drawing are managed using the Drawing Preferences dialog, in the Dimensions tab.

`XXX_IMAGE`

Here the list of dimension styles is present, as well as a set of corresponding related commands, such as:

*   New Style
*   Remove Style
*   Edit Style
*   Rename Style
*   Mark as Active (so it will be used as default for newly created dimensions)
*   Export/Import

A live preview of dimensions with the applied dimension style is also available.

#### Editing Dimension Style

Editing of the dimension style attributes is performed via a new dialog that contains UI for entering attributes and a preview area that reflects dimension style rendering.

`XXX_IMAGE`

**Demo Video:** https://youtu.be/bYCbS-gcDR8

#### Dimension Entity Properties, Style Override

The dialog for dimension entity properties now includes functionality for selecting a dimension style.

Also, using that dialog, it is possible to specify a dimension style override. A style override is a set of dimension style attributes that is specific for a particular dimension only.

`XXX_IMAGE`

**Demo Video:** https://youtu.be/aOacuCBkwFA

Using style override, it is possible to fine-tune individual dimension entities.

#### Specifying Dimension Style During Dimension Entity Creation

The dimension options toolbar now includes a style selection combobox.

`XXX_IMAGE`

### Dimension Actions

Several changes were introduced to actions related to dimensions.

#### New Actions

Several new actions related to dimensions were added:

1.  **Copy Style Action** - allows you to pick the style from one dimension entity and apply it to another one.
2.  **Regenerate Dimensions Action** - forcefully re-creates a dimension.
3.  **Dimension Styles Action** - invokes the Dimension Styles manager.

`XXX_IMAGE`

**Demo Video:**  https://youtu.be/lGD_dR25VOU

#### Changes in Dimension Related actions - faster dimensioning

1. For linear dimensions, it's possible to select line entity (that might be standalone or a part of polyline) using context menu or pressed CTRL key.
Linear, Aligned, Horizontal or Vertical dimension will be created with base points that corresponds to the line start and end points.
   
2. In action used for creation of Horizontal or Vertical dimension, now it is possible to change direction using CTRL key modifier. 

**Demo Video:** https://youtu.be/Qt2Q3daDQhU

### Support of "Context Entity" in Actions

Now all actions support the notion of a "Context" entity - the entity that is passed to the action on action's invocation.

If a context entity is available, the action will be executed for that entity only (regardless of whether a selection is present in the drawing). Also, where possible, the initial entity selection state of the action is skipped, and so the user is not prompted to select an entity for the action's execution.

At the moment, the context entity is set to the action via the context popup menu (if the user invoked the menu near an entity).

`XXX_IMAGE`

However, later this concept may be used, for example, in plugins.

#### Context Menu

The default popup menu that is shown in the drawing area by a right mouse button click was reworked to include more operations and support more efficient drawing operations. The popup menu in the graphic view is now truly context-aware.

There are two major modes of the menu's operation:

1.  **Generic Popup menu:** Shown when no selected entities are present and the click is not on an entity.
    XXX_IMAGE 
 
2.  **Selection Popup menu:** Show it there are selected entities in the drawing and the click point this is outside of drawing entity;
XXX_IMAGE

3.  **Entity Context menu:** Shown when menu is invoked with a click on an entity.
    `XXX_IMAGE`

For the context menu, the structure of the menu is adapted to correspond to the clicked entity type, and it reflects operations that are relevant for the selected entity.

`XXX_IMAGE`

The entity for the context menu is pre-selected and is passed to the invoked action as a "context" entity.

Due to this, the user is not required to select the entity again during the action's invocation, and thus the drawing operation is performed in a more convenient and faster way.

**Demo Video:** https://youtu.be/Px-JoJdCJg4

### Custom Toolbar and Custom Menu Creator

The Custom Toolbar Creator tool was reworked internally; from a UI point of view, it's a small facelift without significant changes to the functionality.

`XXX_IMAGE`

The Custom Menu Creator tool was also reworked internally, with a couple of important improvements:

1.  Added wider support of invocation shortcuts. The user may specify various key modifiers and mouse buttons that are used for the menu invocation.
    `XXX_IMAGE`
2.  Assigned invocation shortcuts are visible in the list of menus.
    `XXX_IMAGE`
3.  Automatic invocation of actions in menus that contain only one action. This mechanism allows the invocation of a specified action using an assigned mouse/keys shortcut.

Custom menus are also context-aware; if a menu is clicked on an entity, that entity will be passed to the action.

Also, it is possible to create several menus for the same shortcut but for different entity types. 

**Demo Video:** https://youtu.be/m-RyW-Vq8r4

### Support of Metadata for Drawing

It is now possible to manage DXF metadata fields in the Drawing Preferences dialog.

`XXX_IMAGE`

### Support of User Data

User data of the drawing document are manageable in the Drawing Preferences dialog.

`XXX_IMAGE`

### Other Changes in Actions

In addition to support of context entity, there are small changes in action workflows:

1.  "Select Intersecting Entities" action now allows you to specify an entity, and all entities that intersect it will be selected.
2.  Linear dimensions actions allow you to select a line for faster creation of the dimension.
3.  Dim Ordinate Action - if invoked with a context, may snap to a line endpoint or circle/arc center.
4.  Trim and Trim Two actions - if invoked with a context, consider the provided entity as an entity that should be trimmed.
5.  Draw Arc Tangential action - if invoked with a context, starts arc creation from the nearest endpoint of the selected entity.
6.  Draw tangential lines actions - state depends on provided context entity type.
   
**Demo Video:**

### Interactive Input

This update introduces a new user experience. The user is now able to measure a value on the drawing instead of directly typing that value into an input field. This makes it possible to enter values that are equal to other geometry values present in the drawing.

The following types of values are supported:

*   Position coordinates
*   Linear value (length)
*   Angle value

#### Interactive Input in Tool Options Toolbar

The Tool Options toolbar may now include buttons that initiate interactive input of values.

There is a setting in the Application Preferences dialog that allows you to enable or disable interactive input support for Tool Options.

`XXX_IMAGE`

#### Entity Properties Dialog

The Entity Properties dialog now includes support for interactive input.

`XXX_IMAGE`

#### Specifics of Measuring Values

When interactive input is initiated, the user is prompted to measure the corresponding value on the drawing.

Supported functionality depends on the type of value being measured. 

##### Pick Point Coordinates

For point coordinates, the user should select a location and click on it. The coordinates of that location will be set to the appropriate inputs. 

##### Pick Distance

For measuring distance, there are several possible modes:

1.  **Major flow:** The user specifies positions of the first and second point; the resulting length is calculated as the distance between these points.
2.  **SHIFT pressed:** Distance is determined based on the geometry of the object under the mouse cursor:
    *   Line - the length of the line is selected.
    *   Circle, Arc - the radius is selected. With CTRL pressed - the diameter is picked instead of the radius.
3.  **CTRL pressed:** For a line under the cursor, the length of the line segment between points of intersections with other entities will be used.

##### Pick Angle

Picking the angle value also supports several modes:

1.  **Major - 3 points angle:** The user specifies 3 points (edge, intersection, second edge) and the angle is measured as the angle between two lines directed from the edge points to the intersection.
2.  **Angle of Line:** Click on a line with the CTRL key pressed will pick the angle of the line. If SHIFT is pressed during the click, a supplementary angle (the angle that supplements the line's angle to 180 degrees) will be used.
3.  **Click on Line with SHIFT key:** Allows the user to select two existing lines and measure the angle between them.
4.  **2 points angle:** If a 3-point angle measurement is started, and the second point is selected with CTRL pressed, the angle between two points will be selected.

Based on the angle selection method, it is possible to pick the measured angle value or the value of its supplementary angle.

**Demo Video:** https://youtu.be/GvpN6Y6PQsk

#### Point Coordinates Info Action

An additional Info action was added - **Point Coordinates**. This action allows the user to select a point on the drawing, and its coordinates are added to the command widget.

`XXX_IMAGE`

### Keyboard Support Improvement

Support for keyboard was slightly improved (though focus management still requires polishing). Changes are:

1.  Better visual support of setting Free Snap via the SPACE key (if enabled by settings).
2.  Scrolling the drawing area by pressing LEFT, RIGHT, UP, or DOWN keys.
3.  Zoom in/out by + and - keys.
4.  Moving currently selected entities by keyboard.

Keyboard support for scroll and moving selected objects may be enabled/disabled via the appropriate setting in the General Preferences dialog.

`XXX_IMAGE`

#### Moving Selected Objects by Keyboard

If there are selected entities within the drawing, their position may be adjusted via the keyboard. Depending on the used modifiers, the offset for moving entities is calculated differently. The offset depends on the current grid size and meta grid step.

The offset is calculated as follows:

1.  **SHIFT + DIRECTION_KEY:** Default mode. The offset is the width of the current grid cell in the specified direction.
2.  **CTRL + DIRECTION_KEY:** Precise mode. The offset is the current grid cell size divided by the meta-grid step value. Used for fine-tuning.
3.  **CTRL + SHIFT + DIRECTION_KEY:** Fast mode. The offset is the current grid cell size multiplied by the meta-grid step value. Used for faster, rough positioning.

Where **DIRECTION_KEY** is one of the **LEFT / RIGHT / UP / DOWN** keys.

**Demo Video:** https://youtu.be/frAccXDuTIk

### Various Minor Fixes and Improvements

1.  Displaying tooltips with toolbar name (plus setting in General Preferences).
2.  Support of the user-defined step of the meta-grid (GRIDMAJOR) - with a corresponding setting in General Preferences.
3.  Default location of custom icons engine is added as library path (directory is APP_DIR + "/iconengines"), so if icon engine dll is located there icon theeming settings should work. 
3.  Lots of small polishings and minor improvements.

### Changes in General Preferences

1.  Added option for meta-grid step.
2.  Added option for displaying tooltips for toolbars.
3.  Added option for enabling/disabling interactive input controls in tool options toolbars.
4.  Added option for enabling/disabling moving entities via keyboard.
 
 XXX_IMAGE

#### Widget Preferences

Added an option that controls whether buttons for interactive input should be flat or not.

### Fixed Issues
1) LibreCAD#2144
2) LibreCAD#2174
3) LibreCAD#2177
4) LibreCAD#2190
5) LibreCAD#2194
6) LibreCAD#2197
7) LibreCAD#2198
8) LibreCAD#2199
9) LibreCAD#2203
