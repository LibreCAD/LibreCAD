# LibreCAD project file
# (c) Ries van Twisk (librecad@rvt.dds.nl)
TEMPLATE = app
DEFINES += QC_APPKEY="\"/LibreCAD\""
DEFINES += QC_APPNAME="\"LibreCAD\""
DEFINES += QC_COMPANYNAME="\"LibreCAD\""
DEFINES += QC_COMPANYKEY="\"LibreCAD\""
DEFINES += QC_VERSION="\"master\""
DEFINES += QC_DELAYED_SPLASH_SCREEN=1
#uncomment to use 2D rs_vector instead of 3D
#DEFINES += RS_VECTOR2D=1

DEFINES += USE_DXFRW=1

HAS_CPP11 =
#HAS_CPP11 += 1
count(HAS_CPP11, 1) {
    DEFINES += HAS_CPP11=1
    QMAKE_CXXFLAGS_DEBUG += -std=c++0x
    QMAKE_CXXFLAGS += -std=c++0x
}


CONFIG += qt \
    warn_on \
    link_prl \
    help
QMAKE_CXXFLAGS_DEBUG +=
QMAKE_CXXFLAGS +=

PRE_TARGETDEPS += ../intermediate/libdxfrw.a
PRE_TARGETDEPS += ../intermediate/libdxflib.a
PRE_TARGETDEPS += ../intermediate/libjwwlib.a
PRE_TARGETDEPS += ../intermediate/libfparser.a

# Make translations at the end of the process
unix {
    # Get SVN revision number
    # SVNREVISION = $$system(svn info -R | grep -o \"Revision: [0-9]*\" | sed -e \"s/Revision: //\" | head -n1)
    # Temporary disabled getting SCM version
    #SCMREVISION=$$system(git describe --tags)
    SCMREVISION=$$system([ "$(which git)x" != "x" -a -d .git ] && echo "$(git describe --tags)" || echo "1.0.0rc4")

    DEFINES += QC_SCMREVISION=\"$$SCMREVISION\"
    macx {
        CONFIG += x86 x86_64
        TARGET = LibreCAD
        DEFINES += QC_APPDIR="\"LibreCAD\""
        DEFINES += QINITIMAGES_LIBRECAD="qInitImages_LibreCAD"
        RC_FILE = ../res/main/librecad.icns
        DESTDIR = ..

        QMAKE_POST_LINK = cd .. && scripts/postprocess-osx.sh
    }
    else {
        TARGET = librecad
#fixme , need to detect whether boost is there
        DEFINES += HAS_BOOST=1
        DEFINES += QC_APPDIR="\"librecad\""
        DEFINES += QINITIMAGES_LIBRECAD="qInitImages_librecad"
        RC_FILE = ../res/main/librecad.icns
        DESTDIR = ../unix

#fixme , boost, how to handle boost properly for win32 and unix
#    CONFIG += link_pkgconfig
#    PKGCONFIG += boost
#
        QMAKE_POST_LINK = cd .. && scripts/postprocess-unix.sh
    }
}
win32 {
    QMAKE_CFLAGS_THREAD -= -mthreads
    QMAKE_LFLAGS_THREAD -= -mthreads
    TARGET = LibreCAD
    DEFINES += QC_APPDIR="\"LibreCAD\""
    DEFINES += QINITIMAGES_LIBRECAD="qInitImages_LibreCAD"

    RC_FILE = ..\\res\\main\\librecad.rc
    DESTDIR = ..
    QMAKE_POST_LINK = cd .. && scripts\\postprocess-win.bat
}


# Additional libraries to load
# LIBS += \
# -Ldxflib/lib -ldxf \
# Store intermedia stuff somewhere else
#LIBS += -lboost
LIBS += \
 -L../intermediate -ldxfrw -ldxflib -ljwwlib -lfparser

OBJECTS_DIR = ../intermediate/obj
MOC_DIR = ../intermediate/moc
RCC_DIR = ../intermediate/rcc
TS_DIR = ../intermediate/ts
UI_DIR = ../intermediate/ui
UI_HEADERS_DIR = ../intermediate/ui
UI_SOURCES_DIR = ../intermediate/ui
RESOURCES += ../res/extui/extui.qrc

INCLUDEPATH += \
    ../dxflib/src \
    ../libdxfrw/src \
    ../jwwlib/src \
    ../fparser \
    cmd \
    lib/actions \
    lib/creation \
    lib/debug \
    lib/engine \
    lib/fileio \
    lib/filters \
    lib/gui \
    lib/information \
    lib/math \
    lib/modification \
    lib/scripting \
    actions \
    main \
    plugins \
    ui \
    ui/forms \
    res

