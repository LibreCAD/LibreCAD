# LibreCAD project file
# (c) Ries van Twisk (librecad@rvt.dds.nl)
TEMPLATE = app
DEFINES += QC_APPKEY="\"/LibreCAD\""
DEFINES += QC_APPNAME="\"LibreCAD\""
DEFINES += QC_COMPANYNAME="\"LibreCAD\""
DEFINES += QC_COMPANYKEY="\"LibreCAD\""
DEFINES += QC_VERSION="\"1.0.0beta5\""

# Add qt3support
QT += qt3support
CONFIG += qt \
    warn_on \
    link_prl \
    help

QMAKE_CXXFLAGS_DEBUG += 
QMAKE_CXXFLAGS += 

# Make translations at the end of the process
unix { 
    # Get SVN revision number
    # SVNREVISION = $$system(svn info -R | grep -o \"Revision: [0-9]*\" | sed -e \"s/Revision: //\" | head -n1)
    # Temporary disabled getting SCM version
    SVNREVISION="Git Version"
    DEFINES += QC_SVNREVISION=\"$$SVNREVISION\"
    macx { 
        TARGET = LibreCAD
        DEFINES += QC_APPDIR="\"LibreCAD\""
        DEFINES += QINITIMAGES_LIBRECAD="qInitImages_LibreCAD"
        RC_FILE = res/main/librecad.icns
        DESTDIR = .
        
        QMAKE_POST_LINK = scripts/postprocess-osx.sh
    }
    else { 
        TARGET = librecad
        DEFINES += QC_APPDIR="\"librecad\""
        DEFINES += QINITIMAGES_LIBRECAD="qInitImages_librecad"
        RC_FILE = res/main/librecad.icns
        DESTDIR = unix
        
        QMAKE_POST_LINK = scripts/postprocess-unix.sh
    }
}
win32 { 
    CONFIG += release
    QMAKE_CFLAGS_THREAD -= -mthreads
    QMAKE_LFLAGS_THREAD -= -mthreads
    DEFINES += QC_SVNREVISION=\"\"
    TARGET = LibreCAD
    DEFINES += QC_APPDIR="\"LibreCAD\""
    DEFINES += QINITIMAGES_LIBRECAD="qInitImages_LibreCAD"
    
    # RC_FILE = res/main/librecad.icns
    DESTDIR = .
    QMAKE_POST_LINK = scripts\postprocess-win.bat
}


# Additional libraries to load
# LIBS += \
# -Ldxflib/lib -ldxf \
# Store intermedia stuff somewhere else
OBJECTS_DIR = intermediate/obj
MOC_DIR = intermediate/moc
RCC_DIR = intermediate/rcc
TS_DIR = intermediate/ts
UI_DIR = intermediate/ui
UI_HERADERS_DIR = intermediate/ui
UI_SOURCES_DIR = intermediate/ui
RESOURCES += res/extui/extui.qrc
INCLUDEPATH += dxflib/src \
    fparser \
    src/cmd \
    src/lib/actions \
    src/lib/creation \
    src/lib/debug \
    src/lib/engine \
    src/lib/fileio \
    src/lib/filters \
    src/lib/gui \
    src/lib/information \
    src/lib/math \
    src/lib/modification \
    src/lib/scripting \
    src/actions \
    src/main \
    src/ui \
    src/ui/forms \
    res

