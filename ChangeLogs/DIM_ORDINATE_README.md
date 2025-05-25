This PR adds support of Ordinate dimension. 

#### Generic support of the Ordinate Dimension

Demo: https://youtu.be/56kGlFMSZlg

Common functionality related to Ordinate Dimension was added: 

1) Ordinate Dimension entity added (for x and y ordinates)
2) Support of ordinate dimension entity was added to DXF reading/writing
3) Ordinate dimension properties dialog added
4) Support of ordinate dimension was added to informational cursor and entity info widget. 

Ordinate dimensions supports both WCS and UCS. 

#### Related actions

Several actions related to ordinate dimensions were added (located in Dimensions group): 

1) Ordinate - the text command is **"dimord"**. Allows to create new ordinate dimension. 
The origin/definition point of the dimension - zero (0,0) point is located in the origin of the current Coordinate System (UCS or WCS).

Therefore, before creation of ordinate dimensions, a proper UCS should be set.  

2) Select Ordinates by base - allows the user to pick one ordinate dimension and select all ordinate dimensions within drawing with the same defining point and direction of horizontal.  

3) Ordinates Re-base - (command is **"dimordrebase"**). Allows to set the defining point for selected ordinate dimensions to the origin point of the current UCS. 

This command is useful for editing ordinate dimensions, if either base point was changed, or if dimension entities were moved. 

4) "Set UCS by Ordinate Dimension" - this action allows the user to set the UCS that corresponds the defining point and horizontal angle of the selected Ordinate Dimension. 

### Other changes

1) Small fix for styling of linear dimensions line (if the text is inline of dimension line) 
2) Very initial support for DIMSTYLES (this is work in progress)
3) Very initial support of other dimension-related entities (also WIP)

In general, the next PR will include more changes for dimension entities and actions are planned.

### Small Fixes