#depends check, bug#3411161
DEPENDPATH = $$INCLUDEPATH
# ################################################################################
# Library
HEADERS = \
    lib/actions/rs_actioninterface.h \
    lib/actions/rs_preview.h \
    lib/actions/rs_previewactioninterface.h \
    lib/actions/rs_snapper.h \
    lib/creation/rs_creation.h \
    lib/debug/rs_debug.h \
    lib/engine/rs.h \
    lib/engine/rs_arc.h \
    lib/engine/rs_atomicentity.h \
    lib/engine/rs_block.h \
    lib/engine/rs_blocklist.h \
    lib/engine/rs_blocklistlistener.h \
    lib/engine/rs_clipboard.h \
    lib/engine/rs_circle.h \
    lib/engine/rs_color.h \
    lib/engine/rs_constructionline.h \
    lib/engine/rs_dimaligned.h \
    lib/engine/rs_dimangular.h \
    lib/engine/rs_dimdiametric.h \
    lib/engine/rs_dimension.h \
    lib/engine/rs_dimlinear.h \
    lib/engine/rs_dimradial.h \
    lib/engine/rs_document.h \
    lib/engine/rs_ellipse.h \
    lib/engine/rs_entity.h \
    lib/engine/rs_entitycontainer.h \
    lib/engine/rs_flags.h \
    lib/engine/rs_font.h \
    lib/engine/rs_fontchar.h \
    lib/engine/rs_fontlist.h \
    lib/engine/rs_graphic.h \
    lib/engine/rs_hatch.h \
    lib/engine/rs_insert.h \
    lib/engine/rs_image.h \
    lib/engine/rs_layer.h \
    lib/engine/rs_layerlist.h \
    lib/engine/rs_layerlistlistener.h \
    lib/engine/rs_leader.h \
    lib/engine/rs_line.h \
    lib/engine/rs_overlayline.h \
    lib/engine/rs_overlaybox.h \
    lib/engine/rs_pattern.h \
    lib/engine/rs_patternlist.h \
    lib/engine/rs_pen.h \
    lib/engine/rs_point.h \
    lib/engine/rs_polyline.h \
    lib/engine/rs_settings.h \
    lib/engine/rs_solid.h \
    lib/engine/rs_spline.h \
    lib/engine/rs_system.h \
    lib/engine/rs_text.h \
    lib/engine/rs_undo.h \
    lib/engine/rs_undoable.h \
    lib/engine/rs_undocycle.h \
    lib/engine/rs_units.h \
    lib/engine/rs_utility.h \
    lib/engine/rs_variable.h \
    lib/engine/rs_variabledict.h \
    lib/engine/rs_vector.h \
    lib/fileio/rs_fileio.h \
    lib/filters/rs_filtercxf.h \
    lib/filters/rs_filterdxf.h \
    lib/filters/rs_filterdxfrw.h \
    lib/filters/rs_filterdxf1.h \
    lib/filters/rs_filterjww.h \
    lib/filters/rs_filterlff.h \
    lib/filters/rs_filterinterface.h \
    lib/gui/rs_commandevent.h \
    lib/gui/rs_coordinateevent.h \
    lib/gui/rs_dialogfactory.h \
    lib/gui/rs_dialogfactoryinterface.h \
    lib/gui/rs_dialogfactoryadapter.h \
    lib/gui/rs_eventhandler.h \
    lib/gui/rs_graphicview.h \
    lib/gui/rs_grid.h \
    lib/gui/rs_linetypepattern.h \
    lib/gui/rs_mainwindowinterface.h \
    lib/gui/rs_painter.h \
    lib/gui/rs_painterqt.h \
    lib/gui/rs_staticgraphicview.h \
    lib/information/rs_locale.h \
    lib/information/rs_information.h \
    lib/information/rs_infoarea.h \
    lib/modification/rs_modification.h \
    lib/modification/rs_selection.h \
    lib/math/rs_math.h \
    lib/scripting/rs_python.h \
    lib/scripting/rs_simplepython.h \
    lib/scripting/rs_python_wrappers.h \
    lib/scripting/rs_script.h \
    lib/scripting/rs_scriptlist.h \
    ui/forms/qg_snaptoolbar.h