# ################################################################################
# Library
HEADERS = \
    dxflib/src/dl_attributes.h \
    dxflib/src/dl_codes.h \
    dxflib/src/dl_creationadapter.h \
    dxflib/src/dl_creationinterface.h \
    dxflib/src/dl_dxf.h \
    dxflib/src/dl_entities.h \
    dxflib/src/dl_exception.h \
    dxflib/src/dl_extrusion.h \
    dxflib/src/dl_writer.h \
    dxflib/src/dl_writer_ascii.h \
    fparser/fparser.hh \
    src/lib/actions/rs_actioninterface.h \
    src/lib/actions/rs_preview.h \
    src/lib/actions/rs_previewactioninterface.h \
    src/lib/actions/rs_snapper.h \
    src/lib/creation/rs_creation.h \
    src/lib/debug/rs_debug.h \
    src/lib/engine/rs.h \
    src/lib/engine/rs_application.h \
    src/lib/engine/rs_arc.h \
    src/lib/engine/rs_atomicentity.h \
    src/lib/engine/rs_block.h \
    src/lib/engine/rs_blocklist.h \
    src/lib/engine/rs_blocklistlistener.h \
    src/lib/engine/rs_clipboard.h \
    src/lib/engine/rs_circle.h \
    src/lib/engine/rs_color.h \
    src/lib/engine/rs_constructionline.h \
    src/lib/engine/rs_datetime.h \
    src/lib/engine/rs_dimaligned.h \
    src/lib/engine/rs_dimangular.h \
    src/lib/engine/rs_dimdiametric.h \
    src/lib/engine/rs_dimension.h \
    src/lib/engine/rs_dimlinear.h \
    src/lib/engine/rs_dimradial.h \
    src/lib/engine/rs_dir.h \
    src/lib/engine/rs_document.h \
    src/lib/engine/rs_ellipse.h \
    src/lib/engine/rs_entity.h \
    src/lib/engine/rs_entitycontainer.h \
    src/lib/engine/rs_file.h \
    src/lib/engine/rs_fileinfo.h \
    src/lib/engine/rs_flags.h \
    src/lib/engine/rs_font.h \
    src/lib/engine/rs_fontchar.h \
    src/lib/engine/rs_fontlist.h \
    src/lib/engine/rs_graphic.h \
    src/lib/engine/rs_hatch.h \
    src/lib/engine/rs_insert.h \
    src/lib/engine/rs_img.h \
    src/lib/engine/rs_image.h \
    src/lib/engine/rs_layer.h \
    src/lib/engine/rs_layerlist.h \
    src/lib/engine/rs_layerlistlistener.h \
    src/lib/engine/rs_leader.h \
    src/lib/engine/rs_line.h \
    src/lib/engine/rs_overlayline.h \
    src/lib/engine/rs_overlaybox.h \
    src/lib/engine/rs_pattern.h \
    src/lib/engine/rs_patternlist.h \
    src/lib/engine/rs_pen.h \
    src/lib/engine/rs_point.h \
    src/lib/engine/rs_polyline.h \
    src/lib/engine/rs_ptrlist.h \
    src/lib/engine/rs_regexp.h \
    src/lib/engine/rs_settings.h \
    src/lib/engine/rs_solid.h \
    src/lib/engine/rs_spline.h \
    src/lib/engine/rs_string.h \
    src/lib/engine/rs_stringlist.h \
    src/lib/engine/rs_system.h \
    src/lib/engine/rs_text.h \
    src/lib/engine/rs_textstream.h \
    src/lib/engine/rs_undo.h \
    src/lib/engine/rs_undoable.h \
    src/lib/engine/rs_undocycle.h \
    src/lib/engine/rs_units.h \
    src/lib/engine/rs_utility.h \
    src/lib/engine/rs_valuelist.h \
    src/lib/engine/rs_variabledict.h \
    src/lib/engine/rs_vector.h \
    src/lib/fileio/rs_fileio.h \
    src/lib/filters/rs_filtercxf.h \
    src/lib/filters/rs_filterdxf.h \
    src/lib/filters/rs_filterdxf1.h \
    src/lib/filters/rs_filterinterface.h \
    src/lib/gui/rs_commandevent.h \
    src/lib/gui/rs_coordinateevent.h \
    src/lib/gui/rs_dialogfactory.h \
    src/lib/gui/rs_dialogfactoryinterface.h \
    src/lib/gui/rs_dialogfactoryadapter.h \
    src/lib/gui/rs_event.h \
    src/lib/gui/rs_eventhandler.h \
    src/lib/gui/rs_graphicview.h \
    src/lib/gui/rs_grid.h \
    src/lib/gui/rs_keyevent.h \
    src/lib/gui/rs_linetypepattern.h \
    src/lib/gui/rs_mainwindowinterface.h \
    src/lib/gui/rs_mouseevent.h \
    src/lib/gui/rs_painter.h \
    src/lib/gui/rs_painteradapter.h \
    src/lib/gui/rs_painterqt.h \
    src/lib/gui/rs_staticgraphicview.h \
    src/lib/information/rs_locale.h \
    src/lib/information/rs_information.h \
    src/lib/information/rs_infoarea.h \
    src/lib/modification/rs_modification.h \
    src/lib/modification/rs_selection.h \
    src/lib/math/rs_math.h \
    src/lib/scripting/rs_python.h \
    src/lib/scripting/rs_simplepython.h \
    src/lib/scripting/rs_python_wrappers.h \
    src/lib/scripting/rs_script.h \
    src/lib/scripting/rs_scriptlist.h 

SOURCES = dxflib/src/dl_dxf.cpp \
    dxflib/src/dl_writer_ascii.cpp \
    fparser/fparser.cc \
    src/lib/actions/rs_actioninterface.cpp \
    src/lib/actions/rs_preview.cpp \
    src/lib/actions/rs_previewactioninterface.cpp \
    src/lib/actions/rs_snapper.cpp \
    src/lib/creation/rs_creation.cpp \
    src/lib/debug/rs_debug.cpp \
    src/lib/engine/rs_arc.cpp \
    src/lib/engine/rs_block.cpp \
    src/lib/engine/rs_blocklist.cpp \
    src/lib/engine/rs_clipboard.cpp \
    src/lib/engine/rs_circle.cpp \
    src/lib/engine/rs_constructionline.cpp \
    src/lib/engine/rs_dimaligned.cpp \
    src/lib/engine/rs_dimangular.cpp \
    src/lib/engine/rs_dimdiametric.cpp \
    src/lib/engine/rs_dimension.cpp \
    src/lib/engine/rs_dimlinear.cpp \
    src/lib/engine/rs_dimradial.cpp \
    src/lib/engine/rs_document.cpp \
    src/lib/engine/rs_ellipse.cpp \
    src/lib/engine/rs_entity.cpp \
    src/lib/engine/rs_entitycontainer.cpp \
    src/lib/engine/rs_font.cpp \
    src/lib/engine/rs_fontlist.cpp \
    src/lib/engine/rs_graphic.cpp \
    src/lib/engine/rs_hatch.cpp \
    src/lib/engine/rs_insert.cpp \
    src/lib/engine/rs_image.cpp \
    src/lib/engine/rs_layer.cpp \
    src/lib/engine/rs_layerlist.cpp \
    src/lib/engine/rs_leader.cpp \
    src/lib/engine/rs_line.cpp \
    src/lib/engine/rs_overlayline.cpp \
    src/lib/engine/rs_overlaybox.cpp \
    src/lib/engine/rs_pattern.cpp \
    src/lib/engine/rs_patternlist.cpp \
    src/lib/engine/rs_point.cpp \
    src/lib/engine/rs_polyline.cpp \
    src/lib/engine/rs_settings.cpp \
    src/lib/engine/rs_solid.cpp \
    src/lib/engine/rs_spline.cpp \
    src/lib/engine/rs_string.cpp \
    src/lib/engine/rs_system.cpp \
    src/lib/engine/rs_text.cpp \
    src/lib/engine/rs_undo.cpp \
    src/lib/engine/rs_undoable.cpp \
    src/lib/engine/rs_units.cpp \
    src/lib/engine/rs_utility.cpp \
    src/lib/engine/rs_variabledict.cpp \
    src/lib/engine/rs_vector.cpp \
    src/lib/fileio/rs_fileio.cpp \
    src/lib/filters/rs_filtercxf.cpp \
    src/lib/filters/rs_filterdxf.cpp \
    src/lib/filters/rs_filterdxf1.cpp \
    src/lib/gui/rs_dialogfactory.cpp \
    src/lib/gui/rs_eventhandler.cpp \
    src/lib/gui/rs_graphicview.cpp \
    src/lib/gui/rs_grid.cpp \
    src/lib/gui/rs_linetypepattern.cpp \
    src/lib/gui/rs_painter.cpp \
    src/lib/gui/rs_painterqt.cpp \
    src/lib/gui/rs_staticgraphicview.cpp \
    src/lib/information/rs_locale.cpp \
    src/lib/information/rs_information.cpp \
    src/lib/information/rs_infoarea.cpp \
    src/lib/math/rs_math.cpp \
    src/lib/modification/rs_modification.cpp \
    src/lib/modification/rs_selection.cpp \
    src/lib/scripting/rs_python.cpp \
    src/lib/scripting/rs_simplepython.cpp \
    src/lib/scripting/rs_python_wrappers.cpp \
    src/lib/scripting/rs_script.cpp \
    src/lib/scripting/rs_scriptlist.cpp \

