cmake_minimum_required(VERSION 3.27)
project(LibreCAD VERSION 2.2.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(MIN_GW_PATH_PREFIX "c:\\DevTools\\QT\\6.7.2\\mingw_64")
set(MIN_GW_PATH_PREFIX "d:\\Qt\\QT6\\6.7.0\\mingw_64")
#set(MIN_GW_PATH_PREFIX "c:\\DevTools\\QT\\6.7.2\\mingw_64")
set(BOOST_DIR "c:\\Local\\boost_1_84_0")
set(Boost_INCLUDE_DIR "c:\\Local\\boost_1_84_0")

#set(CMAKE_PREFIX_PATH "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6")

set(Qt6_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6")
set(Qt6Core_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6Core")
set(Qt6Svg_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6Svg")
set(Qt6CoreTools_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6CoreTools")
set(Qt6Gui_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\QtGui")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC_OPTIONS --connections string)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_compile_definitions(DWGSUPPORT)
add_compile_definitions(MUPARSER_STATIC)


find_package(Qt6 COMPONENTS Gui Core Widgets PrintSupport Svg Core5Compat REQUIRED)
#find_package(Qt5 COMPONENTS Widgets REQUIRED)
#qt_standard_project_setup()

find_package(Boost REQUIRED COMPONENTS)

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOUIC OFF)
#set(CMAKE_AUTORCC ON)

#qt5_wrap_cpp(plugins/asciifile.cpp)


include_directories(${MIN_GW_PATH_PREFIX}\\include\\QtSvg)

include_directories(${Boost_INCLUDE_DIR})

include_directories(librecad/res/actions)
include_directories(librecad/res/extui)
include_directories(librecad/res/icons)
include_directories(librecad/res/main)
include_directories(librecad/res/ui)
include_directories(libraries/jwwlib/src)
include_directories(libraries/libdxfrw/src)
include_directories(libraries/libdxfrw/src/intern)
include_directories(libraries/muparser/include)
include_directories(librecad/src/actions)
include_directories(librecad/src/actions/drawing)
include_directories(librecad/src/actions/drawing/draw)
include_directories(librecad/src/actions/drawing/draw/line)
include_directories(librecad/src/actions/drawing/draw/line/shapes)
include_directories(librecad/src/actions/drawing/draw/line/shapes/rect)
include_directories(librecad/src/actions/drawing/draw/line/shapes/polygon)
include_directories(librecad/src/actions/drawing/draw/circle)
include_directories(librecad/src/actions/drawing/draw/curve)
include_directories(librecad/src/actions/drawing/draw/polyline)
include_directories(librecad/src/actions/drawing/draw/ellipse)
include_directories(librecad/src/actions/drawing/draw/dimensions)
include_directories(librecad/src/actions/drawing/draw/text)
include_directories(librecad/src/actions/drawing/draw/image)
include_directories(librecad/src/actions/drawing/draw/hatch)
include_directories(librecad/src/actions/drawing/modify)
include_directories(librecad/src/actions/drawing/info)
include_directories(librecad/src/actions/drawing/selection)
include_directories(librecad/src/actions/drawing/zoom)
include_directories(librecad/src/actions/drawing/pen)
include_directories(librecad/src/actions/drawing/snap)
include_directories(librecad/src/actions/drawing/edit)
include_directories(librecad/src/actions/drawing/rel_zero)

include_directories(librecad/src/actions/dock_widgets)
include_directories(librecad/src/actions/dock_widgets/block)
include_directories(librecad/src/actions/dock_widgets/library)
include_directories(librecad/src/actions/dock_widgets/layer)
include_directories(librecad/src/actions/dock_widgets/entity_info)


include_directories(librecad/src/actions/file)
include_directories(librecad/src/actions/options)
include_directories(librecad/src/actions/print_preview)

include_directories(librecad/src/cmd)
include_directories(librecad/src/lib/actions)
include_directories(librecad/src/lib/creation)
include_directories(librecad/src/lib/debug)
include_directories(librecad/src/lib/engine)
include_directories(librecad/src/lib/fileio)
include_directories(librecad/src/lib/filters)
include_directories(librecad/src/lib/generators)
include_directories(librecad/src/lib/gui)
include_directories(librecad/src/lib/information)
include_directories(librecad/src/lib/math)
include_directories(librecad/src/lib/modification)
include_directories(librecad/src/lib/printing)
include_directories(librecad/src/lib/scripting)
include_directories(librecad/src/main)
include_directories(librecad/src/main/console_dxf2pdf)
include_directories(librecad/src/plugins)
include_directories(librecad/src/plugins/intern)
include_directories(librecad/src/test)
include_directories(librecad/src/ui)
include_directories(librecad/src/ui/dialogs)
include_directories(librecad/src/ui/dialogs/actions)
include_directories(librecad/src/ui/dialogs/actions/modify)
include_directories(librecad/src/ui/dialogs/settings/device_options)
include_directories(librecad/src/ui/dialogs/settings/options_device)
include_directories(librecad/src/ui/dialogs/settings/options_general)
include_directories(librecad/src/ui/dialogs/settings/options_drawing)
include_directories(librecad/src/ui/dialogs/settings/options_widget)
include_directories(librecad/src/ui/dialogs/settings/shortcuts)
include_directories(librecad/src/ui/dialogs/settings)
include_directories(librecad/src/ui/dialogs/entity)
include_directories(librecad/src/ui/dialogs/file)
include_directories(librecad/src/ui/dialogs/file/export)
include_directories(librecad/src/ui/dialogs/main)
include_directories(librecad/src/ui/action_options)
include_directories(librecad/src/ui/action_options/draw)
include_directories(librecad/src/ui/action_options/modify)
include_directories(librecad/src/ui/action_options/info)
include_directories(librecad/src/ui/action_options/select)
include_directories(librecad/src/ui/action_options/line)
include_directories(librecad/src/ui/action_options/circle)
include_directories(librecad/src/ui/action_options/polyline)
include_directories(librecad/src/ui/action_options/curve)
include_directories(librecad/src/ui/action_options/ellipse)
include_directories(librecad/src/ui/action_options/dimensions)
include_directories(librecad/src/ui/action_options/selection)
include_directories(librecad/src/ui/action_options/snap)
include_directories(librecad/src/ui/action_options/edit)
include_directories(librecad/src/ui/action_options/print_preview)
include_directories(librecad/src/ui/action_options/text)
include_directories(librecad/src/ui/action_options/image)
include_directories(librecad/src/ui/action_options/insert)
include_directories(librecad/src/ui/action_options/other)
#include_directories(librecad/src/ui/not_used)
include_directories(librecad/src/ui/actions)
include_directories(librecad/src/ui/components)
include_directories(librecad/src/ui/components/comboboxes)
include_directories(librecad/src/ui/components/toolbars)
include_directories(librecad/src/ui/components/containers)
include_directories(librecad/src/ui/components/status_bar)
include_directories(librecad/src/ui/components/layouts)
include_directories(librecad/src/ui/components/creators)
include_directories(librecad/src/ui/components/pen)
include_directories(librecad/src/ui/forms)
include_directories(librecad/src/ui/generic)
include_directories(librecad/src/ui/view)
include_directories(librecad/src/ui/main)
include_directories(librecad/src/ui/dock_widgets)
include_directories(librecad/src/ui/dock_widgets/command_line)
include_directories(librecad/src/ui/dock_widgets/pen_wizard)
include_directories(librecad/src/ui/dock_widgets/layers_tree)
include_directories(librecad/src/ui/dock_widgets/entity_info)
include_directories(librecad/src/ui/dock_widgets/pen_palette)
include_directories(librecad/src/ui/dock_widgets/block_widget)
include_directories(librecad/src/ui/dock_widgets/library_widget)
include_directories(librecad/src/ui/dock_widgets/layer_widget)
#Temporary disabled for build
#include_directories(plugins/align)
#include_directories(plugins/asciifile)
#include_directories(plugins/divide)
#include_directories(plugins/gear)
#include_directories(plugins/list)
#include_directories(plugins/picfile)
#include_directories(plugins/plotequation)
#include_directories(plugins/sameprop)
#include_directories(plugins/sample)

set(SOURCES
        librecad/res/actions/actions.qrc
        librecad/res/extui/extui.qrc
        librecad/res/ui/ui.qrc
        librecad/res/icons/icons.qrc
        librecad/res/main/main.qrc
        libraries/jwwlib/src/dl_attributes.h
        libraries/jwwlib/src/dl_codes.h
        libraries/jwwlib/src/dl_creationinterface.h
        libraries/jwwlib/src/dl_entities.h
        libraries/jwwlib/src/dl_exception.h
        libraries/jwwlib/src/dl_extrusion.h
#        libraries/jwwlib/src/dl_jww-copy.cpp
        libraries/jwwlib/src/dl_jww.cpp
        libraries/jwwlib/src/dl_jww.h
        libraries/jwwlib/src/dl_writer.h
        libraries/jwwlib/src/dl_writer_ascii.cpp
        libraries/jwwlib/src/dl_writer_ascii.h
        libraries/jwwlib/src/jwtype.h
        libraries/jwwlib/src/jwwdoc.cpp
        libraries/jwwlib/src/jwwdoc.h
        libraries/libdxfrw/src/intern/drw_cptable932.h
        libraries/libdxfrw/src/intern/drw_cptable936.h
        libraries/libdxfrw/src/intern/drw_cptable949.h
        libraries/libdxfrw/src/intern/drw_cptable950.h
        libraries/libdxfrw/src/intern/drw_cptables.h
        libraries/libdxfrw/src/intern/drw_dbg.cpp
        libraries/libdxfrw/src/intern/drw_dbg.h
        libraries/libdxfrw/src/intern/drw_reserve.h
        libraries/libdxfrw/src/intern/drw_textcodec.cpp
        libraries/libdxfrw/src/intern/drw_textcodec.h
        libraries/libdxfrw/src/intern/dwgbuffer.cpp
        libraries/libdxfrw/src/intern/dwgbuffer.h
        libraries/libdxfrw/src/intern/dwgreader.cpp
        libraries/libdxfrw/src/intern/dwgreader.h
        libraries/libdxfrw/src/intern/dwgreader15.cpp
        libraries/libdxfrw/src/intern/dwgreader15.h
        libraries/libdxfrw/src/intern/dwgreader18.cpp
        libraries/libdxfrw/src/intern/dwgreader18.h
        libraries/libdxfrw/src/intern/dwgreader21.cpp
        libraries/libdxfrw/src/intern/dwgreader21.h
        libraries/libdxfrw/src/intern/dwgreader24.cpp
        libraries/libdxfrw/src/intern/dwgreader24.h
        libraries/libdxfrw/src/intern/dwgreader27.cpp
        libraries/libdxfrw/src/intern/dwgreader27.h
        libraries/libdxfrw/src/intern/dwgutil.cpp
        libraries/libdxfrw/src/intern/dwgutil.h
        libraries/libdxfrw/src/intern/dxfreader.cpp
        libraries/libdxfrw/src/intern/dxfreader.h
        libraries/libdxfrw/src/intern/dxfwriter.cpp
        libraries/libdxfrw/src/intern/dxfwriter.h
        libraries/libdxfrw/src/intern/rscodec.cpp
        libraries/libdxfrw/src/intern/rscodec.h
        libraries/libdxfrw/src/drw_base.cpp
        libraries/libdxfrw/src/drw_base.h
        libraries/libdxfrw/src/drw_classes.cpp
        libraries/libdxfrw/src/drw_classes.h
        libraries/libdxfrw/src/drw_entities.cpp
        libraries/libdxfrw/src/drw_entities.h
        libraries/libdxfrw/src/drw_header.cpp
        libraries/libdxfrw/src/drw_header.h
        libraries/libdxfrw/src/drw_interface.h
        libraries/libdxfrw/src/drw_objects.cpp
        libraries/libdxfrw/src/drw_objects.h
        libraries/libdxfrw/src/libdwgr.cpp
        libraries/libdxfrw/src/libdwgr.h
        libraries/libdxfrw/src/libdxfrw.cpp
        libraries/libdxfrw/src/libdxfrw.h
        libraries/libdxfrw/src/main_doc.h
        libraries/muparser/include/muParser.h
        libraries/muparser/include/muParserBase.h
        libraries/muparser/include/muParserBytecode.h
        libraries/muparser/include/muParserCallback.h
        libraries/muparser/include/muParserDef.h
        libraries/muparser/include/muParserDLL.h
        libraries/muparser/include/muParserError.h
        libraries/muparser/include/muParserFixes.h
        libraries/muparser/include/muParserInt.h
        libraries/muparser/include/muParserTemplateMagic.h
        libraries/muparser/include/muParserToken.h
        libraries/muparser/include/muParserTokenReader.h
        libraries/muparser/src/muParser.cpp
        libraries/muparser/src/muParserBase.cpp
        libraries/muparser/src/muParserBytecode.cpp
        libraries/muparser/src/muParserCallback.cpp
        libraries/muparser/src/muParserDLL.cpp
        libraries/muparser/src/muParserError.cpp
        libraries/muparser/src/muParserInt.cpp
        libraries/muparser/src/muParserTokenReader.cpp
		librecad/src/actions/drawing/draw/dimensions/lc_actiondimarc.cpp
		librecad/src/actions/drawing/draw/dimensions/lc_actiondimarc.h
		librecad/src/actions/drawing/draw/circle/lc_actiondrawcircle2pr.cpp
		librecad/src/actions/drawing/draw/circle/lc_actiondrawcircle2pr.h
		librecad/src/actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygon3.cpp
		librecad/src/actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygon3.h
		librecad/src/actions/drawing/draw/curve/lc_actiondrawsplinepoints.cpp
		librecad/src/actions/drawing/draw/curve/lc_actiondrawsplinepoints.h
		librecad/src/actions/file/lc_actionfileexportmakercam.cpp
		librecad/src/actions/file/lc_actionfileexportmakercam.h
        librecad/src/actions/dock_widgets/layer/lc_actionlayersexport.cpp
        librecad/src/actions/dock_widgets/layer/lc_actionlayersexport.h
        librecad/src/actions/dock_widgets/layer/lc_actionlayerstoggleconstruction.cpp
        librecad/src/actions/dock_widgets/layer/lc_actionlayerstoggleconstruction.h
		librecad/src/actions/dock_widgets/block/rs_actionblocksadd.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblocksadd.h
		librecad/src/actions/dock_widgets/block/rs_actionblocksattributes.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblocksattributes.h
		librecad/src/actions/dock_widgets/block/rs_actionblockscreate.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblockscreate.h
		librecad/src/actions/dock_widgets/block/rs_actionblocksedit.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblocksedit.h
		librecad/src/actions/drawing/modify/rs_actionblocksexplode.cpp
		librecad/src/actions/drawing/modify/rs_actionblocksexplode.h
		librecad/src/actions/dock_widgets/block/rs_actionblocksfreezeall.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblocksfreezeall.h
		librecad/src/actions/dock_widgets/block/rs_actionblocksinsert.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblocksinsert.h
		librecad/src/actions/dock_widgets/block/rs_actionblocksremove.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblocksremove.h
		librecad/src/actions/dock_widgets/block/rs_actionblockssave.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblockssave.h
		librecad/src/actions/dock_widgets/block/rs_actionblockstoggleview.cpp
		librecad/src/actions/dock_widgets/block/rs_actionblockstoggleview.h
		librecad/src/actions/drawing/rs_actiondefault.cpp
		librecad/src/actions/drawing/rs_actiondefault.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimaligned.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimaligned.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimangular.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimangular.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimdiametric.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimdiametric.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimension.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimension.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimleader.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimleader.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimlinear.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimlinear.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimradial.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiondimradial.h
		librecad/src/actions/drawing/draw/curve/rs_actiondrawarc.cpp
		librecad/src/actions/drawing/draw/curve/rs_actiondrawarc.h
		librecad/src/actions/drawing/draw/curve/rs_actiondrawarc3p.cpp
		librecad/src/actions/drawing/draw/curve/rs_actiondrawarc3p.h
		librecad/src/actions/drawing/draw/curve/rs_actiondrawarctangential.cpp
		librecad/src/actions/drawing/draw/curve/rs_actiondrawarctangential.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircle.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircle.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircle2p.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircle2p.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircle3p.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircle3p.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcirclecr.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcirclecr.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircleinscribe.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircleinscribe.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan1_2p.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan1_2p.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan2.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan2.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan2_1p.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan2_1p.h
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan3.cpp
		librecad/src/actions/drawing/draw/circle/rs_actiondrawcircletan3.h
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipse4points.cpp
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipse4points.h
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipseaxis.cpp
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipseaxis.h
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipsecenter3points.cpp
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipsecenter3points.h
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipsefocipoint.cpp
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipsefocipoint.h
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipseinscribe.cpp
		librecad/src/actions/drawing/draw/ellipse/rs_actiondrawellipseinscribe.h
		librecad/src/actions/drawing/draw/hatch/rs_actiondrawhatch.cpp
		librecad/src/actions/drawing/draw/hatch/rs_actiondrawhatch.h
		librecad/src/actions/drawing/draw/image/rs_actiondrawimage.cpp
		librecad/src/actions/drawing/draw/image/rs_actiondrawimage.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawline.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawline.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineangle.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineangle.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinebisector.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinebisector.h
		librecad/src/actions/drawing/draw/curve/rs_actiondrawlinefree.cpp
		librecad/src/actions/drawing/draw/curve/rs_actiondrawlinefree.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinehorvert.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinehorvert.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineorthtan.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineorthtan.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineparallel.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineparallel.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineparallelthrough.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlineparallelthrough.h
		librecad/src/actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon.cpp
		librecad/src/actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon.h
		librecad/src/actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon2.cpp
		librecad/src/actions/drawing/draw/line/shapes/polygon/rs_actiondrawlinepolygon2.h
		librecad/src/actions/drawing/draw/line/shapes/rect/rs_actiondrawlinerectangle.cpp
		librecad/src/actions/drawing/draw/line/shapes/rect/rs_actiondrawlinerectangle.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinerelangle.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinerelangle.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinetangent1.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinetangent1.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinetangent2.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawlinetangent2.h
		librecad/src/actions/drawing/draw/text/rs_actiondrawmtext.cpp
		librecad/src/actions/drawing/draw/text/rs_actiondrawmtext.h
		librecad/src/actions/drawing/draw/line/rs_actiondrawpoint.cpp
		librecad/src/actions/drawing/draw/line/rs_actiondrawpoint.h
		librecad/src/actions/drawing/draw/polyline/rs_actiondrawpolyline.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actiondrawpolyline.h
		librecad/src/actions/drawing/draw/curve/rs_actiondrawspline.cpp
		librecad/src/actions/drawing/draw/curve/rs_actiondrawspline.h
		librecad/src/actions/drawing/draw/text/rs_actiondrawtext.cpp
		librecad/src/actions/drawing/draw/text/rs_actiondrawtext.h
		librecad/src/actions/drawing/edit/rs_actioneditcopy.cpp
		librecad/src/actions/drawing/edit/rs_actioneditcopy.h
		librecad/src/actions/not_used/rs_actioneditpaste.cpp
		librecad/src/actions/not_used/rs_actioneditpaste.h
		librecad/src/actions/drawing/edit/rs_actioneditundo.cpp
		librecad/src/actions/drawing/edit/rs_actioneditundo.h
		librecad/src/actions/file/rs_actionfilenew.cpp
		librecad/src/actions/file/rs_actionfilenew.h
		librecad/src/actions/file/rs_actionfilenewtemplate.cpp
		librecad/src/actions/file/rs_actionfilenewtemplate.h
		librecad/src/actions/file/rs_actionfileopen.cpp
		librecad/src/actions/file/rs_actionfileopen.h
		librecad/src/actions/file/rs_actionfilesave.cpp
		librecad/src/actions/file/rs_actionfilesave.h
		librecad/src/actions/file/rs_actionfilesaveas.cpp
		librecad/src/actions/file/rs_actionfilesaveas.h
		librecad/src/actions/drawing/info/rs_actioninfoangle.cpp
		librecad/src/actions/drawing/info/rs_actioninfoangle.h
		librecad/src/actions/drawing/info/rs_actioninfoarea.cpp
		librecad/src/actions/drawing/info/rs_actioninfoarea.h
		librecad/src/actions/drawing/info/rs_actioninfodist.cpp
		librecad/src/actions/drawing/info/rs_actioninfodist.h
		librecad/src/actions/drawing/info/rs_actioninfodist2.cpp
		librecad/src/actions/drawing/info/rs_actioninfodist2.h
        librecad/src/actions/drawing/info/rs_actioninfoinside.cpp
        librecad/src/actions/drawing/info/rs_actioninfoinside.h
		librecad/src/actions/drawing/info/rs_actioninfototallength.cpp
		librecad/src/actions/drawing/info/rs_actioninfototallength.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayersadd.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayersadd.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayersedit.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayersedit.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayersfreezeall.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayersfreezeall.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayerslockall.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayerslockall.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayersremove.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayersremove.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayerstogglelock.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayerstogglelock.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayerstoggleprint.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayerstoggleprint.h
        librecad/src/actions/dock_widgets/layer/rs_actionlayerstoggleview.cpp
        librecad/src/actions/dock_widgets/layer/rs_actionlayerstoggleview.h
		librecad/src/actions/dock_widgets/library/rs_actionlibraryinsert.cpp
		librecad/src/actions/dock_widgets/library/rs_actionlibraryinsert.h
		librecad/src/actions/drawing/rel_zero/rs_actionlockrelativezero.cpp
		librecad/src/actions/drawing/rel_zero/rs_actionlockrelativezero.h
		librecad/src/actions/drawing/modify/rs_actionmodifyattributes.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyattributes.h
		librecad/src/actions/drawing/modify/rs_actionmodifybevel.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifybevel.h
		librecad/src/actions/drawing/modify/rs_actionmodifycut.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifycut.h
		librecad/src/actions/drawing/modify/rs_actionmodifydelete.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifydelete.h
		librecad/src/actions/drawing/modify/rs_actionmodifydeletefree.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifydeletefree.h
		librecad/src/actions/not_used/rs_actionmodifydeletequick.cpp
		librecad/src/actions/not_used/rs_actionmodifydeletequick.h
		librecad/src/actions/drawing/modify/rs_actionmodifyentity.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyentity.h
		librecad/src/actions/drawing/modify/rs_actionmodifyexplodetext.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyexplodetext.h
		librecad/src/actions/drawing/modify/rs_actionmodifymirror.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifymirror.h
		librecad/src/actions/drawing/modify/rs_actionmodifymove.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifymove.h
		librecad/src/actions/drawing/modify/rs_actionmodifymoverotate.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifymoverotate.h
		librecad/src/actions/drawing/modify/rs_actionmodifyoffset.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyoffset.h
		librecad/src/actions/drawing/modify/rs_actionmodifyrevertdirection.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyrevertdirection.h
		librecad/src/actions/drawing/modify/rs_actionmodifyrotate.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyrotate.h
		librecad/src/actions/drawing/modify/rs_actionmodifyrotate2.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyrotate2.h
		librecad/src/actions/drawing/modify/rs_actionmodifyround.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyround.h
		librecad/src/actions/drawing/modify/rs_actionmodifyscale.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifyscale.h
		librecad/src/actions/drawing/modify/rs_actionmodifystretch.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifystretch.h
		librecad/src/actions/drawing/modify/rs_actionmodifytrim.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifytrim.h
		librecad/src/actions/drawing/modify/rs_actionmodifytrimamount.cpp
		librecad/src/actions/drawing/modify/rs_actionmodifytrimamount.h
		librecad/src/actions/options/rs_actionoptionsdrawing.cpp
		librecad/src/actions/options/rs_actionoptionsdrawing.h
		librecad/src/actions/drawing/modify/rs_actionorder.cpp
		librecad/src/actions/drawing/modify/rs_actionorder.h
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylineadd.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylineadd.h
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylineappend.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylineappend.h
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinedel.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinedel.h
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinedelbetween.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinedelbetween.h
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylineequidistant.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylineequidistant.h
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinesegment.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinesegment.h
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinetrim.cpp
		librecad/src/actions/drawing/draw/polyline/rs_actionpolylinetrim.h
		librecad/src/actions/print_preview/rs_actionprintpreview.cpp
		librecad/src/actions/print_preview/rs_actionprintpreview.h
		librecad/src/actions/drawing/selection/rs_actionselect.cpp
		librecad/src/actions/drawing/selection/rs_actionselect.h
		librecad/src/actions/drawing/selection/rs_actionselectall.cpp
		librecad/src/actions/drawing/selection/rs_actionselectall.h
		librecad/src/actions/drawing/selection/rs_actionselectbase.cpp
		librecad/src/actions/drawing/selection/rs_actionselectbase.h
		librecad/src/actions/drawing/selection/rs_actionselectcontour.cpp
		librecad/src/actions/drawing/selection/rs_actionselectcontour.h
		librecad/src/actions/drawing/selection/rs_actionselectintersected.cpp
		librecad/src/actions/drawing/selection/rs_actionselectintersected.h
		librecad/src/actions/drawing/selection/rs_actionselectinvert.cpp
		librecad/src/actions/drawing/selection/rs_actionselectinvert.h
		librecad/src/actions/drawing/selection/rs_actionselectlayer.cpp
		librecad/src/actions/drawing/selection/rs_actionselectlayer.h
		librecad/src/actions/drawing/selection/rs_actionselectsingle.cpp
		librecad/src/actions/drawing/selection/rs_actionselectsingle.h
		librecad/src/actions/drawing/selection/rs_actionselectwindow.cpp
		librecad/src/actions/drawing/selection/rs_actionselectwindow.h
		librecad/src/actions/drawing/rel_zero/rs_actionsetrelativezero.cpp
		librecad/src/actions/drawing/rel_zero/rs_actionsetrelativezero.h
		librecad/src/actions/not_used/rs_actionsetsnapmode.cpp
		librecad/src/actions/not_used/rs_actionsetsnapmode.h
		librecad/src/actions/not_used/rs_actionsetsnaprestriction.cpp
		librecad/src/actions/not_used/rs_actionsetsnaprestriction.h
		librecad/src/actions/drawing/snap/rs_actionsnapintersectionmanual.cpp
		librecad/src/actions/drawing/snap/rs_actionsnapintersectionmanual.h
		librecad/src/actions/drawing/draw/dimensions/rs_actiontoolregeneratedimensions.cpp
		librecad/src/actions/drawing/draw/dimensions/rs_actiontoolregeneratedimensions.h
		librecad/src/actions/drawing/zoom/rs_actionzoomauto.cpp
		librecad/src/actions/drawing/zoom/rs_actionzoomauto.h
		librecad/src/actions/not_used/rs_actionzoomautoy.cpp
		librecad/src/actions/not_used/rs_actionzoomautoy.h
		librecad/src/actions/drawing/zoom/rs_actionzoomin.cpp
		librecad/src/actions/drawing/zoom/rs_actionzoomin.h
		librecad/src/actions/drawing/zoom/rs_actionzoompan.cpp
		librecad/src/actions/drawing/zoom/rs_actionzoompan.h
		librecad/src/actions/drawing/zoom/rs_actionzoomprevious.cpp
		librecad/src/actions/drawing/zoom/rs_actionzoomprevious.h
		librecad/src/actions/drawing/zoom/rs_actionzoomredraw.cpp
		librecad/src/actions/drawing/zoom/rs_actionzoomredraw.h
		librecad/src/actions/drawing/zoom/rs_actionzoomscroll.cpp
		librecad/src/actions/drawing/zoom/rs_actionzoomscroll.h
		librecad/src/actions/drawing/zoom/rs_actionzoomwindow.cpp
		librecad/src/actions/drawing/zoom/rs_actionzoomwindow.h
        librecad/src/cmd/rs_commands.cpp
        librecad/src/cmd/rs_commands.h
	librecad/src/cmd/lc_commandItems.h
        librecad/src/lib/actions/rs_actioninterface.cpp
        librecad/src/lib/actions/rs_actioninterface.h
        librecad/src/lib/actions/rs_preview.cpp
        librecad/src/lib/actions/rs_preview.h
        librecad/src/lib/actions/rs_previewactioninterface.cpp
        librecad/src/lib/actions/rs_previewactioninterface.h
        librecad/src/lib/actions/rs_snapper.cpp
        librecad/src/lib/actions/rs_snapper.h
        librecad/src/lib/creation/rs_creation.cpp
        librecad/src/lib/creation/rs_creation.h
        librecad/src/lib/debug/rs_debug.cpp
        librecad/src/lib/debug/rs_debug.h
        librecad/src/lib/engine/dxf_format.h
        librecad/src/lib/engine/lc_defaults.h
        librecad/src/lib/engine/lc_dimarc.cpp
        librecad/src/lib/engine/lc_dimarc.h
        librecad/src/lib/engine/lc_hyperbola.cpp
        librecad/src/lib/engine/lc_hyperbola.h
        librecad/src/lib/engine/lc_looputils.cpp
        librecad/src/lib/engine/lc_looputils.h
        librecad/src/lib/engine/lc_rect.cpp
        librecad/src/lib/engine/lc_rect.h
        librecad/src/lib/engine/lc_splinepoints.cpp
        librecad/src/lib/engine/lc_splinepoints.h
        librecad/src/lib/engine/lc_undosection.cpp
        librecad/src/lib/engine/lc_undosection.h
        librecad/src/lib/engine/rs.cpp
        librecad/src/lib/engine/rs.h
        librecad/src/lib/engine/rs_arc.cpp
        librecad/src/lib/engine/rs_arc.h
        librecad/src/lib/engine/rs_atomicentity.cpp
        librecad/src/lib/engine/rs_atomicentity.h
        librecad/src/lib/engine/rs_block.cpp
        librecad/src/lib/engine/rs_block.h
        librecad/src/lib/engine/rs_blocklist.cpp
        librecad/src/lib/engine/rs_blocklist.h
        librecad/src/lib/engine/rs_blocklistlistener.h
        librecad/src/lib/engine/rs_circle.cpp
        librecad/src/lib/engine/rs_circle.h
        librecad/src/lib/engine/rs_clipboard.cpp
        librecad/src/lib/engine/rs_clipboard.h
        librecad/src/lib/engine/rs_color.cpp
        librecad/src/lib/engine/rs_color.h
        librecad/src/lib/engine/rs_constructionline.cpp
        librecad/src/lib/engine/rs_constructionline.h
        librecad/src/lib/engine/rs_dimaligned.cpp
        librecad/src/lib/engine/rs_dimaligned.h
        librecad/src/lib/engine/rs_dimangular.cpp
        librecad/src/lib/engine/rs_dimangular.h
        librecad/src/lib/engine/rs_dimdiametric.cpp
        librecad/src/lib/engine/rs_dimdiametric.h
        librecad/src/lib/engine/rs_dimension.cpp
        librecad/src/lib/engine/rs_dimension.h
        librecad/src/lib/engine/rs_dimlinear.cpp
        librecad/src/lib/engine/rs_dimlinear.h
        librecad/src/lib/engine/rs_dimradial.cpp
        librecad/src/lib/engine/rs_dimradial.h
        librecad/src/lib/engine/rs_document.cpp
        librecad/src/lib/engine/rs_document.h
        librecad/src/lib/engine/rs_ellipse.cpp
        librecad/src/lib/engine/rs_ellipse.h
        librecad/src/lib/engine/rs_entity.cpp
        librecad/src/lib/engine/rs_entity.h
        librecad/src/lib/engine/rs_entitycontainer.cpp
        librecad/src/lib/engine/rs_entitycontainer.h
        librecad/src/lib/engine/rs_flags.cpp
        librecad/src/lib/engine/rs_flags.h
        librecad/src/lib/engine/rs_font.cpp
        librecad/src/lib/engine/rs_font.h
        librecad/src/lib/engine/rs_fontchar.h
        librecad/src/lib/engine/rs_fontlist.cpp
        librecad/src/lib/engine/rs_fontlist.h
        librecad/src/lib/engine/rs_graphic.cpp
        librecad/src/lib/engine/rs_graphic.h
        librecad/src/lib/engine/rs_hatch.cpp
        librecad/src/lib/engine/rs_hatch.h
        librecad/src/lib/engine/rs_image.cpp
        librecad/src/lib/engine/rs_image.h
        librecad/src/lib/engine/rs_insert.cpp
        librecad/src/lib/engine/rs_insert.h
        librecad/src/lib/engine/rs_layer.cpp
        librecad/src/lib/engine/rs_layer.h
        librecad/src/lib/engine/rs_layerlist.cpp
        librecad/src/lib/engine/rs_layerlist.h
        librecad/src/lib/engine/rs_layerlistlistener.h
        librecad/src/lib/engine/rs_leader.cpp
        librecad/src/lib/engine/rs_leader.h
        librecad/src/lib/engine/rs_line.cpp
        librecad/src/lib/engine/rs_line.h
        librecad/src/lib/engine/rs_mtext.cpp
        librecad/src/lib/engine/rs_mtext.h
        librecad/src/lib/engine/rs_overlaybox.cpp
        librecad/src/lib/engine/rs_overlaybox.h
        librecad/src/lib/engine/rs_overlayline.cpp
        librecad/src/lib/engine/rs_overlayline.h
        librecad/src/lib/engine/rs_pattern.cpp
        librecad/src/lib/engine/rs_pattern.h
        librecad/src/lib/engine/rs_patternlist.cpp
        librecad/src/lib/engine/rs_patternlist.h
        librecad/src/lib/engine/rs_pen.cpp
        librecad/src/lib/engine/rs_pen.h
        librecad/src/lib/engine/rs_point.cpp
        librecad/src/lib/engine/rs_point.h
        librecad/src/lib/engine/rs_polyline.cpp
        librecad/src/lib/engine/rs_polyline.h
        librecad/src/lib/engine/rs_settings.cpp
        librecad/src/lib/engine/rs_settings.h
        librecad/src/lib/engine/rs_solid.cpp
        librecad/src/lib/engine/rs_solid.h
        librecad/src/lib/engine/rs_spline.cpp
        librecad/src/lib/engine/rs_spline.h
        librecad/src/lib/engine/rs_system.cpp
        librecad/src/lib/engine/rs_system.h
        librecad/src/lib/engine/rs_text.cpp
        librecad/src/lib/engine/rs_text.h
        librecad/src/lib/engine/rs_undo.cpp
        librecad/src/lib/engine/rs_undo.h
        librecad/src/lib/engine/rs_undoable.cpp
        librecad/src/lib/engine/rs_undoable.h
        librecad/src/lib/engine/rs_undocycle.cpp
        librecad/src/lib/engine/rs_undocycle.h
        librecad/src/lib/engine/rs_units.cpp
        librecad/src/lib/engine/rs_units.h
        librecad/src/lib/engine/rs_utility.cpp
        librecad/src/lib/engine/rs_utility.h
        librecad/src/lib/engine/rs_variable.h
        librecad/src/lib/engine/rs_variabledict.cpp
        librecad/src/lib/engine/rs_variabledict.h
        librecad/src/lib/engine/rs_vector.cpp
        librecad/src/lib/engine/rs_vector.h
        librecad/src/lib/fileio/rs_fileio.cpp
        librecad/src/lib/fileio/rs_fileio.h
        librecad/src/lib/filters/rs_filtercxf.cpp
#        librecad/src/lib/filters/rs_filtercxf.h
#        librecad/src/lib/filters/rs_filterdxf.cpp
#        librecad/src/lib/filters/rs_filterdxf.h
        librecad/src/lib/filters/rs_filterdxf1.cpp
        librecad/src/lib/filters/rs_filterdxf1.h
        librecad/src/lib/filters/rs_filterdxfrw.cpp
        librecad/src/lib/filters/rs_filterdxfrw.h
        librecad/src/lib/filters/rs_filterinterface.h
        librecad/src/lib/filters/rs_filterjww.cpp
        librecad/src/lib/filters/rs_filterjww.h
        librecad/src/lib/filters/rs_filterlff.cpp
        librecad/src/lib/filters/rs_filterlff.h
        librecad/src/lib/generators/lc_makercamsvg.cpp
        librecad/src/lib/generators/lc_makercamsvg.h
        librecad/src/lib/generators/lc_xmlwriterinterface.h
        librecad/src/lib/generators/lc_xmlwriterqxmlstreamwriter.cpp
        librecad/src/lib/generators/lc_xmlwriterqxmlstreamwriter.h
        librecad/src/lib/gui/rs_commandevent.h
        librecad/src/lib/gui/rs_coordinateevent.h
        librecad/src/lib/gui/rs_dialogfactory.cpp
        librecad/src/lib/gui/rs_dialogfactory.h
        librecad/src/lib/gui/rs_dialogfactoryadapter.h
        librecad/src/lib/gui/rs_dialogfactoryinterface.h
        librecad/src/lib/gui/rs_eventhandler.cpp
        librecad/src/lib/gui/rs_eventhandler.h
        librecad/src/lib/gui/rs_graphicview.cpp
        librecad/src/lib/gui/rs_graphicview.h
        librecad/src/lib/gui/rs_grid.cpp
        librecad/src/lib/gui/rs_grid.h
        librecad/src/lib/gui/rs_linetypepattern.cpp
        librecad/src/lib/gui/rs_linetypepattern.h
        librecad/src/lib/gui/rs_mainwindowinterface.h
        librecad/src/lib/gui/rs_painter.cpp
        librecad/src/lib/gui/rs_painter.h
        librecad/src/lib/gui/rs_painterqt.cpp
        librecad/src/lib/gui/rs_painterqt.h
        librecad/src/lib/gui/rs_staticgraphicview.cpp
        librecad/src/lib/gui/rs_staticgraphicview.h
        librecad/src/lib/information/rs_infoarea.cpp
        librecad/src/lib/information/rs_infoarea.h
        librecad/src/lib/information/rs_information.cpp
        librecad/src/lib/information/rs_information.h
        librecad/src/lib/information/rs_locale.cpp
        librecad/src/lib/information/rs_locale.h
        librecad/src/lib/math/lc_quadratic.cpp
        librecad/src/lib/math/lc_quadratic.h
        librecad/src/lib/math/rs_math.cpp
        librecad/src/lib/math/rs_math.h
        librecad/src/lib/modification/rs_modification.cpp
        librecad/src/lib/modification/rs_modification.h
        librecad/src/lib/modification/rs_selection.cpp
        librecad/src/lib/modification/rs_selection.h
        librecad/src/lib/printing/lc_printing.cpp
        librecad/src/lib/printing/lc_printing.h
        librecad/src/lib/scripting/rs_python.cpp
        librecad/src/lib/scripting/rs_python.h
        librecad/src/lib/scripting/rs_python_wrappers.cpp
        librecad/src/lib/scripting/rs_python_wrappers.h
        librecad/src/lib/scripting/rs_script.cpp
        librecad/src/lib/scripting/rs_script.h
        librecad/src/lib/scripting/rs_scriptlist.cpp
        librecad/src/lib/scripting/rs_scriptlist.h
        librecad/src/lib/scripting/rs_simplepython.cpp
        librecad/src/lib/scripting/rs_simplepython.h
        librecad/src/main/console_dxf2pdf/console_dxf2pdf.cpp
        librecad/src/main/console_dxf2pdf/console_dxf2pdf.h
        librecad/src/main/console_dxf2pdf/pdf_print_loop.cpp
        librecad/src/main/console_dxf2pdf/pdf_print_loop.h
        librecad/src/main/console_dxf2png.cpp
        librecad/src/main/console_dxf2png.h
        librecad/src/main/doc_plugin_interface.cpp
        librecad/src/main/doc_plugin_interface.h
	#librecad/src/main/emu_c99.cpp
	#librecad/src/main/emu_c99.h
#        librecad/src/main/helpbrowser.cpp
#        librecad/src/main/helpbrowser.h
        librecad/src/main/lc_application.cpp
        librecad/src/main/lc_application.h
        librecad/src/main/main.cpp
        librecad/src/main/main.h
		librecad/src/ui/main/mainwindowx.cpp
		librecad/src/ui/main/mainwindowx.h
		librecad/src/ui/main/qc_applicationwindow.cpp
		librecad/src/ui/main/qc_applicationwindow.h
        librecad/src/main/qc_dialogfactory.cpp
        librecad/src/main/qc_dialogfactory.h
#        librecad/src/main/qc_graphicview.cpp
#        librecad/src/main/qc_graphicview.h
		librecad/src/ui/main/qc_mdiwindow.cpp
		librecad/src/ui/main/qc_mdiwindow.h
        librecad/src/plugins/intern/qc_actiongetent.cpp
        librecad/src/plugins/intern/qc_actiongetent.h
        librecad/src/plugins/intern/qc_actiongetpoint.cpp
        librecad/src/plugins/intern/qc_actiongetpoint.h
        librecad/src/plugins/intern/qc_actiongetselect.cpp
        librecad/src/plugins/intern/qc_actiongetselect.h
        librecad/src/plugins/document_interface.h
        librecad/src/plugins/qc_plugininterface.h
#        librecad/src/test/lc_simpletests.cpp
#        librecad/src/test/lc_simpletests.h
#        librecad/src/ui/forms/lc_cadtoolbarinterface.cpp
#        librecad/src/ui/forms/lc_cadtoolbarinterface.h
		librecad/src/ui/dialogs/entity/lc_dlgsplinepoints.cpp
		librecad/src/ui/dialogs/entity/lc_dlgsplinepoints.h
		librecad/src/ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.cpp
		librecad/src/ui/dialogs/settings/options_widget/lc_widgetoptionsdialog.h
		librecad/src/ui/components/status_bar/qg_activelayername.cpp
		librecad/src/ui/components/status_bar/qg_activelayername.h
		librecad/src/ui/action_options/curve/qg_arcoptions.cpp
		librecad/src/ui/action_options/curve/qg_arcoptions.h
		librecad/src/ui/action_options/curve/qg_arctangentialoptions.cpp
		librecad/src/ui/action_options/curve/qg_arctangentialoptions.h
		librecad/src/ui/action_options/modify/qg_beveloptions.cpp
		librecad/src/ui/action_options/modify/qg_beveloptions.h
		librecad/src/ui/dialogs/entity/qg_blockdialog.cpp
		librecad/src/ui/dialogs/entity/qg_blockdialog.h
#        librecad/src/ui/forms/qg_cadtoolbar.cpp
#        librecad/src/ui/forms/qg_cadtoolbar.h
#        librecad/src/ui/forms/qg_cadtoolbararcs.cpp
#        librecad/src/ui/forms/qg_cadtoolbararcs.h
#        librecad/src/ui/forms/qg_cadtoolbarcircles.cpp
#        librecad/src/ui/forms/qg_cadtoolbarcircles.h
#        librecad/src/ui/forms/qg_cadtoolbardim.cpp
#        librecad/src/ui/forms/qg_cadtoolbardim.h
#        librecad/src/ui/forms/qg_cadtoolbarellipses.cpp
#        librecad/src/ui/forms/qg_cadtoolbarellipses.h
#        librecad/src/ui/forms/qg_cadtoolbarinfo.cpp
#        librecad/src/ui/forms/qg_cadtoolbarinfo.h
#        librecad/src/ui/forms/qg_cadtoolbarlines.cpp
#        librecad/src/ui/forms/qg_cadtoolbarlines.h
#        librecad/src/ui/forms/qg_cadtoolbarmain.cpp
#        librecad/src/ui/forms/qg_cadtoolbarmain.h
#        librecad/src/ui/forms/qg_cadtoolbarmodify.cpp
#        librecad/src/ui/forms/qg_cadtoolbarmodify.h
#        librecad/src/ui/forms/qg_cadtoolbarpolylines.cpp
#        librecad/src/ui/forms/qg_cadtoolbarpolylines.h
#        librecad/src/ui/forms/qg_cadtoolbarselect.cpp
#        librecad/src/ui/forms/qg_cadtoolbarselect.h
#        librecad/src/ui/forms/qg_cadtoolbarsplines.cpp
#        librecad/src/ui/forms/qg_cadtoolbarsplines.h
		librecad/src/ui/action_options/circle/qg_circleoptions.cpp
		librecad/src/ui/action_options/circle/qg_circleoptions.h
		librecad/src/ui/action_options/circle/qg_circletan2options.cpp
		librecad/src/ui/action_options/circle/qg_circletan2options.h
		librecad/src/ui/dock_widgets/command_line/qg_commandwidget.cpp
		librecad/src/ui/dock_widgets/command_line/qg_commandwidget.h
		librecad/src/ui/components/status_bar/qg_coordinatewidget.cpp
		librecad/src/ui/components/status_bar/qg_coordinatewidget.h
		librecad/src/ui/dialogs/entity/qg_dimensionlabeleditor.cpp
		librecad/src/ui/dialogs/entity/qg_dimensionlabeleditor.h
		#[[	librecad/src/ui/not_used/qg_dimlinearoptions.cpp
            librecad/src/ui/not_used/qg_dimlinearoptions.h]]
		librecad/src/ui/action_options/dimensions/qg_dimoptions.cpp
		librecad/src/ui/action_options/dimensions/qg_dimoptions.h
		librecad/src/ui/dialogs/entity/qg_dlgarc.cpp
		librecad/src/ui/dialogs/entity/qg_dlgarc.h
		librecad/src/ui/dialogs/entity/qg_dlgattributes.cpp
		librecad/src/ui/dialogs/entity/qg_dlgattributes.h
		librecad/src/ui/dialogs/entity/qg_dlgcircle.cpp
		librecad/src/ui/dialogs/entity/qg_dlgcircle.h
		librecad/src/ui/dialogs/entity/qg_dlgdimension.cpp
		librecad/src/ui/dialogs/entity/qg_dlgdimension.h
		librecad/src/ui/dialogs/entity/qg_dlgdimlinear.cpp
		librecad/src/ui/dialogs/entity/qg_dlgdimlinear.h
		librecad/src/ui/dialogs/entity/qg_dlgellipse.cpp
		librecad/src/ui/dialogs/entity/qg_dlgellipse.h
		librecad/src/ui/dialogs/entity/qg_dlghatch.cpp
		librecad/src/ui/dialogs/entity/qg_dlghatch.h
		librecad/src/ui/dialogs/entity/qg_dlgimage.cpp
		librecad/src/ui/dialogs/entity/qg_dlgimage.h
		librecad/src/ui/dialogs/entity/qg_dlgimageoptions.cpp
		librecad/src/ui/dialogs/entity/qg_dlgimageoptions.h
		librecad/src/ui/dialogs/main/qg_dlginitial.cpp
		librecad/src/ui/dialogs/main/qg_dlginitial.h
		librecad/src/ui/dialogs/entity/qg_dlginsert.cpp
		librecad/src/ui/dialogs/entity/qg_dlginsert.h
		librecad/src/ui/dialogs/entity/qg_dlgline.cpp
		librecad/src/ui/dialogs/entity/qg_dlgline.h
		librecad/src/ui/dialogs/actions/modify/qg_dlgmirror.cpp
		librecad/src/ui/dialogs/actions/modify/qg_dlgmirror.h
		librecad/src/ui/dialogs/actions/modify/qg_dlgmove.cpp
		librecad/src/ui/dialogs/actions/modify/qg_dlgmove.h
		librecad/src/ui/dialogs/actions/modify/qg_dlgmoverotate.cpp
		librecad/src/ui/dialogs/actions/modify/qg_dlgmoverotate.h
		librecad/src/ui/dialogs/entity/qg_dlgmtext.cpp
		librecad/src/ui/dialogs/entity/qg_dlgmtext.h
		librecad/src/ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.cpp
		librecad/src/ui/dialogs/settings/options_drawing/qg_dlgoptionsdrawing.h
		librecad/src/ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.cpp
		librecad/src/ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.h
		librecad/src/ui/dialogs/file/export/qg_dlgoptionsmakercam.cpp
		librecad/src/ui/dialogs/file/export/qg_dlgoptionsmakercam.h
#        librecad/src/ui/forms/qg_dlgoptionsvariables.cpp
#        librecad/src/ui/forms/qg_dlgoptionsvariables.h
		librecad/src/ui/dialogs/entity/qg_dlgpoint.cpp
		librecad/src/ui/dialogs/entity/qg_dlgpoint.h
		librecad/src/ui/dialogs/entity/qg_dlgpolyline.cpp
		librecad/src/ui/dialogs/entity/qg_dlgpolyline.h
		librecad/src/ui/dialogs/actions/modify/qg_dlgrotate.cpp
		librecad/src/ui/dialogs/actions/modify/qg_dlgrotate.h
		librecad/src/ui/dialogs/actions/modify/qg_dlgrotate2.cpp
		librecad/src/ui/dialogs/actions/modify/qg_dlgrotate2.h
		librecad/src/ui/dialogs/actions/modify/qg_dlgscale.cpp
		librecad/src/ui/dialogs/actions/modify/qg_dlgscale.h
		librecad/src/ui/dialogs/entity/qg_dlgspline.cpp
		librecad/src/ui/dialogs/entity/qg_dlgspline.h
		librecad/src/ui/dialogs/entity/qg_dlgtext.cpp
		librecad/src/ui/dialogs/entity/qg_dlgtext.h
		librecad/src/ui/dialogs/main/qg_exitdialog.cpp
		librecad/src/ui/dialogs/main/qg_exitdialog.h
		librecad/src/ui/action_options/image/qg_imageoptions.cpp
		librecad/src/ui/action_options/image/qg_imageoptions.h
		librecad/src/ui/action_options/insert/qg_insertoptions.cpp
		librecad/src/ui/action_options/insert/qg_insertoptions.h
		librecad/src/ui/dialogs/actions/qg_layerdialog.cpp
		librecad/src/ui/dialogs/actions/qg_layerdialog.h
		librecad/src/ui/dock_widgets/layers_tree/lc_layerdialog_ex.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layerdialog_ex.h
		librecad/src/ui/action_options/insert/qg_libraryinsertoptions.cpp
		librecad/src/ui/action_options/insert/qg_libraryinsertoptions.h
		librecad/src/ui/action_options/line/qg_lineangleoptions.cpp
		librecad/src/ui/action_options/line/qg_lineangleoptions.h
		librecad/src/ui/action_options/line/qg_linebisectoroptions.cpp
		librecad/src/ui/action_options/line/qg_linebisectoroptions.h
		librecad/src/ui/action_options/line/qg_lineoptions.cpp
		librecad/src/ui/action_options/line/qg_lineoptions.h
		librecad/src/ui/action_options/line/qg_lineparalleloptions.cpp
		librecad/src/ui/action_options/line/qg_lineparalleloptions.h
		librecad/src/ui/action_options/line/qg_lineparallelthroughoptions.cpp
		librecad/src/ui/action_options/line/qg_lineparallelthroughoptions.h
        librecad/src/ui/not_used/qg_linepolygon2options.cpp
        librecad/src/ui/not_used/qg_linepolygon2options.h
		librecad/src/ui/action_options/line/qg_linepolygonoptions.cpp
		librecad/src/ui/action_options/line/qg_linepolygonoptions.h
		librecad/src/ui/action_options/line/qg_linerelangleoptions.cpp
		librecad/src/ui/action_options/line/qg_linerelangleoptions.h
		librecad/src/ui/action_options/modify/qg_modifyoffsetoptions.cpp
		librecad/src/ui/action_options/modify/qg_modifyoffsetoptions.h
		librecad/src/ui/components/status_bar/qg_mousewidget.cpp
		librecad/src/ui/components/status_bar/qg_mousewidget.h
		librecad/src/ui/action_options/modify/qg_moverotateoptions.cpp
		librecad/src/ui/action_options/modify/qg_moverotateoptions.h
		librecad/src/ui/action_options/text/qg_mtextoptions.cpp
		librecad/src/ui/action_options/text/qg_mtextoptions.h
		librecad/src/ui/action_options/polyline/qg_polylineequidistantoptions.cpp
		librecad/src/ui/action_options/polyline/qg_polylineequidistantoptions.h
		librecad/src/ui/action_options/polyline/qg_polylineoptions.cpp
		librecad/src/ui/action_options/polyline/qg_polylineoptions.h
		librecad/src/ui/action_options/print_preview/qg_printpreviewoptions.cpp
		librecad/src/ui/action_options/print_preview/qg_printpreviewoptions.h
		librecad/src/ui/action_options/modify/qg_roundoptions.cpp
		librecad/src/ui/action_options/modify/qg_roundoptions.h
		librecad/src/ui/components/status_bar/qg_selectionwidget.cpp
		librecad/src/ui/components/status_bar/qg_selectionwidget.h
		librecad/src/ui/action_options/snap/qg_snapdistoptions.cpp
		librecad/src/ui/action_options/snap/qg_snapdistoptions.h
		librecad/src/ui/action_options/snap/qg_snapmiddleoptions.cpp
		librecad/src/ui/action_options/snap/qg_snapmiddleoptions.h
		librecad/src/ui/components/toolbars/qg_snaptoolbar.cpp
		librecad/src/ui/components/toolbars/qg_snaptoolbar.h
		librecad/src/ui/action_options/curve/qg_splineoptions.cpp
		librecad/src/ui/action_options/curve/qg_splineoptions.h
		librecad/src/ui/action_options/text/qg_textoptions.cpp
		librecad/src/ui/action_options/text/qg_textoptions.h
		librecad/src/ui/action_options/modify/qg_trimamountoptions.cpp
		librecad/src/ui/action_options/modify/qg_trimamountoptions.h
		librecad/src/ui/components/pen/qg_widgetpen.cpp
		librecad/src/ui/components/pen/qg_widgetpen.h
 		librecad/src/ui/components/creators/actionlist.cpp
		librecad/src/ui/components/creators/actionlist.h
		librecad/src/ui/dock_widgets/pen_wizard/colorcombobox.cpp
		librecad/src/ui/dock_widgets/pen_wizard/colorcombobox.h
		librecad/src/ui/dock_widgets/pen_wizard/colorwizard.cpp
		librecad/src/ui/dock_widgets/pen_wizard/colorwizard.h
		librecad/src/ui/components/comboboxes/comboboxoption.cpp
		librecad/src/ui/components/comboboxes/comboboxoption.h
		librecad/src/ui/dialogs/entity/LC_DlgParabola.h
		librecad/src/ui/dialogs/entity/LC_DlgParabola.cpp
#        librecad/src/ui/generic/customtoolbarcreator.cpp
#        librecad/src/ui/generic/customtoolbarcreator.h
#        librecad/src/ui/generic/customwidgetcreator.cpp
#        librecad/src/ui/generic/customwidgetcreator.h
#		librecad/src/ui/not_used/linklist.cpp
#		librecad/src/ui/not_used/linklist.h
		librecad/src/ui/components/textfileviewer.cpp
		librecad/src/ui/components/textfileviewer.h
		librecad/src/ui/components/creators/widgetcreator.cpp
		librecad/src/ui/components/creators/widgetcreator.h
        librecad/src/ui/lc_actionfactory.cpp
        librecad/src/ui/lc_actionfactory.h
		librecad/src/ui/actions/lc_actiongroupmanager.cpp
		librecad/src/ui/actions/lc_actiongroupmanager.h
		librecad/src/ui/view/lc_centralwidget.cpp
		librecad/src/ui/view/lc_centralwidget.h
#		librecad/src/ui/not_used/lc_customtoolbar.cpp
#		librecad/src/ui/not_used/lc_customtoolbar.h
		librecad/src/ui/dialogs/settings/options_device/lc_deviceoptions.cpp
		librecad/src/ui/dialogs/settings/options_device/lc_deviceoptions.h
		librecad/src/ui/dock_widgets/lc_dockwidget.cpp
		librecad/src/ui/dock_widgets/lc_dockwidget.h
		librecad/src/ui/dialogs/file/lc_filedialogservice.cpp
		librecad/src/ui/dialogs/file/lc_filedialogservice.h
		librecad/src/ui/dock_widgets/pen_wizard/lc_penwizard.cpp
		librecad/src/ui/dock_widgets/pen_wizard/lc_penwizard.h
        librecad/src/ui/lc_widgetfactory.cpp
        librecad/src/ui/lc_widgetfactory.h
		librecad/src/ui/dialogs/settings/options_drawing/lg_dimzerosbox.cpp
		librecad/src/ui/dialogs/settings/options_drawing/lg_dimzerosbox.h
        librecad/src/ui/qg_actionhandler.cpp
        librecad/src/ui/qg_actionhandler.h
		librecad/src/ui/dock_widgets/block_widget/qg_blockwidget.cpp
		librecad/src/ui/dock_widgets/block_widget/qg_blockwidget.h
		librecad/src/ui/components/comboboxes/qg_colorbox.cpp
		librecad/src/ui/components/comboboxes/qg_colorbox.h
		librecad/src/ui/dock_widgets/command_line/qg_commandedit.cpp
		librecad/src/ui/dock_widgets/command_line/qg_commandedit.h
		librecad/src/ui/dock_widgets/command_line/qg_commandhistory.cpp
		librecad/src/ui/dock_widgets/command_line/qg_commandhistory.h
		librecad/src/ui/dialogs/qg_dialogfactory.cpp
		librecad/src/ui/dialogs/qg_dialogfactory.h
		librecad/src/ui/dialogs/file/qg_filedialog.cpp
		librecad/src/ui/dialogs/file/qg_filedialog.h
		librecad/src/ui/components/comboboxes/qg_fontbox.cpp
		librecad/src/ui/components/comboboxes/qg_fontbox.h
		librecad/src/ui/view/qg_graphicview.cpp
		librecad/src/ui/view/qg_graphicview.h
		librecad/src/ui/components/comboboxes/qg_layerbox.cpp
		librecad/src/ui/components/comboboxes/qg_layerbox.h
		librecad/src/ui/dock_widgets/layer_widget/qg_layerwidget.cpp
		librecad/src/ui/dock_widgets/layer_widget/qg_layerwidget.h
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreeitem.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreeitem.h
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreemodel.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreemodel.h
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreewidget.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreewidget.h
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreeview.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreeview.h
		librecad/src/ui/dock_widgets/library_widget/qg_librarywidget.cpp
		librecad/src/ui/dock_widgets/library_widget/qg_librarywidget.h
		librecad/src/ui/components/comboboxes/qg_linetypebox.cpp
		librecad/src/ui/components/comboboxes/qg_linetypebox.h
#		librecad/src/ui/not_used/qg_mainwindowinterface.h
		librecad/src/ui/components/comboboxes/qg_patternbox.cpp
		librecad/src/ui/components/comboboxes/qg_patternbox.h
		librecad/src/ui/components/toolbars/qg_pentoolbar.cpp
		librecad/src/ui/components/toolbars/qg_pentoolbar.h
		librecad/src/ui/main/qg_recentfiles.cpp
		librecad/src/ui/main/qg_recentfiles.h
		librecad/src/ui/components/qg_scrollbar.h
		librecad/src/ui/components/comboboxes/qg_widthbox.cpp
		librecad/src/ui/components/comboboxes/qg_widthbox.h
		librecad/src/ui/components/status_bar/twostackedlabels.cpp
		librecad/src/ui/components/status_bar/twostackedlabels.h
		librecad/src/ui/dock_widgets/pen_palette/lc_penitem.cpp
		librecad/src/ui/dock_widgets/pen_palette/lc_penitem.h
		librecad/src/ui/dock_widgets/pen_palette/lc_penpalettewidget.cpp
		librecad/src/ui/dock_widgets/pen_palette/lc_penpalettewidget.h
		librecad/src/ui/dock_widgets/pen_palette/lc_peninforegistry.h
		librecad/src/ui/dock_widgets/pen_palette/lc_peninforegistry.cpp
		librecad/src/ui/dock_widgets/pen_palette/lc_penpalettemodel.cpp
		librecad/src/ui/dock_widgets/pen_palette/lc_penpaletteoptions.cpp
		librecad/src/ui/dock_widgets/pen_palette/lc_penpalettedata.h
		librecad/src/ui/dock_widgets/pen_palette/lc_penpalettedata.cpp
		librecad/src/ui/dock_widgets/pen_palette/lc_penpaletteoptionsdialog.cpp
		librecad/src/actions/drawing/draw/curve/lc_actiondrawparabola4points.cpp
		librecad/src/actions/drawing/draw/curve/lc_actiondrawparabolaFD.cpp
		librecad/src/ui/dialogs/entity/LC_DlgParabola.cpp
        librecad/src/lib/engine/lc_parabola.cpp
		librecad/src/actions/drawing/snap/lc_actionsnapmiddlemanual.cpp
		librecad/src/actions/drawing/pen/lc_actionpenapply.cpp
		librecad/src/actions/drawing/pen/lc_actionpenpick.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreeoptionsdialog.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreemodel_options.cpp
		librecad/src/ui/dock_widgets/layers_tree/lc_layertreemodel_options.cpp
		librecad/src/actions/drawing/pen/lc_actionpensyncactivebylayer.h
		librecad/src/actions/drawing/pen/lc_actionpensyncactivebylayer.cpp
		librecad/src/ui/components/layouts/lc_flexlayout.h
		librecad/src/ui/components/layouts/lc_flexlayout.cpp
		librecad/src/ui/components/layouts/lc_flexlayout.cpp
		librecad/src/actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle3points.cpp
		librecad/src/actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle3points.h
		librecad/src/actions/drawing/draw/line/lc_actiondrawcross.cpp
		librecad/src/actions/drawing/draw/line/lc_actiondrawcross.h
		librecad/src/ui/action_options/line/lc_crossoptions.cpp
		librecad/src/actions/drawing/draw/line/lc_actiondrawlinesnake.h
		librecad/src/actions/drawing/draw/line/lc_actiondrawlinesnake.h
		librecad/src/actions/drawing/draw/line/lc_actiondrawlinesnake.cpp
		librecad/src/ui/action_options/line/lc_lineoptions.cpp
		librecad/src/actions/drawing/draw/line/lc_actiondrawlineanglerel.h
		librecad/src/actions/drawing/draw/line/lc_actiondrawlineanglerel.cpp
		librecad/src/ui/action_options/line/lc_lineanglereloptions.cpp
		librecad/src/ui/action_options/line/lc_slicedivideoptions.cpp
		librecad/src/actions/drawing/draw/line/lc_actiondrawslicedivide.cpp
		librecad/src/actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle1point.cpp
		librecad/src/ui/action_options/line/lc_rectangle1pointoptions.cpp
        librecad/src/lib/engine/lc_parabola.cpp
		librecad/src/actions/drawing/draw/curve/lc_actiondrawparabola4points.cpp
		librecad/src/actions/drawing/draw/curve/lc_actiondrawparabolaFD.cpp
		librecad/src/ui/action_options/lc_actionoptionswidget.h
		librecad/src/ui/action_options/lc_actionoptionswidget.cpp
		librecad/src/actions/drawing/draw/line/shapes/rect/lc_abstractactiondrawrectangle.h
		librecad/src/actions/drawing/draw/line/shapes/rect/lc_abstractactiondrawrectangle.cpp
		librecad/src/actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle2points.h
		librecad/src/actions/drawing/draw/line/shapes/rect/lc_actiondrawrectangle2points.cpp
		librecad/src/ui/action_options/line/lc_rectangle2pointsoptions.h
		librecad/src/ui/action_options/line/lc_rectangle2pointsoptions.cpp
		librecad/src/ui/action_options/circle/lc_circlebyarcoptions.cpp
		librecad/src/actions/drawing/draw/circle/lc_actiondrawcirclebyarc.cpp
		librecad/src/actions/drawing/modify/lc_actionmodifylinejoin.cpp
		librecad/src/ui/action_options/modify/lc_linejoinoptions.cpp
		librecad/src/actions/drawing/draw/line/lc_actiondrawlinepoints.h
		librecad/src/actions/drawing/draw/line/lc_actiondrawlinepoints.cpp
		librecad/src/ui/action_options/line/lc_linepointsoptions.h
		librecad/src/ui/action_options/line/lc_linepointsoptions.cpp
        librecad/src/lib/math/lc_linemath.h
        librecad/src/lib/math/lc_linemath.cpp
		librecad/src/actions/drawing/draw/line/lc_abstractactiondrawline.cpp
		librecad/src/actions/drawing/lc_abstractactionwithpreview.h
		librecad/src/actions/drawing/lc_abstractactionwithpreview.cpp
		librecad/src/actions/drawing/modify/lc_actionmodifyduplicate.h
		librecad/src/actions/drawing/modify/lc_actionmodifyduplicate.cpp
		librecad/src/ui/action_options/modify/lc_duplicateoptions.cpp
		librecad/src/ui/action_options/line/lc_rectangle3pointsoptions.cpp
		librecad/src/ui/action_options/line/lc_linefrompointtolineoptions.cpp
		librecad/src/actions/drawing/draw/line/lc_actiondrawlinefrompointtoline.cpp
		librecad/src/actions/drawing/draw/line/shapes/polygon/lc_actiondrawstar.cpp
		librecad/src/ui/action_options/line/lc_staroptions.cpp
		librecad/src/actions/drawing/modify/lc_actionmodifybreakdivide.cpp
		librecad/src/actions/drawing/modify/lc_actionmodifybreakdivide.h
		librecad/src/ui/action_options/modify/lc_modifybreakdivideoptions.h
		librecad/src/ui/action_options/modify/lc_modifybreakdivideoptions.cpp
		librecad/src/actions/drawing/modify/lc_actionmodifylinegap.cpp
		librecad/src/ui/action_options/modify/lc_modifygapoptions.h
		librecad/src/ui/action_options/modify/lc_modifygapoptions.cpp
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfowidget.cpp
		librecad/src/ui/components/lc_plaintextedit.h
		librecad/src/actions/drawing/info/lc_actioninfoproperties.cpp
		librecad/src/actions/dock_widgets/entity_info/lc_actioninfopickcoordinates.cpp
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfopointsdata.h
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfopointsdata.cpp
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfobasedata.cpp
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfoentitydata.cpp
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfowidgetoptionsdialog.cpp
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfowidgetoptions.h
		librecad/src/ui/dock_widgets/entity_info/lc_quickinfowidgetoptions.cpp
		librecad/src/actions/drawing/draw/circle/lc_actiondrawcirclebase.h
		librecad/src/actions/drawing/draw/circle/lc_actiondrawcirclebase.h
		librecad/src/actions/drawing/draw/circle/lc_actiondrawcirclebase.cpp
		librecad/src/actions/drawing/draw/circle/lc_actiondrawcirclebase.cpp
        librecad/src/lib/actions/lc_highlight.cpp
		librecad/src/actions/drawing/draw/polyline/lc_actionpolylinedeletebase.cpp
		librecad/src/ui/action_options/info/lc_infodist2options.cpp
		librecad/src/ui/action_options/info/lc_infodist2options.cpp
		librecad/src/actions/drawing/draw/dimensions/lc_actiondimlinearbase.cpp
        librecad/src/lib/engine/lc_refpoint.cpp
        librecad/src/lib/engine/lc_refpoint.h
        librecad/src/lib/engine/lc_refpoint.cpp
        librecad/src/lib/engine/lc_refline.h
        librecad/src/lib/engine/lc_refline.cpp
        librecad/src/lib/engine/lc_refcircle.h
        librecad/src/lib/engine/lc_refcircle.cpp
        librecad/src/lib/engine/lc_refarc.cpp
        librecad/src/lib/engine/lc_refarc.h
		librecad/src/ui/action_options/lc_actionoptionswidgetbase.cpp
		librecad/src/ui/action_options/lc_actionoptionswidgetbase.h
		librecad/src/actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygonbase.h
		librecad/src/actions/drawing/draw/line/shapes/polygon/lc_actiondrawlinepolygonbase.cpp
        librecad/src/lib/engine/lc_refellipse.cpp
		librecad/src/ui/action_options/modify/lc_modifymirroroptions.cpp
		librecad/src/actions/drawing/modify/lc_actionmodifyselectionbase.h
		librecad/src/actions/drawing/modify/lc_actionmodifyselectionbase.cpp
        librecad/src/lib/actions/lc_modifiersinfo.h
        librecad/src/lib/actions/lc_modifiersinfo.cpp
        librecad/src/lib/actions/lc_modifiersinfo.h
        librecad/src/lib/actions/lc_modifiersinfo.h
		librecad/src/actions/drawing/modify/lc_actionpreselectionawarebase.h
		librecad/src/actions/drawing/modify/lc_actionpreselectionawarebase.cpp
		librecad/src/ui/action_options/modify/lc_modifystretchoptions.cpp
		librecad/src/ui/action_options/modify/lc_modifystretchoptions.h
		librecad/src/ui/action_options/modify/lc_moveoptions.cpp
		librecad/src/ui/action_options/modify/lc_moveoptions.h
		librecad/src/actions/drawing/modify/lc_actionmodifybase.cpp
		librecad/src/actions/drawing/modify/lc_actionmodifybase.h
		librecad/src/ui/actions/lc_actionfactorybase.h
		librecad/src/ui/actions/lc_actionfactorybase.cpp
		librecad/src/ui/components/containers/lc_optionswidgetsholder.cpp
		librecad/src/ui/components/containers/lc_optionswidgetsholder.h
		librecad/src/ui/components/containers/lc_snapoptionswidgetsholder.cpp
		librecad/src/ui/components/containers/lc_snapoptionswidgetsholder.h
		librecad/src/ui/action_options/modify/lc_rotate2options.cpp
		librecad/src/ui/action_options/modify/lc_rotate2options.h
		librecad/src/actions/drawing/edit/lc_actioneditpastetransform.cpp
		librecad/src/actions/drawing/edit/lc_actioneditpastetransform.h
		librecad/src/ui/action_options/edit/lc_pastetransformoptions.cpp
		librecad/src/ui/action_options/edit/lc_pastetransformoptions.cpp
		librecad/src/ui/action_options/modify/lc_modifyscaleoptions.h
		librecad/src/ui/action_options/modify/lc_modifyscaleoptions.cpp
		librecad/src/ui/action_options/modify/lc_modifyscaleoptions.h
		librecad/src/ui/action_options/modify/lc_modifyrotateoptions.cpp
		librecad/src/actions/drawing/info/lc_actioninfo3pointsangle.h
		librecad/src/actions/drawing/info/lc_actioninfo3pointsangle.cpp
		librecad/src/actions/drawing/info/lc_actioninfo3pointsangle.cpp
		librecad/src/actions/drawing/info/lc_actioninfo3pointsangle.cpp
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcutbutton.cpp
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcutbutton.h
		librecad/src/ui/actions/lc_shortcutinfo.h
		librecad/src/ui/actions/lc_shortcutsstorage.h
		librecad/src/ui/actions/lc_shortcutsstorage.cpp
		librecad/src/ui/dialogs/settings/shortcuts/lc_actionsshortcutsdialog.cpp
		librecad/src/ui/dialogs/settings/shortcuts/lc_actionsshortcutsdialog.h
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcutstreemodel.h
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcutstreemodel.cpp
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcuttreeitem.cpp
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcuttreeitem.h
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcutstreeview.h
		librecad/src/ui/dialogs/settings/shortcuts/lc_shortcutstreeview.cpp
		librecad/src/ui/actions/lc_actiongroup.h
		librecad/src/ui/actions/lc_actiongroup.cpp
		librecad/src/ui/actions/lc_actiongroup.cpp
		librecad/src/ui/actions/lc_shortcutinfo.cpp
		librecad/src/ui/actions/lc_shortcuts_manager.cpp
		librecad/src/ui/actions/lc_shortcuts_manager.h
		librecad/src/actions/drawing/draw/ellipse/lc_actiondrawellipse1point.h
		librecad/src/actions/drawing/draw/ellipse/lc_actiondrawellipse1point.cpp
		librecad/src/ui/action_options/ellipse/lc_ellipse1pointoptions.h
		librecad/src/ui/action_options/ellipse/lc_ellipse1pointoptions.cpp
		librecad/src/actions/drawing/draw/dimensions/lc_actiondrawdimbaseline.h
		librecad/src/actions/drawing/draw/dimensions/lc_actiondrawdimbaseline.cpp
		librecad/src/actions/drawing/draw/dimensions/lc_actioncircledimbase.h
		librecad/src/actions/drawing/draw/dimensions/lc_actioncircledimbase.cpp
		librecad/src/ui/action_options/curve/lc_ellipsearcoptions.cpp
		librecad/src/ui/dialogs/lc_dialog.h
		librecad/src/ui/dialogs/lc_dialog.cpp
		#        plugins/align/align.cpp
#        plugins/align/align.h
#        plugins/asciifile/asciifile.cpp
#        plugins/asciifile/asciifile.h
#        plugins/divide/divide.cpp
#        plugins/divide/divide.h
#        plugins/divide/dividedlg.cpp
#        plugins/divide/dividedlg.h
#        plugins/gear/gear.cpp
#        plugins/gear/gear.h
#        plugins/list/list.cpp
#        plugins/list/list.h
#        plugins/picfile/picfile.cpp
#        plugins/picfile/picfile.h
#        plugins/plotequation/plot.cpp
#        plugins/plotequation/plot.h
#        plugins/plotequation/plotdialog.cpp
#        plugins/plotequation/plotdialog.h
#        plugins/sameprop/sameprop.cpp
#        plugins/sameprop/sameprop.h
#        plugins/sample/sample.cpp
#        plugins/sample/sample.h

        #        tools/ttf2lff/main.cpp )
)


#qt5_wrap_cpp(TARGET LibreCAD ${SOURCES})
#[[
qt6_wrap_ui(SOURCES
./librecad/src/ui/forms/lc_dlgsplinepoints.ui
./librecad/src/ui/forms/qg_beveloptions.ui
./librecad/src/ui/forms/qg_circleoptions.ui
./librecad/src/ui/forms/qg_circletan2options.ui
./librecad/src/ui/forms/qg_dimensionlabeleditor.ui
./librecad/src/ui/forms/qg_dimoptions.ui
./librecad/src/ui/forms/qg_dlgattributes.ui
./librecad/src/ui/forms/qg_dlgcircle.ui
./librecad/src/ui/forms/qg_dlgdimension.ui
./librecad/src/ui/forms/qg_dlgdimlinear.ui
./librecad/src/ui/forms/qg_dlgimageoptions.ui
./librecad/src/ui/forms/qg_dlginitial.ui
./librecad/src/ui/forms/qg_dlginsert.ui
./librecad/src/ui/forms/qg_dlgmirror.ui
./librecad/src/ui/forms/qg_dlgmove.ui
./librecad/src/ui/forms/qg_dlgmoverotate.ui
./librecad/src/ui/forms/qg_dlgoptionsvariables.ui
./librecad/src/ui/forms/qg_dlgpoint.ui
./librecad/src/ui/forms/qg_dlgpolyline.ui
./librecad/src/ui/forms/qg_dlgrotate.ui
./librecad/src/ui/forms/qg_dlgrotate2.ui
./librecad/src/ui/forms/qg_exitdialog.ui
./librecad/src/ui/forms/qg_imageoptions.ui
./librecad/src/ui/forms/qg_layerdialog.ui
./librecad/src/ui/forms/qg_libraryinsertoptions.ui
./librecad/src/ui/forms/qg_linebisectoroptions.ui
./librecad/src/ui/forms/qg_lineoptions.ui
./librecad/src/ui/forms/qg_lineparalleloptions.ui
./librecad/src/ui/forms/qg_lineparallelthroughoptions.ui
./librecad/src/ui/forms/qg_linepolygon2options.ui
./librecad/src/ui/forms/qg_linepolygonoptions.ui
./librecad/src/ui/forms/qg_linerelangleoptions.ui
./librecad/src/ui/forms/qg_modifyoffsetoptions.ui
./librecad/src/ui/forms/qg_moverotateoptions.ui
./librecad/src/ui/forms/qg_mtextoptions.ui
./librecad/src/ui/forms/qg_polylineequidistantoptions.ui
./librecad/src/ui/forms/qg_polylineoptions.ui
./librecad/src/ui/forms/qg_roundoptions.ui
./librecad/src/ui/forms/qg_snapdistoptions.ui
./librecad/src/ui/forms/qg_snapmiddleoptions.ui
./librecad/src/ui/forms/qg_splineoptions.ui
./librecad/src/ui/forms/qg_textoptions.ui
./librecad/src/ui/forms/qg_trimamountoptions.ui
./librecad/src/ui/forms/qg_snaptoolbar.ui
./librecad/src/ui/forms/qg_printpreviewoptions.ui
./librecad/src/ui/forms/qg_insertoptions.ui
./librecad/src/ui/forms/qg_blockdialog.ui
./librecad/src/ui/forms/LC_DlgParabola.ui
./librecad/src/ui/forms/lc_circlebyarcoptions.ui
./librecad/src/ui/forms/lc_crossoptions.ui
./librecad/src/ui/forms/lc_duplicateoptions.ui
./librecad/src/ui/forms/lc_layerdialog_ex.ui
./librecad/src/ui/forms/lc_layertreeoptionsdialog.ui
./librecad/src/ui/forms/lc_lineanglereloptions.ui
./librecad/src/ui/forms/lc_linefrompointtolineoptions.ui
./librecad/src/ui/forms/lc_linejoinoptions.ui
./librecad/src/ui/forms/lc_lineoptions.ui
./librecad/src/ui/forms/lc_linepointsoptions.ui
./librecad/src/ui/forms/lc_modifybreakdivideoptions.ui
./librecad/src/ui/forms/lc_modifygapoptions.ui
./librecad/src/ui/forms/lc_rectangle1pointoptions.ui
./librecad/src/ui/forms/lc_rectangle2pointsoptions.ui
./librecad/src/ui/forms/lc_rectangle3pointsoptions.ui
./librecad/src/ui/forms/lc_slicedivideoptions.ui
./librecad/src/ui/forms/lc_staroptions.ui
./librecad/src/ui/forms/lc_widgetoptionsdialog.ui
./librecad/src/ui/forms/qg_activelayername.ui
./librecad/src/ui/forms/qg_arcoptions.ui
./librecad/src/ui/forms/qg_arctangentialoptions.ui
./librecad/src/ui/forms/qg_commandwidget.ui
./librecad/src/ui/forms/qg_coordinatewidget.ui
./librecad/src/ui/forms/qg_dimlinearoptions.ui
./librecad/src/ui/forms/qg_dlgarc.ui
./librecad/src/ui/forms/qg_dlgellipse.ui
./librecad/src/ui/forms/qg_dlghatch.ui
./librecad/src/ui/forms/qg_dlgimage.ui
./librecad/src/ui/forms/qg_dlgline.ui
./librecad/src/ui/forms/qg_dlgmtext.ui
./librecad/src/ui/forms/qg_dlgoptionsdrawing.ui
./librecad/src/ui/forms/qg_dlgoptionsgeneral.ui
./librecad/src/ui/forms/qg_dlgoptionsmakercam.ui
./librecad/src/ui/forms/qg_dlgscale.ui
./librecad/src/ui/forms/qg_dlgspline.ui
./librecad/src/ui/forms/qg_dlgtext.ui
./librecad/src/ui/forms/qg_lineangleoptions.ui
./librecad/src/ui/forms/qg_mousewidget.ui
./librecad/src/ui/forms/qg_selectionwidget.ui
./librecad/src/ui/forms/qg_widgetpen.ui
./librecad/src/ui/generic/colorwizard.ui
./librecad/src/ui/generic/comboboxoption.ui
./librecad/src/ui/generic/customtoolbarcreator.ui
./librecad/src/ui/generic/customwidgetcreator.ui
./librecad/src/ui/generic/textfileviewer.ui
./librecad/src/ui/generic/widgetcreator.ui
./librecad/src/ui/lc_deviceoptions.ui
./librecad/src/ui/lc_penpaletteoptionsdialog.ui
./librecad/src/ui/lc_penpalettewidget.ui
./librecad/src/ui/lc_quickinfowidget.ui
./librecad/src/ui/lc_quickinfowidgetoptionsdialog.ui)
]]
qt_add_executable(librecad
	${SOURCES})

#add_executable(LibreCAD ${SOURCES})
target_link_libraries(librecad PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::PrintSupport Qt6::Svg)

#set_target_properties(librecad PROPERTIES
#	WIN32_EXECUTABLE ON
#	MACOSX_BUNDLE ON
#   AUTOUIC_OPTIONS "--connections string")
set_property(TARGET librecad PROPERTY AUTOUIC_OPTIONS --connections string)

set(CMAKE_AUTOUIC_OPTIONS --connections string)

#set(autouic_options
#		--connections string
#)

#set_property(TARGET LibreCAD APPEND PROPERTY
#  INTERFACE_AUTOUIC_OPTIONS ${autouic_options}
#)
#target_link_libraries(LibreCAD Qt5::Core Qt5::Widgets Qt5::Gui Qt5::PrintSupport Qt5::Svg)
#target_link_libraries(LibreCAD Qt6::Core Qt6::Widgets Qt6::Gui Qt6::PrintSupport Qt6::Svg)