SOURCES = \
    lib/actions/rs_actioninterface.cpp \
    lib/actions/rs_preview.cpp \
    lib/actions/rs_previewactioninterface.cpp \
    lib/actions/rs_snapper.cpp \
    lib/creation/rs_creation.cpp \
    lib/debug/rs_debug.cpp \
    lib/engine/rs_arc.cpp \
    lib/engine/rs_block.cpp \
    lib/engine/rs_blocklist.cpp \
    lib/engine/rs_clipboard.cpp \
    lib/engine/rs_circle.cpp \
    lib/engine/rs_constructionline.cpp \
    lib/engine/rs_dimaligned.cpp \
    lib/engine/rs_dimangular.cpp \
    lib/engine/rs_dimdiametric.cpp \
    lib/engine/rs_dimension.cpp \
    lib/engine/rs_dimlinear.cpp \
    lib/engine/rs_dimradial.cpp \
    lib/engine/rs_document.cpp \
    lib/engine/rs_ellipse.cpp \
    lib/engine/rs_entity.cpp \
    lib/engine/rs_entitycontainer.cpp \
    lib/engine/rs_font.cpp \
    lib/engine/rs_fontlist.cpp \
    lib/engine/rs_graphic.cpp \
    lib/engine/rs_hatch.cpp \
    lib/engine/rs_insert.cpp \
    lib/engine/rs_image.cpp \
    lib/engine/rs_layer.cpp \
    lib/engine/rs_layerlist.cpp \
    lib/engine/rs_leader.cpp \
    lib/engine/rs_line.cpp \
    lib/engine/rs_overlayline.cpp \
    lib/engine/rs_overlaybox.cpp \
    lib/engine/rs_pattern.cpp \
    lib/engine/rs_patternlist.cpp \
    lib/engine/rs_point.cpp \
    lib/engine/rs_polyline.cpp \
    lib/engine/rs_settings.cpp \
    lib/engine/rs_solid.cpp \
    lib/engine/rs_spline.cpp \
    lib/engine/rs_system.cpp \
    lib/engine/rs_text.cpp \
    lib/engine/rs_undo.cpp \
    lib/engine/rs_undoable.cpp \
    lib/engine/rs_units.cpp \
    lib/engine/rs_utility.cpp \
    lib/engine/rs_variabledict.cpp \
    lib/engine/rs_vector.cpp \
    lib/fileio/rs_fileio.cpp \
    lib/filters/rs_filtercxf.cpp \
    lib/filters/rs_filterdxf.cpp \
    lib/filters/rs_filterdxfrw.cpp \
    lib/filters/rs_filterdxf1.cpp \
    lib/filters/rs_filterjww.cpp \
    lib/filters/rs_filterlff.cpp \
    lib/gui/rs_dialogfactory.cpp \
    lib/gui/rs_eventhandler.cpp \
    lib/gui/rs_graphicview.cpp \
    lib/gui/rs_grid.cpp \
    lib/gui/rs_linetypepattern.cpp \
    lib/gui/rs_painter.cpp \
    lib/gui/rs_painterqt.cpp \
    lib/gui/rs_staticgraphicview.cpp \
    lib/information/rs_locale.cpp \
    lib/information/rs_information.cpp \
    lib/information/rs_infoarea.cpp \
    lib/math/rs_math.cpp \
    lib/modification/rs_modification.cpp \
    lib/modification/rs_selection.cpp \
    lib/scripting/rs_python.cpp \
    lib/scripting/rs_simplepython.cpp \
    lib/scripting/rs_python_wrappers.cpp \
    lib/scripting/rs_script.cpp \
    lib/scripting/rs_scriptlist.cpp \
    ui/forms/qg_snaptoolbar.cpp

# ################################################################################
# Command
HEADERS += cmd/rs_commands.h
SOURCES += cmd/rs_commands.cpp

# ################################################################################
# Actions
HEADERS += actions/rs_actionblocksadd.h \
    actions/rs_actionblocksattributes.h \
    actions/rs_actionblockscreate.h \
    actions/rs_actionblocksedit.h \
    actions/rs_actionblocksexplode.h \
    actions/rs_actionblocksinsert.h \
    actions/rs_actionblocksfreezeall.h \
    actions/rs_actionblocksremove.h \
    actions/rs_actionblockstoggleview.h \
    actions/rs_actiondefault.h \
    actions/rs_actiondimaligned.h \
    actions/rs_actiondimangular.h \
    actions/rs_actiondimdiametric.h \
    actions/rs_actiondimension.h \
    actions/rs_actiondimleader.h \
    actions/rs_actiondimlinear.h \
    actions/rs_actiondimradial.h \
    actions/rs_actiondrawarc.h \
    actions/rs_actiondrawarc3p.h \
    actions/rs_actiondrawarctangential.h \
    actions/rs_actiondrawcircle.h \
    actions/rs_actiondrawcircle2p.h \
    actions/rs_actiondrawcircle3p.h \
    actions/rs_actiondrawcirclecr.h \
    actions/rs_actiondrawcircleinscribe.h \
    actions/rs_actiondrawellipseaxis.h \
    actions/rs_actiondrawellipsefocipoint.h \
    actions/rs_actiondrawellipse4points.h \
    actions/rs_actiondrawellipsecenter3points.h \
    actions/rs_actiondrawellipseinscribe.h \
    actions/rs_actiondrawhatch.h \
    actions/rs_actiondrawimage.h \
    actions/rs_actiondrawline.h \
    actions/rs_actiondrawlineangle.h \
    actions/rs_actiondrawlinebisector.h \
    actions/rs_actiondrawlinefree.h \
    actions/rs_actiondrawlinehorvert.h \
    actions/rs_actiondrawlineparallel.h \
    actions/rs_actiondrawlineparallelthrough.h \
    actions/rs_actiondrawlineorthtan.h \
    actions/rs_actiondrawlinepolygon.h \
    actions/rs_actiondrawlinepolygon2.h \
    actions/rs_actiondrawlinerectangle.h \
    actions/rs_actiondrawlinerelangle.h \
    actions/rs_actiondrawlinetangent1.h \
    actions/rs_actiondrawlinetangent2.h \
    actions/rs_actiondrawpoint.h \
    actions/rs_actiondrawspline.h \
    actions/rs_actiondrawtext.h \
    actions/rs_actioneditcopy.h \
    actions/rs_actioneditpaste.h \
    actions/rs_actioneditundo.h \
    actions/rs_actionfilenew.h \
    actions/rs_actionfileopen.h \
    actions/rs_actionfilesave.h \
    actions/rs_actionfilesaveas.h \
    actions/rs_actioninfoangle.h \
    actions/rs_actioninfoarea.h \
    actions/rs_actioninfoinside.h \
    actions/rs_actioninfodist.h \
    actions/rs_actioninfodist2.h \
    actions/rs_actioninfototallength.h \
    actions/rs_actionlayersadd.h \
    actions/rs_actionlayersedit.h \
    actions/rs_actionlayersfreezeall.h \
    actions/rs_actionlayersremove.h \
    actions/rs_actionlayerstogglelock.h \
    actions/rs_actionlayerstoggleview.h \
    actions/rs_actionlibraryinsert.h \
    actions/rs_actionlockrelativezero.h \
    actions/rs_actionmodifyattributes.h \
    actions/rs_actionmodifybevel.h \
    actions/rs_actionmodifycut.h \
    actions/rs_actionmodifydelete.h \
    actions/rs_actionmodifydeletefree.h \
    actions/rs_actionmodifydeletequick.h \
    actions/rs_actionmodifyentity.h \
    actions/rs_actionmodifymirror.h \
    actions/rs_actionmodifymove.h \
    actions/rs_actionmodifymoverotate.h \
    actions/rs_actionmodifyoffset.h \
    actions/rs_actionmodifyrotate.h \
    actions/rs_actionmodifyrotate2.h \
    actions/rs_actionmodifyround.h \
    actions/rs_actionmodifyscale.h \
    actions/rs_actionmodifystretch.h \
    actions/rs_actionmodifytrim.h \
    actions/rs_actionmodifytrimamount.h \
    actions/rs_actionmodifyexplodetext.h \
    actions/rs_actionoptionsdrawing.h \
    actions/rs_actionparisdebugcreatecontainer.h \
    actions/rs_actionprintpreview.h \
    actions/rs_actionselect.h \
    actions/rs_actionselectall.h \
    actions/rs_actionselectbase.h \
    actions/rs_actionselectcontour.h \
    actions/rs_actionselectintersected.h \
    actions/rs_actionselectinvert.h \
    actions/rs_actionselectsingle.h \
    actions/rs_actionselectwindow.h \
    actions/rs_actionselectlayer.h \
    actions/rs_actionsetrelativezero.h \
    actions/rs_actionsetsnapmode.h \
    actions/rs_actionsetsnaprestriction.h \
    actions/rs_actionsnapintersectionmanual.h \
    actions/rs_actiontoolregeneratedimensions.h \
    actions/rs_actionzoomauto.h \
    actions/rs_actionzoomautoy.h \
    actions/rs_actionzoomin.h \
    actions/rs_actionzoompan.h \
    actions/rs_actionzoomprevious.h \
    actions/rs_actionzoomredraw.h \
    actions/rs_actionzoomscroll.h \
    actions/rs_actionzoomwindow.h \
    actions/rs_actiondrawpolyline.h \
    actions/rs_actionpolylineadd.h \
    actions/rs_actionpolylineappend.h \
    actions/rs_actionpolylinedel.h \
    actions/rs_actionpolylinedelbetween.h \
    actions/rs_actionpolylinetrim.h \
    actions/rs_actionpolylineequidistant.h \
    actions/rs_actionpolylinesegment.h