# ################################################################################
# Command
HEADERS += src/cmd/rs_commands.h
SOURCES += src/cmd/rs_commands.cpp

# ################################################################################
# Actions
HEADERS += src/actions/rs_actionblocksadd.h \
    src/actions/rs_actionblocksattributes.h \
    src/actions/rs_actionblockscreate.h \
    src/actions/rs_actionblocksedit.h \
    src/actions/rs_actionblocksexplode.h \
    src/actions/rs_actionblocksinsert.h \
    src/actions/rs_actionblocksfreezeall.h \
    src/actions/rs_actionblocksremove.h \
    src/actions/rs_actionblockstoggleview.h \
    src/actions/rs_actiondefault.h \
    src/actions/rs_actiondimaligned.h \
    src/actions/rs_actiondimangular.h \
    src/actions/rs_actiondimdiametric.h \
    src/actions/rs_actiondimension.h \
    src/actions/rs_actiondimleader.h \
    src/actions/rs_actiondimlinear.h \
    src/actions/rs_actiondimradial.h \
    src/actions/rs_actiondrawarc.h \
    src/actions/rs_actiondrawarc3p.h \
    src/actions/rs_actiondrawarctangential.h \
    src/actions/rs_actiondrawcircle.h \
    src/actions/rs_actiondrawcircle2p.h \
    src/actions/rs_actiondrawcircle3p.h \
    src/actions/rs_actiondrawcirclecr.h \
    src/actions/rs_actiondrawellipseaxis.h \
    src/actions/rs_actiondrawhatch.h \
    src/actions/rs_actiondrawimage.h \
    src/actions/rs_actiondrawline.h \
    src/actions/rs_actiondrawlineangle.h \
    src/actions/rs_actiondrawlinebisector.h \
    src/actions/rs_actiondrawlinefree.h \
    src/actions/rs_actiondrawlinehorvert.h \
    src/actions/rs_actiondrawlineparallel.h \
    src/actions/rs_actiondrawlineparallelthrough.h \
    src/actions/rs_actiondrawlinepolygon.h \
    src/actions/rs_actiondrawlinepolygon2.h \
    src/actions/rs_actiondrawlinerectangle.h \
    src/actions/rs_actiondrawlinerelangle.h \
    src/actions/rs_actiondrawlinetangent1.h \
    src/actions/rs_actiondrawlinetangent2.h \
    src/actions/rs_actiondrawpoint.h \
    src/actions/rs_actiondrawspline.h \
    src/actions/rs_actiondrawtext.h \
    src/actions/rs_actioneditcopy.h \
    src/actions/rs_actioneditpaste.h \
    src/actions/rs_actioneditundo.h \
    src/actions/rs_actionfilenew.h \
    src/actions/rs_actionfileopen.h \
    src/actions/rs_actionfilesave.h \
    src/actions/rs_actionfilesaveas.h \
    src/actions/rs_actioninfoangle.h \
    src/actions/rs_actioninfoarea.h \
    src/actions/rs_actioninfoinside.h \
    src/actions/rs_actioninfodist.h \
    src/actions/rs_actioninfodist2.h \
    src/actions/rs_actioninfototallength.h \
    src/actions/rs_actionlayersadd.h \
    src/actions/rs_actionlayersedit.h \
    src/actions/rs_actionlayersfreezeall.h \
    src/actions/rs_actionlayersremove.h \
    src/actions/rs_actionlayerstogglelock.h \
    src/actions/rs_actionlayerstoggleview.h \
    src/actions/rs_actionlibraryinsert.h \
    src/actions/rs_actionlockrelativezero.h \
    src/actions/rs_actionmodifyattributes.h \
    src/actions/rs_actionmodifybevel.h \
    src/actions/rs_actionmodifycut.h \
    src/actions/rs_actionmodifydelete.h \
    src/actions/rs_actionmodifydeletefree.h \
    src/actions/rs_actionmodifydeletequick.h \
    src/actions/rs_actionmodifyentity.h \
    src/actions/rs_actionmodifymirror.h \
    src/actions/rs_actionmodifymove.h \
    src/actions/rs_actionmodifymoverotate.h \
    src/actions/rs_actionmodifyrotate.h \
    src/actions/rs_actionmodifyrotate2.h \
    src/actions/rs_actionmodifyround.h \
    src/actions/rs_actionmodifyscale.h \
    src/actions/rs_actionmodifystretch.h \
    src/actions/rs_actionmodifytrim.h \
    src/actions/rs_actionmodifytrimamount.h \
    src/actions/rs_actionmodifyexplodetext.h \
    src/actions/rs_actionoptionsdrawing.h \
    src/actions/rs_actionparisdebugcreatecontainer.h \
    src/actions/rs_actionprintpreview.h \
    src/actions/rs_actionselect.h \
    src/actions/rs_actionselectall.h \
    src/actions/rs_actionselectbase.h \
    src/actions/rs_actionselectcontour.h \
    src/actions/rs_actionselectintersected.h \
    src/actions/rs_actionselectinvert.h \
    src/actions/rs_actionselectsingle.h \
    src/actions/rs_actionselectwindow.h \
    src/actions/rs_actionselectlayer.h \
    src/actions/rs_actionsetrelativezero.h \
    src/actions/rs_actionsetsnapmode.h \
    src/actions/rs_actionsetsnaprestriction.h \
    src/actions/rs_actionsnapintersectionmanual.h \
    src/actions/rs_actiontoolregeneratedimensions.h \
    src/actions/rs_actionzoomauto.h \
    src/actions/rs_actionzoomautoy.h \
    src/actions/rs_actionzoomin.h \
    src/actions/rs_actionzoompan.h \
    src/actions/rs_actionzoomprevious.h \
    src/actions/rs_actionzoomredraw.h \
    src/actions/rs_actionzoomscroll.h \
    src/actions/rs_actionzoomwindow.h \
    src/actions/rs_actiondrawpolyline.h \
    src/actions/rs_actionpolylineadd.h \
    src/actions/rs_actionpolylineappend.h \
    src/actions/rs_actionpolylinedel.h \
    src/actions/rs_actionpolylinedelbetween.h \
    src/actions/rs_actionpolylinetrim.h \
    src/actions/rs_actionpolylineequidistant.h \
    src/actions/rs_actionpolylinesegment.h
