# LibreCAD project file
# (c) Ries van Twisk (librecad@rvt.dds.nl)
TEMPLATE = app

DISABLE_POSTSCRIPT = false

#uncomment to enable a Debugging menu entry for basic unit testing
#DEFINES += LC_DEBUGGING

DEFINES += DWGSUPPORT
DEFINES -= JWW_WRITE_SUPPORT

LC_VERSION="2.2.2.6-alpha"
LC_PRERELEASE = "true";

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

QT += widgets printsupport network
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
        ICON = ../res/images/librecad.icns
        contains(DISABLE_POSTSCRIPT, false) {
            QMAKE_POST_LINK = /bin/sh $$_PRO_FILE_PWD_/../../scripts/postprocess-osx.sh $$OUT_PWD/$${DESTDIR}/$${TARGET}.app/ $$[QT_INSTALL_BINS];
            QMAKE_POST_LINK += /usr/libexec/PlistBuddy -c \"Set :CFBundleGetInfoString string $${TARGET} $${LC_VERSION}\" $$OUT_PWD/$${DESTDIR}/$${TARGET}.app/Contents/Info.plist;
        }
    }
    else {
        TARGET = librecad
        DEFINES += QC_APPDIR="\"librecad\""
        RC_FILE = ../res/images/librecad.icns
        contains(DISABLE_POSTSCRIPT, false) {
            QMAKE_POST_LINK = cd $$_PRO_FILE_PWD_/../.. && scripts/postprocess-unix.sh
        }
        DEFINES -=  QT_NO_SHORTCUT
    }
}
win32 {
    TARGET = LibreCAD

    CONFIG += console
    DEFINES += QC_APPDIR="\"LibreCAD\""

    # add MSYSGIT_DIR = PathToGitBinFolder (without quotes) in custom.pro file, for commit hash in about dialog
    !isEmpty( MSYSGIT_DIR ) {
        LC_VERSION = $$system( \"$$MSYSGIT_DIR/git.exe\" describe || echo "$${LC_VERSION}")
    }

    RC_FILE = ../res/images/librecad.rc
    RC_ICONS = ../res/images/librecad.ico
    contains(DISABLE_POSTSCRIPT, false) {
        QMAKE_POST_LINK = "$$_PRO_FILE_PWD_/../../scripts/postprocess-win.bat" $$LC_VERSION
    }
}

DEFINES += LC_VERSION=\"$$LC_VERSION\"
DEFINES += LC_PRERELEASE=\"$$LC_PRERELEASE\"

# Additional libraries to load
LIBS += -L../../generated/lib  \
    -ldxfrw \
    -ljwwlib

INCLUDEPATH += \
    ../../libraries/lciconengine \
    ../../libraries/libdxfrw/src \
    ../../libraries/jwwlib/src \
    cmd \
    lib/actions \
    lib/creation \
    lib/debug \
    lib/engine \
    lib/engine/document \
    lib/engine/document/blocks \
    lib/engine/document/container \
    lib/engine/document/dimstyles \
    lib/engine/document/entities \
    lib/engine/document/entities/support \
    lib/engine/document/fonts \
    lib/engine/document/io \
    lib/engine/document/layers \
    lib/engine/document/patterns \
    lib/engine/document/textstyles \
    lib/engine/document/ucs \
    lib/engine/document/variables \
    lib/engine/document/views \
    lib/engine/clipboard \
    lib/engine/overlays \
    lib/engine/overlays/angles_base \
    lib/engine/overlays/highlight \
    lib/engine/overlays/preview \
    lib/engine/overlays/references \
    lib/engine/overlays/crosshair \
    lib/engine/overlays/info_cursor \
    lib/engine/overlays/overlay_box \
    lib/engine/overlays/ucs_mark \
    lib/engine/undo \
    lib/engine/utils \
    lib/engine/settings \
    lib/fileio \
    lib/filters \
    lib/generators \
    lib/generators/makercamsvg \
    lib/generators/layers \
    lib/generators/image \
    lib/gui \
    lib/gui/grid \
    lib/gui/render \
    lib/gui/render/headless \
    lib/gui/render/widget \
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
    actions/dock_widgets/ucs_list \
    actions/drawing \
    actions/drawing/draw \
    actions/drawing/draw/circle \
    actions/drawing/draw/curve \
    actions/drawing/draw/spline \
    actions/drawing/draw/dimensions \
    actions/drawing/draw/ellipse \
    actions/drawing/draw/hatch \
    actions/drawing/draw/image \
    actions/drawing/draw/line \
    actions/drawing/draw/point \
    actions/drawing/draw/rect \
    actions/drawing/draw/polygon \
    actions/drawing/draw/misc \
    actions/drawing/draw/line/shapes \
    actions/drawing/draw/line/misc \
    actions/drawing/draw/line/shapes/rect \
    actions/drawing/draw/line/shapes/polygon \
    actions/drawing/draw/polyline \
    actions/drawing/draw/text \
    actions/drawing/edit \
    actions/drawing/info \
    actions/drawing/pick \
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
    ui/action_options/spline \
    ui/action_options/dimensions \
    ui/action_options/edit \
    ui/action_options/image \
    ui/action_options/info \
    ui/action_options/insert \
    ui/action_options/line \
    ui/action_options/rect \
    ui/action_options/polygon \
    ui/action_options/misc \
    ui/action_options/modify \
    ui/action_options/ellipse \
    ui/action_options/other \
    ui/action_options/polyline \
    ui/action_options/point \
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
    ui/dialogs/creators \
    ui/dialogs/file \
    ui/dialogs/file/export \
    ui/dialogs/file/export/layers \
    ui/dialogs/file/export/image \
    ui/dialogs/file/export/makercam \        
    ui/dialogs/main \
    ui/dialogs/settings \    
    ui/dialogs/settings/dimstyles \
    ui/dialogs/settings/dimstyles/dimstyle_manager \
    ui/dialogs/settings/dimstyles/dimstyle_manager/support \
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
    ui/dock_widgets/views_list \
    ui/dock_widgets/ucs_list \
    ui/dock_widgets/workspaces \
    ui/dock_widgets/cad \
    ui/main \
    ui/main/init \
    ui/main/persistence \
    ui/main/release_check \
    ui/main/support \
    ui/main/fontviewer \
    ui/main/workspaces \
    ui/view \
    # ui/not_used \
    # actions/not_used \
    main \
    main/console_dxf2pdf \
    test \
    plugins \
    ../res  \
    ../res/arrows \
    ../res/controls \
    ../res/dxf \
    ../res/gdt \
    ../res/icons \
    ../res/images \


RESOURCES += ../res/arrows/arrows.qrc
RESOURCES += ../res/controls/controls.qrc
RESOURCES += ../res/dxf/dxf.qrc
RESOURCES += ../res/gdt/gdt.qrc
RESOURCES += ../res/icons/icons.qrc
RESOURCES += ../res/images/images.qrc
RESOURCES += ../../licenses/licenses.qrc


# ################################################################################
# Library
HEADERS += \
    actions/dock_widgets/layer/lc_actionentitylayerbase.h \
    actions/dock_widgets/layer/lc_actionentitylayertoggle.h \
    actions/dock_widgets/layer/lc_actionlayerscmd.h \
    actions/dock_widgets/ucs_list/lc_actionucsbydimordinate.h \
    actions/dock_widgets/ucs_list/lc_actionucscreate.h \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsangle.h \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsbase.h \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsheight.h \
    actions/drawing/draw/curve/lc_actiondrawarc2pointslength.h \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsradius.h \
    actions/drawing/draw/curve/lc_actiondrawhyperbolafp.h \
    actions/drawing/draw/dimensions/lc_actiondimordinate.h \
    actions/drawing/draw/dimensions/lc_actiondimordinaterebase.h \
    actions/drawing/draw/dimensions/lc_actiondimstyleapply.h \
    actions/drawing/draw/dimensions/lc_actionselectdimordinatesameorigin.h \
    actions/drawing/draw/dimensions/lc_actiondrawgdtfeaturecontrolframe.h \
    actions/drawing/draw/line/misc/lc_actiondrawmidline.h \
    actions/drawing/draw/line/misc/lc_actiondrawboundingbox.h \
    actions/drawing/draw/polygon/lc_actiondrawlinepolygon4.h \
    actions/drawing/draw/spline/lc_actionremovesplinepoints.h \
    actions/drawing/draw/spline/lc_actionsplineaddpoint.h \
    actions/drawing/draw/spline/lc_actionsplineappendpoint.h \
    actions/drawing/draw/spline/lc_actionsplineexplode.h \
    actions/drawing/draw/spline/lc_actionsplinefrompolyline.h \
    actions/drawing/draw/spline/lc_actionsplinemodifybase.h \
    actions/drawing/draw/spline/lc_actionsplineremovebetween.h \
    actions/drawing/draw/dimensions/lc_actioncircledimbase.h \
    actions/drawing/draw/dimensions/lc_actiondrawdimbaseline.h \
    actions/drawing/draw/ellipse/lc_actiondrawellipse1point.h \
    actions/drawing/draw/point/lc_actiondrawpointslattice.h \
    actions/drawing/draw/point/lc_actionpastetopoints.h \
    actions/drawing/draw/point/lc_actionselectpoints.h \
    actions/drawing/draw/polyline/lc_actionpolylinearcstolines.h \
    actions/drawing/draw/polyline/lc_actionpolylinechangesegmenttype.h \
    actions/drawing/info/lc_actioninfopoint.h \
    actions/drawing/modify/lc_actionmodifyalign.h \
    actions/drawing/modify/lc_actionmodifyalignref.h \
    actions/drawing/modify/lc_actionmodifyalignsingle.h \
    actions/drawing/modify/lc_actionmodifymoveadjust.h \
    actions/drawing/pick/lc_actioninteractivepickangle.h \
    actions/drawing/pick/lc_actioninteractivepickbase.h \
    actions/drawing/pick/lc_actioninteractivepickdistance.h \
    actions/drawing/pick/lc_actioninteractivepickposition.h \
    actions/drawing/selection/lc_actionsingleentityselectbase.h \
    lib/actions/lc_actioninfomessagebuilder.h \
    lib/actions/lc_overlayboxaction.h \
    lib/engine/document/dimstyles/lc_dimstyle.h \
    lib/engine/document/dimstyles/lc_dimstyleslist.h \
    lib/engine/document/dimstyles/lc_dimarrowregistry.h \
    lib/engine/document/dimstyles/lc_dimstyletovariablesmapper.h \
    lib/engine/document/entities/lc_extentitydata.h \
    lib/engine/document/container/lc_containertraverser.h \
    lib/engine/document/entities/lc_mleader.h \
    lib/engine/document/entities/lc_splinehelper.h \
    lib/engine/document/entities/lc_tolerance.h \
    lib/engine/document/entities/support/lc_arrow_box.h \
    lib/engine/document/entities/support/lc_arrow_circle.h \
    lib/engine/document/entities/support/lc_arrow_datum.h \
    lib/engine/document/entities/support/lc_arrow_dot.h \
    lib/engine/document/entities/support/lc_arrow_headclosed.h \
    lib/engine/document/entities/support/lc_arrow_headclosed2.h \
    lib/engine/document/entities/support/lc_arrow_headclosed_blank.h \
    lib/engine/document/entities/support/lc_arrow_headopen.h \
    lib/engine/document/entities/support/lc_arrow_integral.h \
    lib/engine/document/entities/support/lc_arrow_none.h \
    lib/engine/document/entities/support/lc_arrow_tick.h \
    lib/engine/document/entities/support/lc_dimarrowblock.h \
    lib/engine/document/entities/support/lc_dimarrowblockpoly.h \
    lib/engine/document/lc_graphicvariables.h \
    lib/engine/document/textstyles/lc_textstyle.h \
    lib/engine/document/textstyles/lc_textstylelist.h \
    lib/engine/document/ucs/lc_ucslist.h \
    lib/engine/overlays/angles_base/lc_overlayanglesbasemark.h \
    lib/engine/overlays/highlight/lc_highlight.h \
    lib/actions/lc_modifiersinfo.h \
    lib/actions/rs_actioninterface.h \
    lib/engine/overlays/info_cursor/lc_cursoroverlayinfo.h \
    lib/engine/overlays/lc_overlayentitiescontainer.h \
    lib/engine/overlays/lc_overlayentity.h \
    lib/engine/overlays/lc_overlaysmanager.h \
    lib/engine/overlays/preview/rs_preview.h \
    lib/actions/rs_previewactioninterface.h \
    lib/actions/rs_snapper.h \
    lib/creation/rs_creation.h \
    lib/debug/rs_debug.h \
    lib/engine/document/ucs/lc_ucs.h \
    lib/engine/document/views/lc_view.h \
    lib/engine/document/views/lc_viewslist.h \
    lib/engine/document/entities/lc_cachedlengthentity.h \
    lib/engine/overlays/crosshair/lc_crosshair.h \
    lib/engine/document/container/lc_looputils.h \
    lib/engine/document/entities/lc_parabola.h \
    lib/engine/overlays/references/lc_refarc.h \
    lib/engine/overlays/references/lc_refcircle.h \
    lib/engine/overlays/references/lc_refconstructionline.h \
    lib/engine/overlays/references/lc_refellipse.h \
    lib/engine/overlays/references/lc_refline.h \
    lib/engine/overlays/references/lc_refpoint.h \
    lib/engine/overlays/ucs_mark/lc_overlayrelativezero.h \
    lib/engine/overlays/ucs_mark/lc_overlayucszero.h \
    lib/engine/overlays/ucs_mark/lc_ucs_mark.h \
    lib/engine/rs.h \
    lib/engine/document/entities/rs_arc.h \
    lib/engine/document/entities/rs_atomicentity.h \
    lib/engine/document/blocks/rs_block.h \
    lib/engine/document/blocks/rs_blocklist.h \
    lib/engine/document/blocks/rs_blocklistlistener.h \
    lib/engine/clipboard/rs_clipboard.h \
    lib/engine/document/entities/rs_circle.h \
    lib/engine/rs_color.h \
    lib/engine/document/entities/rs_constructionline.h \
    lib/engine/document/entities/rs_dimaligned.h \
    lib/engine/document/entities/rs_dimangular.h \
    lib/engine/document/entities/rs_dimdiametric.h \
    lib/engine/document/entities/rs_dimension.h \
    lib/engine/document/entities/rs_dimlinear.h \
    lib/engine/document/entities/lc_dimordinate.h \
    lib/engine/document/entities/rs_dimradial.h \
    lib/engine/document/entities/lc_dimarc.h \
    lib/engine/document/rs_document.h \
    lib/engine/document/entities/rs_ellipse.h \
    lib/engine/document/entities/rs_entity.h \
    lib/engine/document/container/rs_entitycontainer.h \
    lib/engine/rs_flags.h \
    lib/engine/document/fonts/rs_font.h \
    lib/engine/document/fonts/rs_fontchar.h \
    lib/engine/document/fonts/rs_fontlist.h \
    lib/engine/document/rs_graphic.h \
    lib/engine/document/entities/rs_hatch.h \
    lib/engine/document/entities/lc_hyperbola.h \
    lib/engine/document/entities/rs_insert.h \
    lib/engine/document/entities/rs_image.h \
    lib/engine/document/layers/rs_layer.h \
    lib/engine/document/layers/rs_layerlist.h \
    lib/engine/document/layers/rs_layerlistlistener.h \
    lib/engine/document/entities/rs_leader.h \
    lib/engine/document/entities/rs_line.h \
    lib/engine/document/entities/rs_mtext.h \
    lib/engine/overlays/rs_overlayline.h \
    lib/engine/overlays/overlay_box/rs_overlaybox.h \
    lib/engine/document/patterns/rs_pattern.h \
    lib/engine/document/patterns/rs_patternlist.h \
    lib/engine/rs_pen.h \
    lib/engine/document/entities/rs_point.h \
    lib/engine/document/entities/rs_polyline.h \
    lib/engine/settings/lc_settingsexporter.h \
    lib/engine/settings/rs_settings.h \
    lib/engine/document/entities/rs_solid.h \
    lib/engine/document/entities/rs_spline.h \
    lib/engine/document/entities/lc_splinepoints.h \
    lib/engine/rs_system.h \
    lib/engine/document/entities/rs_text.h \
    lib/engine/undo/lc_undoablerelzero.h \
    lib/engine/undo/rs_undo.h \
    lib/engine/undo/rs_undoable.h \
    lib/engine/undo/rs_undocycle.h \
    lib/engine/rs_units.h \
    lib/engine/lc_drawable.h \
    lib/engine/utils/lc_rectregion.h \
    lib/engine/utils/rs_utility.h \
    lib/engine/document/variables/rs_variable.h \
    lib/engine/document/variables/rs_variabledict.h \
    lib/engine/rs_vector.h \
    lib/fileio/rs_fileio.h \
    lib/fileio/lc_filenameselectionservice.h \
    lib/filters/lc_hyperbolaspline.h \
    lib/filters/rs_filtercxf.h \
    lib/filters/rs_filterdxfrw.h \
    lib/filters/rs_filterdxf1.h \
    lib/filters/rs_filterjww.h \
    lib/filters/rs_filterlff.h \
    lib/filters/rs_filterinterface.h \
    lib/generators/layers/lc_layersexporter.h \
    lib/generators/image/lc_imageexporter.h \
    lib/gui/lc_coordinates_parser.h \
    lib/gui/lc_eventhandler.h \
    lib/gui/lc_graphicviewport.h \
    lib/gui/lc_graphicviewportlistener.h \
    lib/gui/lc_latecompletionrequestor.h \
    lib/gui/render/headless/lc_printviewportrenderer.h \
    lib/gui/render/lc_graphicviewportrenderer.h \
    lib/modification/lc_division.h \
    plugins/lc_plugininvoker.h \
    lib/actions/lc_actioncontext.h \
    ui/components/creators/lc_creatorinvoker.h \
    # ui/components/toolbars/lc_snapoptionsholdermanager.h \
    ui/dialogs/creators/lc_dlgmenuassigner.h \
    ui/dialogs/creators/lc_dlgwidgetcreator.h \
    ui/components/creators/lc_menuactivator.h \
    ui/dialogs/creators/lc_dlgnewwidget.h \
    ui/dialogs/entity/lc_arcpropertieseditingwidget.h \
    ui/dialogs/entity/lc_circlepropertieseditingwidget.h \
    ui/dialogs/entity/lc_dlgdimension.h \
    ui/dialogs/entity/lc_dlgentityproperties.h \
    ui/dialogs/entity/lc_ellipsepropertieseditingwidget.h \
    ui/dialogs/entity/lc_entitypropertieseditor.h \
    ui/dialogs/entity/lc_entitypropertieseditorsupport.h \
    ui/dialogs/entity/lc_entitypropertieseditorwidget.h \
    ui/dialogs/entity/lc_hyperbolapropertieseditingwidget.h \
    ui/dialogs/entity/lc_imagepropertieseditingwidget.h \
    ui/dialogs/entity/lc_insertpropertieseditingwidget.h \
    ui/dialogs/entity/lc_linepropertieseditingwidget.h \
    ui/dialogs/entity/lc_parabolapropertieseditingwidget.h \
    ui/dialogs/entity/lc_pointpickbutton.h \
    ui/dialogs/entity/lc_pointpropertieseditingwidget.h \
    ui/dialogs/entity/lc_polylinepropertieseditingwidget.h \
    ui/dialogs/entity/lc_splinepointspropertieseditingwidget.h \
    ui/dialogs/entity/lc_splinepropertieseditingwidget.h \
    ui/dialogs/file/export/image/lc_exporttoimageservice.h \
    ui/dialogs/file/export/layers/lc_exportlayersdialogservice.h \
    ui/dialogs/lc_inputtextdialog.h \
    ui/dialogs/settings/dimstyles/lc_dimstylestreemodel.h \
    ui/dialogs/settings/options_drawing/lc_dlgnewcustomvariable.h \
    ui/dialogs/settings/options_drawing/lc_dlgnewdimstyle.h \
    ui/dialogs/settings/options_drawing/lc_dimstylesexporter.h \
    ui/dialogs/settings/options_widget/lc_dlgiconssetup.h \
    ui/dialogs/settings/dimstyles/lc_dimstyleitem.h \
    ui/dialogs/settings/dimstyles/lc_dimstyleslistmodel.h \
    ui/dialogs/settings/dimstyles/lc_dimstylestreemodel.h \
    ui/dialogs/settings/dimstyles/dimstyle_manager/lc_dlgdimstylemanager.h \
    ui/dialogs/settings/dimstyles/dimstyle_manager/support/lc_dimstylepreviewgraphicview.h \
    ui/dialogs/settings/dimstyles/dimstyle_manager/support/lc_dimstylepreviewpanel.h \
    ui/dialogs/settings/dimstyles/dimstyle_manager/support/lc_tabproxywidget.h \
    ui/dialogs/file/export/layers/lc_layerexportoptions.h \
    ui/dock_widgets/lc_dockwidget.h \
    ui/dock_widgets/lc_graphicviewawarewidget.h \
    ui/dock_widgets/lc_widgets_common.h \
    #ui/dock_widgets/library_widget/lc_librarywidget.h \
    ui/lc_actionhandlerfactory.h \
    ui/lc_graphicviewaware.h \
    ui/lc_snapmanager.h \
    ui/lc_uiutils.h \
    ui/main/fontviewer/lc_fontfileviewer.h \
    ui/main/init/lc_applicationwindowinitializer.h \
    ui/main/support/lc_appwindowdialogsinvoker.h \
    ui/main/lc_appwindowaware.h \
    ui/main/lc_defaultactioncontext.h \
    ui/main/persistence/lc_documentsstorage.h \
    lib/gui/render/widget/lc_graphicviewrenderer.cpp \
    lib/gui/render/widget/lc_printpreviewviewrenderer.cpp \
    lib/gui/render/widget/lc_widgetviewportrenderer.cpp \
    lib/modification/lc_align.h \
    ui/action_options/curve/lc_actiondrawarc2poptions.h \
    ui/action_options/misc/lc_midlineoptions.h \
    ui/action_options/misc/lc_drawboundingboxoptions.h \
    ui/action_options/modify/lc_modifyalignoptions.h \
    ui/action_options/modify/lc_modifyalignrefoptions.h \
    ui/action_options/other/lc_ucssetoptions.h \
    ui/action_options/spline/lc_splineexplodeoptions.h \
    ui/action_options/spline/lc_splinefrompolylineoptions.h \
    ui/action_options/point/lc_pastetopointsoptions.h \
    ui/action_options/point/lc_pointslatticeoptions.h \
    ui/action_options/selection/lc_selectwindowoptions.h \
    ui/components/status_bar/lc_anglesbasiswidget.h \
    ui/components/status_bar/lc_qtstatusbarmanager.h \
    ui/components/status_bar/lc_ucsstatewidget.h \
    ui/dialogs/entity/lc_entitypropertiesdlg.h \
    ui/dialogs/main/lc_dlgabout.h \
    ui/dialogs/main/lc_dlgnewversionavailable.h \
    ui/dialogs/settings/options_widget/lc_iconcolorsoptions.h \
    ui/dock_widgets/ucs_list/lc_dlgucslistoptions.h \
    ui/dock_widgets/ucs_list/lc_dlgucsproperties.h \
    ui/dock_widgets/ucs_list/lc_ucslistbutton.h \
    ui/dock_widgets/ucs_list/lc_ucslistmodel.h \
    ui/dock_widgets/ucs_list/lc_ucslistoptions.h \
    ui/dock_widgets/ucs_list/lc_ucslistwidget.h \
    ui/dock_widgets/views_list/lc_dlgnamedviewslistoptions.h \
    ui/dock_widgets/views_list/lc_namedviewsbutton.h \
    ui/dock_widgets/views_list/lc_namedviewslistoptions.h \
    ui/dock_widgets/views_list/lc_namedviewslistwidget.h \
    ui/dock_widgets/views_list/lc_namedviewsmodel.h \
    ui/dock_widgets/workspaces/lc_workspacelistbutton.h \
    ui/dock_widgets/cad/lc_caddockwidget.h \
    ui/main/support/lc_lastopenfilesopener.h \
    ui/main/workspaces/lc_workspacesmanager.h \
    ui/main/release_check/lc_releasechecker.h \
    lib/gui/grid/lc_gridsystem.h \
    lib/gui/grid/lc_isometricgrid.h \
    lib/gui/grid/lc_lattice.h \
    lib/gui/grid/lc_orthogonalgrid.h \
    lib/gui/rs_commandevent.h \
    lib/gui/rs_coordinateevent.h \
    lib/gui/rs_dialogfactory.h \
    lib/gui/rs_dialogfactoryinterface.h \
    #lib/gui/rs_eventhandler.h \
    lib/gui/rs_graphicview.h \
    lib/gui/grid/rs_grid.h \
    lib/gui/rs_linetypepattern.h \
    lib/gui/rs_mainwindowinterface.h \
    lib/gui/render/rs_painter.h \
    lib/gui/lc_coordinates_mapper.h \
    ui/main/support/lc_customstylehelper.h \
    ui/main/support/lc_gridviewinvoker.h \
    ui/main/support/lc_infocursorsettingsmanager.h \
    ui/main/workspaces/lc_workspacesinvoker.h \
    ui/view/lc_printpreviewview.h \
    lib/information/rs_locale.h \
    lib/information/rs_information.h \
    lib/information/rs_infoarea.h \
    lib/math/lc_convert.h \
    lib/math/lc_linemath.h \
    lib/modification/rs_modification.h \
    lib/modification/rs_selection.h \
    lib/math/rs_math.h \
    lib/math/lc_quadratic.h \
    main/console_dxf2png.h \
    test/lc_simpletests.h \
    lib/generators/makercamsvg/lc_makercamsvg.h \
    lib/generators/makercamsvg/lc_xmlwriterinterface.h \
    lib/generators/makercamsvg/lc_xmlwriterqxmlstreamwriter.h \
    lib/engine/document/entities/lc_rect.h \
    lib/engine/utils/lc_rtree.h \
    lib/engine/undo/lc_undosection.h \
    lib/printing/lc_printing.h \
    main/lc_application.h \
    ui/action_options/curve/lc_ellipsearcoptions.h \
    ui/action_options/ellipse/lc_ellipse1pointoptions.h \
    ui/components/status_bar/lc_relzerocoordinateswidget.h \
    ui/dialogs/lc_dialog.h \
    ui/main/lc_mdiapplicationwindow.h

SOURCES += \
    actions/dock_widgets/layer/lc_actionentitylayerbase.cpp \
    actions/dock_widgets/layer/lc_actionentitylayertoggle.cpp \
    actions/dock_widgets/layer/lc_actionlayerscmd.cpp \
    actions/dock_widgets/ucs_list/lc_actionucsbydimordinate.cpp \
    actions/dock_widgets/ucs_list/lc_actionucscreate.cpp \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsangle.cpp \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsbase.cpp \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsheight.cpp \
    actions/drawing/draw/curve/lc_actiondrawarc2pointslength.cpp \
    actions/drawing/draw/curve/lc_actiondrawarc2pointsradius.cpp \
    actions/drawing/draw/curve/lc_actiondrawhyperbolafp.cpp \
    actions/drawing/draw/dimensions/lc_actiondimstyleapply.cpp \
    actions/drawing/draw/dimensions/lc_actiondrawgdtfeaturecontrolframe.cpp \
    actions/drawing/draw/dimensions/lc_actiondimordinate.cpp \
    actions/drawing/draw/dimensions/lc_actiondimordinaterebase.cpp \
    actions/drawing/draw/dimensions/lc_actionselectdimordinatesameorigin.cpp \
    actions/drawing/draw/line/misc/lc_actiondrawmidline.cpp \
    actions/drawing/draw/line/misc/lc_actiondrawboundingbox.cpp \
    actions/drawing/draw/polygon/lc_actiondrawlinepolygon4.cpp \
    actions/drawing/draw/spline/lc_actionremovesplinepoints.cpp \
    actions/drawing/draw/spline/lc_actionsplineaddpoint.cpp \
    actions/drawing/draw/spline/lc_actionsplineexplode.cpp \
    actions/drawing/draw/spline/lc_actionsplinefrompolyline.cpp \
    actions/drawing/draw/spline/lc_actionsplineremovebetween.cpp \
    actions/drawing/draw/point/lc_actiondrawpointslattice.cpp \
    actions/drawing/draw/point/lc_actionpastetopoints.cpp \
    actions/drawing/draw/point/lc_actionselectpoints.cpp \
    actions/drawing/info/lc_actioninfopoint.cpp \
    actions/drawing/modify/lc_actionmodifyalign.cpp \
    actions/drawing/modify/lc_actionmodifyalignref.cpp \
    actions/drawing/modify/lc_actionmodifyalignsingle.cpp \
    actions/drawing/modify/lc_actionmodifymoveadjust.cpp \
    actions/drawing/pick/lc_actioninteractivepickangle.cpp \
    actions/drawing/pick/lc_actioninteractivepickbase.cpp \
    actions/drawing/pick/lc_actioninteractivepickdistance.cpp \
    actions/drawing/pick/lc_actioninteractivepickposition.cpp \
    actions/drawing/selection/lc_actionsingleentityselectbase.cpp \
    lib/actions/lc_actioninfomessagebuilder.cpp \
    lib/actions/lc_overlayboxaction.cpp \
    lib/engine/document/dimstyles/lc_dimstyle.cpp \
    lib/engine/document/dimstyles/lc_dimstyleslist.cpp \
    lib/engine/document/dimstyles/lc_dimarrowregistry.cpp \
    lib/engine/document/dimstyles/lc_dimstyletovariablesmapper.cpp \
    lib/engine/document/entities/lc_extentitydata.cpp \
    lib/engine/document/container/lc_containertraverser.cpp \
    lib/engine/document/entities/lc_mleader.cpp \
    lib/engine/document/entities/lc_splinehelper.cpp \
    lib/engine/document/entities/lc_tolerance.cpp \
    lib/engine/document/entities/support/lc_arrow_box.cpp \
    lib/engine/document/entities/support/lc_arrow_circle.cpp \
    lib/engine/document/entities/support/lc_arrow_datum.cpp \
    lib/engine/document/entities/support/lc_arrow_dot.cpp \
    lib/engine/document/entities/support/lc_arrow_headclosed.cpp \
    lib/engine/document/entities/support/lc_arrow_headclosed2.cpp \
    lib/engine/document/entities/support/lc_arrow_headclosed_blank.cpp \
    lib/engine/document/entities/support/lc_arrow_headopen.cpp \
    lib/engine/document/entities/support/lc_arrow_integral.cpp \
    lib/engine/document/entities/support/lc_arrow_none.cpp \
    lib/engine/document/entities/support/lc_arrow_tick.cpp \
    lib/engine/document/entities/support/lc_dimarrowblock.cpp \
    lib/engine/document/entities/support/lc_dimarrowblockpoly.cpp \
    lib/engine/document/lc_graphicvariables.cpp \
    lib/engine/document/textstyles/lc_textstyle.cpp \
    lib/engine/document/textstyles/lc_textstylelist.cpp \
    lib/engine/document/ucs/lc_ucslist.cpp \
    lib/engine/overlays/angles_base/lc_overlayanglesbasemark.cpp \
    lib/engine/overlays/info_cursor/lc_cursoroverlayinfo.cpp \
    lib/engine/overlays/lc_overlayentitiescontainer.cpp \
    lib/engine/overlays/lc_overlayentity.cpp \
    lib/engine/overlays/lc_overlaysmanager.cpp \
    lib/engine/overlays/references/lc_refconstructionline.cpp \
    lib/engine/overlays/ucs_mark/lc_overlayrelativezero.cpp \
    lib/engine/overlays/ucs_mark/lc_overlayucszero.cpp \
    lib/engine/overlays/ucs_mark/lc_ucs_mark.cpp \
    lib/engine/settings/lc_settingsexporter.cpp \
    lib/engine/undo/lc_undoablerelzero.cpp \
    lib/engine/utils/lc_rectregion.cpp \
    lib/filters/lc_hyperbolaspline.cpp \
    lib/generators/layers/lc_layersexporter.cpp \
    lib/generators/image/lc_imageexporter.cpp \
    lib/gui/lc_coordinates_parser.cpp \
    lib/gui/lc_eventhandler.cpp \
    lib/gui/lc_graphicviewport.cpp \
    lib/gui/lc_graphicviewportlistener.cpp \
    lib/gui/lc_latecompletionrequestor.cpp \
    lib/gui/render/headless/lc_printviewportrenderer.cpp \
    lib/modification/lc_division.cpp \
    plugins/lc_plugininvoker.cpp \
    lib/actions/lc_actioncontext.cpp \
    ui/components/creators/lc_creatorinvoker.cpp \
    #ui/components/toolbars/lc_snapoptionsholdermanager.cpp \
    ui/dialogs/creators/lc_dlgmenuassigner.cpp \
    ui/dialogs/creators/lc_dlgwidgetcreator.cpp \
    ui/components/creators/lc_menuactivator.cpp \
    ui/dialogs/creators/lc_dlgnewwidget.cpp \
    ui/dialogs/entity/lc_arcpropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_circlepropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_dlgdimension.cpp \
    ui/dialogs/entity/lc_dlgentityproperties.cpp \
    ui/dialogs/entity/lc_dlgtolerance.cpp \
    ui/dialogs/entity/lc_ellipsepropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_entitypropertieseditor.cpp \
    ui/dialogs/entity/lc_entitypropertieseditorsupport.cpp \
    ui/dialogs/entity/lc_entitypropertieseditorwidget.cpp \
    ui/dialogs/entity/lc_hyperbolapropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_imagepropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_insertpropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_linepropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_parabolapropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_pointpickbutton.cpp \
    ui/dialogs/entity/lc_pointpropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_polylinepropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_splinepointspropertieseditingwidget.cpp \
    ui/dialogs/entity/lc_splinepropertieseditingwidget.cpp \
    ui/dialogs/file/export/image/lc_exporttoimageservice.cpp \
    ui/dialogs/file/export/layers/lc_exportlayersdialogservice.cpp \
    ui/dialogs/lc_inputtextdialog.cpp \
    ui/dialogs/settings/dimstyles/lc_dimstyleitem.cpp \
    ui/dialogs/settings/dimstyles/lc_dimstyleslistmodel.cpp \
    ui/dialogs/settings/dimstyles/lc_dimstylestreemodel.cpp \
    ui/dialogs/settings/dimstyles/dimstyle_manager/lc_dlgdimstylemanager.cpp \
    ui/dialogs/settings/dimstyles/dimstyle_manager/support/lc_dimstylepreviewgraphicview.cpp \
    ui/dialogs/settings/dimstyles/dimstyle_manager/support/lc_dimstylepreviewpanel.cpp \
    ui/dialogs/settings/dimstyles/dimstyle_manager/support/lc_tabproxywidget.cpp \
    ui/dialogs/settings/options_drawing/lc_dimstylesexporter.cpp \
    ui/dialogs/settings/options_drawing/lc_dlgnewcustomvariable.cpp \
    ui/dialogs/settings/options_drawing/lc_dlgnewdimstyle.cpp \
    ui/dialogs/settings/options_widget/lc_dlgiconssetup.cpp \
    ui/dialogs/file/export/layers/lc_layerexportoptions.cpp \
    #ui/dock_widgets/library_widget/lc_librarywidget.cpp \
    ui/dock_widgets/cad/lc_caddockwidget.cpp \
    ui/dock_widgets/lc_dockwidget.cpp \
    ui/dock_widgets/lc_graphicviewawarewidget.cpp \
    ui/lc_actionhandlerfactory.cpp \
    ui/lc_snapmanager.cpp \
    ui/lc_uiutils.cpp \
    ui/main/fontviewer/lc_fontfileviewer.cpp \
    ui/main/init/lc_applicationwindowinitializer.cpp \
    ui/main/support/lc_appwindowdialogsinvoker.cpp \
    ui/main/lc_appwindowaware.cpp \
    ui/main/lc_defaultactioncontext.cpp \
    ui/main/persistence/lc_documentsstorage.cpp \
    lib/gui/render/lc_graphicviewportrenderer.cpp \
    lib/gui/render/widget/lc_graphicviewrenderer.cpp \
    lib/gui/render/widget/lc_printpreviewviewrenderer.cpp \
    lib/gui/render/widget/lc_widgetviewportrenderer.cpp \
    lib/modification/lc_align.cpp \
    ui/action_options/curve/lc_actiondrawarc2poptions.cpp \
    ui/action_options/misc/lc_midlineoptions.cpp \
    ui/action_options/misc/lc_drawboundingboxoptions.cpp \
    ui/action_options/modify/lc_modifyalignoptions.cpp \
    ui/action_options/modify/lc_modifyalignrefoptions.cpp \
    ui/action_options/other/lc_ucssetoptions.cpp \
    ui/action_options/spline/lc_splineexplodeoptions.cpp \
    ui/action_options/spline/lc_splinefrompolylineoptions.cpp \
    actions/drawing/draw/spline/lc_actionsplineappendpoint.cpp \
    actions/drawing/draw/spline/lc_actionsplinemodifybase.cpp \
    actions/drawing/draw/dimensions/lc_actioncircledimbase.cpp \
    actions/drawing/draw/dimensions/lc_actiondrawdimbaseline.cpp \
    actions/drawing/draw/ellipse/lc_actiondrawellipse1point.cpp \
    actions/drawing/draw/polyline/lc_actionpolylinearcstolines.cpp \
    actions/drawing/draw/polyline/lc_actionpolylinechangesegmenttype.cpp \
    lib/engine/overlays/highlight/lc_highlight.cpp \
    lib/actions/lc_modifiersinfo.cpp \
    lib/actions/rs_actioninterface.cpp \
    lib/engine/overlays/preview/rs_preview.cpp \
    lib/actions/rs_previewactioninterface.cpp \
    lib/actions/rs_snapper.cpp \
    lib/creation/rs_creation.cpp \
    lib/debug/rs_debug.cpp \
    lib/engine/document/ucs/lc_ucs.cpp \
    lib/engine/document/views/lc_view.cpp \
    lib/engine/document/views/lc_viewslist.cpp \
    lib/engine/document/entities/lc_cachedlengthentity.cpp \
    lib/engine/overlays/crosshair/lc_crosshair.cpp \
    lib/engine/document/container/lc_looputils.cpp \
    lib/engine/document/entities/lc_parabola.cpp \
    lib/engine/overlays/references/lc_refarc.cpp \
    lib/engine/overlays/references/lc_refcircle.cpp \
    lib/engine/overlays/references/lc_refellipse.cpp \
    lib/engine/overlays/references/lc_refline.cpp \
    lib/engine/overlays/references/lc_refpoint.cpp \
    lib/engine/document/entities/rs_arc.cpp \
    lib/engine/document/blocks/rs_block.cpp \
    lib/engine/document/blocks/rs_blocklist.cpp \
    lib/engine/clipboard/rs_clipboard.cpp \
    lib/engine/document/entities/rs_circle.cpp \
    lib/engine/document/entities/rs_constructionline.cpp \
    lib/engine/document/entities/rs_dimaligned.cpp \
    lib/engine/document/entities/rs_dimangular.cpp \
    lib/engine/document/entities/rs_dimdiametric.cpp \
    lib/engine/document/entities/rs_dimension.cpp \
    lib/engine/document/entities/rs_dimlinear.cpp \
    lib/engine/document/entities/lc_dimordinate.cpp \
    lib/engine/document/entities/rs_dimradial.cpp \
    lib/engine/document/entities/lc_dimarc.cpp \
    lib/engine/document/rs_document.cpp \
    lib/engine/document/entities/rs_ellipse.cpp \
    lib/engine/document/entities/rs_entity.cpp \
    lib/engine/document/container/rs_entitycontainer.cpp \
    lib/engine/document/fonts/rs_font.cpp \
    lib/engine/document/fonts/rs_fontlist.cpp \
    lib/engine/document/rs_graphic.cpp \
    lib/engine/document/entities/rs_hatch.cpp \
    lib/engine/document/entities/lc_hyperbola.cpp \
    lib/engine/document/entities/rs_insert.cpp \
    lib/engine/document/entities/rs_image.cpp \
    lib/engine/document/layers/rs_layer.cpp \
    lib/engine/document/layers/rs_layerlist.cpp \
    lib/engine/document/entities/rs_leader.cpp \
    lib/engine/document/entities/rs_line.cpp \
    lib/engine/document/entities/rs_mtext.cpp \
    lib/engine/overlays/rs_overlayline.cpp \
    lib/engine/overlays/overlay_box/rs_overlaybox.cpp \
    lib/engine/document/patterns/rs_pattern.cpp \
    lib/engine/document/patterns/rs_patternlist.cpp \
    lib/engine/document/entities/rs_point.cpp \
    lib/engine/document/entities/rs_polyline.cpp \
    lib/engine/settings/rs_settings.cpp \
    lib/engine/document/entities/rs_solid.cpp \
    lib/engine/document/entities/rs_spline.cpp \
    lib/engine/document/entities/lc_splinepoints.cpp \
    lib/engine/rs_system.cpp \
    lib/engine/document/entities/rs_text.cpp \
    lib/engine/undo/rs_undo.cpp \
    lib/engine/undo/rs_undoable.cpp \
    lib/engine/rs_units.cpp \
    lib/engine/utils/rs_utility.cpp \
    lib/engine/document/variables/rs_variabledict.cpp \
    lib/engine/rs_vector.cpp \
    lib/fileio/rs_fileio.cpp \
    lib/fileio/lc_filenameselectionservice.cpp \
    lib/filters/rs_filtercxf.cpp \
    lib/filters/rs_filterdxfrw.cpp \
    lib/filters/rs_filterdxf1.cpp \
    lib/filters/rs_filterjww.cpp \
    lib/filters/rs_filterlff.cpp \
    #lib/gui/no_used/rs_painterold.cpp \
   # lib/gui/no_used/rs_painterqtold.cpp \
    ui/action_options/point/lc_pastetopointsoptions.cpp \
    ui/action_options/point/lc_pointslatticeoptions.cpp \
    ui/action_options/selection/lc_selectwindowoptions.cpp \
    ui/components/status_bar/lc_anglesbasiswidget.cpp \
    ui/components/status_bar/lc_qtstatusbarmanager.cpp \
    ui/components/status_bar/lc_ucsstatewidget.cpp \
    ui/dialogs/entity/lc_entitypropertiesdlg.cpp \
    ui/dialogs/main/lc_dlgabout.cpp \
    ui/dialogs/main/lc_dlgnewversionavailable.cpp \
    ui/dialogs/settings/options_widget/lc_iconcolorsoptions.cpp \
    ui/dock_widgets/ucs_list/lc_dlgucslistoptions.cpp \
    ui/dock_widgets/ucs_list/lc_dlgucsproperties.cpp \
    ui/dock_widgets/ucs_list/lc_ucslistbutton.cpp \
    ui/dock_widgets/ucs_list/lc_ucslistmodel.cpp \
    ui/dock_widgets/ucs_list/lc_ucslistoptions.cpp \
    ui/dock_widgets/ucs_list/lc_ucslistwidget.cpp \
    ui/dock_widgets/views_list/lc_dlgnamedviewslistoptions.cpp \
    ui/dock_widgets/views_list/lc_namedviewsbutton.cpp \
    ui/dock_widgets/views_list/lc_namedviewslistoptions.cpp \
    ui/dock_widgets/views_list/lc_namedviewslistwidget.cpp \
    ui/dock_widgets/views_list/lc_namedviewsmodel.cpp \
    ui/dock_widgets/workspaces/lc_workspacelistbutton.cpp \
    ui/main/release_check/lc_releasechecker.cpp \
    ui/main/support/lc_lastopenfilesopener.cpp \
    ui/main/workspaces/lc_workspacesmanager.cpp\
    lib/gui/grid/lc_gridsystem.cpp \
    lib/gui/grid/lc_isometricgrid.cpp \
    lib/gui/grid/lc_lattice.cpp \
    lib/gui/grid/lc_orthogonalgrid.cpp \
    lib/gui/rs_dialogfactory.cpp \
    #lib/gui/rs_eventhandler.cpp \
    lib/gui/rs_graphicview.cpp \
    lib/gui/grid/rs_grid.cpp \
    lib/gui/rs_linetypepattern.cpp \
    lib/gui/render/rs_painter.cpp \
    lib/gui/lc_coordinates_mapper.cpp \
    ui/main/support/lc_customstylehelper.cpp \
    ui/main/support/lc_gridviewinvoker.cpp \
    ui/main/support/lc_infocursorsettingsmanager.cpp \
    ui/main/workspaces/lc_workspacesinvoker.cpp \
    ui/view/lc_printpreviewview.cpp \
    lib/information/rs_locale.cpp \
    lib/information/rs_information.cpp \
    lib/information/rs_infoarea.cpp \
    lib/math/lc_convert.cpp \
    lib/math/lc_linemath.cpp \
    lib/math/rs_math.cpp \
    lib/math/lc_quadratic.cpp \
    lib/modification/rs_modification.cpp \
    lib/modification/rs_selection.cpp \
    lib/engine/rs_color.cpp \
    lib/engine/rs_pen.cpp \
    main/console_dxf2png.cpp \
    test/lc_simpletests.cpp \
    lib/generators/makercamsvg/lc_xmlwriterqxmlstreamwriter.cpp \
    lib/generators/makercamsvg/lc_makercamsvg.cpp \
    lib/engine/document/entities/rs_atomicentity.cpp \
    lib/engine/undo/rs_undocycle.cpp \
    lib/engine/rs_flags.cpp \
    lib/engine/document/entities/lc_rect.cpp \
    lib/engine/utils/lc_rtree.cpp \
    lib/engine/undo/lc_undosection.cpp \
    lib/engine/rs.cpp \
    lib/printing/lc_printing.cpp \
    main/lc_application.cpp \
    ui/action_options/curve/lc_ellipsearcoptions.cpp \
    ui/action_options/ellipse/lc_ellipse1pointoptions.cpp \
    ui/components/status_bar/lc_relzerocoordinateswidget.cpp \
    ui/dialogs/lc_dialog.cpp \
    ui/main/lc_mdiapplicationwindow.cpp
    # ui/not_used/lc_dlgdimordinate.cpp \

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
    actions/drawing/draw/spline/lc_actiondrawsplinepoints.h \
    actions/drawing/draw/curve/rs_actiondrawarc.h \
    actions/drawing/draw/curve/rs_actiondrawarc3p.h \
    actions/drawing/draw/curve/rs_actiondrawarctangential.h \
    actions/drawing/draw/curve/rs_actiondrawlinefree.h \
    actions/drawing/draw/spline/rs_actiondrawspline.h \
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
    actions/drawing/draw/line/misc/lc_actiondrawcross.h \
    actions/drawing/draw/line/lc_actiondrawlineanglerel.h \
    actions/drawing/draw/line/lc_actiondrawlinefrompointtoline.h \
    actions/drawing/draw/point/lc_actiondrawlinepoints.h \
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
    actions/drawing/draw/point/rs_actiondrawpoint.h \
    actions/drawing/draw/polygon/lc_actiondrawlinepolygon3.h \
    actions/drawing/draw/polygon/lc_actiondrawlinepolygonbase.h \
    actions/drawing/draw/polygon/lc_actiondrawstar.h \
    actions/drawing/draw/polygon/rs_actiondrawlinepolygon.h \
    actions/drawing/draw/polygon/rs_actiondrawlinepolygon2.h \
    actions/drawing/draw/rect/lc_abstractactiondrawrectangle.h \
    actions/drawing/draw/rect/lc_actiondrawrectangle1point.h \
    actions/drawing/draw/rect/lc_actiondrawrectangle2points.h \
    actions/drawing/draw/rect/lc_actiondrawrectangle3points.h \
    actions/drawing/draw/rect/rs_actiondrawlinerectangle.h \
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
    lib/actions/lc_actionpreselectionawarebase.h \
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
    # actions/drawing/selection/rs_actionselect.h \
    actions/drawing/selection/rs_actionselectall.h \
    lib/actions/rs_actionselectbase.h \
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
    actions/drawing/draw/spline/lc_actiondrawsplinepoints.cpp \
    actions/drawing/draw/curve/rs_actiondrawarc.cpp \
    actions/drawing/draw/curve/rs_actiondrawarc3p.cpp \
    actions/drawing/draw/curve/rs_actiondrawarctangential.cpp \
    actions/drawing/draw/curve/rs_actiondrawlinefree.cpp \
    actions/drawing/draw/spline/rs_actiondrawspline.cpp \
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
    actions/drawing/draw/line/misc/lc_actiondrawcross.cpp \
    actions/drawing/draw/line/lc_actiondrawlineanglerel.cpp \
    actions/drawing/draw/line/lc_actiondrawlinefrompointtoline.cpp \
    actions/drawing/draw/point/lc_actiondrawlinepoints.cpp \
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
    actions/drawing/draw/point/rs_actiondrawpoint.cpp \
    actions/drawing/draw/polygon/lc_actiondrawlinepolygon3.cpp \
    actions/drawing/draw/polygon/lc_actiondrawlinepolygonbase.cpp \
    actions/drawing/draw/polygon/lc_actiondrawstar.cpp \
    actions/drawing/draw/polygon/rs_actiondrawlinepolygon.cpp \
    actions/drawing/draw/polygon/rs_actiondrawlinepolygon2.cpp \
    actions/drawing/draw/rect/lc_abstractactiondrawrectangle.cpp \
    actions/drawing/draw/rect/lc_actiondrawrectangle1point.cpp \
    actions/drawing/draw/rect/lc_actiondrawrectangle2points.cpp \
    actions/drawing/draw/rect/lc_actiondrawrectangle3points.cpp \
    actions/drawing/draw/rect/rs_actiondrawlinerectangle.cpp \
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
    lib/actions/lc_actionpreselectionawarebase.cpp \
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
    # actions/drawing/selection/rs_actionselect.cpp \
    actions/drawing/selection/rs_actionselectall.cpp \
    lib/actions/rs_actionselectbase.cpp \
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
    actions/options/rs_actionoptionsdrawing.cpp \
    actions/print_preview/rs_actionprintpreview.cpp


# ################################################################################
# UI
HEADERS += ui/action_options/lc_actionoptionsmanager.h \
    ui/action_options/circle/lc_circlebyarcoptions.h \
    ui/action_options/circle/qg_circleoptions.h \
    ui/action_options/circle/qg_circletan2options.h \
    ui/action_options/curve/qg_arcoptions.h \
    ui/action_options/curve/qg_arctangentialoptions.h \
    ui/action_options/spline/qg_splineoptions.h \
    ui/action_options/dimensions/qg_dimoptions.h \
    ui/action_options/edit/lc_pastetransformoptions.h \
    ui/action_options/image/qg_imageoptions.h \
    ui/action_options/info/lc_infodist2options.h \
    ui/action_options/insert/qg_insertoptions.h \
    ui/action_options/insert/qg_libraryinsertoptions.h \
    ui/action_options/lc_actionoptionswidget.h \
    ui/action_options/lc_actionoptionswidgetbase.h \
    ui/action_options/misc/lc_crossoptions.h \
    ui/action_options/line/lc_lineanglereloptions.h \
    ui/action_options/line/lc_linefrompointtolineoptions.h \
    ui/action_options/line/lc_lineoptions.h \
    ui/action_options/point/lc_linepointsoptions.h \
    ui/action_options/rect/lc_rectangle1pointoptions.h \
    ui/action_options/rect/lc_rectangle2pointsoptions.h \
    ui/action_options/rect/lc_rectangle3pointsoptions.h \
    ui/action_options/line/lc_slicedivideoptions.h \
    ui/action_options/polygon/lc_staroptions.h \
    ui/action_options/line/qg_lineangleoptions.h \
    ui/action_options/line/qg_linebisectoroptions.h \
    ui/action_options/line/qg_lineoptions.h \
    ui/action_options/line/qg_lineparalleloptions.h \
    ui/action_options/line/qg_lineparallelthroughoptions.h \
    ui/action_options/polygon/qg_linepolygonoptions.h \
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
    #ui/dialogs/entity/LC_DlgParabola.h \
    #ui/dialogs/entity/lc_dlgsplinepoints.h \
    ui/dialogs/entity/qg_blockdialog.h \
    ui/dialogs/entity/qg_dimensionlabeleditor.h \
    # ui/dialogs/entity/qg_dlgarc.h \
    ui/dialogs/entity/qg_dlgattributes.h \
    # ui/dialogs/entity/qg_dlgcircle.h \
    ui/dialogs/entity/lc_dlgtolerance.h \
    # ui/dialogs/entity/qg_dlgellipse.h \
    ui/dialogs/entity/qg_dlghatch.h \
    # ui/dialogs/entity/qg_dlgimage.h \
    ui/dialogs/file/export/image/qg_dlgimageoptions.h \
    # ui/dialogs/entity/qg_dlginsert.h \
    # ui/dialogs/entity/qg_dlgline.h \
    ui/dialogs/entity/qg_dlgmtext.h \
    # ui/dialogs/entity/qg_dlgpoint.h \
    # ui/dialogs/entity/qg_dlgpolyline.h \
    # ui/dialogs/entity/qg_dlgspline.h \
    ui/dialogs/entity/qg_dlgtext.h \
    ui/dialogs/file/export/makercam/qg_dlgoptionsmakercam.h \
    ui/dialogs/file/export/layers/lc_filedialogservice.h \
    ui/dialogs/file/qg_filedialog.h \
    ui/dialogs/main/qg_dlginitial.h \
    ui/dialogs/main/qg_exitdialog.h \
    ui/dialogs/qg_dialogfactory.h \
    ui/dialogs/settings/options_device/lc_deviceoptions.h \
    ui/dialogs/settings/options_drawing/lg_dimzerosbox.h \
    ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.h \
    ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.h \
    ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.h \
    ui/dialogs/settings/options_widget/lc_iconengineshared.h \
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
    ui/main/init/lc_actionfactory.h \
    ui/main/init/lc_widgetfactory.h \
    ui/main/init/lc_menufactory.h \
    ui/main/init/lc_toolbarfactory.h \
    ui/main/mainwindowx.h \
    ui/main/qc_applicationwindow.h \
    ui/main/qc_mdiwindow.h \
    ui/main/support/qg_recentfiles.h\
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
    # ui/not_used/qg_dlgdimlinear.h \
    # ui/not_used/lc_dlgdimordinate.h \
    # ui/not_used/qg_dlgdimension.h \
    ui/qg_actionhandler.h \
    ui/view/lc_centralwidget.h \
    ui/view/qg_graphicview.h

SOURCES +=ui/action_options/lc_actionoptionsmanager.cpp \
    ui/action_options/circle/lc_circlebyarcoptions.cpp \
    ui/action_options/circle/qg_circleoptions.cpp \
    ui/action_options/circle/qg_circletan2options.cpp \
    ui/action_options/curve/qg_arcoptions.cpp \
    ui/action_options/curve/qg_arctangentialoptions.cpp \
    ui/action_options/spline/qg_splineoptions.cpp \
    ui/action_options/dimensions/qg_dimoptions.cpp \
    ui/action_options/edit/lc_pastetransformoptions.cpp \
    ui/action_options/image/qg_imageoptions.cpp \
    ui/action_options/info/lc_infodist2options.cpp \
    ui/action_options/insert/qg_insertoptions.cpp \
    ui/action_options/insert/qg_libraryinsertoptions.cpp \
    ui/action_options/lc_actionoptionswidget.cpp \
    ui/action_options/lc_actionoptionswidgetbase.cpp \
    ui/action_options/misc/lc_crossoptions.cpp \
    ui/action_options/line/lc_lineanglereloptions.cpp \
    ui/action_options/line/lc_linefrompointtolineoptions.cpp \
    ui/action_options/line/lc_lineoptions.cpp \
    ui/action_options/point/lc_linepointsoptions.cpp \
    ui/action_options/rect/lc_rectangle1pointoptions.cpp \
    ui/action_options/rect/lc_rectangle2pointsoptions.cpp \
    ui/action_options/rect/lc_rectangle3pointsoptions.cpp \
    ui/action_options/line/lc_slicedivideoptions.cpp \
    ui/action_options/polygon/lc_staroptions.cpp \
    ui/action_options/line/qg_lineangleoptions.cpp \
    ui/action_options/line/qg_linebisectoroptions.cpp \
    ui/action_options/line/qg_lineoptions.cpp \
    ui/action_options/line/qg_lineparalleloptions.cpp \
    ui/action_options/line/qg_lineparallelthroughoptions.cpp \
    ui/action_options/polygon/qg_linepolygonoptions.cpp \
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
    # ui/dialogs/entity/LC_DlgParabola.cpp \
    #ui/dialogs/entity/lc_dlgsplinepoints.cpp \
    ui/dialogs/entity/qg_blockdialog.cpp \
    ui/dialogs/entity/qg_dimensionlabeleditor.cpp \
    # ui/dialogs/entity/qg_dlgarc.cpp \
    ui/dialogs/entity/qg_dlgattributes.cpp \
    # ui/dialogs/entity/qg_dlgcircle.cpp \
    # ui/dialogs/entity/qg_dlgellipse.cpp \
    ui/dialogs/entity/qg_dlghatch.cpp \
    # ui/dialogs/entity/qg_dlgimage.cpp \
    ui/dialogs/file/export/image/qg_dlgimageoptions.cpp \
    # ui/dialogs/entity/qg_dlginsert.cpp \
    # ui/dialogs/entity/qg_dlgline.cpp \
    ui/dialogs/entity/qg_dlgmtext.cpp \
    # ui/dialogs/entity/qg_dlgpoint.cpp \
    # ui/dialogs/entity/qg_dlgpolyline.cpp \
    # ui/dialogs/entity/qg_dlgspline.cpp \
    ui/dialogs/entity/qg_dlgtext.cpp \
    ui/dialogs/file/export/makercam/qg_dlgoptionsmakercam.cpp \
    ui/dialogs/file/export/layers/lc_filedialogservice.cpp \
    ui/dialogs/file/qg_filedialog.cpp \
    ui/dialogs/main/qg_dlginitial.cpp \
    ui/dialogs/main/qg_exitdialog.cpp \
    ui/dialogs/qg_dialogfactory.cpp \
    ui/dialogs/settings/options_device/lc_deviceoptions.cpp \
    ui/dialogs/settings/options_drawing/lg_dimzerosbox.cpp \
    ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.cpp \
    ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.cpp \
    ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.cpp \
    ui/dialogs/settings/options_widget/lc_iconengineshared.cpp \
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
    ui/main/init/lc_actionfactory.cpp \
    ui/main/init/lc_widgetfactory.cpp \
    ui/main/init/lc_menufactory.cpp \
    ui/main/init/lc_toolbarfactory.cpp \
    ui/main/mainwindowx.cpp \
    ui/main/qc_applicationwindow.cpp \
    ui/main/qc_mdiwindow.cpp \
    ui/main/support/qg_recentfiles.cpp \
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
    # ui/not_used/qg_dlgdimlinear.cpp \
    # ui/not_used/qg_dlgdimension.cpp \
    ui/qg_actionhandler.cpp \
    ui/view/lc_centralwidget.cpp \
    ui/view/qg_graphicview.cpp

FORMS = ui/action_options/circle/lc_circlebyarcoptions.ui \
       ui/action_options/circle/qg_circleoptions.ui \
       ui/action_options/circle/qg_circletan2options.ui \
       ui/action_options/curve/lc_actiondrawarc2poptions.ui \
       ui/action_options/curve/lc_ellipsearcoptions.ui \
       ui/action_options/misc/lc_midlineoptions.ui \
       ui/action_options/misc/lc_drawboundingboxoptions.ui \
       ui/action_options/modify/lc_modifyalignoptions.ui \
       ui/action_options/modify/lc_modifyalignrefoptions.ui \
       ui/action_options/other/lc_ucssetoptions.ui \
       ui/action_options/spline/lc_splineexplodeoptions.ui \
       ui/action_options/spline/lc_splinefrompolylineoptions.ui \
       ui/action_options/curve/qg_arcoptions.ui \
       ui/action_options/curve/qg_arctangentialoptions.ui \
       ui/action_options/spline/qg_splineoptions.ui \
       ui/action_options/dimensions/qg_dimoptions.ui \
       ui/action_options/edit/lc_pastetransformoptions.ui \
       ui/action_options/ellipse/lc_ellipse1pointoptions.ui \
       ui/action_options/image/qg_imageoptions.ui \
       ui/action_options/info/lc_infodist2options.ui \
       ui/action_options/insert/qg_insertoptions.ui \
       ui/action_options/insert/qg_libraryinsertoptions.ui \
       ui/action_options/misc/lc_crossoptions.ui \
       ui/action_options/line/lc_lineanglereloptions.ui \
       ui/action_options/line/lc_linefrompointtolineoptions.ui \
       ui/action_options/line/lc_lineoptions.ui \
       ui/action_options/point/lc_linepointsoptions.ui \
       ui/action_options/rect/lc_rectangle1pointoptions.ui \
       ui/action_options/rect/lc_rectangle2pointsoptions.ui \
       ui/action_options/rect/lc_rectangle3pointsoptions.ui \
       ui/action_options/line/lc_slicedivideoptions.ui \
       ui/action_options/polygon/lc_staroptions.ui \
       ui/action_options/line/qg_lineangleoptions.ui \
       ui/action_options/line/qg_linebisectoroptions.ui \
       ui/action_options/line/qg_lineoptions.ui \
       ui/action_options/line/qg_lineparalleloptions.ui \
       ui/action_options/line/qg_lineparallelthroughoptions.ui \
       ui/action_options/polygon/qg_linepolygonoptions.ui \
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
       ui/action_options/point/lc_pastetopointsoptions.ui \
       ui/action_options/point/lc_pointslatticeoptions.ui \
       ui/action_options/polyline/qg_polylineequidistantoptions.ui \
       ui/action_options/polyline/qg_polylineoptions.ui \
       ui/action_options/print_preview/qg_printpreviewoptions.ui \
       ui/action_options/selection/lc_selectwindowoptions.ui \
       ui/action_options/snap/qg_snapdistoptions.ui \
       ui/action_options/snap/qg_snapmiddleoptions.ui \
       ui/action_options/text/qg_mtextoptions.ui \
       ui/action_options/text/qg_textoptions.ui \
       ui/components/comboboxes/comboboxoption.ui \
       ui/components/containers/lc_optionswidgetsholder.ui \
       ui/components/containers/lc_snapoptionswidgetsholder.ui \
       ui/dialogs/creators/lc_dlgmenuassigner.ui \
       ui/dialogs/creators/lc_dlgwidgetcreator.ui \
       ui/components/pen/qg_widgetpen.ui \
       ui/components/status_bar/lc_anglesbasiswidget.ui \
       ui/components/status_bar/lc_relzerocoordinateswidget.ui \
       ui/components/status_bar/lc_ucsstatewidget.ui \
       ui/components/status_bar/qg_activelayername.ui \
       ui/components/status_bar/qg_coordinatewidget.ui \
       ui/components/status_bar/qg_mousewidget.ui \
       ui/components/status_bar/qg_selectionwidget.ui \
       ui/components/textfileviewer.ui \
       ui/dialogs/actions/modify/qg_dlgmirror.ui \
       ui/dialogs/actions/modify/qg_dlgmove.ui \
       ui/dialogs/actions/modify/qg_dlgmoverotate.ui \
       ui/dialogs/actions/modify/qg_dlgrotate.ui \
       ui/dialogs/actions/modify/qg_dlgrotate2.ui \
       ui/dialogs/actions/modify/qg_dlgscale.ui \
       ui/dialogs/actions/qg_layerdialog.ui \
       ui/dialogs/creators/lc_dlgnewwidget.ui \
       #ui/dialogs/entity/LC_DlgParabola.ui \
       ui/dialogs/entity/lc_arcpropertieseditingwidget.ui \
       ui/dialogs/entity/lc_circlepropertieseditingwidget.ui \
       ui/dialogs/entity/lc_dlgdimension.ui \
       ui/dialogs/entity/lc_dlgentityproperties.ui \
       ui/dialogs/entity/lc_dlgtolerance.ui \
       #ui/dialogs/entity/lc_dlgsplinepoints.ui \
       ui/dialogs/entity/lc_ellipsepropertieseditingwidget.ui \
       ui/dialogs/entity/lc_hyperbolapropertieseditingwidget.ui \
       ui/dialogs/entity/lc_imagepropertieseditingwidget.ui \
       ui/dialogs/entity/lc_insertpropertieseditingwidget.ui \
       ui/dialogs/entity/lc_linepropertieseditingwidget.ui \
       ui/dialogs/entity/lc_parabolapropertieseditingwidget.ui \
       ui/dialogs/entity/lc_pointpickbutton.ui \
       ui/dialogs/entity/lc_pointpropertieseditingwidget.ui \
       ui/dialogs/entity/lc_polylinepropertieseditingwidget.ui \
       ui/dialogs/entity/lc_splinepointspropertieseditingwidget.ui \
       ui/dialogs/entity/lc_splinepropertieseditingwidget.ui \
       ui/dialogs/entity/qg_blockdialog.ui \
       ui/dialogs/entity/qg_dimensionlabeleditor.ui \
       # ui/dialogs/entity/qg_dlgarc.ui \
       ui/dialogs/entity/qg_dlgattributes.ui \
       # ui/dialogs/entity/qg_dlgcircle.ui \
       # ui/dialogs/entity/qg_dlgellipse.ui \
       ui/dialogs/entity/qg_dlghatch.ui \
       # ui/dialogs/entity/qg_dlgimage.ui \
       ui/dialogs/file/export/image/qg_dlgimageoptions.ui \
       # ui/dialogs/entity/qg_dlginsert.ui \
       # ui/dialogs/entity/qg_dlgline.ui \
       ui/dialogs/entity/qg_dlgmtext.ui \
       # ui/dialogs/entity/qg_dlgpoint.ui \
       # ui/dialogs/entity/qg_dlgpolyline.ui \
       # ui/dialogs/entity/qg_dlgspline.ui \
       ui/dialogs/entity/qg_dlgtext.ui \
       ui/dialogs/file/export/makercam/qg_dlgoptionsmakercam.ui \
       ui/dialogs/lc_inputtextdialog.ui \
       ui/dialogs/main/lc_dlgabout.ui \
       ui/dialogs/main/lc_dlgnewversionavailable.ui \
       ui/dialogs/main/qg_dlginitial.ui \
       ui/dialogs/main/qg_exitdialog.ui \
       ui/dialogs/settings/options_device/lc_deviceoptions.ui \
       ui/dialogs/settings/options_drawing/lc_dlgnewcustomvariable.ui \
       ui/dialogs/settings/options_drawing/lc_dlgnewdimstyle.ui \
       ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.ui \
       ui/dialogs/settings/dimstyles/dimstyle_manager/lc_dlgdimstylemanager.ui \
       ui/dialogs/settings/dimstyles/dimstyle_manager/support/lc_dimstylepreviewpanel.ui \
       ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.ui \
       ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.ui \
       ui/dialogs/settings/options_widget/lc_dlgiconssetup.ui \
       ui/dialogs/settings/shortcuts/lc_actionsshortcutsdialog.ui \
       ui/dock_widgets/command_line/qg_commandwidget.ui \
       ui/dock_widgets/entity_info/lc_quickinfowidget.ui \
       ui/dock_widgets/entity_info/lc_quickinfowidgetoptionsdialog.ui \
       ui/dock_widgets/layers_tree/lc_layerdialog_ex.ui \
       ui/dialogs/file/export/layers/lc_layerexportoptions.ui \
       ui/dock_widgets/layers_tree/lc_layertreeoptionsdialog.ui \
       # ui/dock_widgets/library_widget/lc_librarywidget.ui \
       ui/dock_widgets/pen_palette/lc_penpaletteoptionsdialog.ui \
       ui/dock_widgets/pen_palette/lc_penpalettewidget.ui \
       ui/dock_widgets/pen_wizard/colorwizard.ui \
       ui/dock_widgets/ucs_list/lc_dlgucslistoptions.ui \
       ui/dock_widgets/ucs_list/lc_dlgucsproperties.ui \
       ui/dock_widgets/ucs_list/lc_ucslistwidget.ui \
       ui/dock_widgets/views_list/lc_dlgnamedviewslistoptions.ui \
       ui/dock_widgets/views_list/lc_namedviewslistwidget.ui
       # ui/not_used/customtoolbarcreator.ui \
       # ui/not_used/customwidgetcreator.ui \
       # ui/not_used/qg_dimlinearoptions.ui \
       # ui/not_used/qg_dlgoptionsvariables.ui \
       # ui/not_used/qg_snaptoolbar.ui \
       # ui/not_used/qg_linepolygon2options.ui
       # ui/not_used/qg_dlgdimension.ui \
       # ui/not_used/qg_dlgdimlinear.ui \
       # ui/not_used/lc_dlgdimordinate.ui \


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