SOURCES += actions/rs_actionblocksadd.cpp \
    actions/rs_actionblocksattributes.cpp \
    actions/rs_actionblockscreate.cpp \
    actions/rs_actionblocksedit.cpp \
    actions/rs_actionblocksexplode.cpp \
    actions/rs_actionblocksinsert.cpp \
    actions/rs_actionblocksfreezeall.cpp \
    actions/rs_actionblocksremove.cpp \
    actions/rs_actionblockstoggleview.cpp \
    actions/rs_actiondefault.cpp \
    actions/rs_actiondimaligned.cpp \
    actions/rs_actiondimangular.cpp \
    actions/rs_actiondimdiametric.cpp \
    actions/rs_actiondimension.cpp \
    actions/rs_actiondimleader.cpp \
    actions/rs_actiondimlinear.cpp \
    actions/rs_actiondimradial.cpp \
    actions/rs_actiondrawarc.cpp \
    actions/rs_actiondrawarc3p.cpp \
    actions/rs_actiondrawarctangential.cpp \
    actions/rs_actiondrawcircle.cpp \
    actions/rs_actiondrawcircle2p.cpp \
    actions/rs_actiondrawcircle3p.cpp \
    actions/rs_actiondrawcirclecr.cpp \
    actions/rs_actiondrawcircleinscribe.cpp \
    actions/rs_actiondrawellipseaxis.cpp \
    actions/rs_actiondrawellipsefocipoint.cpp \
    actions/rs_actiondrawellipse4points.cpp \
    actions/rs_actiondrawellipsecenter3points.cpp \
    actions/rs_actiondrawellipseinscribe.cpp \
    actions/rs_actiondrawhatch.cpp \
    actions/rs_actiondrawimage.cpp \
    actions/rs_actiondrawline.cpp \
    actions/rs_actiondrawlineangle.cpp \
    actions/rs_actiondrawlinebisector.cpp \
    actions/rs_actiondrawlinefree.cpp \
    actions/rs_actiondrawlinehorvert.cpp \
    actions/rs_actiondrawlineparallel.cpp \
    actions/rs_actiondrawlineparallelthrough.cpp \
    actions/rs_actiondrawlineorthtan.cpp \
    actions/rs_actiondrawlinepolygon.cpp \
    actions/rs_actiondrawlinepolygon2.cpp \
    actions/rs_actiondrawlinerectangle.cpp \
    actions/rs_actiondrawlinerelangle.cpp \
    actions/rs_actiondrawlinetangent1.cpp \
    actions/rs_actiondrawlinetangent2.cpp \
    actions/rs_actiondrawpoint.cpp \
    actions/rs_actiondrawpolyline.cpp \
    actions/rs_actiondrawspline.cpp \
    actions/rs_actiondrawtext.cpp \
    actions/rs_actioneditcopy.cpp \
    actions/rs_actioneditpaste.cpp \
    actions/rs_actioneditundo.cpp \
    actions/rs_actionfilenew.cpp \
    actions/rs_actionfileopen.cpp \
    actions/rs_actionfilesave.cpp \
    actions/rs_actionfilesaveas.cpp \
    actions/rs_actioninfoangle.cpp \
    actions/rs_actioninfoarea.cpp \
    actions/rs_actioninfoinside.cpp \
    actions/rs_actioninfodist.cpp \
    actions/rs_actioninfodist2.cpp \
    actions/rs_actioninfototallength.cpp \
    actions/rs_actionlayersadd.cpp \
    actions/rs_actionlayersedit.cpp \
    actions/rs_actionlayersfreezeall.cpp \
    actions/rs_actionlayersremove.cpp \
    actions/rs_actionlayerstogglelock.cpp \
    actions/rs_actionlayerstoggleview.cpp \
    actions/rs_actionlibraryinsert.cpp \
    actions/rs_actionlockrelativezero.cpp \
    actions/rs_actionmodifyattributes.cpp \
    actions/rs_actionmodifybevel.cpp \
    actions/rs_actionmodifycut.cpp \
    actions/rs_actionmodifydelete.cpp \
    actions/rs_actionmodifydeletefree.cpp \
    actions/rs_actionmodifydeletequick.cpp \
    actions/rs_actionmodifyentity.cpp \
    actions/rs_actionmodifymirror.cpp \
    actions/rs_actionmodifymove.cpp \
    actions/rs_actionmodifymoverotate.cpp \
    actions/rs_actionmodifyoffset.cpp \
    actions/rs_actionmodifyrotate.cpp \
    actions/rs_actionmodifyrotate2.cpp \
    actions/rs_actionmodifyround.cpp \
    actions/rs_actionmodifyscale.cpp \
    actions/rs_actionmodifystretch.cpp \
    actions/rs_actionmodifytrim.cpp \
    actions/rs_actionmodifytrimamount.cpp \
    actions/rs_actionmodifyexplodetext.cpp \
    actions/rs_actionoptionsdrawing.cpp \
    actions/rs_actionparisdebugcreatecontainer.cpp \
    actions/rs_actionpolylineadd.cpp \
    actions/rs_actionpolylineappend.cpp \
    actions/rs_actionpolylinedel.cpp \
    actions/rs_actionpolylinedelbetween.cpp \
    actions/rs_actionpolylinetrim.cpp \
    actions/rs_actionpolylineequidistant.cpp \
    actions/rs_actionpolylinesegment.cpp \
    actions/rs_actionprintpreview.cpp \
    actions/rs_actionselect.cpp \
    actions/rs_actionselectall.cpp \
    actions/rs_actionselectbase.cpp \
    actions/rs_actionselectcontour.cpp \
    actions/rs_actionselectintersected.cpp \
    actions/rs_actionselectinvert.cpp \
    actions/rs_actionselectsingle.cpp \
    actions/rs_actionselectwindow.cpp \
    actions/rs_actionselectlayer.cpp \
    actions/rs_actionsetrelativezero.cpp \
    actions/rs_actionsetsnapmode.cpp \
    actions/rs_actionsetsnaprestriction.cpp \
    actions/rs_actionsnapintersectionmanual.cpp \
    actions/rs_actiontoolregeneratedimensions.cpp \
    actions/rs_actionzoomauto.cpp \
    actions/rs_actionzoomautoy.cpp \
    actions/rs_actionzoomin.cpp \
    actions/rs_actionzoompan.cpp \
    actions/rs_actionzoomprevious.cpp \
    actions/rs_actionzoomredraw.cpp \
    actions/rs_actionzoomscroll.cpp \
    actions/rs_actionzoomwindow.cpp