SOURCES += src/actions/rs_actionblocksadd.cpp \
    src/actions/rs_actionblocksattributes.cpp \
    src/actions/rs_actionblockscreate.cpp \
    src/actions/rs_actionblocksedit.cpp \
    src/actions/rs_actionblocksexplode.cpp \
    src/actions/rs_actionblocksinsert.cpp \
    src/actions/rs_actionblocksfreezeall.cpp \
    src/actions/rs_actionblocksremove.cpp \
    src/actions/rs_actionblockstoggleview.cpp \
    src/actions/rs_actiondefault.cpp \
    src/actions/rs_actiondimaligned.cpp \
    src/actions/rs_actiondimangular.cpp \
    src/actions/rs_actiondimdiametric.cpp \
    src/actions/rs_actiondimension.cpp \
    src/actions/rs_actiondimleader.cpp \
    src/actions/rs_actiondimlinear.cpp \
    src/actions/rs_actiondimradial.cpp \
    src/actions/rs_actiondrawarc.cpp \
    src/actions/rs_actiondrawarc3p.cpp \
    src/actions/rs_actiondrawarctangential.cpp \
    src/actions/rs_actiondrawcircle.cpp \
    src/actions/rs_actiondrawcircle2p.cpp \
    src/actions/rs_actiondrawcircle3p.cpp \
    src/actions/rs_actiondrawcirclecr.cpp \
    src/actions/rs_actiondrawellipseaxis.cpp \
    src/actions/rs_actiondrawhatch.cpp \
    src/actions/rs_actiondrawimage.cpp \
    src/actions/rs_actiondrawline.cpp \
    src/actions/rs_actiondrawlineangle.cpp \
    src/actions/rs_actiondrawlinebisector.cpp \
    src/actions/rs_actiondrawlinefree.cpp \
    src/actions/rs_actiondrawlinehorvert.cpp \
    src/actions/rs_actiondrawlineparallel.cpp \
    src/actions/rs_actiondrawlineparallelthrough.cpp \
    src/actions/rs_actiondrawlinepolygon.cpp \
    src/actions/rs_actiondrawlinepolygon2.cpp \
    src/actions/rs_actiondrawlinerectangle.cpp \
    src/actions/rs_actiondrawlinerelangle.cpp \
    src/actions/rs_actiondrawlinetangent1.cpp \
    src/actions/rs_actiondrawlinetangent2.cpp \
    src/actions/rs_actiondrawpoint.cpp \
    src/actions/rs_actiondrawspline.cpp \
    src/actions/rs_actiondrawtext.cpp \
    src/actions/rs_actioneditcopy.cpp \
    src/actions/rs_actioneditpaste.cpp \
    src/actions/rs_actioneditundo.cpp \
    src/actions/rs_actionfilenew.cpp \
    src/actions/rs_actionfileopen.cpp \
    src/actions/rs_actionfilesave.cpp \
    src/actions/rs_actionfilesaveas.cpp \
    src/actions/rs_actioninfoangle.cpp \
    src/actions/rs_actioninfoarea.cpp \
    src/actions/rs_actioninfoinside.cpp \
    src/actions/rs_actioninfodist.cpp \
    src/actions/rs_actioninfodist2.cpp \
    src/actions/rs_actioninfototallength.cpp \
    src/actions/rs_actionlayersadd.cpp \
    src/actions/rs_actionlayersedit.cpp \
    src/actions/rs_actionlayersfreezeall.cpp \
    src/actions/rs_actionlayersremove.cpp \
    src/actions/rs_actionlayerstogglelock.cpp \
    src/actions/rs_actionlayerstoggleview.cpp \
    src/actions/rs_actionlibraryinsert.cpp \
    src/actions/rs_actionlockrelativezero.cpp \
    src/actions/rs_actionmodifyattributes.cpp \
    src/actions/rs_actionmodifybevel.cpp \
    src/actions/rs_actionmodifycut.cpp \
    src/actions/rs_actionmodifydelete.cpp \
    src/actions/rs_actionmodifydeletefree.cpp \
    src/actions/rs_actionmodifydeletequick.cpp \
    src/actions/rs_actionmodifyentity.cpp \
    src/actions/rs_actionmodifymirror.cpp \
    src/actions/rs_actionmodifymove.cpp \
    src/actions/rs_actionmodifymoverotate.cpp \
    src/actions/rs_actionmodifyrotate.cpp \
    src/actions/rs_actionmodifyrotate2.cpp \
    src/actions/rs_actionmodifyround.cpp \
    src/actions/rs_actionmodifyscale.cpp \
    src/actions/rs_actionmodifystretch.cpp \
    src/actions/rs_actionmodifytrim.cpp \
    src/actions/rs_actionmodifytrimamount.cpp \
    src/actions/rs_actionmodifyexplodetext.cpp \
    src/actions/rs_actionoptionsdrawing.cpp \
    src/actions/rs_actionparisdebugcreatecontainer.cpp \
    src/actions/rs_actionprintpreview.cpp \
    src/actions/rs_actionselect.cpp \
    src/actions/rs_actionselectall.cpp \
    src/actions/rs_actionselectbase.cpp \
    src/actions/rs_actionselectcontour.cpp \
    src/actions/rs_actionselectintersected.cpp \
    src/actions/rs_actionselectinvert.cpp \
    src/actions/rs_actionselectsingle.cpp \
    src/actions/rs_actionselectwindow.cpp \
    src/actions/rs_actionselectlayer.cpp \
    src/actions/rs_actionsetrelativezero.cpp \
    src/actions/rs_actionsetsnapmode.cpp \
    src/actions/rs_actionsetsnaprestriction.cpp \
    src/actions/rs_actionsnapintersectionmanual.cpp \
    src/actions/rs_actiontoolregeneratedimensions.cpp \
    src/actions/rs_actionzoomauto.cpp \
    src/actions/rs_actionzoomautoy.cpp \
    src/actions/rs_actionzoomin.cpp \
    src/actions/rs_actionzoompan.cpp \
    src/actions/rs_actionzoomprevious.cpp \
    src/actions/rs_actionzoomredraw.cpp \
    src/actions/rs_actionzoomscroll.cpp \
    src/actions/rs_actionzoomwindow.cpp \
    src/actions/rs_actiondrawpolyline.cpp \
    src/actions/rs_actionpolylineadd.cpp \
    src/actions/rs_actionpolylineappend.cpp \
    src/actions/rs_actionpolylinedel.cpp \
    src/actions/rs_actionpolylinedelbetween.cpp \
    src/actions/rs_actionpolylinetrim.cpp \
    src/actions/rs_actionpolylineequidistant.cpp \
    src/actions/rs_actionpolylinesegment.cpp
