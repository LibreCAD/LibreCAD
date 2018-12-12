# LibreCAD project file
# (c) Ries van Twisk (librecad@rvt.dds.nl)
TEMPLATE = app

DISABLE_POSTSCRIPT = false

#uncomment to enable a Debugging menu entry for basic unit testing
#DEFINES += LC_DEBUGGING

DEFINES += DWGSUPPORT
DEFINES -= JWW_WRITE_SUPPORT

LC_VERSION="2.2.0-alpha"
VERSION=$${LC_VERSION}

# Store intermedia stuff somewhere else
GENERATED_DIR = ../../generated/librecad
# Use common project definitions.
include(../../common.pri)
include(./boost.pri)
include(./muparser.pri)

CONFIG += qt \
    warn_on \
    link_prl \
    verbose \
    depend_includepath

QT += widgets printsupport
CONFIG += c++11
*-g++ {
    QMAKE_CXXFLAGS += -fext-numeric-literals
}

GEN_LIB_DIR = ../../generated/lib
PRE_TARGETDEPS += $$GEN_LIB_DIR/libdxfrw.a \
		$$GEN_LIB_DIR/libjwwlib.a

DESTDIR = $${INSTALLDIR}

# Make translations at the end of the process
unix {
    LC_VERSION=$$system([ "$(which git)x" != "x" -a -d ../../.git ] && echo "$(git describe --tags)" || echo "$${LC_VERSION}")

    macx {
        TARGET = LibreCAD
        VERSION=$$system(echo "$${LC_VERSION}" | sed -e 's/\-.*//g')
        QMAKE_INFO_PLIST = Info.plist.app
        DEFINES += QC_APPDIR="\"LibreCAD\""
        ICON = ../res/main/librecad.icns
        contains(DISABLE_POSTSCRIPT, false) {
            QMAKE_POST_LINK = /bin/sh $$_PRO_FILE_PWD_/../../scripts/postprocess-osx.sh $$OUT_PWD/$${DESTDIR}/$${TARGET}.app/ $$[QT_INSTALL_BINS];
            QMAKE_POST_LINK += /usr/libexec/PlistBuddy -c \"Set :CFBundleGetInfoString string $${TARGET} $${LC_VERSION}\" $$OUT_PWD/$${DESTDIR}/$${TARGET}.app/Contents/Info.plist;
        }
    }
    else {
        TARGET = librecad
        DEFINES += QC_APPDIR="\"librecad\""
        RC_FILE = ../res/main/librecad.icns
        contains(DISABLE_POSTSCRIPT, false) {
            QMAKE_POST_LINK = cd $$_PRO_FILE_PWD_/../.. && scripts/postprocess-unix.sh
        }
    }
}
win32 {
    TARGET = LibreCAD
    DEFINES += QC_APPDIR="\"LibreCAD\""

    # add MSYSGIT_DIR = PathToGitBinFolder (without quotes) in custom.pro file, for commit hash in about dialog
    !isEmpty( MSYSGIT_DIR ) {
        LC_VERSION = $$system( \"$$MSYSGIT_DIR/git.exe\" describe --tags || echo "$${LC_VERSION}")
    }

    RC_FILE = ../res/main/librecad.rc
    contains(DISABLE_POSTSCRIPT, false) {
        QMAKE_POST_LINK = "$$_PRO_FILE_PWD_/../../scripts/postprocess-win.bat" $$LC_VERSION
    }
}

DEFINES += LC_VERSION=\"$$LC_VERSION\"

# Additional libraries to load
LIBS += -L../../generated/lib  \
    -ldxfrw \
    -ljwwlib

INCLUDEPATH += \
    ../../libraries/libdxfrw/src \
    ../../libraries/jwwlib/src \
    cmd \
    lib/actions \
    lib/creation \
    lib/debug \
    lib/engine \
    lib/fileio \
    lib/filters \
    lib/generators \
    lib/gui \
    lib/information \
    lib/math \
    lib/modification \
    lib/printing \
    actions \
    main \
    main/console_dxf2pdf \
    test \
    plugins \
    ui \
    ui/forms \
    ui/generic \
    ../res

RESOURCES += ../res/extui/extui.qrc
RESOURCES += ../res/actions/actions.qrc
RESOURCES += ../res/icons/icons.qrc
RESOURCES += ../res/ui/ui.qrc
RESOURCES += ../res/main/main.qrc
RESOURCES += ../../licenses/licenses.qrc

# ################################################################################
# Library
HEADERS += \
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
    lib/engine/lc_hyperbola.h \
    lib/engine/rs_insert.h \
    lib/engine/rs_image.h \
    lib/engine/rs_layer.h \
    lib/engine/rs_layerlist.h \
    lib/engine/rs_layerlistlistener.h \
    lib/engine/rs_leader.h \
    lib/engine/rs_line.h \
    lib/engine/rs_mtext.h \
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
    lib/engine/lc_splinepoints.h \
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
    lib/math/lc_quadratic.h \
    actions/lc_actiondrawcircle2pr.h \
    test/lc_simpletests.h \
    lib/generators/lc_makercamsvg.h \
    lib/generators/lc_xmlwriterinterface.h \
    lib/generators/lc_xmlwriterqxmlstreamwriter.h \
    actions/lc_actionfileexportmakercam.h \
    lib/engine/lc_rect.h \
    lib/engine/lc_undosection.h \
    lib/printing/lc_printing.h \
    actions/lc_actiondrawlinepolygon3.h \
    main/lc_application.h

SOURCES += \
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
    lib/engine/lc_hyperbola.cpp \
    lib/engine/rs_insert.cpp \
    lib/engine/rs_image.cpp \
    lib/engine/rs_layer.cpp \
    lib/engine/rs_layerlist.cpp \
    lib/engine/rs_leader.cpp \
    lib/engine/rs_line.cpp \
    lib/engine/rs_mtext.cpp \
    lib/engine/rs_overlayline.cpp \
    lib/engine/rs_overlaybox.cpp \
    lib/engine/rs_pattern.cpp \
    lib/engine/rs_patternlist.cpp \
    lib/engine/rs_point.cpp \
    lib/engine/rs_polyline.cpp \
    lib/engine/rs_settings.cpp \
    lib/engine/rs_solid.cpp \
    lib/engine/rs_spline.cpp \
    lib/engine/lc_splinepoints.cpp \
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
    lib/math/lc_quadratic.cpp \
    lib/modification/rs_modification.cpp \
    lib/modification/rs_selection.cpp \
    lib/engine/rs_color.cpp \
    lib/engine/rs_pen.cpp \
    actions/lc_actiondrawcircle2pr.cpp \
    test/lc_simpletests.cpp \
    lib/generators/lc_xmlwriterqxmlstreamwriter.cpp \
    lib/generators/lc_makercamsvg.cpp \
    actions/lc_actionfileexportmakercam.cpp \
    lib/engine/rs_atomicentity.cpp \
    lib/engine/rs_undocycle.cpp \
    lib/engine/rs_flags.cpp \
    lib/engine/lc_rect.cpp \
    lib/engine/lc_undosection.cpp \
    lib/engine/rs.cpp \
    lib/printing/lc_printing.cpp \
    actions/lc_actiondrawlinepolygon3.cpp \
    main/lc_application.cpp

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
    actions/rs_actionblockssave.h \
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
    actions/rs_actiondrawcircletan1_2p.h \
    actions/rs_actiondrawcircletan2_1p.h \
    actions/rs_actiondrawcircletan2.h \
    actions/rs_actiondrawcircletan3.h \
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
    actions/rs_actiondrawmtext.h \
    actions/rs_actiondrawpoint.h \
    actions/rs_actiondrawpolyline.h \
    actions/rs_actiondrawspline.h \
    actions/lc_actiondrawsplinepoints.h \
    actions/rs_actiondrawtext.h \
    actions/rs_actioneditcopy.h \
    actions/rs_actioneditpaste.h \
    actions/rs_actioneditundo.h \
    actions/rs_actionfilenew.h \
    actions/rs_actionfilenewtemplate.h \
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
    actions/rs_actionlayerslockall.h \
    actions/rs_actionlayersremove.h \
    actions/rs_actionlayerstogglelock.h \
    actions/rs_actionlayerstoggleview.h \
    actions/rs_actionlayerstoggleprint.h \
    actions/lc_actionlayerstoggleconstruction.h \
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
    actions/rs_actionmodifyrevertdirection.h \
    actions/rs_actionmodifyrotate.h \
    actions/rs_actionmodifyrotate2.h \
    actions/rs_actionmodifyround.h \
    actions/rs_actionmodifyscale.h \
    actions/rs_actionmodifystretch.h \
    actions/rs_actionmodifytrim.h \
    actions/rs_actionmodifytrimamount.h \
    actions/rs_actionmodifyexplodetext.h \
    actions/rs_actionoptionsdrawing.h \
    actions/rs_actionorder.h \
    actions/rs_actionpolylineadd.h \
    actions/rs_actionpolylineappend.h \
    actions/rs_actionpolylinedel.h \
    actions/rs_actionpolylinedelbetween.h \
    actions/rs_actionpolylinetrim.h \
    actions/rs_actionpolylineequidistant.h \
    actions/rs_actionpolylinesegment.h \
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
    actions/rs_actionzoomwindow.h

SOURCES += actions/rs_actionblocksadd.cpp \
    actions/rs_actionblocksattributes.cpp \
    actions/rs_actionblockscreate.cpp \
    actions/rs_actionblocksedit.cpp \
    actions/rs_actionblockssave.cpp \
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
    actions/rs_actiondrawcircletan1_2p.cpp \
    actions/rs_actiondrawcircletan2_1p.cpp \
    actions/rs_actiondrawcircletan2.cpp \
    actions/rs_actiondrawcircletan3.cpp \
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
    actions/rs_actiondrawmtext.cpp \
    actions/rs_actiondrawpoint.cpp \
    actions/rs_actiondrawpolyline.cpp \
    actions/rs_actiondrawspline.cpp \
    actions/lc_actiondrawsplinepoints.cpp \
    actions/rs_actiondrawtext.cpp \
    actions/rs_actioneditcopy.cpp \
    actions/rs_actioneditpaste.cpp \
    actions/rs_actioneditundo.cpp \
    actions/rs_actionfilenew.cpp \
    actions/rs_actionfilenewtemplate.cpp \
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
    actions/rs_actionlayerslockall.cpp \
    actions/rs_actionlayersremove.cpp \
    actions/rs_actionlayerstogglelock.cpp \
    actions/rs_actionlayerstoggleview.cpp \
    actions/rs_actionlayerstoggleprint.cpp \
    actions/lc_actionlayerstoggleconstruction.cpp \
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
    actions/rs_actionmodifyrevertdirection.cpp \
    actions/rs_actionmodifyrotate.cpp \
    actions/rs_actionmodifyrotate2.cpp \
    actions/rs_actionmodifyround.cpp \
    actions/rs_actionmodifyscale.cpp \
    actions/rs_actionmodifystretch.cpp \
    actions/rs_actionmodifytrim.cpp \
    actions/rs_actionmodifytrimamount.cpp \
    actions/rs_actionmodifyexplodetext.cpp \
    actions/rs_actionoptionsdrawing.cpp \
    actions/rs_actionorder.cpp \
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



# ################################################################################
# UI
HEADERS += ui/lc_actionfactory.h \
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
    ui/lg_dimzerosbox.h \
    ui/forms/qg_arcoptions.h \
    ui/forms/qg_arctangentialoptions.h \
    ui/forms/qg_beveloptions.h \
    ui/forms/qg_blockdialog.h \
    ui/forms/qg_commandwidget.h \
    ui/forms/qg_circleoptions.h \
    ui/forms/qg_circletan2options.h \
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
    ui/forms/qg_dlgimage.h \
    ui/forms/qg_dlgimageoptions.h \
    ui/forms/qg_dlginitial.h \
    ui/forms/qg_dlginsert.h \
    ui/forms/qg_dlgline.h \
    ui/forms/qg_dlgmirror.h \
    ui/forms/qg_dlgmove.h \
    ui/forms/qg_dlgmoverotate.h \
    ui/forms/qg_dlgmtext.h \
    ui/forms/qg_dlgoptionsdrawing.h \
    ui/forms/qg_dlgoptionsgeneral.h \
    ui/forms/qg_dlgoptionsmakercam.h \
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
    ui/forms/qg_mtextoptions.h \
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
    ui/forms/qg_widgetpen.h \
    ui/lc_centralwidget.h \
    ui/lc_widgetfactory.h \
    ui/twostackedlabels.h \
    ui/qg_commandhistory.h \
    ui/lc_dockwidget.h \
    ui/forms/lc_dlgsplinepoints.h \
    ui/forms/lc_widgetoptionsdialog.h \
    ui/forms/qg_snaptoolbar.h \
    ui/forms/qg_activelayername.h \
    ui/lc_deviceoptions.h \
    ui/generic/comboboxoption.h \
    ui/generic/actionlist.h \
    ui/generic/widgetcreator.h \
    ui/lc_actiongroupmanager.h \
    ui/generic/linklist.h \
    ui/generic/colorcombobox.h \
    ui/generic/colorwizard.h \
    ui/lc_penwizard.h \
    ui/generic/textfileviewer.h

SOURCES += ui/lc_actionfactory.cpp \
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
    ui/lg_dimzerosbox.cpp \
    ui/forms/qg_arcoptions.cpp \
    ui/forms/qg_arctangentialoptions.cpp \
    ui/forms/qg_beveloptions.cpp \
    ui/forms/qg_blockdialog.cpp \
    ui/forms/qg_circleoptions.cpp \
    ui/forms/qg_circletan2options.cpp \
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
    ui/forms/qg_dlgimage.cpp \
    ui/forms/qg_dlgimageoptions.cpp \
    ui/forms/qg_dlginitial.cpp \
    ui/forms/qg_dlginsert.cpp \
    ui/forms/qg_dlgline.cpp \
    ui/forms/qg_dlgmirror.cpp \
    ui/forms/qg_dlgmove.cpp \
    ui/forms/qg_dlgmoverotate.cpp \
    ui/forms/qg_dlgmtext.cpp \
    ui/forms/qg_dlgoptionsdrawing.cpp \
    ui/forms/qg_dlgoptionsgeneral.cpp \
    ui/forms/qg_dlgoptionsmakercam.cpp \
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
    ui/forms/qg_mtextoptions.cpp \
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
    ui/forms/qg_widgetpen.cpp \
    ui/lc_centralwidget.cpp \
    ui/lc_widgetfactory.cpp \
    ui/twostackedlabels.cpp \
    ui/qg_commandhistory.cpp \
    ui/lc_dockwidget.cpp \
    ui/forms/lc_dlgsplinepoints.cpp \
    ui/forms/lc_widgetoptionsdialog.cpp \
    ui/forms/qg_snaptoolbar.cpp \
    ui/forms/qg_activelayername.cpp \
    ui/lc_deviceoptions.cpp \
    ui/generic/comboboxoption.cpp \
    ui/generic/actionlist.cpp \
    ui/generic/widgetcreator.cpp \
    ui/lc_actiongroupmanager.cpp \
    ui/generic/linklist.cpp \
    ui/generic/colorcombobox.cpp \
    ui/generic/colorwizard.cpp \
    ui/lc_penwizard.cpp \
    ui/generic/textfileviewer.cpp

FORMS = ui/forms/qg_commandwidget.ui \
    ui/forms/qg_arcoptions.ui \
    ui/forms/qg_arctangentialoptions.ui \
    ui/forms/qg_beveloptions.ui \
    ui/forms/qg_blockdialog.ui \
    ui/forms/qg_circleoptions.ui \
    ui/forms/qg_circletan2options.ui \
    ui/forms/qg_coordinatewidget.ui \
    ui/forms/qg_dimensionlabeleditor.ui \
    ui/forms/qg_dimlinearoptions.ui \
    ui/forms/qg_dimoptions.ui \
    ui/forms/qg_dlgattributes.ui \
    ui/forms/qg_dlghatch.ui \
    ui/forms/qg_dlginitial.ui \
    ui/forms/qg_dlginsert.ui \
    ui/forms/qg_dlgimage.ui \
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
    ui/forms/qg_dlgmtext.ui \
    ui/forms/qg_dlgoptionsdrawing.ui \
    ui/forms/qg_dlgoptionsgeneral.ui \
    ui/forms/qg_dlgoptionsmakercam.ui \
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
    ui/forms/qg_mtextoptions.ui \
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
    ui/forms/qg_snaptoolbar.ui \
    ui/forms/qg_activelayername.ui \
    ui/forms/lc_dlgsplinepoints.ui \
    ui/forms/lc_widgetoptionsdialog.ui \
    ui/lc_deviceoptions.ui \
    ui/generic/comboboxoption.ui \
    ui/generic/widgetcreator.ui \
    ui/generic/colorwizard.ui \
    ui/generic/textfileviewer.ui

# ################################################################################
# Main
HEADERS += \
    main/qc_applicationwindow.h \
    main/qc_dialogfactory.h \
    main/qc_mdiwindow.h \
    main/doc_plugin_interface.h \
    plugins/document_interface.h \
    plugins/qc_plugininterface.h \
    plugins/intern/qc_actiongetpoint.h \
    plugins/intern/qc_actiongetselect.h \
    plugins/intern/qc_actiongetent.h \
    main/main.h \
    main/mainwindowx.h \
    main/console_dxf2pdf/console_dxf2pdf.h \
    main/console_dxf2pdf/pdf_print_loop.h

SOURCES += \
    main/qc_applicationwindow.cpp \
    main/qc_dialogfactory.cpp \
    main/qc_mdiwindow.cpp \
    main/doc_plugin_interface.cpp \
    plugins/intern/qc_actiongetpoint.cpp \
    plugins/intern/qc_actiongetselect.cpp \
    plugins/intern/qc_actiongetent.cpp \
    main/main.cpp \
    main/mainwindowx.cpp \
    main/console_dxf2pdf/console_dxf2pdf.cpp \
    main/console_dxf2pdf/pdf_print_loop.cpp

# If C99 emulation is needed, add the respective source files.
contains(DEFINES, EMU_C99) {
    !build_pass:verbose:message(Emulating C99 math features.)
    SOURCES += main/emu_c99.cpp
    HEADERS += main/emu_c99.h
}


# ################################################################################
# Translations
TRANSLATIONS = ../ts/librecad_ar.ts \
    ../ts/librecad_ca.ts \
    ../ts/librecad_cs.ts \
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
    ../ts/librecad_et.ts \
    ../ts/librecad_eu.ts \
    ../ts/librecad_fi.ts \
    ../ts/librecad_fr.ts \
    ../ts/librecad_gl.ts \
    ../ts/librecad_hi.ts \
    ../ts/librecad_hu.ts \
    ../ts/librecad_id_ID.ts \
    ../ts/librecad_it.ts \
    ../ts/librecad_ja.ts \
    ../ts/librecad_ko.ts \
    ../ts/librecad_lv.ts \
    ../ts/librecad_mk.ts \
    ../ts/librecad_nl.ts \
    ../ts/librecad_no.ts \
    ../ts/librecad_pa.ts \
    ../ts/librecad_pl.ts \
    ../ts/librecad_pt_br.ts \
    ../ts/librecad_pt_pt.ts \
    ../ts/librecad_ro_ro.ts \
    ../ts/librecad_ru.ts \
    ../ts/librecad_sk.ts \
    ../ts/librecad_sl.ts \
    ../ts/librecad_sq_al.ts \
    ../ts/librecad_sv.ts \
    ../ts/librecad_ta.ts \
    ../ts/librecad_tr.ts \
    ../ts/librecad_uk.ts \
    ../ts/librecad_zh_cn.ts \
    ../ts/librecad_zh_tw.ts