RESOURCES += ../res/actions/actions.qrc

# ################################################################################
# UI
HEADERS += ui/qg_actionfactory.h \
    ui/qg_actionhandler.h \
    ui/qg_blockwidget.h \
    ui/qg_colorbox.h \
    ui/qg_commandedit.h \
    ui/qg_dialogfactory.h \
    ui/qg_filedialog.h \
    ui/qg_fontbox.h \
    ui/qg_graphicview.h \
    ui/qg_layerbox.h \
    ui/qg_layerwidget.h \
    ui/qg_librarywidget.h \
    ui/qg_linetypebox.h \
    ui/qg_mainwindowinterface.h \
    ui/qg_patternbox.h \
    ui/qg_pentoolbar.h \
    ui/qg_recentfiles.h \
    ui/qg_scrollbar.h \
    ui/qg_widthbox.h \
    ui/forms/qg_arcoptions.h \
    ui/forms/qg_arctangentialoptions.h \
    ui/forms/qg_beveloptions.h \
    ui/forms/qg_blockdialog.h \
    ui/forms/qg_cadtoolbar.h \
    ui/forms/qg_cadtoolbardim.h \
    ui/forms/qg_cadtoolbarellipses.h \
    ui/forms/qg_cadtoolbarcircles.h \
    ui/forms/qg_cadtoolbarlines.h \
    ui/forms/qg_cadtoolbarpoints.h \
    ui/forms/qg_cadtoolbarselect.h \
    ui/forms/qg_cadtoolbarpolylines.h \
    ui/forms/qg_cadtoolbarsplines.h \
    ui/forms/qg_cadtoolbarinfo.h \
    ui/forms/qg_cadtoolbarmain.h \
    ui/forms/qg_cadtoolbarmodify.h \
    ui/forms/qg_commandwidget.h \
    ui/forms/qg_cadtoolbararcs.h \
    ui/forms/qg_circleoptions.h \
    ui/forms/qg_coordinatewidget.h \
    ui/forms/qg_dimensionlabeleditor.h \
    ui/forms/qg_dimlinearoptions.h \
    ui/forms/qg_dimoptions.h \
    ui/forms/qg_dlgarc.h \
    ui/forms/qg_dlgattributes.h \
    ui/forms/qg_dlgcircle.h \
    ui/forms/qg_dlgdimension.h \
    ui/forms/qg_dlgdimlinear.h \
    ui/forms/qg_dlgellipse.h \
    ui/forms/qg_dlghatch.h \
    ui/forms/qg_dlgimageoptions.h \
    ui/forms/qg_dlginitial.h \
    ui/forms/qg_dlginsert.h \
    ui/forms/qg_dlgline.h \
    ui/forms/qg_dlgmirror.h \
    ui/forms/qg_dlgmove.h \
    ui/forms/qg_dlgmoverotate.h \
    ui/forms/qg_dlgoptionsdrawing.h \
    ui/forms/qg_dlgoptionsgeneral.h \
    ui/forms/qg_dlgpoint.h \
    ui/forms/qg_dlgpolyline.h \
    ui/forms/qg_dlgrotate.h \
    ui/forms/qg_dlgrotate2.h \
    ui/forms/qg_dlgscale.h \
    ui/forms/qg_dlgspline.h \
    ui/forms/qg_dlgtext.h \
    ui/forms/qg_exitdialog.h \
    ui/forms/qg_imageoptions.h \
    ui/forms/qg_insertoptions.h \
    ui/forms/qg_layerdialog.h \
    ui/forms/qg_libraryinsertoptions.h \
    ui/forms/qg_lineangleoptions.h \
    ui/forms/qg_linebisectoroptions.h \
    ui/forms/qg_lineoptions.h \
    ui/forms/qg_lineparalleloptions.h \
    ui/forms/qg_lineparallelthroughoptions.h \
    ui/forms/qg_linepolygon2options.h \
    ui/forms/qg_linepolygonoptions.h \
    ui/forms/qg_linerelangleoptions.h \
    ui/forms/qg_modifyoffsetoptions.h \
    ui/forms/qg_mousewidget.h \
    ui/forms/qg_moverotateoptions.h \
    ui/forms/qg_polylineoptions.h \
    ui/forms/qg_polylineequidistantoptions.h \
    ui/forms/qg_printpreviewoptions.h \
    ui/forms/qg_roundoptions.h \
    ui/forms/qg_selectionwidget.h \
    ui/forms/qg_snapdistoptions.h \
    ui/forms/qg_snapmiddleoptions.h \
    ui/forms/qg_splineoptions.h \
    ui/forms/qg_textoptions.h \
    ui/forms/qg_trimamountoptions.h \
    ui/forms/qg_widgetpen.h

