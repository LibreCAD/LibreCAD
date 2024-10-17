***Issues To Close***

This issues in GitHub seems to be obsolete and not actual, please review:

#359 - functionality is included into Entity Info widget (pick coordinates)

#399 - implemented in LayerTree Widget

#458 - supported by LayerTree Widget

#586 - supported by LayerTree Widget

#597 - fixed

#648 - seems to be fixed already

#731 - supported by Line from Point to Line action

#795 - PenPalette widget? 

#874 - supported either by plugin, or by Entity Info Widget

#939 - not relevant anymore, persistent positions of dialogs are supported

#959 - supported by plugin, supported by Slice/Divide action

#960 - PR merged to master already

#1047 - fixed already (in my previous PR)

#1124 - implemented already

#1173 - implemented already

#1192 - implemented by Slice/Divide action

#1509 - supported via *chordlen* command 

#1338 - supported already added to Scale command

#1340 - seems to be fixed? 

#1352 - break of polyline, break of line are supported 

#1374 - partially supported via Line Gap action  

#1210 - supported by Line Snake command

#1480 - merged already

#1614 - supported by Entity Info Widget

#1631 - fixed already?

#1805 - fixed

#1787 - fixed

#1762 - fixed

#1736 - fixed

#1858 - fixed

#1846 - fixed

#1813 - supported by Layer Tree Widget

***Implemented in this PR***

This MR includes the following major changes: 

1) Proper rendering of X,Y-axis in drawing, options for them;
2) Rework of grid rendering, support of lines grid in isometric view, grid-related options  
3) Restructured Application Preferences Dialog
4) Dockable status bar components (enabled by options) + widget with relative point position (so now they may be mixed up with toolbars);
5) Indication of currently active action in MouseWidget
6) Check for New Version functionality (on startup and explicit), rework of About dialog. NOTE: during preparing public build, in addition to LC_VERSION it's necessary to set LC_PRERELEASE define (true for pre-release version, false - for release ones). Also, it's better use unique release versions even for "latest" updates (like 2.2.2.1-latest, 2.2.2.2-latest etc. instead of one generic 2.2.2-latest), otherwise it might be hard to distinguish fresher updates. 
7) Changes for draft mode for text/mtext rendering - now instead of rectangle around the text, baselines for text lines are drawn.  
8) Bugfixes

The following list of issues were addressed by this MR:

#1550

#1688

#1477

#1852

#1728

#1849

#1861

#1870

#1875

#1877

#1878

#1879

#1881

#1864

#1885

#1896


**Issues to consider (sand):**

#601

#606

#467 

#599

#584

#614 - as focus on field in widget option, focus

#570

#712 - may be fixed already, check

#710 - ordinal dimensions

#626 - text improvement by counter

#622 - action in tb? and close

#825 - individual dimensions format

#853 - Feature Request: Provide Mapping Angular Layout - review in details

#846  -[feature] Changing fonts of all texts

#951 - Huge dxf: delete all (unused) blocks 

#949 - Dogbone?

#910 - metadata

#906 - Snap on entity does not work in front of imported image - bug?

#1033 - implemented already, yet not completely

#1005 - tooltip for command improvement (Command Line Commands in Tooltips, Menus)

#1110 - new position for grip by command (Grip points, a small improvement
#1090 - Feature Request: Filter selection by properties #1090

#1082 ??

#1127 - dimension scale should apply to measurement tools too
#1129 - groupping

##1131 - apply colors from prefs? 

#1627
#1651
#1648
