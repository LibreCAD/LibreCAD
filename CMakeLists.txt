cmake_minimum_required(VERSION 3.27)
project(LibreCAD)

set(CMAKE_CXX_STANDARD 17)

#set(MIN_GW_PATH_PREFIX "d:\\Qt\\Qt5.14.2\\5.14.2\\mingw73_32")
set(MIN_GW_PATH_PREFIX "d:\\Qt\\QT6\\6.7.0\\mingw_64")
set(BOOST_DIR "c:\\Local\\boost_1_84_0")
set(Boost_INCLUDE_DIR "c:\\Local\\boost_1_84_0")

#set(CMAKE_PREFIX_PATH "d:\\Qt\\Qt5.14.2\\5.14.2\\mingw73_32")
#set(CMAKE_PREFIX_PATH "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt5")
set(CMAKE_PREFIX_PATH "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6")

#set(Qt5_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt5")
set(Qt6_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6")
#set(Qt5Core_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt5Core")
set(Qt6Core_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6Core")
set(Qt6CoreTools_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6CoreTools")
#set(Qt5Svg_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt5Svg")
set(Qt6Svg_DIR "${MIN_GW_PATH_PREFIX}\\lib\\cmake\\Qt6Svg")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC_OPTIONS --connections string)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_compile_definitions(DWGSUPPORT)
add_compile_definitions(MUPARSER_STATIC)

#find_package(Qt5 COMPONENTS Gui Core Widgets PrintSupport SVG REQUIRED)
find_package(Qt6 COMPONENTS Gui Core Widgets PrintSupport SVG REQUIRED)
find_package(Boost REQUIRED COMPONENTS)

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
include_directories(librecad/src/ui/forms)
include_directories(librecad/src/ui/generic)
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

add_executable(LibreCAD
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
        librecad/src/actions/lc_actiondimarc.cpp
        librecad/src/actions/lc_actiondimarc.h
        librecad/src/actions/lc_actiondrawcircle2pr.cpp
        librecad/src/actions/lc_actiondrawcircle2pr.h
        librecad/src/actions/lc_actiondrawlinepolygon3.cpp
        librecad/src/actions/lc_actiondrawlinepolygon3.h
        librecad/src/actions/lc_actiondrawsplinepoints.cpp
        librecad/src/actions/lc_actiondrawsplinepoints.h
        librecad/src/actions/lc_actionfileexportmakercam.cpp
        librecad/src/actions/lc_actionfileexportmakercam.h
        librecad/src/actions/lc_actionlayersexport.cpp
        librecad/src/actions/lc_actionlayersexport.h
        librecad/src/actions/lc_actionlayerstoggleconstruction.cpp
        librecad/src/actions/lc_actionlayerstoggleconstruction.h
        librecad/src/actions/rs_actionblocksadd.cpp
        librecad/src/actions/rs_actionblocksadd.h
        librecad/src/actions/rs_actionblocksattributes.cpp
        librecad/src/actions/rs_actionblocksattributes.h
        librecad/src/actions/rs_actionblockscreate.cpp
        librecad/src/actions/rs_actionblockscreate.h
        librecad/src/actions/rs_actionblocksedit.cpp
        librecad/src/actions/rs_actionblocksedit.h
        librecad/src/actions/rs_actionblocksexplode.cpp
        librecad/src/actions/rs_actionblocksexplode.h
        librecad/src/actions/rs_actionblocksfreezeall.cpp
        librecad/src/actions/rs_actionblocksfreezeall.h
        librecad/src/actions/rs_actionblocksinsert.cpp
        librecad/src/actions/rs_actionblocksinsert.h
        librecad/src/actions/rs_actionblocksremove.cpp
        librecad/src/actions/rs_actionblocksremove.h
        librecad/src/actions/rs_actionblockssave.cpp
        librecad/src/actions/rs_actionblockssave.h
        librecad/src/actions/rs_actionblockstoggleview.cpp
        librecad/src/actions/rs_actionblockstoggleview.h
        librecad/src/actions/rs_actiondefault.cpp
        librecad/src/actions/rs_actiondefault.h
        librecad/src/actions/rs_actiondimaligned.cpp
        librecad/src/actions/rs_actiondimaligned.h
        librecad/src/actions/rs_actiondimangular.cpp
        librecad/src/actions/rs_actiondimangular.h
        librecad/src/actions/rs_actiondimdiametric.cpp
        librecad/src/actions/rs_actiondimdiametric.h
        librecad/src/actions/rs_actiondimension.cpp
        librecad/src/actions/rs_actiondimension.h
        librecad/src/actions/rs_actiondimleader.cpp
        librecad/src/actions/rs_actiondimleader.h
        librecad/src/actions/rs_actiondimlinear.cpp
        librecad/src/actions/rs_actiondimlinear.h
        librecad/src/actions/rs_actiondimradial.cpp
        librecad/src/actions/rs_actiondimradial.h
        librecad/src/actions/rs_actiondrawarc.cpp
        librecad/src/actions/rs_actiondrawarc.h
        librecad/src/actions/rs_actiondrawarc3p.cpp
        librecad/src/actions/rs_actiondrawarc3p.h
        librecad/src/actions/rs_actiondrawarctangential.cpp
        librecad/src/actions/rs_actiondrawarctangential.h
        librecad/src/actions/rs_actiondrawcircle.cpp
        librecad/src/actions/rs_actiondrawcircle.h
        librecad/src/actions/rs_actiondrawcircle2p.cpp
        librecad/src/actions/rs_actiondrawcircle2p.h
        librecad/src/actions/rs_actiondrawcircle3p.cpp
        librecad/src/actions/rs_actiondrawcircle3p.h
        librecad/src/actions/rs_actiondrawcirclecr.cpp
        librecad/src/actions/rs_actiondrawcirclecr.h
        librecad/src/actions/rs_actiondrawcircleinscribe.cpp
        librecad/src/actions/rs_actiondrawcircleinscribe.h
        librecad/src/actions/rs_actiondrawcircletan1_2p.cpp
        librecad/src/actions/rs_actiondrawcircletan1_2p.h
        librecad/src/actions/rs_actiondrawcircletan2.cpp
        librecad/src/actions/rs_actiondrawcircletan2.h
        librecad/src/actions/rs_actiondrawcircletan2_1p.cpp
        librecad/src/actions/rs_actiondrawcircletan2_1p.h
        librecad/src/actions/rs_actiondrawcircletan3.cpp
        librecad/src/actions/rs_actiondrawcircletan3.h
        librecad/src/actions/rs_actiondrawellipse4points.cpp
        librecad/src/actions/rs_actiondrawellipse4points.h
        librecad/src/actions/rs_actiondrawellipseaxis.cpp
        librecad/src/actions/rs_actiondrawellipseaxis.h
        librecad/src/actions/rs_actiondrawellipsecenter3points.cpp
        librecad/src/actions/rs_actiondrawellipsecenter3points.h
        librecad/src/actions/rs_actiondrawellipsefocipoint.cpp
        librecad/src/actions/rs_actiondrawellipsefocipoint.h
        librecad/src/actions/rs_actiondrawellipseinscribe.cpp
        librecad/src/actions/rs_actiondrawellipseinscribe.h
        librecad/src/actions/rs_actiondrawhatch.cpp
        librecad/src/actions/rs_actiondrawhatch.h
        librecad/src/actions/rs_actiondrawimage.cpp
        librecad/src/actions/rs_actiondrawimage.h
        librecad/src/actions/rs_actiondrawline.cpp
        librecad/src/actions/rs_actiondrawline.h
        librecad/src/actions/rs_actiondrawlineangle.cpp
        librecad/src/actions/rs_actiondrawlineangle.h
        librecad/src/actions/rs_actiondrawlinebisector.cpp
        librecad/src/actions/rs_actiondrawlinebisector.h
        librecad/src/actions/rs_actiondrawlinefree.cpp
        librecad/src/actions/rs_actiondrawlinefree.h
        librecad/src/actions/rs_actiondrawlinehorvert.cpp
        librecad/src/actions/rs_actiondrawlinehorvert.h
        librecad/src/actions/rs_actiondrawlineorthtan.cpp
        librecad/src/actions/rs_actiondrawlineorthtan.h
        librecad/src/actions/rs_actiondrawlineparallel.cpp
        librecad/src/actions/rs_actiondrawlineparallel.h
        librecad/src/actions/rs_actiondrawlineparallelthrough.cpp
        librecad/src/actions/rs_actiondrawlineparallelthrough.h
        librecad/src/actions/rs_actiondrawlinepolygon.cpp
        librecad/src/actions/rs_actiondrawlinepolygon.h
        librecad/src/actions/rs_actiondrawlinepolygon2.cpp
        librecad/src/actions/rs_actiondrawlinepolygon2.h
        librecad/src/actions/rs_actiondrawlinerectangle.cpp
        librecad/src/actions/rs_actiondrawlinerectangle.h
        librecad/src/actions/rs_actiondrawlinerelangle.cpp
        librecad/src/actions/rs_actiondrawlinerelangle.h
        librecad/src/actions/rs_actiondrawlinetangent1.cpp
        librecad/src/actions/rs_actiondrawlinetangent1.h
        librecad/src/actions/rs_actiondrawlinetangent2.cpp
        librecad/src/actions/rs_actiondrawlinetangent2.h
        librecad/src/actions/rs_actiondrawmtext.cpp
        librecad/src/actions/rs_actiondrawmtext.h
        librecad/src/actions/rs_actiondrawpoint.cpp
        librecad/src/actions/rs_actiondrawpoint.h
        librecad/src/actions/rs_actiondrawpolyline.cpp
        librecad/src/actions/rs_actiondrawpolyline.h
        librecad/src/actions/rs_actiondrawspline.cpp
        librecad/src/actions/rs_actiondrawspline.h
        librecad/src/actions/rs_actiondrawtext.cpp
        librecad/src/actions/rs_actiondrawtext.h
        librecad/src/actions/rs_actioneditcopy.cpp
        librecad/src/actions/rs_actioneditcopy.h
        librecad/src/actions/rs_actioneditpaste.cpp
        librecad/src/actions/rs_actioneditpaste.h
        librecad/src/actions/rs_actioneditundo.cpp
        librecad/src/actions/rs_actioneditundo.h
        librecad/src/actions/rs_actionfilenew.cpp
        librecad/src/actions/rs_actionfilenew.h
        librecad/src/actions/rs_actionfilenewtemplate.cpp
        librecad/src/actions/rs_actionfilenewtemplate.h
        librecad/src/actions/rs_actionfileopen.cpp
        librecad/src/actions/rs_actionfileopen.h
        librecad/src/actions/rs_actionfilesave.cpp
        librecad/src/actions/rs_actionfilesave.h
        librecad/src/actions/rs_actionfilesaveas.cpp
        librecad/src/actions/rs_actionfilesaveas.h
        librecad/src/actions/rs_actioninfoangle.cpp
        librecad/src/actions/rs_actioninfoangle.h
        librecad/src/actions/rs_actioninfoarea.cpp
        librecad/src/actions/rs_actioninfoarea.h
        librecad/src/actions/rs_actioninfodist.cpp
        librecad/src/actions/rs_actioninfodist.h
        librecad/src/actions/rs_actioninfodist2.cpp
        librecad/src/actions/rs_actioninfodist2.h
        librecad/src/actions/rs_actioninfoinside.cpp
        librecad/src/actions/rs_actioninfoinside.h
        librecad/src/actions/rs_actioninfototallength.cpp
        librecad/src/actions/rs_actioninfototallength.h
        librecad/src/actions/rs_actionlayersadd.cpp
        librecad/src/actions/rs_actionlayersadd.h
        librecad/src/actions/rs_actionlayersedit.cpp
        librecad/src/actions/rs_actionlayersedit.h
        librecad/src/actions/rs_actionlayersfreezeall.cpp
        librecad/src/actions/rs_actionlayersfreezeall.h
        librecad/src/actions/rs_actionlayerslockall.cpp
        librecad/src/actions/rs_actionlayerslockall.h
        librecad/src/actions/rs_actionlayersremove.cpp
        librecad/src/actions/rs_actionlayersremove.h
        librecad/src/actions/rs_actionlayerstogglelock.cpp
        librecad/src/actions/rs_actionlayerstogglelock.h
        librecad/src/actions/rs_actionlayerstoggleprint.cpp
        librecad/src/actions/rs_actionlayerstoggleprint.h
        librecad/src/actions/rs_actionlayerstoggleview.cpp
        librecad/src/actions/rs_actionlayerstoggleview.h
        librecad/src/actions/rs_actionlibraryinsert.cpp
        librecad/src/actions/rs_actionlibraryinsert.h
        librecad/src/actions/rs_actionlockrelativezero.cpp
        librecad/src/actions/rs_actionlockrelativezero.h
        librecad/src/actions/rs_actionmodifyattributes.cpp
        librecad/src/actions/rs_actionmodifyattributes.h
        librecad/src/actions/rs_actionmodifybevel.cpp
        librecad/src/actions/rs_actionmodifybevel.h
        librecad/src/actions/rs_actionmodifycut.cpp
        librecad/src/actions/rs_actionmodifycut.h
        librecad/src/actions/rs_actionmodifydelete.cpp
        librecad/src/actions/rs_actionmodifydelete.h
        librecad/src/actions/rs_actionmodifydeletefree.cpp
        librecad/src/actions/rs_actionmodifydeletefree.h
        librecad/src/actions/rs_actionmodifydeletequick.cpp
        librecad/src/actions/rs_actionmodifydeletequick.h
        librecad/src/actions/rs_actionmodifyentity.cpp
        librecad/src/actions/rs_actionmodifyentity.h
        librecad/src/actions/rs_actionmodifyexplodetext.cpp
        librecad/src/actions/rs_actionmodifyexplodetext.h
        librecad/src/actions/rs_actionmodifymirror.cpp
        librecad/src/actions/rs_actionmodifymirror.h
        librecad/src/actions/rs_actionmodifymove.cpp
        librecad/src/actions/rs_actionmodifymove.h
        librecad/src/actions/rs_actionmodifymoverotate.cpp
        librecad/src/actions/rs_actionmodifymoverotate.h
        librecad/src/actions/rs_actionmodifyoffset.cpp
        librecad/src/actions/rs_actionmodifyoffset.h
        librecad/src/actions/rs_actionmodifyrevertdirection.cpp
        librecad/src/actions/rs_actionmodifyrevertdirection.h
        librecad/src/actions/rs_actionmodifyrotate.cpp
        librecad/src/actions/rs_actionmodifyrotate.h
        librecad/src/actions/rs_actionmodifyrotate2.cpp
        librecad/src/actions/rs_actionmodifyrotate2.h
        librecad/src/actions/rs_actionmodifyround.cpp
        librecad/src/actions/rs_actionmodifyround.h
        librecad/src/actions/rs_actionmodifyscale.cpp
        librecad/src/actions/rs_actionmodifyscale.h
        librecad/src/actions/rs_actionmodifystretch.cpp
        librecad/src/actions/rs_actionmodifystretch.h
        librecad/src/actions/rs_actionmodifytrim.cpp
        librecad/src/actions/rs_actionmodifytrim.h
        librecad/src/actions/rs_actionmodifytrimamount.cpp
        librecad/src/actions/rs_actionmodifytrimamount.h
        librecad/src/actions/rs_actionoptionsdrawing.cpp
        librecad/src/actions/rs_actionoptionsdrawing.h
        librecad/src/actions/rs_actionorder.cpp
        librecad/src/actions/rs_actionorder.h
        librecad/src/actions/rs_actionpolylineadd.cpp
        librecad/src/actions/rs_actionpolylineadd.h
        librecad/src/actions/rs_actionpolylineappend.cpp
        librecad/src/actions/rs_actionpolylineappend.h
        librecad/src/actions/rs_actionpolylinedel.cpp
        librecad/src/actions/rs_actionpolylinedel.h
        librecad/src/actions/rs_actionpolylinedelbetween.cpp
        librecad/src/actions/rs_actionpolylinedelbetween.h
        librecad/src/actions/rs_actionpolylineequidistant.cpp
        librecad/src/actions/rs_actionpolylineequidistant.h
        librecad/src/actions/rs_actionpolylinesegment.cpp
        librecad/src/actions/rs_actionpolylinesegment.h
        librecad/src/actions/rs_actionpolylinetrim.cpp
        librecad/src/actions/rs_actionpolylinetrim.h
        librecad/src/actions/rs_actionprintpreview.cpp
        librecad/src/actions/rs_actionprintpreview.h
        librecad/src/actions/rs_actionselect.cpp
        librecad/src/actions/rs_actionselect.h
        librecad/src/actions/rs_actionselectall.cpp
        librecad/src/actions/rs_actionselectall.h
        librecad/src/actions/rs_actionselectbase.cpp
        librecad/src/actions/rs_actionselectbase.h
        librecad/src/actions/rs_actionselectcontour.cpp
        librecad/src/actions/rs_actionselectcontour.h
        librecad/src/actions/rs_actionselectintersected.cpp
        librecad/src/actions/rs_actionselectintersected.h
        librecad/src/actions/rs_actionselectinvert.cpp
        librecad/src/actions/rs_actionselectinvert.h
        librecad/src/actions/rs_actionselectlayer.cpp
        librecad/src/actions/rs_actionselectlayer.h
        librecad/src/actions/rs_actionselectsingle.cpp
        librecad/src/actions/rs_actionselectsingle.h
        librecad/src/actions/rs_actionselectwindow.cpp
        librecad/src/actions/rs_actionselectwindow.h
        librecad/src/actions/rs_actionsetrelativezero.cpp
        librecad/src/actions/rs_actionsetrelativezero.h
        librecad/src/actions/rs_actionsetsnapmode.cpp
        librecad/src/actions/rs_actionsetsnapmode.h
        librecad/src/actions/rs_actionsetsnaprestriction.cpp
        librecad/src/actions/rs_actionsetsnaprestriction.h
        librecad/src/actions/rs_actionsnapintersectionmanual.cpp
        librecad/src/actions/rs_actionsnapintersectionmanual.h
        librecad/src/actions/rs_actiontoolregeneratedimensions.cpp
        librecad/src/actions/rs_actiontoolregeneratedimensions.h
        librecad/src/actions/rs_actionzoomauto.cpp
        librecad/src/actions/rs_actionzoomauto.h
        librecad/src/actions/rs_actionzoomautoy.cpp
        librecad/src/actions/rs_actionzoomautoy.h
        librecad/src/actions/rs_actionzoomin.cpp
        librecad/src/actions/rs_actionzoomin.h
        librecad/src/actions/rs_actionzoompan.cpp
        librecad/src/actions/rs_actionzoompan.h
        librecad/src/actions/rs_actionzoomprevious.cpp
        librecad/src/actions/rs_actionzoomprevious.h
        librecad/src/actions/rs_actionzoomredraw.cpp
        librecad/src/actions/rs_actionzoomredraw.h
        librecad/src/actions/rs_actionzoomscroll.cpp
        librecad/src/actions/rs_actionzoomscroll.h
        librecad/src/actions/rs_actionzoomwindow.cpp
        librecad/src/actions/rs_actionzoomwindow.h
        librecad/src/cmd/rs_commands.cpp
        librecad/src/cmd/rs_commands.h
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
        librecad/src/main/emu_c99.cpp
        librecad/src/main/emu_c99.h
#        librecad/src/main/helpbrowser.cpp
#        librecad/src/main/helpbrowser.h
        librecad/src/main/lc_application.cpp
        librecad/src/main/lc_application.h
        librecad/src/main/main.cpp
        librecad/src/main/main.h
        librecad/src/main/mainwindowx.cpp
        librecad/src/main/mainwindowx.h
        librecad/src/main/qc_applicationwindow.cpp
        librecad/src/main/qc_applicationwindow.h
        librecad/src/main/qc_dialogfactory.cpp
        librecad/src/main/qc_dialogfactory.h
#        librecad/src/main/qc_graphicview.cpp
#        librecad/src/main/qc_graphicview.h
        librecad/src/main/qc_mdiwindow.cpp
        librecad/src/main/qc_mdiwindow.h
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
        librecad/src/ui/forms/lc_dlgsplinepoints.cpp
        librecad/src/ui/forms/lc_dlgsplinepoints.h
        librecad/src/ui/forms/lc_widgetoptionsdialog.cpp
        librecad/src/ui/forms/lc_widgetoptionsdialog.h
        librecad/src/ui/forms/qg_activelayername.cpp
        librecad/src/ui/forms/qg_activelayername.h
        librecad/src/ui/forms/qg_arcoptions.cpp
        librecad/src/ui/forms/qg_arcoptions.h
        librecad/src/ui/forms/qg_arctangentialoptions.cpp
        librecad/src/ui/forms/qg_arctangentialoptions.h
        librecad/src/ui/forms/qg_beveloptions.cpp
        librecad/src/ui/forms/qg_beveloptions.h
        librecad/src/ui/forms/qg_blockdialog.cpp
        librecad/src/ui/forms/qg_blockdialog.h
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
        librecad/src/ui/forms/qg_circleoptions.cpp
        librecad/src/ui/forms/qg_circleoptions.h
        librecad/src/ui/forms/qg_circletan2options.cpp
        librecad/src/ui/forms/qg_circletan2options.h
        librecad/src/ui/forms/qg_commandwidget.cpp
        librecad/src/ui/forms/qg_commandwidget.h
        librecad/src/ui/forms/qg_coordinatewidget.cpp
        librecad/src/ui/forms/qg_coordinatewidget.h
        librecad/src/ui/forms/qg_dimensionlabeleditor.cpp
        librecad/src/ui/forms/qg_dimensionlabeleditor.h
        librecad/src/ui/forms/qg_dimlinearoptions.cpp
        librecad/src/ui/forms/qg_dimlinearoptions.h
        librecad/src/ui/forms/qg_dimoptions.cpp
        librecad/src/ui/forms/qg_dimoptions.h
        librecad/src/ui/forms/qg_dlgarc.cpp
        librecad/src/ui/forms/qg_dlgarc.h
        librecad/src/ui/forms/qg_dlgattributes.cpp
        librecad/src/ui/forms/qg_dlgattributes.h
        librecad/src/ui/forms/qg_dlgcircle.cpp
        librecad/src/ui/forms/qg_dlgcircle.h
        librecad/src/ui/forms/qg_dlgdimension.cpp
        librecad/src/ui/forms/qg_dlgdimension.h
        librecad/src/ui/forms/qg_dlgdimlinear.cpp
        librecad/src/ui/forms/qg_dlgdimlinear.h
        librecad/src/ui/forms/qg_dlgellipse.cpp
        librecad/src/ui/forms/qg_dlgellipse.h
        librecad/src/ui/forms/qg_dlghatch.cpp
        librecad/src/ui/forms/qg_dlghatch.h
        librecad/src/ui/forms/qg_dlgimage.cpp
        librecad/src/ui/forms/qg_dlgimage.h
        librecad/src/ui/forms/qg_dlgimageoptions.cpp
        librecad/src/ui/forms/qg_dlgimageoptions.h
        librecad/src/ui/forms/qg_dlginitial.cpp
        librecad/src/ui/forms/qg_dlginitial.h
        librecad/src/ui/forms/qg_dlginsert.cpp
        librecad/src/ui/forms/qg_dlginsert.h
        librecad/src/ui/forms/qg_dlgline.cpp
        librecad/src/ui/forms/qg_dlgline.h
        librecad/src/ui/forms/qg_dlgmirror.cpp
        librecad/src/ui/forms/qg_dlgmirror.h
        librecad/src/ui/forms/qg_dlgmove.cpp
        librecad/src/ui/forms/qg_dlgmove.h
        librecad/src/ui/forms/qg_dlgmoverotate.cpp
        librecad/src/ui/forms/qg_dlgmoverotate.h
        librecad/src/ui/forms/qg_dlgmtext.cpp
        librecad/src/ui/forms/qg_dlgmtext.h
        librecad/src/ui/forms/qg_dlgoptionsdrawing.cpp
        librecad/src/ui/forms/qg_dlgoptionsdrawing.h
        librecad/src/ui/forms/qg_dlgoptionsgeneral.cpp
        librecad/src/ui/forms/qg_dlgoptionsgeneral.h
        librecad/src/ui/forms/qg_dlgoptionsmakercam.cpp
        librecad/src/ui/forms/qg_dlgoptionsmakercam.h
#        librecad/src/ui/forms/qg_dlgoptionsvariables.cpp
#        librecad/src/ui/forms/qg_dlgoptionsvariables.h
        librecad/src/ui/forms/qg_dlgpoint.cpp
        librecad/src/ui/forms/qg_dlgpoint.h
        librecad/src/ui/forms/qg_dlgpolyline.cpp
        librecad/src/ui/forms/qg_dlgpolyline.h
        librecad/src/ui/forms/qg_dlgrotate.cpp
        librecad/src/ui/forms/qg_dlgrotate.h
        librecad/src/ui/forms/qg_dlgrotate2.cpp
        librecad/src/ui/forms/qg_dlgrotate2.h
        librecad/src/ui/forms/qg_dlgscale.cpp
        librecad/src/ui/forms/qg_dlgscale.h
        librecad/src/ui/forms/qg_dlgspline.cpp
        librecad/src/ui/forms/qg_dlgspline.h
        librecad/src/ui/forms/qg_dlgtext.cpp
        librecad/src/ui/forms/qg_dlgtext.h
        librecad/src/ui/forms/qg_exitdialog.cpp
        librecad/src/ui/forms/qg_exitdialog.h
        librecad/src/ui/forms/qg_imageoptions.cpp
        librecad/src/ui/forms/qg_imageoptions.h
        librecad/src/ui/forms/qg_insertoptions.cpp
        librecad/src/ui/forms/qg_insertoptions.h
        librecad/src/ui/forms/qg_layerdialog.cpp
        librecad/src/ui/forms/qg_layerdialog.h
#        librecad/src/ui/forms/lc_layerdialog_ex.cpp
#        librecad/src/ui/forms/lc_layerdialog_ex.h
        librecad/src/ui/forms/lc_layerdialog_ex.cpp
        librecad/src/ui/forms/lc_layerdialog_ex.h
        librecad/src/ui/forms/qg_libraryinsertoptions.cpp
        librecad/src/ui/forms/qg_libraryinsertoptions.h
        librecad/src/ui/forms/qg_lineangleoptions.cpp
        librecad/src/ui/forms/qg_lineangleoptions.h
        librecad/src/ui/forms/qg_linebisectoroptions.cpp
        librecad/src/ui/forms/qg_linebisectoroptions.h
        librecad/src/ui/forms/qg_lineoptions.cpp
        librecad/src/ui/forms/qg_lineoptions.h
        librecad/src/ui/forms/qg_lineparalleloptions.cpp
        librecad/src/ui/forms/qg_lineparalleloptions.h
        librecad/src/ui/forms/qg_lineparallelthroughoptions.cpp
        librecad/src/ui/forms/qg_lineparallelthroughoptions.h
        librecad/src/ui/forms/qg_linepolygon2options.cpp
        librecad/src/ui/forms/qg_linepolygon2options.h
        librecad/src/ui/forms/qg_linepolygonoptions.cpp
        librecad/src/ui/forms/qg_linepolygonoptions.h
        librecad/src/ui/forms/qg_linerelangleoptions.cpp
        librecad/src/ui/forms/qg_linerelangleoptions.h
        librecad/src/ui/forms/qg_modifyoffsetoptions.cpp
        librecad/src/ui/forms/qg_modifyoffsetoptions.h
        librecad/src/ui/forms/qg_mousewidget.cpp
        librecad/src/ui/forms/qg_mousewidget.h
        librecad/src/ui/forms/qg_moverotateoptions.cpp
        librecad/src/ui/forms/qg_moverotateoptions.h
        librecad/src/ui/forms/qg_mtextoptions.cpp
        librecad/src/ui/forms/qg_mtextoptions.h
        librecad/src/ui/forms/qg_polylineequidistantoptions.cpp
        librecad/src/ui/forms/qg_polylineequidistantoptions.h
        librecad/src/ui/forms/qg_polylineoptions.cpp
        librecad/src/ui/forms/qg_polylineoptions.h
        librecad/src/ui/forms/qg_printpreviewoptions.cpp
        librecad/src/ui/forms/qg_printpreviewoptions.h
        librecad/src/ui/forms/qg_roundoptions.cpp
        librecad/src/ui/forms/qg_roundoptions.h
        librecad/src/ui/forms/qg_selectionwidget.cpp
        librecad/src/ui/forms/qg_selectionwidget.h
        librecad/src/ui/forms/qg_snapdistoptions.cpp
        librecad/src/ui/forms/qg_snapdistoptions.h
        librecad/src/ui/forms/qg_snapmiddleoptions.cpp
        librecad/src/ui/forms/qg_snapmiddleoptions.h
        librecad/src/ui/forms/qg_snaptoolbar.cpp
        librecad/src/ui/forms/qg_snaptoolbar.h
        librecad/src/ui/forms/qg_splineoptions.cpp
        librecad/src/ui/forms/qg_splineoptions.h
        librecad/src/ui/forms/qg_textoptions.cpp
        librecad/src/ui/forms/qg_textoptions.h
        librecad/src/ui/forms/qg_trimamountoptions.cpp
        librecad/src/ui/forms/qg_trimamountoptions.h
        librecad/src/ui/forms/qg_widgetpen.cpp
        librecad/src/ui/forms/qg_widgetpen.h
        librecad/src/ui/generic/actionlist.cpp
        librecad/src/ui/generic/actionlist.h
        librecad/src/ui/generic/colorcombobox.cpp
        librecad/src/ui/generic/colorcombobox.h
        librecad/src/ui/generic/colorwizard.cpp
        librecad/src/ui/generic/colorwizard.h
        librecad/src/ui/generic/comboboxoption.cpp
        librecad/src/ui/generic/comboboxoption.h
        librecad/src/ui/forms/LC_DlgParabola.h
        librecad/src/ui/forms/LC_DlgParabola.cpp
#        librecad/src/ui/generic/customtoolbarcreator.cpp
#        librecad/src/ui/generic/customtoolbarcreator.h
#        librecad/src/ui/generic/customwidgetcreator.cpp
#        librecad/src/ui/generic/customwidgetcreator.h
        librecad/src/ui/generic/linklist.cpp
        librecad/src/ui/generic/linklist.h
        librecad/src/ui/generic/textfileviewer.cpp
        librecad/src/ui/generic/textfileviewer.h
        librecad/src/ui/generic/widgetcreator.cpp
        librecad/src/ui/generic/widgetcreator.h
        librecad/src/ui/lc_actionfactory.cpp
        librecad/src/ui/lc_actionfactory.h
        librecad/src/ui/lc_actiongroupmanager.cpp
        librecad/src/ui/lc_actiongroupmanager.h
        librecad/src/ui/lc_centralwidget.cpp
        librecad/src/ui/lc_centralwidget.h
        librecad/src/ui/lc_customtoolbar.cpp
        librecad/src/ui/lc_customtoolbar.h
        librecad/src/ui/lc_deviceoptions.cpp
        librecad/src/ui/lc_deviceoptions.h
        librecad/src/ui/lc_dockwidget.cpp
        librecad/src/ui/lc_dockwidget.h
        librecad/src/ui/lc_filedialogservice.cpp
        librecad/src/ui/lc_filedialogservice.h
        librecad/src/ui/lc_penwizard.cpp
        librecad/src/ui/lc_penwizard.h
        librecad/src/ui/lc_widgetfactory.cpp
        librecad/src/ui/lc_widgetfactory.h
        librecad/src/ui/lg_dimzerosbox.cpp
        librecad/src/ui/lg_dimzerosbox.h
        librecad/src/ui/qg_actionhandler.cpp
        librecad/src/ui/qg_actionhandler.h
        librecad/src/ui/qg_blockwidget.cpp
        librecad/src/ui/qg_blockwidget.h
        librecad/src/ui/qg_colorbox.cpp
        librecad/src/ui/qg_colorbox.h
        librecad/src/ui/qg_commandedit.cpp
        librecad/src/ui/qg_commandedit.h
        librecad/src/ui/qg_commandhistory.cpp
        librecad/src/ui/qg_commandhistory.h
        librecad/src/ui/qg_dialogfactory.cpp
        librecad/src/ui/qg_dialogfactory.h
        librecad/src/ui/qg_filedialog.cpp
        librecad/src/ui/qg_filedialog.h
        librecad/src/ui/qg_fontbox.cpp
        librecad/src/ui/qg_fontbox.h
        librecad/src/ui/qg_graphicview.cpp
        librecad/src/ui/qg_graphicview.h
        librecad/src/ui/qg_layerbox.cpp
        librecad/src/ui/qg_layerbox.h
        librecad/src/ui/qg_layerwidget.cpp
        librecad/src/ui/qg_layerwidget.h
        librecad/src/ui/lc_layertreeitem.cpp
        librecad/src/ui/lc_layertreeitem.h
        librecad/src/ui/lc_layertreemodel.cpp
        librecad/src/ui/lc_layertreemodel.h
        librecad/src/ui/lc_layertreewidget.cpp
        librecad/src/ui/lc_layertreewidget.h
        librecad/src/ui/lc_layertreeview.cpp
        librecad/src/ui/lc_layertreeview.h
        librecad/src/ui/qg_librarywidget.cpp
        librecad/src/ui/qg_librarywidget.h
        librecad/src/ui/qg_linetypebox.cpp
        librecad/src/ui/qg_linetypebox.h
        librecad/src/ui/qg_mainwindowinterface.h
        librecad/src/ui/qg_patternbox.cpp
        librecad/src/ui/qg_patternbox.h
        librecad/src/ui/qg_pentoolbar.cpp
        librecad/src/ui/qg_pentoolbar.h
        librecad/src/ui/qg_recentfiles.cpp
        librecad/src/ui/qg_recentfiles.h
        librecad/src/ui/qg_scrollbar.h
        librecad/src/ui/qg_widthbox.cpp
        librecad/src/ui/qg_widthbox.h
        librecad/src/ui/twostackedlabels.cpp
        librecad/src/ui/twostackedlabels.h
        librecad/src/ui/lc_penitem.cpp
        librecad/src/ui/lc_penitem.h
        librecad/src/ui/lc_penpalettewidget.cpp
        librecad/src/ui/lc_penpalettewidget.h
        librecad/src/ui/lc_peninforegistry.h
        librecad/src/ui/lc_peninforegistry.cpp
        librecad/src/ui/lc_penpalettemodel.cpp
        librecad/src/ui/lc_penpaletteoptions.cpp
        librecad/src/ui/lc_penpalettedata.h
        librecad/src/ui/lc_penpalettedata.cpp
        librecad/src/ui/lc_penpaletteoptionsdialog.cpp
        librecad/src/actions/lc_actiondrawparabola4points.cpp
        librecad/src/actions/lc_actiondrawparabolaFD.cpp
        librecad/src/ui/forms/LC_DlgParabola.cpp
        librecad/src/lib/engine/lc_parabola.cpp
        librecad/src/actions/lc_actionsnapmiddlemanual.cpp
        librecad/src/actions/lc_actionpenapply.cpp
        librecad/src/actions/lc_actionpenpick.cpp
        librecad/src/ui/forms/lc_layertreeoptionsdialog.cpp
        librecad/src/ui/lc_layertreemodel_options.cpp
        librecad/src/ui/lc_layertreemodel_options.cpp
        librecad/src/actions/lc_actionpensyncactivebylayer.h
        librecad/src/actions/lc_actionpensyncactivebylayer.cpp
        librecad/src/ui/generic/lc_flexlayout.h
        librecad/src/ui/generic/lc_flexlayout.cpp
        librecad/src/ui/generic/lc_flexlayout.cpp
        librecad/src/actions/lc_actiondrawrectangle3points.cpp
        librecad/src/actions/lc_actiondrawrectangle3points.h
        librecad/src/actions/lc_actiondrawcross.cpp
        librecad/src/actions/lc_actiondrawcross.h
        librecad/src/ui/forms/lc_crossoptions.cpp
        librecad/src/actions/lc_actiondrawlinesnake.h
        librecad/src/actions/lc_actiondrawlinesnake.h
        librecad/src/actions/lc_actiondrawlinesnake.cpp
        librecad/src/ui/forms/lc_lineoptions.cpp
        librecad/src/actions/lc_actiondrawlineanglerel.h
        librecad/src/actions/lc_actiondrawlineanglerel.cpp
        librecad/src/ui/forms/lc_lineanglereloptions.cpp
        librecad/src/ui/forms/lc_slicedivideoptions.cpp
        librecad/src/actions/lc_actiondrawslicedivide.cpp
        librecad/src/actions/lc_actiondrawrectangle1point.cpp
        librecad/src/ui/forms/lc_rectangle1pointoptions.cpp
        librecad/src/lib/engine/lc_parabola.cpp
        librecad/src/actions/lc_actiondrawparabola4points.cpp
        librecad/src/actions/lc_actiondrawparabolaFD.cpp
        librecad/src/ui/forms/lc_actionoptionswidget.h
        librecad/src/ui/forms/lc_actionoptionswidget.cpp
        librecad/src/actions/lc_abstractactiondrawrectangle.h
        librecad/src/actions/lc_abstractactiondrawrectangle.cpp
        librecad/src/actions/lc_actiondrawrectangle2points.h
        librecad/src/actions/lc_actiondrawrectangle2points.cpp
        librecad/src/ui/forms/lc_rectangle2pointsoptions.h
        librecad/src/ui/forms/lc_rectangle2pointsoptions.cpp
        librecad/src/ui/forms/lc_circlebyarcoptions.cpp
        librecad/src/actions/lc_actiondrawcirclebyarc.cpp
        librecad/src/actions/lc_actionmodifylinejoin.cpp
        librecad/src/ui/forms/lc_linejoinoptions.cpp
        librecad/src/actions/lc_actiondrawlinepoints.h
        librecad/src/actions/lc_actiondrawlinepoints.cpp
        librecad/src/ui/forms/lc_linepointsoptions.h
        librecad/src/ui/forms/lc_linepointsoptions.cpp
        librecad/src/lib/math/lc_linemath.h
        librecad/src/lib/math/lc_linemath.cpp
        librecad/src/actions/lc_abstractactiondrawline.cpp
        librecad/src/actions/lc_abstractactionwithpreview.h
        librecad/src/actions/lc_abstractactionwithpreview.cpp
        librecad/src/actions/lc_actionmodifyduplicate.h
        librecad/src/actions/lc_actionmodifyduplicate.cpp
        librecad/src/ui/forms/lc_duplicateoptions.cpp
        librecad/src/ui/forms/lc_rectangle3pointsoptions.cpp
        librecad/src/ui/forms/lc_linefrompointtolineoptions.cpp
        librecad/src/actions/lc_actiondrawlinefrompointtoline.cpp
        librecad/src/actions/lc_actiondrawstar.cpp
        librecad/src/ui/forms/lc_staroptions.cpp
        librecad/src/actions/lc_actionmodifybreakdivide.cpp
        librecad/src/actions/lc_actionmodifybreakdivide.h
        librecad/src/ui/forms/lc_modifybreakdivideoptions.h
        librecad/src/ui/forms/lc_modifybreakdivideoptions.cpp
        librecad/src/actions/lc_actionmodifylinegap.cpp
        librecad/src/ui/forms/lc_modifygapoptions.h
        librecad/src/ui/forms/lc_modifygapoptions.cpp
        librecad/src/ui/lc_quickinfowidget.cpp
        librecad/src/ui/generic/lc_plaintextedit.h
        librecad/src/actions/lc_actioninfoproperties.cpp
        librecad/src/actions/lc_actioninfopickcoordinates.cpp
        librecad/src/ui/lc_quickinfopointsdata.h
        librecad/src/ui/lc_quickinfopointsdata.cpp
        librecad/src/ui/lc_quickinfobasedata.cpp
        librecad/src/ui/lc_quickinfoentitydata.cpp
        librecad/src/ui/lc_quickinfowidgetoptionsdialog.cpp
        librecad/src/ui/lc_quickinfowidgetoptions.h
        librecad/src/ui/lc_quickinfowidgetoptions.cpp
        librecad/src/actions/lc_actiondrawcirclebase.h
        librecad/src/actions/lc_actiondrawcirclebase.h
        librecad/src/actions/lc_actiondrawcirclebase.cpp
        librecad/src/actions/lc_actiondrawcirclebase.cpp
        librecad/src/lib/actions/lc_highlight.cpp
        librecad/src/actions/lc_actionpolylinedeletebase.cpp
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


#target_link_libraries(LibreCAD Qt5::Core Qt5::Widgets Qt5::Gui Qt5::PrintSupport Qt5::Svg)
target_link_libraries(LibreCAD Qt6::Core Qt6::Widgets Qt6::Gui Qt6::PrintSupport Qt6::Svg)
