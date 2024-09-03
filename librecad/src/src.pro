# LibreCAD project file
# (c) Ries van Twisk (librecad@rvt.dds.nl)
TEMPLATE = app

DISABLE_POSTSCRIPT = false

#uncomment to enable a Debugging menu entry for basic unit testing
#DEFINES += LC_DEBUGGING

DEFINES += DWGSUPPORT
DEFINES -= JWW_WRITE_SUPPORT

LC_VERSION="2.2.2-alpha"
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
CONFIG += c++17

# using qt5 connections for UI forms
QMAKE_UIC_FLAGS += --connections string

*-g++ {
    QMAKE_CXXFLAGS += -fext-numeric-literals
}

GEN_LIB_DIR = ../../generated/lib
msvc {
    PRE_TARGETDEPS += $$GEN_LIB_DIR/dxfrw.lib \
            $$GEN_LIB_DIR/jwwlib.lib
} else {
    PRE_TARGETDEPS += $$GEN_LIB_DIR/libdxfrw.a \
            $$GEN_LIB_DIR/libjwwlib.a
}

DESTDIR = $${INSTALLDIR}

# Make translations at the end of the process
unix {
    LC_VERSION=$$system([ "$(which git)x" != "x" -a -d ../../.git ] && echo "$(git describe --always)" || echo "$${LC_VERSION}")

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
        DEFINES -=  QT_NO_SHORTCUT
    }
}
win32 {
    TARGET = LibreCAD
    DEFINES += QC_APPDIR="\"LibreCAD\""

    # add MSYSGIT_DIR = PathToGitBinFolder (without quotes) in custom.pro file, for commit hash in about dialog
    !isEmpty( MSYSGIT_DIR ) {
        LC_VERSION = $$system( \"$$MSYSGIT_DIR/git.exe\" describe || echo "$${LC_VERSION}")
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
    actions/dock_widgets \
    actions/dock_widgets/block \
    actions/dock_widgets/entity_info \
    actions/dock_widgets/layer \
    actions/dock_widgets/library \
    actions/drawing \
    actions/drawing/draw \
    actions/drawing/draw/circle \
    actions/drawing/draw/curve \
    actions/drawing/draw/dimensions \
    actions/drawing/draw/ellipse \
    actions/drawing/draw/hatch \
    actions/drawing/draw/image \
    actions/drawing/draw/line \
    actions/drawing/draw/line/shapes \
    actions/drawing/draw/line/shapes/rect \
    actions/drawing/draw/line/shapes/polygon \
    actions/drawing/draw/polyline \
    actions/drawing/draw/text \
    actions/drawing/edit \
    actions/drawing/info \
    actions/drawing/modify \
    actions/drawing/pen \
    actions/drawing/rel_zero \
    actions/drawing/selection \
    actions/drawing/snap \
    actions/drawing/zoom \
    actions/file \
    actions/options \
    actions/print_preview \
    ui \
    ui/action_options \
    ui/action_options/circle \
    ui/action_options/curve \
    ui/action_options/dimensions \
    ui/action_options/edit \
    ui/action_options/image \
    ui/action_options/info \
    ui/action_options/insert \
    ui/action_options/line \
    ui/action_options/modify \
    ui/action_options/ellipse \
    ui/action_options/other \
    ui/action_options/polyline \
    ui/action_options/print_preview \
    ui/action_options/selection \
    ui/action_options/snap \
    ui/action_options/text \
    ui/actions \
    ui/components \
    ui/components \
    ui/components/comboboxes \
    ui/components/containers \
    ui/components/creators \
    ui/components/layouts \
    ui/components/pen \
    ui/components/status_bar \
    ui/components/toolbars \
    ui/dialogs \
    ui/dialogs/actions \
    ui/dialogs/actions/modify \
    ui/dialogs/modify \
    ui/dialogs/entity \
    ui/dialogs/file \
    ui/dialogs/file/export \
    ui/dialogs/main \
    ui/dialogs/settings \
    ui/dialogs/settings/options_device \
    ui/dialogs/settings/options_drawing \
    ui/dialogs/settings/options_general \
    ui/dialogs/settings/options_widget \
    ui/dialogs/settings/shortcuts \
    ui/dock_widgets \
    ui/dock_widgets/block_widget \
    ui/dock_widgets/command_line \
    ui/dock_widgets/entity_info \
    ui/dock_widgets/layer_widget \
    ui/dock_widgets/layers_tree \
    ui/dock_widgets/library_widget \
    ui/dock_widgets/pen_palette \
    ui/dock_widgets/pen_wizard \
    ui/main \
    ui/view \
    # ui/not_used \
    # actions/not_used \
    main \
    main/console_dxf2pdf \
    test \
    plugins \
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
    actions/drawing/draw/dimensions/lc_actioncircledimbase.h \
    actions/drawing/draw/dimensions/lc_actiondrawdimbaseline.h \
    actions/drawing/draw/ellipse/lc_actiondrawellipse1point.h \
    lib/actions/lc_highlight.h \
    lib/actions/lc_modifiersinfo.h \
    lib/actions/rs_actioninterface.h \
    lib/actions/rs_preview.h \
    lib/actions/rs_previewactioninterface.h \
    lib/actions/rs_snapper.h \
    lib/creation/rs_creation.h \
    lib/debug/rs_debug.h \
    lib/engine/lc_looputils.h \
    lib/engine/lc_parabola.h \
    lib/engine/lc_refarc.h \
    lib/engine/lc_refcircle.h \
    lib/engine/lc_refellipse.h \
    lib/engine/lc_refline.h \
    lib/engine/lc_refpoint.h \
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
    lib/engine/lc_dimarc.h \
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
    lib/math/lc_linemath.h \
    lib/modification/rs_modification.h \
    lib/modification/rs_selection.h \
    lib/math/rs_math.h \
    lib/math/lc_quadratic.h \
    main/console_dxf2png.h \
    test/lc_simpletests.h \
    lib/generators/lc_makercamsvg.h \
    lib/generators/lc_xmlwriterinterface.h \
    lib/generators/lc_xmlwriterqxmlstreamwriter.h \
    lib/engine/lc_rect.h \
    lib/engine/lc_undosection.h \
    lib/printing/lc_printing.h \
    main/lc_application.h \
    ui/action_options/curve/lc_ellipsearcoptions.h \
    ui/action_options/ellipse/lc_ellipse1pointoptions.h \
    ui/dialogs/lc_dialog.h
    
SOURCES += \
    actions/drawing/draw/dimensions/lc_actioncircledimbase.cpp \
    actions/drawing/draw/dimensions/lc_actiondrawdimbaseline.cpp \
    actions/drawing/draw/ellipse/lc_actiondrawellipse1point.cpp \
    lib/actions/lc_highlight.cpp \
    lib/actions/lc_modifiersinfo.cpp \
    lib/actions/rs_actioninterface.cpp \
    lib/actions/rs_preview.cpp \
    lib/actions/rs_previewactioninterface.cpp \
    lib/actions/rs_snapper.cpp \
    lib/creation/rs_creation.cpp \
    lib/debug/rs_debug.cpp \
    lib/engine/lc_looputils.cpp \
    lib/engine/lc_parabola.cpp \
    lib/engine/lc_refarc.cpp \
    lib/engine/lc_refcircle.cpp \
    lib/engine/lc_refellipse.cpp \
    lib/engine/lc_refline.cpp \
    lib/engine/lc_refpoint.cpp \
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
    lib/engine/lc_dimarc.cpp \
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
    lib/math/lc_linemath.cpp \
    lib/math/rs_math.cpp \
    lib/math/lc_quadratic.cpp \
    lib/modification/rs_modification.cpp \
    lib/modification/rs_selection.cpp \
    lib/engine/rs_color.cpp \
    lib/engine/rs_pen.cpp \
    main/console_dxf2png.cpp \
    test/lc_simpletests.cpp \
    lib/generators/lc_xmlwriterqxmlstreamwriter.cpp \
    lib/generators/lc_makercamsvg.cpp \
    lib/engine/rs_atomicentity.cpp \
    lib/engine/rs_undocycle.cpp \
    lib/engine/rs_flags.cpp \
    lib/engine/lc_rect.cpp \
    lib/engine/lc_undosection.cpp \
    lib/engine/rs.cpp \
    lib/printing/lc_printing.cpp \
    main/lc_application.cpp \
    ui/action_options/curve/lc_ellipsearcoptions.cpp \
    ui/action_options/ellipse/lc_ellipse1pointoptions.cpp \
    ui/dialogs/lc_dialog.cpp

# ################################################################################
# Command
HEADERS += cmd/rs_commands.h
HEADERS += cmd/lc_commandItems.h
SOURCES += cmd/rs_commands.cpp

# ################################################################################
# Actions
HEADERS += actions/dock_widgets/block/rs_actionblocksadd.h \
    actions/dock_widgets/block/rs_actionblocksattributes.h \
    actions/dock_widgets/block/rs_actionblockscreate.h \
    actions/dock_widgets/block/rs_actionblocksedit.h \
    actions/dock_widgets/block/rs_actionblocksfreezeall.h \
    actions/dock_widgets/block/rs_actionblocksinsert.h \
    actions/dock_widgets/block/rs_actionblocksremove.h \
    actions/dock_widgets/block/rs_actionblockssave.h \
    actions/dock_widgets/block/rs_actionblockstoggleview.h \
    actions/dock_widgets/entity_info/lc_actioninfopickcoordinates.h \
    actions/dock_widgets/layer/lc_actionlayersexport.h \
    actions/dock_widgets/layer/lc_actionlayerstoggleconstruction.h \
    actions/dock_widgets/layer/rs_actionlayersadd.h \
    actions/dock_widgets/layer/rs_actionlayersedit.h \
    actions/dock_widgets/layer/rs_actionlayersfreezeall.h \
    actions/dock_widgets/layer/rs_actionlayerslockall.h \
    actions/dock_widgets/layer/rs_actionlayersremove.h \
    actions/dock_widgets/layer/rs_actionlayerstogglelock.h \
    actions/dock_widgets/layer/rs_actionlayerstoggleprint.h \
    actions/dock_widgets/layer/rs_actionlayerstoggleview.h \
    actions/dock_widgets/library/rs_actionlibraryinsert.h \
    actions/drawing/draw/circle/lc_actiondrawcircle2pr.h \
    actions/drawing/draw/circle/lc_actiondrawcirclebase.h \
    actions/drawing/draw/circle/lc_actiondrawcirclebyarc.h \
    actions/drawing/draw/circle/rs_actiondrawcircle.h \
    actions/drawing/draw/circle/rs_actiondrawcircle2p.h \
    actions/drawing/draw/circle/rs_actiondrawcircle3p.h \
    actions/drawing/draw/circle/rs_actiondrawcirclecr.h \
    actions/drawing/draw/circle/rs_actiondrawcircleinscribe.h \
    actions/drawing/draw/circle/rs_actiondrawcircletan1_2p.h \
    actions/drawing/draw/circle/rs_actiondrawcircletan2.h \
    actions/drawing/draw/circle/rs_actiondrawcircletan2_1p.h \
    actions/drawing/draw/circle/rs_actiondrawcircletan3.h \
    actions/drawing/draw/curve/lc_actiondrawparabola4points.h \
    actions/drawing/draw/curve/lc_actiondrawparabolaFD.h \
    actions/drawing/draw/curve/lc_actiondrawsplinepoints.h \
    actions/drawing/draw/curve/rs_actiondrawarc.h \
    actions/drawing/draw/curve/rs_actiondrawarc3p.h \
    actions/drawing/draw/curve/rs_actiondrawarctangential.h \
    actions/drawing/draw/curve/rs_actiondrawlinefree.h \
    actions/drawing/draw/curve/rs_actiondrawspline.h \
    actions/drawing/draw/dimensions/lc_actiondimarc.h \
    actions/drawing/draw/dimensions/lc_actiondimlinearbase.h \
    actions/drawing/draw/dimensions/rs_actiondimaligned.h \
    actions/drawing/draw/dimensions/rs_actiondimangular.h \
    actions/drawing/draw/dimensions/rs_actiondimdiametric.h \
    actions/drawing/draw/dimensions/rs_actiondimension.h \
    actions/drawing/draw/dimensions/rs_actiondimleader.h \
    actions/drawing/draw/dimensions/rs_actiondimlinear.h \
    actions/drawing/draw/dimensions/rs_actiondimradial.h \
    actions/drawing/draw/dimensions/rs_actiontoolregeneratedimensions.h \
    actions/drawing/draw/ellipse/rs_actiondrawellipse4points.h \
    actions/drawing/draw/ellipse/rs_actiondrawellipseaxis.h \
    actions/drawing/draw/ellipse/rs_actiondrawellipsecenter3points.h \
    actions/drawing/draw/ellipse/rs_actiondrawellipsefocipoint.h \
    actions/drawing/draw/ellipse/rs_actiondrawellipseinscribe.h \
    actions/drawing/draw/hatch/rs_actiondrawhatch.h \
    actions/drawing/draw/image/rs_actiondrawimage.h \
    actions/drawing/draw/line/lc_abstractactiondrawline.h \
    actions/drawing/draw/line/lc_actiondrawcross.h \
    actions/drawing/draw/line/lc_actiondrawlineanglerel.h \
    actions/drawing/draw/line/lc_actiondrawlinefrompointtoline.h \
    actions/drawing/draw/line/lc_actiondrawlinepoints.h \
    actions/drawing/draw/line/lc_actiondrawlinesnake.h \
    actions/drawing/draw/line/lc_actiondrawslicedivide.h \
    actions/drawing/draw/line/rs_actiondrawline.h \
    actions/drawing/draw/line/rs_actiondrawlineangle.h \
    actions/drawing/draw/line/rs_actiondrawlinebisector.h \
    actions/drawing/draw/line/rs_actiondrawlinehorvert.h \
    actions/drawing/draw/line/rs_actiondrawlineorthtan.h \
    actions/drawing/draw/line/rs_actiondrawlineparallel.h \
    actions/drawing/draw/line/rs_actiondrawlineparallelthrough.h \
    actions/drawing/draw/line/rs_actiondrawlinerelangle.h \
    actions/drawing/draw/line/rs_actiondrawlinetangent1.h \
    actions/drawing/draw/line/rs_actiondrawlinetangent2.h \
    actions/drawing/draw/line/rs_actiondrawpoint.h \
    actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygon3.h \
    actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygonbase.h \
    actions/drawing/draw/line/shapes/polygon/lc_actiondrawstar.h \
    actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon.h \
    actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon2.h \
    actions/drawing/draw/line/shapes/rect/lc_abstractactiondrawrectangle.h \
    actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle1point.h \
    actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle2points.h \
    actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle3points.h \
    actions/drawing/draw/line/shapes/rect/rs_actiondrawlinerectangle.h \
    actions/drawing/draw/polyline/lc_actionpolylinedeletebase.h \
    actions/drawing/draw/polyline/rs_actiondrawpolyline.h \
    actions/drawing/draw/polyline/rs_actionpolylineadd.h \
    actions/drawing/draw/polyline/rs_actionpolylineappend.h \
    actions/drawing/draw/polyline/rs_actionpolylinedel.h \
    actions/drawing/draw/polyline/rs_actionpolylinedelbetween.h \
    actions/drawing/draw/polyline/rs_actionpolylineequidistant.h \
    actions/drawing/draw/polyline/rs_actionpolylinesegment.h \
    actions/drawing/draw/polyline/rs_actionpolylinetrim.h \
    actions/drawing/draw/text/rs_actiondrawmtext.h \
    actions/drawing/draw/text/rs_actiondrawtext.h \
    actions/drawing/edit/lc_actioneditpastetransform.h \
    actions/drawing/edit/rs_actioneditcopy.h \
    actions/drawing/edit/rs_actioneditundo.h \
    actions/drawing/info/lc_actioninfo3pointsangle.h \
    actions/drawing/info/lc_actioninfoproperties.h \
    actions/drawing/info/rs_actioninfoangle.h \
    actions/drawing/info/rs_actioninfoarea.h \
    actions/drawing/info/rs_actioninfodist.h \
    actions/drawing/info/rs_actioninfodist2.h \
    actions/drawing/info/rs_actioninfototallength.h \    
    actions/drawing/info/rs_actioninfoinside.h \
    actions/drawing/lc_abstractactionwithpreview.h \
    actions/drawing/modify/lc_actionmodifybase.h \
    actions/drawing/modify/lc_actionmodifybreakdivide.h \
    actions/drawing/modify/lc_actionmodifyduplicate.h \
    actions/drawing/modify/lc_actionmodifylinegap.h \
    actions/drawing/modify/lc_actionmodifylinejoin.h \
    actions/drawing/modify/lc_actionmodifyselectionbase.h \
    actions/drawing/modify/lc_actionpreselectionawarebase.h \
    actions/drawing/modify/rs_actionblocksexplode.h \
    actions/drawing/modify/rs_actionmodifyattributes.h \
    actions/drawing/modify/rs_actionmodifybevel.h \
    actions/drawing/modify/rs_actionmodifycut.h \
    actions/drawing/modify/rs_actionmodifydelete.h \
    actions/drawing/modify/rs_actionmodifydeletefree.h \
    actions/drawing/modify/rs_actionmodifyentity.h \
    actions/drawing/modify/rs_actionmodifyexplodetext.h \
    actions/drawing/modify/rs_actionmodifymirror.h \
    actions/drawing/modify/rs_actionmodifymove.h \
    actions/drawing/modify/rs_actionmodifymoverotate.h \
    actions/drawing/modify/rs_actionmodifyoffset.h \
    actions/drawing/modify/rs_actionmodifyrevertdirection.h \
    actions/drawing/modify/rs_actionmodifyrotate.h \
    actions/drawing/modify/rs_actionmodifyrotate2.h \
    actions/drawing/modify/rs_actionmodifyround.h \
    actions/drawing/modify/rs_actionmodifyscale.h \
    actions/drawing/modify/rs_actionmodifystretch.h \
    actions/drawing/modify/rs_actionmodifytrim.h \
    actions/drawing/modify/rs_actionmodifytrimamount.h \
    actions/drawing/modify/rs_actionorder.h \
    actions/drawing/pen/lc_actionpenapply.h \
    actions/drawing/pen/lc_actionpenpick.h \
    actions/drawing/pen/lc_actionpensyncactivebylayer.h \
    actions/drawing/rel_zero/rs_actionlockrelativezero.h \
    actions/drawing/rel_zero/rs_actionsetrelativezero.h \
    actions/drawing/rs_actiondefault.h \
    actions/drawing/selection/rs_actionselect.h \
    actions/drawing/selection/rs_actionselectall.h \
    actions/drawing/selection/rs_actionselectbase.h \
    actions/drawing/selection/rs_actionselectcontour.h \
    actions/drawing/selection/rs_actionselectintersected.h \
    actions/drawing/selection/rs_actionselectinvert.h \
    actions/drawing/selection/rs_actionselectlayer.h \
    actions/drawing/selection/rs_actionselectsingle.h \
    actions/drawing/selection/rs_actionselectwindow.h \
    actions/drawing/snap/lc_actionsnapmiddlemanual.h \
    actions/drawing/snap/rs_actionsnapintersectionmanual.h \
    actions/drawing/zoom/rs_actionzoomauto.h \
    actions/drawing/zoom/rs_actionzoomin.h \
    actions/drawing/zoom/rs_actionzoompan.h \
    actions/drawing/zoom/rs_actionzoomprevious.h \
    actions/drawing/zoom/rs_actionzoomredraw.h \
    actions/drawing/zoom/rs_actionzoomscroll.h \
    actions/drawing/zoom/rs_actionzoomwindow.h \
    actions/file/lc_actionfileexportmakercam.h \
    actions/file/rs_actionfilenew.h \
    actions/file/rs_actionfilenewtemplate.h \
    actions/file/rs_actionfileopen.h \
    actions/file/rs_actionfilesave.h \
    actions/file/rs_actionfilesaveas.h \
    # actions/not_used/rs_actioneditpaste.h \
    # actions/not_used/rs_actionmodifydeletequick.h \
    # actions/not_used/rs_actionsetsnapmode.h \
    # actions/not_used/rs_actionsetsnaprestriction.h \
    # actions/not_used/rs_actionzoomautoy.h \
    actions/options/rs_actionoptionsdrawing.h \
    actions/print_preview/rs_actionprintpreview.h

SOURCES += actions/dock_widgets/block/rs_actionblocksadd.cpp \
    actions/dock_widgets/block/rs_actionblocksattributes.cpp \
    actions/dock_widgets/block/rs_actionblockscreate.cpp \
    actions/dock_widgets/block/rs_actionblocksedit.cpp \
    actions/dock_widgets/block/rs_actionblocksfreezeall.cpp \
    actions/dock_widgets/block/rs_actionblocksinsert.cpp \
    actions/dock_widgets/block/rs_actionblocksremove.cpp \
    actions/dock_widgets/block/rs_actionblockssave.cpp \
    actions/dock_widgets/block/rs_actionblockstoggleview.cpp \
    actions/dock_widgets/entity_info/lc_actioninfopickcoordinates.cpp \
    actions/dock_widgets/layer/lc_actionlayersexport.cpp \
    actions/dock_widgets/layer/lc_actionlayerstoggleconstruction.cpp \
    actions/dock_widgets/layer/rs_actionlayersadd.cpp \
    actions/dock_widgets/layer/rs_actionlayersedit.cpp \
    actions/dock_widgets/layer/rs_actionlayersfreezeall.cpp \
    actions/dock_widgets/layer/rs_actionlayerslockall.cpp \
    actions/dock_widgets/layer/rs_actionlayersremove.cpp \
    actions/dock_widgets/layer/rs_actionlayerstogglelock.cpp \
    actions/dock_widgets/layer/rs_actionlayerstoggleprint.cpp \
    actions/dock_widgets/layer/rs_actionlayerstoggleview.cpp \
    actions/dock_widgets/library/rs_actionlibraryinsert.cpp \
    actions/drawing/draw/circle/lc_actiondrawcircle2pr.cpp \
    actions/drawing/draw/circle/lc_actiondrawcirclebase.cpp \
    actions/drawing/draw/circle/lc_actiondrawcirclebyarc.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircle.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircle2p.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircle3p.cpp \
    actions/drawing/draw/circle/rs_actiondrawcirclecr.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircleinscribe.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircletan1_2p.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircletan2.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircletan2_1p.cpp \
    actions/drawing/draw/circle/rs_actiondrawcircletan3.cpp \
    actions/drawing/draw/curve/lc_actiondrawparabola4points.cpp \
    actions/drawing/draw/curve/lc_actiondrawparabolaFD.cpp \
    actions/drawing/draw/curve/lc_actiondrawsplinepoints.cpp \
    actions/drawing/draw/curve/rs_actiondrawarc.cpp \
    actions/drawing/draw/curve/rs_actiondrawarc3p.cpp \
    actions/drawing/draw/curve/rs_actiondrawarctangential.cpp \
    actions/drawing/draw/curve/rs_actiondrawlinefree.cpp \
    actions/drawing/draw/curve/rs_actiondrawspline.cpp \
    actions/drawing/draw/dimensions/lc_actiondimarc.cpp \
    actions/drawing/draw/dimensions/lc_actiondimlinearbase.cpp \
    actions/drawing/draw/dimensions/rs_actiondimaligned.cpp \
    actions/drawing/draw/dimensions/rs_actiondimangular.cpp \
    actions/drawing/draw/dimensions/rs_actiondimdiametric.cpp \
    actions/drawing/draw/dimensions/rs_actiondimension.cpp \
    actions/drawing/draw/dimensions/rs_actiondimleader.cpp \
    actions/drawing/draw/dimensions/rs_actiondimlinear.cpp \
    actions/drawing/draw/dimensions/rs_actiondimradial.cpp \
    actions/drawing/draw/dimensions/rs_actiontoolregeneratedimensions.cpp \
    actions/drawing/draw/ellipse/rs_actiondrawellipse4points.cpp \
    actions/drawing/draw/ellipse/rs_actiondrawellipseaxis.cpp \
    actions/drawing/draw/ellipse/rs_actiondrawellipsecenter3points.cpp \
    actions/drawing/draw/ellipse/rs_actiondrawellipsefocipoint.cpp \
    actions/drawing/draw/ellipse/rs_actiondrawellipseinscribe.cpp \
    actions/drawing/draw/hatch/rs_actiondrawhatch.cpp \
    actions/drawing/draw/image/rs_actiondrawimage.cpp \
    actions/drawing/draw/line/lc_abstractactiondrawline.cpp \
    actions/drawing/draw/line/lc_actiondrawcross.cpp \
    actions/drawing/draw/line/lc_actiondrawlineanglerel.cpp \
    actions/drawing/draw/line/lc_actiondrawlinefrompointtoline.cpp \
    actions/drawing/draw/line/lc_actiondrawlinepoints.cpp \
    actions/drawing/draw/line/lc_actiondrawlinesnake.cpp \
    actions/drawing/draw/line/lc_actiondrawslicedivide.cpp \
    actions/drawing/draw/line/rs_actiondrawline.cpp \
    actions/drawing/draw/line/rs_actiondrawlineangle.cpp \
    actions/drawing/draw/line/rs_actiondrawlinebisector.cpp \
    actions/drawing/draw/line/rs_actiondrawlinehorvert.cpp \
    actions/drawing/draw/line/rs_actiondrawlineorthtan.cpp \
    actions/drawing/draw/line/rs_actiondrawlineparallel.cpp \
    actions/drawing/draw/line/rs_actiondrawlineparallelthrough.cpp \
    actions/drawing/draw/line/rs_actiondrawlinerelangle.cpp \
    actions/drawing/draw/line/rs_actiondrawlinetangent1.cpp \
    actions/drawing/draw/line/rs_actiondrawlinetangent2.cpp \
    actions/drawing/draw/line/rs_actiondrawpoint.cpp \
    actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygon3.cpp \
    actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygonbase.cpp \
    actions/drawing/draw/line/shapes/polygon/lc_actiondrawstar.cpp \
    actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon.cpp \
    actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon2.cpp \
    actions/drawing/draw/line/shapes/rect/lc_abstractactiondrawrectangle.cpp \
    actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle1point.cpp \
    actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle2points.cpp \
    actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle3points.cpp \
    actions/drawing/draw/line/shapes/rect/rs_actiondrawlinerectangle.cpp \
    actions/drawing/draw/polyline/lc_actionpolylinedeletebase.cpp \
    actions/drawing/draw/polyline/rs_actiondrawpolyline.cpp \
    actions/drawing/draw/polyline/rs_actionpolylineadd.cpp \
    actions/drawing/draw/polyline/rs_actionpolylineappend.cpp \
    actions/drawing/draw/polyline/rs_actionpolylinedel.cpp \
    actions/drawing/draw/polyline/rs_actionpolylinedelbetween.cpp \
    actions/drawing/draw/polyline/rs_actionpolylineequidistant.cpp \
    actions/drawing/draw/polyline/rs_actionpolylinesegment.cpp \
    actions/drawing/draw/polyline/rs_actionpolylinetrim.cpp \
    actions/drawing/draw/text/rs_actiondrawmtext.cpp \
    actions/drawing/draw/text/rs_actiondrawtext.cpp \
    actions/drawing/edit/lc_actioneditpastetransform.cpp \
    actions/drawing/edit/rs_actioneditcopy.cpp \
    actions/drawing/edit/rs_actioneditundo.cpp \
    actions/drawing/info/lc_actioninfo3pointsangle.cpp \
    actions/drawing/info/lc_actioninfoproperties.cpp \
    actions/drawing/info/rs_actioninfoangle.cpp \
    actions/drawing/info/rs_actioninfoarea.cpp \
    actions/drawing/info/rs_actioninfodist.cpp \
    actions/drawing/info/rs_actioninfodist2.cpp \
    actions/drawing/info/rs_actioninfototallength.cpp \
    actions/drawing/info/rs_actioninfoinside.cpp \
    actions/drawing/lc_abstractactionwithpreview.cpp \
    actions/drawing/modify/lc_actionmodifybase.cpp \
    actions/drawing/modify/lc_actionmodifybreakdivide.cpp \
    actions/drawing/modify/lc_actionmodifyduplicate.cpp \
    actions/drawing/modify/lc_actionmodifylinegap.cpp \
    actions/drawing/modify/lc_actionmodifylinejoin.cpp \
    actions/drawing/modify/lc_actionmodifyselectionbase.cpp \
    actions/drawing/modify/lc_actionpreselectionawarebase.cpp \
    actions/drawing/modify/rs_actionblocksexplode.cpp \
    actions/drawing/modify/rs_actionmodifyattributes.cpp \
    actions/drawing/modify/rs_actionmodifybevel.cpp \
    actions/drawing/modify/rs_actionmodifycut.cpp \
    actions/drawing/modify/rs_actionmodifydelete.cpp \
    actions/drawing/modify/rs_actionmodifydeletefree.cpp \
    actions/drawing/modify/rs_actionmodifyentity.cpp \
    actions/drawing/modify/rs_actionmodifyexplodetext.cpp \
    actions/drawing/modify/rs_actionmodifymirror.cpp \
    actions/drawing/modify/rs_actionmodifymove.cpp \
    actions/drawing/modify/rs_actionmodifymoverotate.cpp \
    actions/drawing/modify/rs_actionmodifyoffset.cpp \
    actions/drawing/modify/rs_actionmodifyrevertdirection.cpp \
    actions/drawing/modify/rs_actionmodifyrotate.cpp \
    actions/drawing/modify/rs_actionmodifyrotate2.cpp \
    actions/drawing/modify/rs_actionmodifyround.cpp \
    actions/drawing/modify/rs_actionmodifyscale.cpp \
    actions/drawing/modify/rs_actionmodifystretch.cpp \
    actions/drawing/modify/rs_actionmodifytrim.cpp \
    actions/drawing/modify/rs_actionmodifytrimamount.cpp \
    actions/drawing/modify/rs_actionorder.cpp \
    actions/drawing/pen/lc_actionpenapply.cpp \
    actions/drawing/pen/lc_actionpenpick.cpp \
    actions/drawing/pen/lc_actionpensyncactivebylayer.cpp \
    actions/drawing/rel_zero/rs_actionlockrelativezero.cpp \
    actions/drawing/rel_zero/rs_actionsetrelativezero.cpp \
    actions/drawing/rs_actiondefault.cpp \
    actions/drawing/selection/rs_actionselect.cpp \
    actions/drawing/selection/rs_actionselectall.cpp \
    actions/drawing/selection/rs_actionselectbase.cpp \
    actions/drawing/selection/rs_actionselectcontour.cpp \
    actions/drawing/selection/rs_actionselectintersected.cpp \
    actions/drawing/selection/rs_actionselectinvert.cpp \
    actions/drawing/selection/rs_actionselectlayer.cpp \
    actions/drawing/selection/rs_actionselectsingle.cpp \
    actions/drawing/selection/rs_actionselectwindow.cpp \
    actions/drawing/snap/lc_actionsnapmiddlemanual.cpp \
    actions/drawing/snap/rs_actionsnapintersectionmanual.cpp \
    actions/drawing/zoom/rs_actionzoomauto.cpp \
    actions/drawing/zoom/rs_actionzoomin.cpp \
    actions/drawing/zoom/rs_actionzoompan.cpp \
    actions/drawing/zoom/rs_actionzoomprevious.cpp \
    actions/drawing/zoom/rs_actionzoomredraw.cpp \
    actions/drawing/zoom/rs_actionzoomscroll.cpp \
    actions/drawing/zoom/rs_actionzoomwindow.cpp \
    actions/file/lc_actionfileexportmakercam.cpp \
    actions/file/rs_actionfilenew.cpp \
    actions/file/rs_actionfilenewtemplate.cpp \
    actions/file/rs_actionfileopen.cpp \
    actions/file/rs_actionfilesave.cpp \
    actions/file/rs_actionfilesaveas.cpp \
    # actions/not_used/rs_actioneditpaste.cpp \
    # actions/not_used/rs_actionmodifydeletequick.cpp \
    # actions/not_used/rs_actionsetsnapmode.cpp \
    # actions/not_used/rs_actionsetsnaprestriction.cpp \
    # actions/not_used/rs_actionzoomautoy.cpp \
    actions/options/rs_actionoptionsdrawing.cpp \
    actions/print_preview/rs_actionprintpreview.cpp


# ################################################################################
# UI
HEADERS += ui/action_options/circle/lc_circlebyarcoptions.h \
    ui/action_options/circle/qg_circleoptions.h \
    ui/action_options/circle/qg_circletan2options.h \
    ui/action_options/curve/qg_arcoptions.h \
    ui/action_options/curve/qg_arctangentialoptions.h \
    ui/action_options/curve/qg_splineoptions.h \
    ui/action_options/dimensions/qg_dimoptions.h \
    ui/action_options/edit/lc_pastetransformoptions.h \
    ui/action_options/image/qg_imageoptions.h \
    ui/action_options/info/lc_infodist2options.h \
    ui/action_options/insert/qg_insertoptions.h \
    ui/action_options/insert/qg_libraryinsertoptions.h \
    ui/action_options/lc_actionoptionswidget.h \
    ui/action_options/lc_actionoptionswidgetbase.h \
    ui/action_options/line/lc_crossoptions.h \
    ui/action_options/line/lc_lineanglereloptions.h \
    ui/action_options/line/lc_linefrompointtolineoptions.h \
    ui/action_options/line/lc_lineoptions.h \
    ui/action_options/line/lc_linepointsoptions.h \
    ui/action_options/line/lc_rectangle1pointoptions.h \
    ui/action_options/line/lc_rectangle2pointsoptions.h \
    ui/action_options/line/lc_rectangle3pointsoptions.h \
    ui/action_options/line/lc_slicedivideoptions.h \
    ui/action_options/line/lc_staroptions.h \
    ui/action_options/line/qg_lineangleoptions.h \
    ui/action_options/line/qg_linebisectoroptions.h \
    ui/action_options/line/qg_lineoptions.h \
    ui/action_options/line/qg_lineparalleloptions.h \
    ui/action_options/line/qg_lineparallelthroughoptions.h \
    ui/action_options/line/qg_linepolygonoptions.h \
    ui/action_options/line/qg_linerelangleoptions.h \
    ui/action_options/modify/lc_duplicateoptions.h \
    ui/action_options/modify/lc_linejoinoptions.h \
    ui/action_options/modify/lc_modifybreakdivideoptions.h \
    ui/action_options/modify/lc_modifygapoptions.h \
    ui/action_options/modify/lc_modifymirroroptions.h \
    ui/action_options/modify/lc_modifyrotateoptions.h \
    ui/action_options/modify/lc_modifyscaleoptions.h \
    ui/action_options/modify/lc_modifystretchoptions.h \
    ui/action_options/modify/lc_moveoptions.h \
    ui/action_options/modify/lc_rotate2options.h \
    ui/action_options/modify/qg_beveloptions.h \
    ui/action_options/modify/qg_modifyoffsetoptions.h \
    ui/action_options/modify/qg_moverotateoptions.h \
    ui/action_options/modify/qg_roundoptions.h \
    ui/action_options/modify/qg_trimamountoptions.h \
    ui/action_options/polyline/qg_polylineequidistantoptions.h \
    ui/action_options/polyline/qg_polylineoptions.h \
    ui/action_options/print_preview/qg_printpreviewoptions.h \
    ui/action_options/snap/qg_snapdistoptions.h \
    ui/action_options/snap/qg_snapmiddleoptions.h \
    ui/action_options/text/qg_mtextoptions.h \
    ui/action_options/text/qg_textoptions.h \
    ui/actions/lc_actionfactorybase.h \
    ui/actions/lc_actiongroup.h \
    ui/actions/lc_actiongroupmanager.h \
    ui/actions/lc_shortcutinfo.h \
    ui/actions/lc_shortcuts_manager.h \
    ui/actions/lc_shortcutsstorage.h \
    ui/components/comboboxes/comboboxoption.h \
    ui/components/comboboxes/qg_colorbox.h \
    ui/components/comboboxes/qg_fontbox.h \
    ui/components/comboboxes/qg_layerbox.h \
    ui/components/comboboxes/qg_linetypebox.h \
    ui/components/comboboxes/qg_patternbox.h \
    ui/components/comboboxes/qg_widthbox.h \
    ui/components/containers/lc_optionswidgetsholder.h \
    ui/components/containers/lc_snapoptionswidgetsholder.h \
    ui/components/creators/actionlist.h \
    ui/components/creators/widgetcreator.h \
    ui/components/layouts/lc_flexlayout.h \
    ui/components/lc_plaintextedit.h \
    ui/components/pen/qg_widgetpen.h \
    ui/components/qg_scrollbar.h \
    ui/components/status_bar/qg_activelayername.h \
    ui/components/status_bar/qg_coordinatewidget.h \
    ui/components/status_bar/qg_mousewidget.h \
    ui/components/status_bar/qg_selectionwidget.h \
    ui/components/status_bar/twostackedlabels.h \
    ui/components/textfileviewer.h \
    ui/components/toolbars/qg_pentoolbar.h \
    ui/components/toolbars/qg_snaptoolbar.h \
    ui/dialogs/actions/modify/qg_dlgmirror.h \
    ui/dialogs/actions/modify/qg_dlgmove.h \
    ui/dialogs/actions/modify/qg_dlgmoverotate.h \
    ui/dialogs/actions/modify/qg_dlgrotate.h \
    ui/dialogs/actions/modify/qg_dlgrotate2.h \
    ui/dialogs/actions/modify/qg_dlgscale.h \
    ui/dialogs/actions/qg_layerdialog.h \
    ui/dialogs/entity/LC_DlgParabola.h \
    ui/dialogs/entity/lc_dlgsplinepoints.h \
    ui/dialogs/entity/qg_blockdialog.h \
    ui/dialogs/entity/qg_dimensionlabeleditor.h \
    ui/dialogs/entity/qg_dlgarc.h \
    ui/dialogs/entity/qg_dlgattributes.h \
    ui/dialogs/entity/qg_dlgcircle.h \
    ui/dialogs/entity/qg_dlgdimension.h \
    ui/dialogs/entity/qg_dlgdimlinear.h \
    ui/dialogs/entity/qg_dlgellipse.h \
    ui/dialogs/entity/qg_dlghatch.h \
    ui/dialogs/entity/qg_dlgimage.h \
    ui/dialogs/entity/qg_dlgimageoptions.h \
    ui/dialogs/entity/qg_dlginsert.h \
    ui/dialogs/entity/qg_dlgline.h \
    ui/dialogs/entity/qg_dlgmtext.h \
    ui/dialogs/entity/qg_dlgpoint.h \
    ui/dialogs/entity/qg_dlgpolyline.h \
    ui/dialogs/entity/qg_dlgspline.h \
    ui/dialogs/entity/qg_dlgtext.h \
    ui/dialogs/file/export/qg_dlgoptionsmakercam.h \
    ui/dialogs/file/lc_filedialogservice.h \
    ui/dialogs/file/qg_filedialog.h \
    ui/dialogs/main/qg_dlginitial.h \
    ui/dialogs/main/qg_exitdialog.h \
    ui/dialogs/qg_dialogfactory.h \
    ui/dialogs/settings/options_device/lc_deviceoptions.h \
    ui/dialogs/settings/options_drawing/lg_dimzerosbox.h \
    ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.h \
    ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.h \
    ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.h \
    ui/dialogs/settings/shortcuts/lc_actionsshortcutsdialog.h \
    ui/dialogs/settings/shortcuts/lc_shortcutbutton.h \
    ui/dialogs/settings/shortcuts/lc_shortcutstreemodel.h \
    ui/dialogs/settings/shortcuts/lc_shortcutstreeview.h \
    ui/dialogs/settings/shortcuts/lc_shortcuttreeitem.h \
    ui/dock_widgets/block_widget/qg_blockwidget.h \
    ui/dock_widgets/command_line/qg_commandedit.h \
    ui/dock_widgets/command_line/qg_commandhistory.h \
    ui/dock_widgets/command_line/qg_commandwidget.h \
    ui/dock_widgets/entity_info/lc_quickinfobasedata.h \
    ui/dock_widgets/entity_info/lc_quickinfoentitydata.h \
    ui/dock_widgets/entity_info/lc_quickinfopointsdata.h \
    ui/dock_widgets/entity_info/lc_quickinfowidget.h \
    ui/dock_widgets/entity_info/lc_quickinfowidgetoptions.h \
    ui/dock_widgets/entity_info/lc_quickinfowidgetoptionsdialog.h \
    ui/dock_widgets/layer_widget/qg_layerwidget.h \
    ui/dock_widgets/layers_tree/lc_layerdialog_ex.h \
    ui/dock_widgets/layers_tree/lc_layertreeitem.h \
    ui/dock_widgets/layers_tree/lc_layertreemodel.h \
    ui/dock_widgets/layers_tree/lc_layertreemodel_options.h \
    ui/dock_widgets/layers_tree/lc_layertreeoptionsdialog.h \
    ui/dock_widgets/layers_tree/lc_layertreeview.h \
    ui/dock_widgets/layers_tree/lc_layertreewidget.h \
    ui/dock_widgets/lc_dockwidget.h \
    ui/dock_widgets/library_widget/qg_librarywidget.h \
    ui/dock_widgets/pen_palette/lc_peninforegistry.h \
    ui/dock_widgets/pen_palette/lc_penitem.h \
    ui/dock_widgets/pen_palette/lc_penpalettedata.h \
    ui/dock_widgets/pen_palette/lc_penpalettemodel.h \
    ui/dock_widgets/pen_palette/lc_penpaletteoptions.h \
    ui/dock_widgets/pen_palette/lc_penpaletteoptionsdialog.h \
    ui/dock_widgets/pen_palette/lc_penpalettewidget.h \
    ui/dock_widgets/pen_wizard/colorcombobox.h \
    ui/dock_widgets/pen_wizard/colorwizard.h \
    ui/dock_widgets/pen_wizard/lc_penwizard.h \
    ui/lc_actionfactory.h \
    ui/lc_widgetfactory.h \
    ui/main/mainwindowx.h \
    ui/main/qc_applicationwindow.h \
    ui/main/qc_mdiwindow.h \
    ui/main/qg_recentfiles.h\
    # ui/not_used/customtoolbarcreator.h \
    # ui/not_used/customwidgetcreator.h \
    # ui/not_used/helpbrowser.h \
    # ui/not_used/lc_cadtoolbarinterface.h \
    # ui/not_used/lc_customtoolbar.h \
    # ui/not_used/linklist.h \
    # ui/not_used/qc_graphicview.h \
    # ui/not_used/qg_cadtoolbar.h \
    # ui/not_used/qg_cadtoolbararcs.h \
    # ui/not_used/qg_cadtoolbarcircles.h \
    # ui/not_used/qg_cadtoolbardim.h \
    # ui/not_used/qg_cadtoolbarellipses.h \
    # ui/not_used/qg_cadtoolbarinfo.h \
    # ui/not_used/qg_cadtoolbarlines.h \
    # ui/not_used/qg_cadtoolbarmain.h \
    # ui/not_used/qg_cadtoolbarmodify.h \
    # ui/not_used/qg_cadtoolbarpolylines.h \
    # ui/not_used/qg_cadtoolbarselect.h \
    # ui/not_used/qg_cadtoolbarsplines.h \
    # ui/not_used/qg_dimlinearoptions.h \
    # ui/not_used/qg_dlgoptionsvariables.h \
    # ui/not_used/qg_linepolygon2options.h \
    # ui/not_used/qg_mainwindowinterface.h \
    ui/qg_actionhandler.h \
    ui/view/lc_centralwidget.h \
    ui/view/qg_graphicview.h

SOURCES += ui/action_options/circle/lc_circlebyarcoptions.cpp \
    ui/action_options/circle/qg_circleoptions.cpp \
    ui/action_options/circle/qg_circletan2options.cpp \
    ui/action_options/curve/qg_arcoptions.cpp \
    ui/action_options/curve/qg_arctangentialoptions.cpp \
    ui/action_options/curve/qg_splineoptions.cpp \
    ui/action_options/dimensions/qg_dimoptions.cpp \
    ui/action_options/edit/lc_pastetransformoptions.cpp \
    ui/action_options/image/qg_imageoptions.cpp \
    ui/action_options/info/lc_infodist2options.cpp \
    ui/action_options/insert/qg_insertoptions.cpp \
    ui/action_options/insert/qg_libraryinsertoptions.cpp \
    ui/action_options/lc_actionoptionswidget.cpp \
    ui/action_options/lc_actionoptionswidgetbase.cpp \
    ui/action_options/line/lc_crossoptions.cpp \
    ui/action_options/line/lc_lineanglereloptions.cpp \
    ui/action_options/line/lc_linefrompointtolineoptions.cpp \
    ui/action_options/line/lc_lineoptions.cpp \
    ui/action_options/line/lc_linepointsoptions.cpp \
    ui/action_options/line/lc_rectangle1pointoptions.cpp \
    ui/action_options/line/lc_rectangle2pointsoptions.cpp \
    ui/action_options/line/lc_rectangle3pointsoptions.cpp \
    ui/action_options/line/lc_slicedivideoptions.cpp \
    ui/action_options/line/lc_staroptions.cpp \
    ui/action_options/line/qg_lineangleoptions.cpp \
    ui/action_options/line/qg_linebisectoroptions.cpp \
    ui/action_options/line/qg_lineoptions.cpp \
    ui/action_options/line/qg_lineparalleloptions.cpp \
    ui/action_options/line/qg_lineparallelthroughoptions.cpp \
    ui/action_options/line/qg_linepolygonoptions.cpp \
    ui/action_options/line/qg_linerelangleoptions.cpp \
    ui/action_options/modify/lc_duplicateoptions.cpp \
    ui/action_options/modify/lc_linejoinoptions.cpp \
    ui/action_options/modify/lc_modifybreakdivideoptions.cpp \
    ui/action_options/modify/lc_modifygapoptions.cpp \
    ui/action_options/modify/lc_modifymirroroptions.cpp \
    ui/action_options/modify/lc_modifyrotateoptions.cpp \
    ui/action_options/modify/lc_modifyscaleoptions.cpp \
    ui/action_options/modify/lc_modifystretchoptions.cpp \
    ui/action_options/modify/lc_moveoptions.cpp \
    ui/action_options/modify/lc_rotate2options.cpp \
    ui/action_options/modify/qg_beveloptions.cpp \
    ui/action_options/modify/qg_modifyoffsetoptions.cpp \
    ui/action_options/modify/qg_moverotateoptions.cpp \
    ui/action_options/modify/qg_roundoptions.cpp \
    ui/action_options/modify/qg_trimamountoptions.cpp \
    ui/action_options/polyline/qg_polylineequidistantoptions.cpp \
    ui/action_options/polyline/qg_polylineoptions.cpp \
    ui/action_options/print_preview/qg_printpreviewoptions.cpp \
    ui/action_options/snap/qg_snapdistoptions.cpp \
    ui/action_options/snap/qg_snapmiddleoptions.cpp \
    ui/action_options/text/qg_mtextoptions.cpp \
    ui/action_options/text/qg_textoptions.cpp \
    ui/actions/lc_actionfactorybase.cpp \
    ui/actions/lc_actiongroup.cpp \
    ui/actions/lc_actiongroupmanager.cpp \
    ui/actions/lc_shortcutinfo.cpp \
    ui/actions/lc_shortcuts_manager.cpp \
    ui/actions/lc_shortcutsstorage.cpp \
    ui/components/comboboxes/comboboxoption.cpp \
    ui/components/comboboxes/qg_colorbox.cpp \
    ui/components/comboboxes/qg_fontbox.cpp \
    ui/components/comboboxes/qg_layerbox.cpp \
    ui/components/comboboxes/qg_linetypebox.cpp \
    ui/components/comboboxes/qg_patternbox.cpp \
    ui/components/comboboxes/qg_widthbox.cpp \
    ui/components/containers/lc_optionswidgetsholder.cpp \
    ui/components/containers/lc_snapoptionswidgetsholder.cpp \
    ui/components/creators/actionlist.cpp \
    ui/components/creators/widgetcreator.cpp \
    ui/components/layouts/lc_flexlayout.cpp \
    ui/components/pen/qg_widgetpen.cpp \
    ui/components/status_bar/qg_activelayername.cpp \
    ui/components/status_bar/qg_coordinatewidget.cpp \
    ui/components/status_bar/qg_mousewidget.cpp \
    ui/components/status_bar/qg_selectionwidget.cpp \
    ui/components/status_bar/twostackedlabels.cpp \
    ui/components/textfileviewer.cpp \
    ui/components/toolbars/qg_pentoolbar.cpp \
    ui/components/toolbars/qg_snaptoolbar.cpp \
    ui/dialogs/actions/modify/qg_dlgmirror.cpp \
    ui/dialogs/actions/modify/qg_dlgmove.cpp \
    ui/dialogs/actions/modify/qg_dlgmoverotate.cpp \
    ui/dialogs/actions/modify/qg_dlgrotate.cpp \
    ui/dialogs/actions/modify/qg_dlgrotate2.cpp \
    ui/dialogs/actions/modify/qg_dlgscale.cpp \
    ui/dialogs/actions/qg_layerdialog.cpp \
    ui/dialogs/entity/LC_DlgParabola.cpp \
    ui/dialogs/entity/lc_dlgsplinepoints.cpp \
    ui/dialogs/entity/qg_blockdialog.cpp \
    ui/dialogs/entity/qg_dimensionlabeleditor.cpp \
    ui/dialogs/entity/qg_dlgarc.cpp \
    ui/dialogs/entity/qg_dlgattributes.cpp \
    ui/dialogs/entity/qg_dlgcircle.cpp \
    ui/dialogs/entity/qg_dlgdimension.cpp \
    ui/dialogs/entity/qg_dlgdimlinear.cpp \
    ui/dialogs/entity/qg_dlgellipse.cpp \
    ui/dialogs/entity/qg_dlghatch.cpp \
    ui/dialogs/entity/qg_dlgimage.cpp \
    ui/dialogs/entity/qg_dlgimageoptions.cpp \
    ui/dialogs/entity/qg_dlginsert.cpp \
    ui/dialogs/entity/qg_dlgline.cpp \
    ui/dialogs/entity/qg_dlgmtext.cpp \
    ui/dialogs/entity/qg_dlgpoint.cpp \
    ui/dialogs/entity/qg_dlgpolyline.cpp \
    ui/dialogs/entity/qg_dlgspline.cpp \
    ui/dialogs/entity/qg_dlgtext.cpp \
    ui/dialogs/file/export/qg_dlgoptionsmakercam.cpp \
    ui/dialogs/file/lc_filedialogservice.cpp \
    ui/dialogs/file/qg_filedialog.cpp \
    ui/dialogs/main/qg_dlginitial.cpp \
    ui/dialogs/main/qg_exitdialog.cpp \
    ui/dialogs/qg_dialogfactory.cpp \
    ui/dialogs/settings/options_device/lc_deviceoptions.cpp \
    ui/dialogs/settings/options_drawing/lg_dimzerosbox.cpp \
    ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.cpp \
    ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.cpp \
    ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.cpp \
    ui/dialogs/settings/shortcuts/lc_actionsshortcutsdialog.cpp \
    ui/dialogs/settings/shortcuts/lc_shortcutbutton.cpp \
    ui/dialogs/settings/shortcuts/lc_shortcutstreemodel.cpp \
    ui/dialogs/settings/shortcuts/lc_shortcutstreeview.cpp \
    ui/dialogs/settings/shortcuts/lc_shortcuttreeitem.cpp \
    ui/dock_widgets/block_widget/qg_blockwidget.cpp \
    ui/dock_widgets/command_line/qg_commandedit.cpp \
    ui/dock_widgets/command_line/qg_commandhistory.cpp \
    ui/dock_widgets/command_line/qg_commandwidget.cpp \
    ui/dock_widgets/entity_info/lc_quickinfobasedata.cpp \
    ui/dock_widgets/entity_info/lc_quickinfoentitydata.cpp \
    ui/dock_widgets/entity_info/lc_quickinfopointsdata.cpp \
    ui/dock_widgets/entity_info/lc_quickinfowidget.cpp \
    ui/dock_widgets/entity_info/lc_quickinfowidgetoptions.cpp \
    ui/dock_widgets/entity_info/lc_quickinfowidgetoptionsdialog.cpp \
    ui/dock_widgets/layer_widget/qg_layerwidget.cpp \
    ui/dock_widgets/layers_tree/lc_layerdialog_ex.cpp \
    ui/dock_widgets/layers_tree/lc_layertreeitem.cpp \
    ui/dock_widgets/layers_tree/lc_layertreemodel.cpp \
    ui/dock_widgets/layers_tree/lc_layertreemodel_options.cpp \
    ui/dock_widgets/layers_tree/lc_layertreeoptionsdialog.cpp \
    ui/dock_widgets/layers_tree/lc_layertreeview.cpp \
    ui/dock_widgets/layers_tree/lc_layertreewidget.cpp \
    ui/dock_widgets/lc_dockwidget.cpp \
    ui/dock_widgets/library_widget/qg_librarywidget.cpp \
    ui/dock_widgets/pen_palette/lc_peninforegistry.cpp \
    ui/dock_widgets/pen_palette/lc_penitem.cpp \
    ui/dock_widgets/pen_palette/lc_penpalettedata.cpp \
    ui/dock_widgets/pen_palette/lc_penpalettemodel.cpp \
    ui/dock_widgets/pen_palette/lc_penpaletteoptions.cpp \
    ui/dock_widgets/pen_palette/lc_penpaletteoptionsdialog.cpp \
    ui/dock_widgets/pen_palette/lc_penpalettewidget.cpp \
    ui/dock_widgets/pen_wizard/colorcombobox.cpp \
    ui/dock_widgets/pen_wizard/colorwizard.cpp \
    ui/dock_widgets/pen_wizard/lc_penwizard.cpp \
    ui/lc_actionfactory.cpp \
    ui/lc_widgetfactory.cpp \
    ui/main/mainwindowx.cpp \
    ui/main/qc_applicationwindow.cpp \
    ui/main/qc_mdiwindow.cpp \
    ui/main/qg_recentfiles.cpp \
    # ui/not_used/customtoolbarcreator.cpp \
    # ui/not_used/customwidgetcreator.cpp \
    # ui/not_used/helpbrowser.cpp \
    # ui/not_used/lc_cadtoolbarinterface.cpp \
    # ui/not_used/lc_customtoolbar.cpp \
    # ui/not_used/linklist.cpp \
    # ui/not_used/qc_graphicview.cpp \
    # ui/not_used/qg_cadtoolbar.cpp \
    # ui/not_used/qg_cadtoolbararcs.cpp \
    # ui/not_used/qg_cadtoolbarcircles.cpp \
    # ui/not_used/qg_cadtoolbardim.cpp \
    # ui/not_used/qg_cadtoolbarellipses.cpp \
    # ui/not_used/qg_cadtoolbarinfo.cpp \
    # ui/not_used/qg_cadtoolbarlines.cpp \
    # ui/not_used/qg_cadtoolbarmain.cpp \
    # ui/not_used/qg_cadtoolbarmodify.cpp \
    # ui/not_used/qg_cadtoolbarpolylines.cpp \
    # ui/not_used/qg_cadtoolbarselect.cpp \
    # ui/not_used/qg_cadtoolbarsplines.cpp \
    # ui/not_used/qg_dimlinearoptions.cpp \
    # ui/not_used/qg_dlgoptionsvariables.cpp \
    # ui/not_used/qg_linepolygon2options.cpp \
    ui/qg_actionhandler.cpp \
    ui/view/lc_centralwidget.cpp \
    ui/view/qg_graphicview.cpp

FORMS = ui/action_options/circle/lc_circlebyarcoptions.ui \
       ui/action_options/circle/qg_circleoptions.ui \
       ui/action_options/circle/qg_circletan2options.ui \
       ui/action_options/curve/lc_ellipsearcoptions.ui \
       ui/action_options/curve/qg_arcoptions.ui \
       ui/action_options/curve/qg_arctangentialoptions.ui \
       ui/action_options/curve/qg_splineoptions.ui \
       ui/action_options/dimensions/qg_dimoptions.ui \
       ui/action_options/edit/lc_pastetransformoptions.ui \
       ui/action_options/ellipse/lc_ellipse1pointoptions.ui \
       ui/action_options/image/qg_imageoptions.ui \
       ui/action_options/info/lc_infodist2options.ui \
       ui/action_options/insert/qg_insertoptions.ui \
       ui/action_options/insert/qg_libraryinsertoptions.ui \
       ui/action_options/line/lc_crossoptions.ui \
       ui/action_options/line/lc_lineanglereloptions.ui \
       ui/action_options/line/lc_linefrompointtolineoptions.ui \
       ui/action_options/line/lc_lineoptions.ui \
       ui/action_options/line/lc_linepointsoptions.ui \
       ui/action_options/line/lc_rectangle1pointoptions.ui \
       ui/action_options/line/lc_rectangle2pointsoptions.ui \
       ui/action_options/line/lc_rectangle3pointsoptions.ui \
       ui/action_options/line/lc_slicedivideoptions.ui \
       ui/action_options/line/lc_staroptions.ui \
       ui/action_options/line/qg_lineangleoptions.ui \
       ui/action_options/line/qg_linebisectoroptions.ui \
       ui/action_options/line/qg_lineoptions.ui \
       ui/action_options/line/qg_lineparalleloptions.ui \
       ui/action_options/line/qg_lineparallelthroughoptions.ui \
       ui/action_options/line/qg_linepolygonoptions.ui \
       ui/action_options/line/qg_linerelangleoptions.ui \
       ui/action_options/modify/lc_duplicateoptions.ui \
       ui/action_options/modify/lc_linejoinoptions.ui \
       ui/action_options/modify/lc_modifybreakdivideoptions.ui \
       ui/action_options/modify/lc_modifygapoptions.ui \
       ui/action_options/modify/lc_modifymirroroptions.ui \
       ui/action_options/modify/lc_modifyrotateoptions.ui \
       ui/action_options/modify/lc_modifyscaleoptions.ui \
       ui/action_options/modify/lc_modifystretchoptions.ui \
       ui/action_options/modify/lc_moveoptions.ui \
       ui/action_options/modify/lc_rotate2options.ui \
       ui/action_options/modify/qg_beveloptions.ui \
       ui/action_options/modify/qg_modifyoffsetoptions.ui \
       ui/action_options/modify/qg_moverotateoptions.ui \
       ui/action_options/modify/qg_roundoptions.ui \
       ui/action_options/modify/qg_trimamountoptions.ui \
       ui/action_options/polyline/qg_polylineequidistantoptions.ui \
       ui/action_options/polyline/qg_polylineoptions.ui \
       ui/action_options/print_preview/qg_printpreviewoptions.ui \
       ui/action_options/snap/qg_snapdistoptions.ui \
       ui/action_options/snap/qg_snapmiddleoptions.ui \
       ui/action_options/text/qg_mtextoptions.ui \
       ui/action_options/text/qg_textoptions.ui \
       ui/components/comboboxes/comboboxoption.ui \
       ui/components/containers/lc_optionswidgetsholder.ui \
       ui/components/containers/lc_snapoptionswidgetsholder.ui \
       ui/components/creators/widgetcreator.ui \
       ui/components/pen/qg_widgetpen.ui \
       ui/components/status_bar/qg_activelayername.ui \
       ui/components/status_bar/qg_coordinatewidget.ui \
       ui/components/status_bar/qg_mousewidget.ui \
       ui/components/status_bar/qg_selectionwidget.ui \
       ui/components/textfileviewer.ui \
       ui/components/toolbars/qg_snaptoolbar.ui \
       ui/dialogs/actions/modify/qg_dlgmirror.ui \
       ui/dialogs/actions/modify/qg_dlgmove.ui \
       ui/dialogs/actions/modify/qg_dlgmoverotate.ui \
       ui/dialogs/actions/modify/qg_dlgrotate.ui \
       ui/dialogs/actions/modify/qg_dlgrotate2.ui \
       ui/dialogs/actions/modify/qg_dlgscale.ui \
       ui/dialogs/actions/qg_layerdialog.ui \
       ui/dialogs/entity/LC_DlgParabola.ui \
       ui/dialogs/entity/lc_dlgsplinepoints.ui \
       ui/dialogs/entity/qg_blockdialog.ui \
       ui/dialogs/entity/qg_dimensionlabeleditor.ui \
       ui/dialogs/entity/qg_dlgarc.ui \
       ui/dialogs/entity/qg_dlgattributes.ui \
       ui/dialogs/entity/qg_dlgcircle.ui \
       ui/dialogs/entity/qg_dlgdimension.ui \
       ui/dialogs/entity/qg_dlgdimlinear.ui \
       ui/dialogs/entity/qg_dlgellipse.ui \
       ui/dialogs/entity/qg_dlghatch.ui \
       ui/dialogs/entity/qg_dlgimage.ui \
       ui/dialogs/entity/qg_dlgimageoptions.ui \
       ui/dialogs/entity/qg_dlginsert.ui \
       ui/dialogs/entity/qg_dlgline.ui \
       ui/dialogs/entity/qg_dlgmtext.ui \
       ui/dialogs/entity/qg_dlgpoint.ui \
       ui/dialogs/entity/qg_dlgpolyline.ui \
       ui/dialogs/entity/qg_dlgspline.ui \
       ui/dialogs/entity/qg_dlgtext.ui \
       ui/dialogs/file/export/qg_dlgoptionsmakercam.ui \
       ui/dialogs/main/qg_dlginitial.ui \
       ui/dialogs/main/qg_exitdialog.ui \
       ui/dialogs/settings/options_device/lc_deviceoptions.ui \
       ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.ui \
       ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.ui \
       ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.ui \
       ui/dialogs/settings/shortcuts/lc_actionsshortcutsdialog.ui \
       ui/dock_widgets/command_line/qg_commandwidget.ui \
       ui/dock_widgets/entity_info/lc_quickinfowidget.ui \
       ui/dock_widgets/entity_info/lc_quickinfowidgetoptionsdialog.ui \
       ui/dock_widgets/layers_tree/lc_layerdialog_ex.ui \
       ui/dock_widgets/layers_tree/lc_layertreeoptionsdialog.ui \
       ui/dock_widgets/pen_palette/lc_penpaletteoptionsdialog.ui \
       ui/dock_widgets/pen_palette/lc_penpalettewidget.ui \
       ui/dock_widgets/pen_wizard/colorwizard.ui \
       ui/not_used/customtoolbarcreator.ui \
       ui/not_used/customwidgetcreator.ui \
       ui/not_used/qg_dimlinearoptions.ui \
       ui/not_used/qg_dlgoptionsvariables.ui \
       ui/not_used/qg_linepolygon2options.ui


# ################################################################################
# Main
HEADERS += \
    main/qc_dialogfactory.h \    
    main/doc_plugin_interface.h \
    plugins/document_interface.h \
    plugins/qc_plugininterface.h \
    plugins/intern/qc_actiongetpoint.h \
    plugins/intern/qc_actiongetselect.h \
    plugins/intern/qc_actiongetent.h \
    main/main.h \    
    main/console_dxf2pdf/console_dxf2pdf.h \
    main/console_dxf2pdf/pdf_print_loop.h

SOURCES += \
    main/qc_dialogfactory.cpp \
    main/doc_plugin_interface.cpp \
    plugins/intern/qc_actiongetpoint.cpp \
    plugins/intern/qc_actiongetselect.cpp \
    plugins/intern/qc_actiongetent.cpp \
    main/main.cpp \    
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
TRANSLATIONS = \
    ../ts/librecad_ar.ts \
    ../ts/librecad_ca.ts \
    ../ts/librecad_cs.ts \
    ../ts/librecad_da.ts \
    ../ts/librecad_de.ts \
    ../ts/librecad_el.ts \
    ../ts/librecad_en_au.ts \
    ../ts/librecad_en.ts \
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
    ../ts/librecad_es.ts \
    ../ts/librecad_es_us.ts \
    ../ts/librecad_es_uy.ts \
    ../ts/librecad_es_ve.ts \
    ../ts/librecad_et.ts \
    ../ts/librecad_eu.ts \
    ../ts/librecad_fi.ts \
    ../ts/librecad_fr.ts \
    ../ts/librecad_gl.ts \
    ../ts/librecad_he.ts \
    ../ts/librecad_hi.ts \
    ../ts/librecad_hu.ts \
    ../ts/librecad_id_ID.ts \
    ../ts/librecad_it.ts \
    ../ts/librecad_ja.ts \
    ../ts/librecad_ka.ts \
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
    ../ts/librecad_sr.ts \
    ../ts/librecad_sv.ts \
    ../ts/librecad_ta.ts \
    ../ts/librecad_th.ts \
    ../ts/librecad_tr.ts \
    ../ts/librecad_uk.ts \
    ../ts/librecad_zh_cn.ts \
    ../ts/librecad_zh_tw.ts
