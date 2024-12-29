This pull requests is focused mostly on adding more interactivity and information to drawing operations. 

### Informational Cursor
Well, while the term may be not the most straightforward one, the idea behind it is quite simple - provide the user with additional context aware feedback.

That feedback displayed by overlay, that is associated with current cursor position.

That functionality should assist new users understand the functionality of specific actions. 

Also, it allows to save some space on the screen by disabling several duplicated widgets (Coordinates Widget, Mouse Widget) from status bar (if status bar is in floating mode).  

#### Zones and shown information 

Informational cursor includes several zones: 

1) Zone 1 - current coordinates of snap point
2) Zone 2 - displays information about current snap mode/restrictions, as well as properties of caught entity, properties of entity to be created or action-specific information.
3) Zone 3 - displays relative coordinates of snap point
4) Zone 4 - displays name of active action, command prompt and information about modifiers keys. 

#### Settings
The content and appearance of informational cursor is controlled by settings in Application Preferences. 
It is possible to disable either specific zones or the complete informational cursor displaying. 

#### Informational cursor toolbar
For the convenience of quick enabling/disabling specific zones of informational cursor, an additional toolbar was added.

#### Quick object properties
An action was added that allows to display properties of the object in informational toolbar. 

### Other Features

There are several other features that are included into this merge request: 

1) Added option for controlling whether text/mtext should be drawn as draft during panning operations or not
2) Flip Horizontally / Flip Vertically was added to Mirror Operation (available if "mirror to line" is selected)

### Bug fixes
The following issues were fixed
LibreCAD#1983  (and probably LibreCAD##1984)
LibreCAD#1973 - Changing of relative point position is now undoable
LibreCAD#1972 - regressing for PDF export