SOURCES += ui/qg_actionfactory.cpp \
    ui/qg_actionhandler.cpp \
    ui/qg_blockwidget.cpp \
    ui/qg_colorbox.cpp \
    ui/qg_commandedit.cpp \
    ui/qg_dialogfactory.cpp \
    ui/qg_filedialog.cpp \
    ui/qg_fontbox.cpp \
    ui/qg_graphicview.cpp \
    ui/qg_layerbox.cpp \
    ui/qg_layerwidget.cpp \
    ui/qg_librarywidget.cpp \
    ui/qg_linetypebox.cpp \
    ui/qg_patternbox.cpp \
    ui/qg_pentoolbar.cpp \
    ui/qg_recentfiles.cpp \
    ui/qg_widthbox.cpp \
    ui/forms/qg_arcoptions.cpp \
    ui/forms/qg_arctangentialoptions.cpp \
    ui/forms/qg_beveloptions.cpp \
    ui/forms/qg_blockdialog.cpp \
    ui/forms/qg_cadtoolbar.cpp \
    ui/forms/qg_cadtoolbararcs.cpp \
    ui/forms/qg_cadtoolbarcircles.cpp \
    ui/forms/qg_cadtoolbardim.cpp \
    ui/forms/qg_cadtoolbarellipses.cpp \
    ui/forms/qg_cadtoolbarinfo.cpp \
    ui/forms/qg_cadtoolbarlines.cpp \
    ui/forms/qg_cadtoolbarmain.cpp \
    ui/forms/qg_cadtoolbarmodify.cpp \
    ui/forms/qg_cadtoolbarpoints.cpp \
    ui/forms/qg_cadtoolbarpolylines.cpp \
    ui/forms/qg_cadtoolbarselect.cpp \
    ui/forms/qg_cadtoolbarsplines.cpp \
    ui/forms/qg_circleoptions.cpp \
    ui/forms/qg_commandwidget.cpp \
    ui/forms/qg_coordinatewidget.cpp \
    ui/forms/qg_dimensionlabeleditor.cpp \
    ui/forms/qg_dimlinearoptions.cpp \
    ui/forms/qg_dimoptions.cpp \
    ui/forms/qg_dlgarc.cpp \
    ui/forms/qg_dlgattributes.cpp \
    ui/forms/qg_dlgcircle.cpp \
    ui/forms/qg_dlgdimension.cpp \
    ui/forms/qg_dlgdimlinear.cpp \
    ui/forms/qg_dlgellipse.cpp \
    ui/forms/qg_dlghatch.cpp \
    ui/forms/qg_dlgimageoptions.cpp \
    ui/forms/qg_dlginitial.cpp \
    ui/forms/qg_dlginsert.cpp \
    ui/forms/qg_dlgline.cpp \
    ui/forms/qg_dlgmirror.cpp \
    ui/forms/qg_dlgmove.cpp \
    ui/forms/qg_dlgmoverotate.cpp \
    ui/forms/qg_dlgoptionsdrawing.cpp \
    ui/forms/qg_dlgoptionsgeneral.cpp \
    ui/forms/qg_dlgpoint.cpp \
    ui/forms/qg_dlgpolyline.cpp \
    ui/forms/qg_dlgrotate.cpp \
    ui/forms/qg_dlgrotate2.cpp \
    ui/forms/qg_dlgscale.cpp \
    ui/forms/qg_dlgspline.cpp \
    ui/forms/qg_dlgtext.cpp \
    ui/forms/qg_exitdialog.cpp \
    ui/forms/qg_imageoptions.cpp \
    ui/forms/qg_insertoptions.cpp \
    ui/forms/qg_layerdialog.cpp \
    ui/forms/qg_libraryinsertoptions.cpp \
    ui/forms/qg_lineangleoptions.cpp \
    ui/forms/qg_linebisectoroptions.cpp \
    ui/forms/qg_lineoptions.cpp \
    ui/forms/qg_lineparalleloptions.cpp \
    ui/forms/qg_lineparallelthroughoptions.cpp \
    ui/forms/qg_linepolygon2options.cpp \
    ui/forms/qg_linepolygonoptions.cpp \
    ui/forms/qg_linerelangleoptions.cpp \
    ui/forms/qg_modifyoffsetoptions.cpp \
    ui/forms/qg_mousewidget.cpp \
    ui/forms/qg_moverotateoptions.cpp \
    ui/forms/qg_polylineoptions.cpp \
    ui/forms/qg_polylineequidistantoptions.cpp \
    ui/forms/qg_printpreviewoptions.cpp \
    ui/forms/qg_roundoptions.cpp \
    ui/forms/qg_selectionwidget.cpp \
    ui/forms/qg_snapdistoptions.cpp \
    ui/forms/qg_snapmiddleoptions.cpp \
    ui/forms/qg_splineoptions.cpp \
    ui/forms/qg_textoptions.cpp \
    ui/forms/qg_trimamountoptions.cpp \
    ui/forms/qg_widgetpen.cpp