RESOURCES += res/actions/actions.qrc
IMAGES += res/actions/configure.png \
    res/actions/editcopy2.png \
    res/actions/editcut2.png \
    res/actions/editpaste2.png \
    res/actions/exit.png \
    res/actions/fileclose.png \
    res/actions/filenew.png \
    res/actions/fileopen2.png \
    res/actions/fileprint.png \
    res/actions/fileprintpreview.png \
    res/actions/filesave2.png \
    res/actions/filesaveas.png \
    res/actions/redo2.png \
    res/actions/undo2.png \
    res/actions/viewgrid.png \
    res/actions/viewdraft.png \
    res/actions/zoomauto.png \
    res/actions/zoomin.png \
    res/actions/zoomout.png \
    res/actions/zoompan.png \
    res/actions/zoomprevious.png \
    res/actions/zoomredraw.png \
    res/actions/zoomwindow.png

# ################################################################################
# UI
HEADERS += src/ui/qg_actionfactory.h \
    src/ui/qg_actionhandler.h \
    src/ui/qg_blockwidget.h \
    src/ui/qg_colorbox.h \
    src/ui/qg_commandedit.h \
    src/ui/qg_dialogfactory.h \
    src/ui/qg_filedialog.h \
    src/ui/qg_fontbox.h \
    src/ui/qg_graphicview.h \
    src/ui/qg_layerbox.h \
    src/ui/qg_layerwidget.h \
    src/ui/qg_linetypebox.h \
    src/ui/qg_listviewitem.h \
    src/ui/qg_mainwindowinterface.h \
    src/ui/qg_patternbox.h \
    src/ui/qg_pentoolbar.h \
    src/ui/qg_qt2rs.h \
    src/ui/qg_recentfiles.h \
    src/ui/qg_scrollbar.h \
    src/ui/qg_widthbox.h \
    src/ui/forms/qg_arcoptions.h \
    src/ui/forms/qg_beveloptions.h \
    src/ui/forms/qg_blockdialog.h \
    src/ui/forms/qg_cadtoolbar.h \
    src/ui/forms/qg_cadtoolbardim.h \
    src/ui/forms/qg_cadtoolbarellipses.h \
    src/ui/forms/qg_cadtoolbarcircles.h \
    src/ui/forms/qg_cadtoolbarlines.h \
    src/ui/forms/qg_cadtoolbarpoints.h \
    src/ui/forms/qg_cadtoolbarselect.h \
    src/ui/forms/qg_cadtoolbarpolylines.h \
    src/ui/forms/qg_cadtoolbarsplines.h \
    src/ui/forms/qg_cadtoolbarinfo.h \
    src/ui/forms/qg_cadtoolbarmain.h \
    src/ui/forms/qg_cadtoolbarsnap.h \
    src/ui/forms/qg_cadtoolbarmodify.h \
    src/ui/forms/qg_commandwidget.h \
    src/ui/forms/qg_cadtoolbararcs.h \
    src/ui/forms/qg_circleoptions.h \
    src/ui/forms/qg_coordinatewidget.h \
    src/ui/forms/qg_arctangentialoptions.h \
    src/ui/forms/qg_dimensionlabeleditor.h \
    src/ui/forms/qg_dimlinearoptions.h \
    src/ui/forms/qg_dimoptions.h \
    src/ui/forms/qg_dlgarc.h \
    src/ui/forms/qg_dlgattributes.h \
    src/ui/forms/qg_dlgcircle.h \
    src/ui/forms/qg_dlgdimension.h \
    src/ui/forms/qg_dlgdimlinear.h \
    src/ui/forms/qg_dlgellipse.h \
    src/ui/forms/qg_dlghatch.h \
    src/ui/forms/qg_dlgimageoptions.h \
    src/ui/forms/qg_dlginitial.h \
    src/ui/forms/qg_dlginsert.h \
    src/ui/forms/qg_dlgline.h \
    src/ui/forms/qg_dlgmirror.h \
    src/ui/forms/qg_dlgmove.h \
    src/ui/forms/qg_dlgmoverotate.h \
    src/ui/forms/qg_dlgoptionsdrawing.h \
    src/ui/forms/qg_dlgoptionsgeneral.h \
    src/ui/forms/qg_dlgoptionsvariables.h \
    src/ui/forms/qg_dlgpoint.h \
    src/ui/forms/qg_dlgrotate.h \
    src/ui/forms/qg_dlgrotate2.h \
    src/ui/forms/qg_dlgscale.h \
    src/ui/forms/qg_dlgspline.h \
    src/ui/forms/qg_dlgtext.h \
    src/ui/forms/qg_exitdialog.h \
    src/ui/forms/qg_imageoptions.h \
    src/ui/forms/qg_insertoptions.h \
    src/ui/forms/qg_layerdialog.h \
    src/ui/forms/qg_libraryinsertoptions.h \
    src/ui/forms/qg_librarywidget.h \
    src/ui/forms/qg_lineangleoptions.h \
    src/ui/forms/qg_linebisectoroptions.h \
    src/ui/forms/qg_lineoptions.h \
    src/ui/forms/qg_lineparalleloptions.h \
    src/ui/forms/qg_lineparallelthroughoptions.h \
    src/ui/forms/qg_linepolygon2options.h \
    src/ui/forms/qg_linepolygonoptions.h \
    src/ui/forms/qg_linerelangleoptions.h \
    src/ui/forms/qg_mousewidget.h \
    src/ui/forms/qg_moverotateoptions.h \
    src/ui/forms/qg_polylineoptions.h \
    src/ui/forms/qg_printpreviewoptions.h \
    src/ui/forms/qg_roundoptions.h \
    src/ui/forms/qg_selectionwidget.h \
    src/ui/forms/qg_snapdistoptions.h \
    src/ui/forms/qg_splineoptions.h \
    src/ui/forms/qg_textoptions.h \
    src/ui/forms/qg_trimamountoptions.h \
    src/ui/forms/qg_widgetpen.h \
    src/ui/forms/qg_arcoptions.ui.h \
    src/ui/forms/qg_arctangentialoptions.ui.h \
    src/ui/forms/qg_beveloptions.ui.h \
    src/ui/forms/qg_blockdialog.ui.h \
    src/ui/forms/qg_cadtoolbar.ui.h \
    src/ui/forms/qg_cadtoolbararcs.ui.h \
    src/ui/forms/qg_cadtoolbarcircles.ui.h \
    src/ui/forms/qg_cadtoolbardim.ui.h \
    src/ui/forms/qg_cadtoolbarellipses.ui.h \
    src/ui/forms/qg_cadtoolbarinfo.ui.h \
    src/ui/forms/qg_cadtoolbarlines.ui.h \
    src/ui/forms/qg_cadtoolbarmain.ui.h \
    src/ui/forms/qg_cadtoolbarmodify.ui.h \
    src/ui/forms/qg_cadtoolbarpoints.ui.h \
    src/ui/forms/qg_cadtoolbarpolylines.ui.h \
    src/ui/forms/qg_cadtoolbarselect.ui.h \
    src/ui/forms/qg_cadtoolbarsnap.ui.h \
    src/ui/forms/qg_cadtoolbarsplines.ui.h \
    src/ui/forms/qg_circleoptions.ui.h \
    src/ui/forms/qg_commandwidget.ui.h \
    src/ui/forms/qg_coordinatewidget.ui.h \
    src/ui/forms/qg_dimensionlabeleditor.ui.h \
    src/ui/forms/qg_dimlinearoptions.ui.h \
    src/ui/forms/qg_dimoptions.ui.h \
    src/ui/forms/qg_dlgarc.ui.h \
    src/ui/forms/qg_dlgattributes.ui.h \
    src/ui/forms/qg_dlgcircle.ui.h \
    src/ui/forms/qg_dlgdimension.ui.h \
    src/ui/forms/qg_dlgdimlinear.ui.h \
    src/ui/forms/qg_dlgellipse.ui.h \
    src/ui/forms/qg_dlghatch.ui.h \
    src/ui/forms/qg_dlgimageoptions.ui.h \
    src/ui/forms/qg_dlginitial.ui.h \
    src/ui/forms/qg_dlginsert.ui.h \
    src/ui/forms/qg_dlgline.ui.h \
    src/ui/forms/qg_dlgmirror.ui.h \
    src/ui/forms/qg_dlgmove.ui.h \
    src/ui/forms/qg_dlgmoverotate.ui.h \
    src/ui/forms/qg_dlgoptionsdrawing.ui.h \
    src/ui/forms/qg_dlgoptionsgeneral.ui.h \
    src/ui/forms/qg_dlgoptionsvariables.ui.h \
    src/ui/forms/qg_dlgpoint.ui.h \
    src/ui/forms/qg_dlgrotate.ui.h \
    src/ui/forms/qg_dlgrotate2.ui.h \
    src/ui/forms/qg_dlgscale.ui.h \
    src/ui/forms/qg_dlgspline.ui.h \
    src/ui/forms/qg_dlgtext.ui.h \
    src/ui/forms/qg_exitdialog.ui.h \
    src/ui/forms/qg_imageoptions.ui.h \
    src/ui/forms/qg_insertoptions.ui.h \
    src/ui/forms/qg_layerdialog.ui.h \
    src/ui/forms/qg_libraryinsertoptions.ui.h \
    src/ui/forms/qg_librarywidget.ui.h \
    src/ui/forms/qg_lineangleoptions.ui.h \
    src/ui/forms/qg_linebisectoroptions.ui.h \
    src/ui/forms/qg_lineoptions.ui.h \
    src/ui/forms/qg_lineparalleloptions.ui.h \
    src/ui/forms/qg_lineparallelthroughoptions.ui.h \
    src/ui/forms/qg_linepolygon2options.ui.h \
    src/ui/forms/qg_linepolygonoptions.ui.h \
    src/ui/forms/qg_linerelangleoptions.ui.h \
    src/ui/forms/qg_mousewidget.ui.h \
    src/ui/forms/qg_moverotateoptions.ui.h \
    src/ui/forms/qg_polylineoptions.ui.h \
    src/ui/forms/qg_printpreviewoptions.ui.h \
    src/ui/forms/qg_roundoptions.ui.h \
    src/ui/forms/qg_selectionwidget.ui.h \
    src/ui/forms/qg_snapdistoptions.ui.h \
    src/ui/forms/qg_splineoptions.ui.h \
    src/ui/forms/qg_textoptions.ui.h \
    src/ui/forms/qg_trimamountoptions.ui.h \
    src/ui/forms/qg_widgetpen.ui.h
