This pull request includes several new actions as well as minor fixes. 

### Ellipse 1 Point action
Demo - https://youtu.be/tLHZan4BUUY

A couple of command were added for drawing actions

1) **Ellipse (1 Point) Action** - This action allows to create an ellipse with specified major and minor radius of specified lengths. 
The user should specify location of ellipse center.
The angle of major axis may be set either to fixed value, or be selected by the mouse (free mode).

2) **Ellipse Arc (1 Point) Action** - This actions allows to draw elliptic arc using similar logic as Ellipse (1 Point) action.

Also, Draw Ellipse Arc (Axis) was improved and now the user may specify whether arc is reversed or not. 


### Dimension Baseline and Continue Actions
Demo - https://youtu.be/DHluOAF86k0

Two handy actions for creating dimensions were added. 

1) **Dimension Baseline Action** - the action creates baseline dimensions base on specified source dimension. 

    The distance between dimensions may be fixed or be specified by the mouse.

2) **Dimension Continue Action** - the allows to create several dimensions that continue selected original dimension.

So far only linear and aligned dimensions are supported (no support for angular dimensions).  

Both actions supports keyboard modifiers: 
- for alternating base extension points that will be used as start points (selected either closer or distant point of original dimension to mouse position);
- for baseline action, for alternating the offset direction for next dimension. 

### Radial and Diametric Dimensions Actions
Demo -  https://youtu.be/dOW5O-37gjo

The overall flow for creation of radial and diametric dimensions is improved and made shorter (fewer mouse clicks).

Also, it is possible to specify the angle for dimensions and so draw dimensions using uniform style.   

With CTRL keyboard modifier, the user may adjust the position of the dimension if fixed (not free) angle mode is used.  

### Minor Fixes

There are some minor fixes well as corrections that address the following issues: 

1) #1854 - for print preview, white color is used instead of foreground color for color correction and previewing black/white colors
2) #1847 - actually, #1847 is just visual effect of more serious bug in settings system, were negative int values were stored in settings incorrectly. That bug was a side effect of #PR1461
3) #1858 - subtle improvement for snap point on Modify Scale action for isotropic scaling

### Persistent Dialogs Positions

If corresponding setting is enabled in Action Preferences Dialog, position and size of the dialog is saved.

On next invocation the dialog will be shown with stored dimensions and (optionally) in previous position.  