FORMS = ui/forms/qg_commandwidget.ui \
    ui/forms/qg_arcoptions.ui \
    ui/forms/qg_arctangentialoptions.ui \
    ui/forms/qg_beveloptions.ui \
    ui/forms/qg_blockdialog.ui \
    ui/forms/qg_cadtoolbar.ui \
    ui/forms/qg_cadtoolbararcs.ui \
    ui/forms/qg_cadtoolbarcircles.ui \
    ui/forms/qg_cadtoolbardim.ui \
    ui/forms/qg_cadtoolbarellipses.ui \
    ui/forms/qg_cadtoolbarinfo.ui \
    ui/forms/qg_cadtoolbarlines.ui \
    ui/forms/qg_cadtoolbarmain.ui \
    ui/forms/qg_cadtoolbarmodify.ui \
    ui/forms/qg_cadtoolbarpoints.ui \
    ui/forms/qg_cadtoolbarpolylines.ui \
    ui/forms/qg_cadtoolbarselect.ui \
    ui/forms/qg_cadtoolbarsplines.ui \
    ui/forms/qg_circleoptions.ui \
    ui/forms/qg_coordinatewidget.ui \
    ui/forms/qg_dimensionlabeleditor.ui \
    ui/forms/qg_dimlinearoptions.ui \
    ui/forms/qg_dimoptions.ui \
    ui/forms/qg_dlgattributes.ui \
    ui/forms/qg_dlghatch.ui \
    ui/forms/qg_dlginitial.ui \
    ui/forms/qg_dlginsert.ui \
    ui/forms/qg_dlgimageoptions.ui \
    ui/forms/qg_dlgarc.ui \
    ui/forms/qg_dlgcircle.ui \
    ui/forms/qg_dlgdimension.ui \
    ui/forms/qg_dlgdimlinear.ui \
    ui/forms/qg_dlgline.ui \
    ui/forms/qg_dlgellipse.ui \
    ui/forms/qg_dlgmirror.ui \
    ui/forms/qg_dlgmove.ui \
    ui/forms/qg_dlgmoverotate.ui \
    ui/forms/qg_dlgoptionsdrawing.ui \
    ui/forms/qg_dlgoptionsgeneral.ui \
    ui/forms/qg_dlgpoint.ui \
    ui/forms/qg_dlgpolyline.ui \
    ui/forms/qg_dlgrotate.ui \
    ui/forms/qg_dlgrotate2.ui \
    ui/forms/qg_dlgscale.ui \
    ui/forms/qg_dlgspline.ui \
    ui/forms/qg_dlgtext.ui \
    ui/forms/qg_exitdialog.ui \
    ui/forms/qg_imageoptions.ui \
    ui/forms/qg_insertoptions.ui \
    ui/forms/qg_layerdialog.ui \
    ui/forms/qg_libraryinsertoptions.ui \
    ui/forms/qg_lineangleoptions.ui \
    ui/forms/qg_linebisectoroptions.ui \
    ui/forms/qg_lineoptions.ui \
    ui/forms/qg_lineparalleloptions.ui \
    ui/forms/qg_lineparallelthroughoptions.ui \
    ui/forms/qg_linepolygon2options.ui \
    ui/forms/qg_linepolygonoptions.ui \
    ui/forms/qg_linerelangleoptions.ui \
    ui/forms/qg_modifyoffsetoptions.ui \
    ui/forms/qg_mousewidget.ui \
    ui/forms/qg_moverotateoptions.ui \
    ui/forms/qg_polylineoptions.ui \
    ui/forms/qg_polylineequidistantoptions.ui \
    ui/forms/qg_printpreviewoptions.ui \
    ui/forms/qg_roundoptions.ui \
    ui/forms/qg_selectionwidget.ui \
    ui/forms/qg_snapdistoptions.ui \
    ui/forms/qg_snapmiddleoptions.ui \
    ui/forms/qg_splineoptions.ui \
    ui/forms/qg_textoptions.ui \
    ui/forms/qg_trimamountoptions.ui \
    ui/forms/qg_widgetpen.ui \
    ui/forms/qg_snaptoolbar.ui