SOURCES += src/ui/qg_actionfactory.cpp \
    src/ui/qg_actionhandler.cpp \
    src/ui/qg_blockwidget.cpp \
    src/ui/qg_colorbox.cpp \
    src/ui/qg_commandedit.cpp \
    src/ui/qg_dialogfactory.cpp \
    src/ui/qg_filedialog.cpp \
    src/ui/qg_fontbox.cpp \
    src/ui/qg_graphicview.cpp \
    src/ui/qg_layerbox.cpp \
    src/ui/qg_layerwidget.cpp \
    src/ui/qg_linetypebox.cpp \
    src/ui/qg_listviewitem.cpp \
    src/ui/qg_patternbox.cpp \
    src/ui/qg_pentoolbar.cpp \
    src/ui/qg_recentfiles.cpp \
    src/ui/qg_widthbox.cpp \
    src/ui/forms/qg_arcoptions.cpp \
    src/ui/forms/qg_arctangentialoptions.cpp \
    src/ui/forms/qg_beveloptions.cpp \
    src/ui/forms/qg_blockdialog.cpp \
    src/ui/forms/qg_cadtoolbar.cpp \
    src/ui/forms/qg_cadtoolbararcs.cpp \
    src/ui/forms/qg_cadtoolbarcircles.cpp \
    src/ui/forms/qg_cadtoolbardim.cpp \
    src/ui/forms/qg_cadtoolbarellipses.cpp \
    src/ui/forms/qg_cadtoolbarinfo.cpp \
    src/ui/forms/qg_cadtoolbarlines.cpp \
    src/ui/forms/qg_cadtoolbarmain.cpp \
    src/ui/forms/qg_cadtoolbarmodify.cpp \
    src/ui/forms/qg_cadtoolbarpoints.cpp \
    src/ui/forms/qg_cadtoolbarpolylines.cpp \
    src/ui/forms/qg_cadtoolbarselect.cpp \
    src/ui/forms/qg_cadtoolbarsnap.cpp \
    src/ui/forms/qg_cadtoolbarsplines.cpp \
    src/ui/forms/qg_circleoptions.cpp \
    src/ui/forms/qg_commandwidget.cpp \
    src/ui/forms/qg_coordinatewidget.cpp \
    src/ui/forms/qg_dimensionlabeleditor.cpp \
    src/ui/forms/qg_dimlinearoptions.cpp \
    src/ui/forms/qg_dimoptions.cpp \
    src/ui/forms/qg_dlgarc.cpp \
    src/ui/forms/qg_dlgattributes.cpp \
    src/ui/forms/qg_dlgcircle.cpp \
    src/ui/forms/qg_dlgdimension.cpp \
    src/ui/forms/qg_dlgdimlinear.cpp \
    src/ui/forms/qg_dlgellipse.cpp \
    src/ui/forms/qg_dlghatch.cpp \
    src/ui/forms/qg_dlgimageoptions.cpp \
    src/ui/forms/qg_dlginitial.cpp \
    src/ui/forms/qg_dlginsert.cpp \
    src/ui/forms/qg_dlgline.cpp \
    src/ui/forms/qg_dlgmirror.cpp \
    src/ui/forms/qg_dlgmove.cpp \
    src/ui/forms/qg_dlgmoverotate.cpp \
    src/ui/forms/qg_dlgoptionsdrawing.cpp \
    src/ui/forms/qg_dlgoptionsgeneral.cpp \
    src/ui/forms/qg_dlgoptionsvariables.cpp \
    src/ui/forms/qg_dlgpoint.cpp \
    src/ui/forms/qg_dlgrotate.cpp \
    src/ui/forms/qg_dlgrotate2.cpp \
    src/ui/forms/qg_dlgscale.cpp \
    src/ui/forms/qg_dlgspline.cpp \
    src/ui/forms/qg_dlgtext.cpp \
    src/ui/forms/qg_exitdialog.cpp \
    src/ui/forms/qg_imageoptions.cpp \
    src/ui/forms/qg_insertoptions.cpp \
    src/ui/forms/qg_layerdialog.cpp \
    src/ui/forms/qg_libraryinsertoptions.cpp \
    src/ui/forms/qg_librarywidget.cpp \
    src/ui/forms/qg_lineangleoptions.cpp \
    src/ui/forms/qg_linebisectoroptions.cpp \
    src/ui/forms/qg_lineoptions.cpp \
    src/ui/forms/qg_lineparalleloptions.cpp \
    src/ui/forms/qg_lineparallelthroughoptions.cpp \
    src/ui/forms/qg_linepolygon2options.cpp \
    src/ui/forms/qg_linepolygonoptions.cpp \
    src/ui/forms/qg_linerelangleoptions.cpp \
    src/ui/forms/qg_mousewidget.cpp \
    src/ui/forms/qg_moverotateoptions.cpp \
    src/ui/forms/qg_polylineoptions.cpp \
    src/ui/forms/qg_printpreviewoptions.cpp \
    src/ui/forms/qg_roundoptions.cpp \
    src/ui/forms/qg_selectionwidget.cpp \
    src/ui/forms/qg_snapdistoptions.cpp \
    src/ui/forms/qg_splineoptions.cpp \
    src/ui/forms/qg_textoptions.cpp \
    src/ui/forms/qg_trimamountoptions.cpp \
    src/ui/forms/qg_widgetpen.cpp
