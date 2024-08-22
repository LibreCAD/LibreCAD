This pull request includes various functionality in different areas, as well as fixes, improvements and new actions.  

Most notable changes are below.

### Entities Highlighting

Demo - https://youtu.be/5w2fVDpUGQQ

Highlighting of entities on mouse move or mouse selection was expanded.

There are  new options in options dialog, that controls whether:
1) entities are highlighted on mouse move;
2) start/stop endpoint indicators should be shown for highlighted entities.

Overlay-based highlighting was added in addition to the original one.

Due to that, highlighting may be applied to individual entities within containers (like segments of polyline) as well as have better visual appearance of highlighted entities (especially, with large line widths).

### Support of keyboard modifiers in actions

Demo - https://youtu.be/wDSFxoWt1gw

All drawing actions now have additional support of keyboard modifiers (SHIFT, CTRL).

The functionality of action is extended, if keyboard modifier is pressed during mouse-move or mouse click events. 

However, some specific actions processes keyboard modifiers in their own specific way (like performing lengthen operation or repeating paste operation). 

For providing the user with additional hint that keyboard operations are available for currently executed action, UI of the Mouse Widget is extended.

If support of keyboard modifier is supported by the current state of active action, appropriate icon is shown as part of Mouse Widget. 

Hint on that icon provides description for details of operation that may be performed with corresponding pressed key. 

### Support of Snap to Relative Point and Angle Snap

Demo - https://youtu.be/wDSFxoWt1gw

Now most actions allows to perform where it is reasonable by the action's context:
1) Snap to relative point position; 
2) Perform angle snap to context-specific point (say, start of line).  The value of single step for angle snap is defined by options (1,3,5,15 degrees).

In order to perform such snaps, specific keyboard modifier (CTRL or SHIFT key) should be pressed together with mouse operation (move, click);

### Preview With Reference entities

Demo - https://youtu.be/9ti4LW-XTA0

Preview mode for all actions was extended by drasing additional reference points and entities that are part of drawing operation. The puprpose is to  make drawing operations more intuitive and understandable to the user (especially new one) and to provide the user with:
1) Better visual feedback for already performed operation (say, previously specified points);
2) Indication of interim calculations which are part of the action (like axes and reference points of ellipse to be created);
3) Projected results of action's execution (like expected positions of reference points or created entities positions);

An additional options were added to the Application Options dialog for: 

1) Enabling/disabling displaying reference entities on preview;
2) Specifying shape/size for temporary reference points shown during preview;
3) Specifying color for reference entities;

Highlighting of entities in preview mode also takes into account whether the entity that the user tries to select is suitable for current operation. Only entities that are valid for the current context are highlighted.
 
### Drawing Actions

Demo - https://youtu.be/N0OEmWbzElM

Preview for drawing actions may include additional important points that are used for building specific entity (centers of circles, tangent points, user selections points etc.).
Support of key modifiers is action and context (action state) dependent.

Some improvements for actions: 

1) "chordlen" command for DrawArc - creation of arch with specified horde
2) Parallel Through Point option - now it's possible to create parallels that are placed in both sides symmetrically to selected entity.

### Polyline Actions

Demo - https://youtu.be/KI6C7FkLRpg

Preview for polyline editing actions improved and allows to perform such operations like deleting node, delete between nodes in more visual and understandable way.

### New Info Actions

Demo - https://youtu.be/aBu8AP-f95M

New simple yet handy info actions were added. They are: 

1) **Distance from Point to Entity** - allows to check the distance from specified point to selected action
2) **Angle between 3 points** - allows to measure the angle set by 3 points that defines 2 crossed lines. Points are line 1 edge, lines intersection point, line 2 edge. By its logic, the action is similar to "Angle Between Lines" action, except that it does not require that lines should exist.

Output for info commands was slightly improved. 

### Modify operations

Demo (quick overview) - https://youtu.be/7ioHtLuqXfA

Modify operations were reworked and dialogs were replaced by options widgets, so now dialogs are not part of action's flow. The code for dialogs is still present and there is a flag that allows old way of options operations, yet, in general, the value of showing dialogs is unclear now.

Preview now includes multiple copies (if specific action supports that if multiple copies are requested).

Other improvements:  
1) **Modify Mirror** - now it's possible select existing line that used as mirror axis;
2) **Rotate** - the flow is changed, the user should select reference point first and rotation center is selected after that. However, the setting for enabling old mode (center-first) may be added, if needed. 
2) **Rotate** - allows to specify second rotation angle (so entity is rotated first via specified center, and rotated copy is rotated on second angle)
3) **Lengthen** - allows to do symmetric lengthen (so needed length change is applied to both ends of the line);

