### Rendering pipeline optimization
It possible to find mentions that LibreCAD is not suitable for large drawings due to slow rendering speed.  
It was not clear for me why LibreCAD renders drawing slower than other similar programs, so I've decided to deep into this deeper. 

Rendering pipeline ("paintEvent") was reviewed as optimized (by eliminating not needed/duplicated checks, pre-calculating values needed for paining, avoiding extra calls and so on).     

The primary goal was to achieve good/acceptable performance, especially for large drawings.  

While internally the code in some places now is less elegant, the reason for that was optimization of painting speed. 

As result, depending on specific entities included into drawing, the performance of rendering (on Windows machine) as increased. 

In my environment, rendering is up **to 7 times faster** comparing to previous AppImage (measurements for painting time may be enabled by DEBUG_RENDERING define in rs_graphicview.h:42).  
On other machines/operating systems, of course, results may be different.

NOTE: As changes in rendering are sensitive, despite that various cases were tested, it still might be that some painting issues/glitches may occur (as they may depend on actual data). 

Files for testing (opening time for them is long): 

lines_large.dxf - 190084 entities in drawing (mostly lines)
circles_large.dxf - 720409 circles
arcs_large.dxf - 180100 arcs
ellipse_large.dxf - 165747 ellipses 
polyline.dxf - 16402 polylines (each polyline has 16 lines/arc)
ellipse_solid - 39631 entities, solid fill
spline_points_large.dxf - 162020 spline point entities
splines.dxf - 3474 splines
solid.dxf - 60024 leaders 
lorem_one.dxf - 10 large mtext entities

Archive is attached

#### Other changes

1) Better preview for MText/Text entities (baselines for text lines are shown instead of boundary rectangle on preview) 
2) Additional button that allows to render entities with default length in non-draft mode. 
3) Restored snapping to grid if grid is not shown. 
4) New version check improvement

#### Changes in settings
##### General Settings
1) Added settings to control colors/linetype/fill for selection overlay box
2) Added settings for minimum values that affects rendering 
3) Added "no grid" default value for new drawings
4) Added setting for default zoom factor used for scroll zoom

##### Drawing Settings
1) Added settings for specifying line joins and line caps. 

### Issues addressed: 

#1916

#1917

#1921