RESOURCES += ../res/ui/ui.qrc

# ################################################################################
# Main
HEADERS += \
    main/qc_applicationwindow.h \
    main/qc_dialogfactory.h \
    main/qc_graphicview.h \
    main/qc_mdiwindow.h \
    main/helpbrowser.h \
    main/doc_plugin_interface.h \
    plugins/document_interface.h \
    plugins/qc_plugininterface.h \
    plugins/intern/qc_actiongetpoint.h \
    plugins/intern/qc_actiongetselect.h \
    plugins/intern/qc_actiongetent.h \
    main/main.h

SOURCES += \
    main/qc_applicationwindow.cpp \
    main/qc_dialogfactory.cpp \
    main/qc_graphicview.cpp \
    main/qc_mdiwindow.cpp \
    main/helpbrowser.cpp \
    main/doc_plugin_interface.cpp \
    plugins/intern/qc_actiongetpoint.cpp \
    plugins/intern/qc_actiongetselect.cpp \
    plugins/intern/qc_actiongetent.cpp \
    main/main.cpp

RESOURCES += ../res/main/main.qrc

# ################################################################################
# Translations
TRANSLATIONS = ../ts/librecad_cs.ts \
    ../ts/librecad_et.ts \
    ../ts/librecad_en.ts \
    ../ts/librecad_en_au.ts \
    ../ts/librecad_da.ts \
    ../ts/librecad_de.ts \
    ../ts/librecad_el.ts \
    ../ts/librecad_es.ts \
    ../ts/librecad_es_ar.ts \
    ../ts/librecad_es_bo.ts \
    ../ts/librecad_es_cl.ts \
    ../ts/librecad_es_co.ts \
    ../ts/librecad_es_cr.ts \
    ../ts/librecad_es_do.ts \
    ../ts/librecad_es_ec.ts \
    ../ts/librecad_es_gt.ts \
    ../ts/librecad_es_hn.ts \
    ../ts/librecad_es_mx.ts \
    ../ts/librecad_es_ni.ts \
    ../ts/librecad_es_pa.ts \
    ../ts/librecad_es_pe.ts \
    ../ts/librecad_es_pr.ts \
    ../ts/librecad_es_py.ts \
    ../ts/librecad_es_sv.ts \
    ../ts/librecad_es_us.ts \
    ../ts/librecad_es_uy.ts \
    ../ts/librecad_es_ve.ts \
    ../ts/librecad_fi.ts \
    ../ts/librecad_fr.ts \
    ../ts/librecad_hu.ts \
    ../ts/librecad_id_ID.ts \
    ../ts/librecad_it.ts \
    ../ts/librecad_ja.ts \
    ../ts/librecad_nl.ts \
    ../ts/librecad_no.ts \
    ../ts/librecad_pa.ts \
    ../ts/librecad_pl.ts \
    ../ts/librecad_pt_br.ts \
    ../ts/librecad_pt_pt.ts \
    ../ts/librecad_ru.ts \
    ../ts/librecad_sk.ts \
    ../ts/librecad_sq_al.ts \
    ../ts/librecad_sv.ts \
    ../ts/librecad_tr.ts \
    ../ts/librecad_uk.ts \
    ../ts/librecad_zh_cn.ts \
    ../ts/librecad_zh_tw.ts

# Include any custom.pro files for personal/special builds
exists( custom.pro ):include( custom.pro )




