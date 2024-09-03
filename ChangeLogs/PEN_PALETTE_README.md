## Pen Palette Widget

The small yet nice widget improves way of dealing with pen attributes. 

It provides possibility to manage a list of named pens and perform various operations related to them, as well as selecting entities and applying pen attributes to selection. 

In addition to this, feature contains improvements for current color comboboxes, allowing better displaying of current (and user-picked color).

Demo for the widget is located on YouTube here:  https://youtu.be/YSU0K3-PfbI

### Widget Structure
The widget is standard dockable widget. UI of it includes several sections:  

1) toolbar with set of actions
2) pen editor area (pressing Enter in pen's name saves pen)
3) filter section with regexp expression (that is used for items filtering/or highlighting according to provided regexp)
4) table view with list of items. 

### Major Features 

1) Managing list of named pens (creation, editing, remove or multi-remove of pens);
2) Persistent storage of created pens;
3) Filtering or highlighting pens in the list according to provided regexp string (with optional case-insensitive match);
4) Support of "Unchanged" value for pen attributes as "any value", allowing partial pen match operations.
5) Support of "By Layer" and "By Block" pen attributes

#### Operations supported
1) Selection of entities  - by specified pen attributes by matching entity's pen attributes and specified pen attributes. Logical AND operations is used and particular match via "unchanged" values of pen's attributes that are considered as "any matched" is supported.
2) Selection of entities  - by specified **DRAWING** pen attributes (i.e - by actual appearance of the entity described by "resolved" pen)
3) Picking pen  -  from selected entity's attributes for further editing or storing;
4) Picking DRAWING pen  - from selected entity for further pen's editing or storing;
5) Applying pen - to selected entities (this is similar to "Modify Attributes" action, yet probably is a bit more convenient);
6) Picking pen  - from active pen (Pen Tool Bar)  for editing or storing;
7) Setting active pen  - in Pen Toolbar by pen from widget (from pen's editor or from table item);
8) Setting active pen - pen from active layer is applied to current pen in Pen Tool Bar
9) Picking pen - from active layer (for editing or storing);
10) Setting pen - for active layer; 
11) Creation, Editing, Removing individual named pens. 
12) Operations may be invoked either from widget's toolbar or via context menu on pens table. 

##### Various features

Options dialog that supports:

1) Customizable columns in table view;
2) Various ways of displaying color names (rbg, hex, color names);
3) Optional tooltip with pen info for table view;
4) Customizable double-click on pen item operation;
5) Customizable UI colors for widget table
6) Customizable path for pen's storage

### Implementation 

Standard QT/C++, no additional dependencies/libraries are introduced. During implementation, one of the goals was minimization of existing codebase affecting in order to eliminate possible regressions. 
Strings withing code are localizable via ::tr(), specific translations for languages are needed. 