FORMS = src/ui/forms/qg_commandwidget.ui \
    src/ui/forms/qg_arcoptions.ui \
    src/ui/forms/qg_arctangentialoptions.ui \
    src/ui/forms/qg_beveloptions.ui \
    src/ui/forms/qg_blockdialog.ui \
    src/ui/forms/qg_cadtoolbar.ui \
    src/ui/forms/qg_cadtoolbararcs.ui \
    src/ui/forms/qg_cadtoolbarcircles.ui \
    src/ui/forms/qg_cadtoolbardim.ui \
    src/ui/forms/qg_cadtoolbarellipses.ui \
    src/ui/forms/qg_cadtoolbarinfo.ui \
    src/ui/forms/qg_cadtoolbarlines.ui \
    src/ui/forms/qg_cadtoolbarmain.ui \
    src/ui/forms/qg_cadtoolbarmodify.ui \
    src/ui/forms/qg_cadtoolbarpoints.ui \
    src/ui/forms/qg_cadtoolbarpolylines.ui \
    src/ui/forms/qg_cadtoolbarselect.ui \
    src/ui/forms/qg_cadtoolbarsnap.ui \
    src/ui/forms/qg_cadtoolbarsplines.ui \
    src/ui/forms/qg_circleoptions.ui \
    src/ui/forms/qg_coordinatewidget.ui \
    src/ui/forms/qg_dimensionlabeleditor.ui \
    src/ui/forms/qg_dimlinearoptions.ui \
    src/ui/forms/qg_dimoptions.ui \
    src/ui/forms/qg_dlgattributes.ui \
    src/ui/forms/qg_dlghatch.ui \
    src/ui/forms/qg_dlginitial.ui \
    src/ui/forms/qg_dlginsert.ui \
    src/ui/forms/qg_dlgimageoptions.ui \
    src/ui/forms/qg_dlgarc.ui \
    src/ui/forms/qg_dlgcircle.ui \
    src/ui/forms/qg_dlgdimension.ui \
    src/ui/forms/qg_dlgdimlinear.ui \
    src/ui/forms/qg_dlgline.ui \
    src/ui/forms/qg_dlgellipse.ui \
    src/ui/forms/qg_dlgmirror.ui \
    src/ui/forms/qg_dlgmove.ui \
    src/ui/forms/qg_dlgmoverotate.ui \
    src/ui/forms/qg_dlgoptionsdrawing.ui \
    src/ui/forms/qg_dlgoptionsgeneral.ui \
    src/ui/forms/qg_dlgoptionsvariables.ui \
    src/ui/forms/qg_dlgpoint.ui \
    src/ui/forms/qg_dlgrotate.ui \
    src/ui/forms/qg_dlgrotate2.ui \
    src/ui/forms/qg_dlgscale.ui \
    src/ui/forms/qg_dlgspline.ui \
    src/ui/forms/qg_dlgtext.ui \
    src/ui/forms/qg_exitdialog.ui \
    src/ui/forms/qg_imageoptions.ui \
    src/ui/forms/qg_insertoptions.ui \
    src/ui/forms/qg_layerdialog.ui \
    src/ui/forms/qg_libraryinsertoptions.ui \
    src/ui/forms/qg_librarywidget.ui \
    src/ui/forms/qg_lineangleoptions.ui \
    src/ui/forms/qg_linebisectoroptions.ui \
    src/ui/forms/qg_lineoptions.ui \
    src/ui/forms/qg_lineparalleloptions.ui \
    src/ui/forms/qg_lineparallelthroughoptions.ui \
    src/ui/forms/qg_linepolygon2options.ui \
    src/ui/forms/qg_linepolygonoptions.ui \
    src/ui/forms/qg_linerelangleoptions.ui \
    src/ui/forms/qg_mousewidget.ui \
    src/ui/forms/qg_moverotateoptions.ui \
    src/ui/forms/qg_polylineoptions.ui \
    src/ui/forms/qg_printpreviewoptions.ui \
    src/ui/forms/qg_roundoptions.ui \
    src/ui/forms/qg_selectionwidget.ui \
    src/ui/forms/qg_snapdistoptions.ui \
    src/ui/forms/qg_splineoptions.ui \
    src/ui/forms/qg_textoptions.ui \
    src/ui/forms/qg_trimamountoptions.ui \
    src/ui/forms/qg_widgetpen.ui