Also, various small improvements for specific actions are added. 


### New Copy/Paste Actions

Demo - https://youtu.be/KFKRDoV8KJc

New actions were added: 

1) **Copy/Cut** operations - if CTRL is pressed during selection of reference point, paste operation is invoked immediately after copy;
2) **Paste** Operation - if CTRL is pressed during paste, paste operation is repeated (**multiple paste**);
3) **Copy Quick/Cut Quick** - current selection is copied, and the user is not asked for specifying reference point. The reference point is calculated as center point of copied selection.
4) **Paste Transform** - actions allows to paste copied entities with **transformation** (scale, rotate) and create **array** of copies with specified number of columns, rows and rotation angle

### Dimensions Snapping

Demo - https://youtu.be/PEJG_3Q54hs

Snapping for adjacent linear dimensions was added, so now it's simpler to draw continuous dimensions. 

### Default Action Changes
Demo - https://youtu.be/kBHrHpn12i0

Functionality of dimension action improved. Now it's possible lengthen line and arc by mouse (by moving endpoint with pressed CTRL key).

Also, generic moving of entities and endpoints now include visual reference line, and move/angle distance is reflected by coordinates widget. 

### Snap options

Demo - https://youtu.be/MOG1ctOK6r4

Settings for position of options widgets for Snap Middle and Snap Distance was added to Application Options dialog. Based on settings value, options may be shown either in options toolbar (as before) or within Snap Toolbar.

### UI in Print Preview Mode

Demo - https://youtu.be/I0M7W51SbqQ

Minor improvements were added to Print Preview: 
1) Not applicable actions and widgets are now disabled in Print Preview mode;
2) Quick buttons added on options toolbar for setting page orientation and print settings;
3) Added ability to zoom to print area (paper);
4) Added explicit settings for number of pages in tiled print.

While print preview is operational, most probably it requires an additional improvements in order to make it more convenient.

### Keyboard Shortcuts Editing Dialog, Tooltips with Shortcuts

Demo - https://youtu.be/DTn0lCbrPn8

Support of keyboard was slightly improved, so: 

1) Now the user may assign keyboard shortcuts to actions via UI  - using Keyboard Shortcuts options dialog. Withing this dialog, now it is possible to manage shortcuts, as well as export and import them.
2) An option was added to Application Preferences dialog that allows to control whether keyboard shortcuts should be shown as part of the action's tooltip or not.

While this functionality is quite operational, some actions may be not invoked by keyboard. 

The overall application-wise processing of keyboard shortcuts and keyboard support requires additional review and improvements. 
It will be added later.

### Other

In addition to visible changes mentioned above, there are lots of minor improvements of existing actions (like support of symmetric mode for "Line Parallel Through Point" and "Lengthen" actions) as well as various bugfixes.

#### Action Option widgets

Action options widgets were reviewed, missing tooltips and options were added. 

The internal implementation of action options subsystem was reworked, memory leak for Option Widgets UI fixed, ownership of all Options Widgets instances is passed to corresponding actions.

#### Some minor changes
1) **Active Action on Tab Switch** - Now switching tab or creation of new drawing finishes current active action, so no confusion by action that is active in other tab.
2) **Selection in selection widget**  - Selected entities count/length is updated on layer hiding/displaying to reflect only visible selected entities.

### Implementation notes
The existing code base (for actions) historically contains lots of code duplications and therefore there are not needed redundancy and verbosity.
Actions code was refactored, and repeating code and obvious copy/paste blocks were eliminated. 
In addition, reusable and glue code was extracted to base methods of action classes and additional base methods for overriding/usage were added. 

The major intents for such refactoring are: 
1) making implementation of specific actions more lightweight, less verbose and easier to read;
2) removing glue code and repetitive code and focus on action-specific logic;
3) improving/cleaning code;
4) using consistent code style and common approaches across the codebase;

Areas which are mostly affected are /actions and /ui. 

The overall layout of source files was also affected. Historically, the amount of files within the same directory reached huge numbers that makes navigation within codebase quite complicated. 

Layout of source files was changed to achieve more granular separation of functionality and simpler navigation. Git history of changes for source files was saved during source code reallocation. 

I'm not aware of any functional regressions, so end users should be not affected by refactoring. 

The functionality is stable enough and may be a good candidate at least for night build for wide user testing. 

However, the changes in the code base are, well, quite significant, so most probably the process of comparing/reviewing changes may be challenging. 

It might be also practical to make a new tag for this PR. 
