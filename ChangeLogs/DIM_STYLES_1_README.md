## Dimensions Styles and Context actions

This pull request contains the following major functionality: 
1) Initial support of dimension styles;
2) Changes in dimension-related actions, new actions for dimension styles
2) Support of the "context entity" by actions;
3) Reworked default context popup menu for drawing area, support of entity-specific context menus; 
4) Reworked Custom Toolbar Creator, Custom Menu Creator, wide support of invocation shortcuts for custom menus.
5) Added support of the drawing's metadata (athor, title, subject, description etc)
6) Added support of predefined and custom user data for the drawing document;
7) Various minor fixes and improvements;

### Dimension Styles

The major focus of this PR is to provide full support of dimension styles reading, editing and storing, by adding DXF parsing and writing functionality, as well as adding UI, needed for dimension styles management and editing.

**NOTE: Support of rendering of dimensions according to dimension styles attributes is STILL LIMITED and NOT ALL attributes of dimension style affects created dimensions!** 

The logic of dimensions creation should be reworked deeper, and full support of dimension style attributes will be included into the next Pull Request.

#### Dimensions styles management

Dimension styles, included into the drawing, are managed using Drawing Preferences dialog, in tab Dimensions. 

Here the list of dimension styles is present, as well as the set of corresponding related commands, such as:
* New Style;
* Remove Style
* Edit Style
* Rename Style
* Mark as Active (so it will be used as default for newly created dimensions)
* Export/Import

Live preview of dimensions with applied dimension style is also available. 

#### Editing Dimension Style

Editing of the dimension style attributes is performed via new dialog, that contains UI for entering attributes and preview area that reflects dimension style rendering. 
 
#### Dimension Entity Properties, Style Override

The dialog for dimension entity properties now includes functionality for selecting dimension style. 

Also, using that dialog it is possible to specify dimension style override. Style override is the set of dimension style attributes, that is specific for particular dimension only. 

Using style override, it is possible to fine tune individual dimension entities. 

#### Specifying Dimension Style during Dimension entity creation

Dimension options toolbar now includes style selection combobox

### Dimension Actions

Several changes were introduced to actions, related to dimensions.

#### New Actions
Several new actions, related to dimensions were added:

1) **Copy Style Action** - allows to pick the style from one dimension entity and apply to another one
2) **Regenerate Dimensions Action** - forcefully re-creates dimension
3) **Dimension Styles Action** - invokes Dimensions Styles manager

#### Changes in Dimension Related actions

1) For linear dimensions, it's possible to select line entity (that might be standalone or a part of polyline) using context menu or pressed CTRL key.
Linear, Aligned, Horizontal or Vertical dimension will be created with base points that corresponds to the line start and end points.

2) In action used for creation of Horizontal or Vertical dimension, now it is possible to change direction using CTRL key modifier. 

### Support of "Context Entity" in Actions

Now all actions support a notion of "Context" entity - the entity that is passed to the action on action's invocation. 

If context entity is available, the action will be executed for that entity only (regardless whether selection is present in drawing). 
Also, where possible, the initial entity selection state of the action is skipped, and so the user is not prompted to select entity for the action's execution.

At the moment, the context entity is set to the action via context popup menu (if the user invoked menu in the point that is near to the entity). 

However, later this concept may be used, for example, in plugins or so.

### Context menu 

Default popup menu that is shown in the drawing area by Right Mouse Button click was reworked, in order to include more operations and support more efficient drawing operations. 

There are to major modes: 

1) Generic menu that is invoked not for some entity (i.e - click point is outside entity);
2) Context menu - invoked with click on entity. 

For the context menu, the structure of the menu is adapted to correspond clicked entity type, and it reflects operations, that are relevant for selected entity.

The entity for context menu is pre-selected and is passed to the invoked action as a "context" entity. 

Due to this, the user is not required to select the entity again during the action's invocation, and thus the drawing operation is performed in more convenient and faster way.

### Custom Toolbar And Custom Menu Creator

The Custom Toolbar Creator tool was reworked initially, yet from UI point of view it's rather a small facelifting without significant changes of the functionality.

The Custom Menu Creator tool as also reworked initially. 

The most important changes: 

1) Added wider support of invocation's shortcuts. Now the user may specify various key modifiers and mouse buttons, that are used for the menu invocation. 
2) Assigned invocation shortcuts are visible in the list of menus. 
3) Automatic invocation of actions in menus, that contains only one action. This mechanism allows to invoke specified action using assigned mouse/keys shortcut.

Custom menus are also context-aware, and if menu is clicked on entity - that entity will be passed to the action.

### Support of metadata for drawing

Now it's possible to manage DXF metadata fields in Drawing Preferences dialog.

### Support of user data

User data of the drawing document are manageable in Drawing Preferences dialog.

### Other changes in actions

In addition to support of context entity, there are small changes in action workflows:

1) "Select Intersecting Entities" action now allows to specify the entity and all entities that intersects it will be selected. 
2) Linear dimensions actions allows to select a line for faster creation of the dimension
3) Dim Ordinate Action - if invoked with a context, may snap to the line endpoint or circle/arc center.
4) Trim and Trim Two actions - if invoked with a context, considers provided entity as an entity that should be trimmed.
5) Draw Arc Tangential action - if invoked with a context, starts arc creation from the nearest endpoint of the selected entity.
6) Draw tangential lines actions - state depends on provided context entity type

### Various minor fixes and improvements  
 
1) Displaying tooltips with toolbar mame (plus setting in Generic Preferences) (LibreCAD#2254); 
2) Support of the user-defined step of meta-grid (GRIDMAJOR) - with corresponding setting in Generic Preferences);
3) lots of small polishings and minor improvements

Fixed issues
1) LibreCAD#2144
2) LibreCAD#2174
3) LibreCAD#2177
4) LibreCAD#2190
5) LibreCAD#2194
6) LibreCAD#2197
7) LibreCAD#2198
8) LibreCAD#2199
9) LibreCAD#2203