RESOURCES += res/ui/ui.qrc

# ################################################################################
# Main
HEADERS += \
    src/main/qc_applicationwindow.h \
    src/main/qc_dialogfactory.h \
    src/main/qc_graphicview.h \
    src/main/qc_mdiwindow.h \
    src/main/helpbrowser.h \
    src/main/main.h
SOURCES += \
    src/main/qc_applicationwindow.cpp \
    src/main/qc_dialogfactory.cpp \
    src/main/qc_graphicview.cpp \
    src/main/qc_mdiwindow.cpp \
    src/main/helpbrowser.cpp \
    src/main/main.cpp
IMAGES += res/main/contents.png \
    res/main/document.png \
    res/main/editclear.png \
    res/main/librecad16.png \
    res/main/librecad.png
RESOURCES += res/main/main.qrc

# ################################################################################
# Translations
TRANSLATIONS = ts/librecad_cs.ts \
    ts/librecad_et.ts \
    ts/librecad_en.ts \
    ts/librecad_da.ts \
    ts/librecad_de.ts \
    ts/librecad_el.ts \
    ts/librecad_es.ts \
    ts/librecad_es_ar.ts \
    ts/librecad_es_bo.ts \
    ts/librecad_es_cl.ts \
    ts/librecad_es_co.ts \
    ts/librecad_es_cr.ts \
    ts/librecad_es_do.ts \
    ts/librecad_es_ec.ts \
    ts/librecad_es_gt.ts \
    ts/librecad_es_hn.ts \
    ts/librecad_es_mx.ts \
    ts/librecad_es_ni.ts \
    ts/librecad_es_pa.ts \
    ts/librecad_es_pe.ts \
    ts/librecad_es_pr.ts \
    ts/librecad_es_py.ts \
    ts/librecad_es_sv.ts \
    ts/librecad_es_us.ts \
    ts/librecad_es_uy.ts \
    ts/librecad_es_ve.ts \
    ts/librecad_fr.ts \
    ts/librecad_hu.ts \
    ts/librecad_it.ts \
    ts/librecad_nl.ts \
    ts/librecad_no.ts \
    ts/librecad_pa.ts \
    ts/librecad_pl.ts \
    ts/librecad_pt.ts \
    ts/librecad_ru.ts \
    ts/librecad_sk.ts \
    ts/librecad_tr.ts


# Include any custom.pro files for personal/special builds
exists( custom.pro ):include( custom.pro )

