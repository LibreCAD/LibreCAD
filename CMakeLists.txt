cmake_minimum_required(VERSION 3.28)
project(LibreCAD VERSION 2.2.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Local paths (update if needed)
set(MIN_GW_PATH_PREFIX "d:\\Qt\\QT6\\6.9.0\\msvc2022_64")
set(BOOST_ROOT "c:\\Local\\boost_1_84_0")
set(Boost_INCLUDE_DIR "c:\\Local\\boost_1_84_0")
#set(CMAKE_VERBOSE_MAKEFILE ON)  # Uncomment for debug
#set(CMAKE_INSTALL_PREFIX "Z:/LC_BUILD_PROD")  # Uncomment if needed

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_OPTIONS --connections string)

add_compile_definitions(DWGSUPPORT MUPARSER_STATIC LC_VERSION=2.2.2.5-alpha LC_PRERELEASE=true)

find_package(Qt6 COMPONENTS Gui Core Widgets PrintSupport Svg Network LinguistTools REQUIRED)
find_package(Boost REQUIRED)  # Removed empty COMPONENTS
find_package(Freetype)

set(LIB_SOURCES)

set(CMAKE_AUTOUIC_SEARCH_PATHS 
librecad/src/ui/dialogs/main/
)

# Group 2: jwwlib
set(JWWLIB_PATH "libraries/jwwlib/src/")
set(JWWLIB_FILES
    dl_attributes.h
    dl_codes.h
    dl_creationinterface.h
    dl_entities.h
    dl_exception.h
    dl_extrusion.h
    dl_jww.cpp
    dl_jww.h
    dl_writer.h
    dl_writer_ascii.cpp
    dl_writer_ascii.h
    jwtype.h
    jwwdoc.cpp
    jwwdoc.h
)
list(TRANSFORM JWWLIB_FILES PREPEND "${JWWLIB_PATH}")
list(APPEND LIB_SOURCES ${JWWLIB_FILES})

# Group 3: libdxfrw base
set(DXFRW_PATH "libraries/libdxfrw/src/")
set(DXFRW_FILES
    drw_base.cpp
    drw_base.h
    drw_classes.cpp
    drw_classes.h
    drw_entities.cpp
    drw_entities.h
    drw_header.cpp
    drw_header.h
    drw_interface.h
    drw_objects.cpp
    drw_objects.h
    libdwgr.cpp
    libdwgr.h
    libdxfrw.cpp
    libdxfrw.h
    main_doc.h
)
list(TRANSFORM DXFRW_FILES PREPEND "${DXFRW_PATH}")
list(APPEND LIB_SOURCES ${DXFRW_FILES})

# Group 4: libdxfrw intern
set(DXFRW_INTERN_PATH "libraries/libdxfrw/src/intern/")
set(DXFRW_INTERN_FILES
    drw_cptable932.h
    drw_cptable936.h
    drw_cptable949.h
    drw_cptable950.h
    drw_cptables.h
    drw_dbg.cpp
    drw_dbg.h
    drw_reserve.h
    drw_textcodec.cpp
    drw_textcodec.h
    dwgbuffer.cpp
    dwgbuffer.h
    dwgreader.cpp
    dwgreader.h
    dwgreader15.cpp
    dwgreader15.h
    dwgreader18.cpp
    dwgreader18.h
    dwgreader21.cpp
    dwgreader21.h
    dwgreader24.cpp
    dwgreader24.h
    dwgreader27.cpp
    dwgreader27.h
    dwgutil.cpp
    dwgutil.h
    dxfreader.cpp
    dxfreader.h
    dxfwriter.cpp
    dxfwriter.h
    rscodec.cpp
    rscodec.h
)
list(TRANSFORM DXFRW_INTERN_FILES PREPEND "${DXFRW_INTERN_PATH}")
list(APPEND LIB_SOURCES ${DXFRW_INTERN_FILES})

# Group 5: muparser include
set(MUPARSER_INCLUDE_PATH "libraries/muparser/include/")
set(MUPARSER_INCLUDE_FILES
    muParser.h
    muParserBase.h
    muParserBytecode.h
    muParserCallback.h
    muParserDef.h
    muParserDLL.h
    muParserError.h
    muParserFixes.h
    muParserInt.h
    muParserTemplateMagic.h
    muParserToken.h
    muParserTokenReader.h
)
list(TRANSFORM MUPARSER_INCLUDE_FILES PREPEND "${MUPARSER_INCLUDE_PATH}")
list(APPEND LIB_SOURCES ${MUPARSER_INCLUDE_FILES})

# Group 6: muparser src
set(MUPARSER_SRC_PATH "libraries/muparser/src/")
set(MUPARSER_SRC_FILES
    muParser.cpp
    muParserBase.cpp
    muParserBytecode.cpp
    muParserCallback.cpp
    muParserDLL.cpp
    muParserError.cpp
    muParserInt.cpp
    muParserTokenReader.cpp
)
list(TRANSFORM MUPARSER_SRC_FILES PREPEND "${MUPARSER_SRC_PATH}")
list(APPEND LIB_SOURCES ${MUPARSER_SRC_FILES})

# Group 7: actions drawing draw dimensions
set(ACTIONS_DRAW_DIM_PATH "librecad/src/actions/drawing/draw/dimensions/")
set(ACTIONS_DRAW_DIM_FILES
    lc_actiondimarc.h
    lc_actiondimarc.cpp
    lc_actiondimordinate.h
    lc_actiondimordinate.cpp
    lc_actiondimstyleapply.h
    lc_actiondimstyleapply.cpp
    lc_actiondrawgdtfeaturecontrolframe.h
    lc_actiondrawgdtfeaturecontrolframe.cpp
    rs_actiondimaligned.cpp
    rs_actiondimaligned.h
    rs_actiondimangular.cpp
    rs_actiondimangular.h
    rs_actiondimdiametric.cpp
    rs_actiondimdiametric.h
    rs_actiondimension.cpp
    rs_actiondimension.h
    rs_actiondimleader.cpp
    rs_actiondimleader.h
    rs_actiondimlinear.cpp
    rs_actiondimlinear.h
    rs_actiondimradial.cpp
    rs_actiondimradial.h
    lc_actionselectdimordinatesameorigin.h
    lc_actionselectdimordinatesameorigin.cpp
    lc_actiondimordinaterebase.h
    lc_actiondimordinaterebase.cpp
    rs_actiontoolregeneratedimensions.cpp
    rs_actiontoolregeneratedimensions.h
    lc_actiondrawdimbaseline.h
    lc_actiondrawdimbaseline.cpp
    lc_actioncircledimbase.h
    lc_actioncircledimbase.cpp
    lc_actiondimlinearbase.cpp
)
list(TRANSFORM ACTIONS_DRAW_DIM_FILES PREPEND "${ACTIONS_DRAW_DIM_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_DIM_FILES})

# Group 8: actions drawing draw circle
set(ACTIONS_DRAW_CIRCLE_PATH "librecad/src/actions/drawing/draw/circle/")
set(ACTIONS_DRAW_CIRCLE_FILES
    lc_actiondrawcircle2pr.cpp
    lc_actiondrawcircle2pr.h
    rs_actiondrawcircle.cpp
    rs_actiondrawcircle.h
    rs_actiondrawcircle2p.cpp
    rs_actiondrawcircle2p.h
    rs_actiondrawcircle3p.cpp
    rs_actiondrawcircle3p.h
    rs_actiondrawcirclecr.cpp
    rs_actiondrawcirclecr.h
    rs_actiondrawcircleinscribe.cpp
    rs_actiondrawcircleinscribe.h
    rs_actiondrawcircletan1_2p.cpp
    rs_actiondrawcircletan1_2p.h
    rs_actiondrawcircletan2.cpp
    rs_actiondrawcircletan2.h
    rs_actiondrawcircletan2_1p.cpp
    rs_actiondrawcircletan2_1p.h
    rs_actiondrawcircletan3.cpp
    rs_actiondrawcircletan3.h
    lc_actiondrawcirclebyarc.cpp
    lc_actiondrawcirclebase.h
    lc_actiondrawcirclebase.cpp
)
list(TRANSFORM ACTIONS_DRAW_CIRCLE_FILES PREPEND "${ACTIONS_DRAW_CIRCLE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_CIRCLE_FILES})

# Group 9: actions drawing draw polygon
set(ACTIONS_DRAW_POLYGON_PATH "librecad/src/actions/drawing/draw/polygon/")
set(ACTIONS_DRAW_POLYGON_FILES
    lc_actiondrawlinepolygon3.cpp
    lc_actiondrawlinepolygon3.h
    rs_actiondrawlinepolygon.cpp
    rs_actiondrawlinepolygon.h
    rs_actiondrawlinepolygon2.cpp
    rs_actiondrawlinepolygon2.h
    lc_actiondrawstar.cpp
    lc_actiondrawlinepolygon4.h
    lc_actiondrawlinepolygon4.cpp
    lc_actiondrawlinepolygonbase.h
    lc_actiondrawlinepolygonbase.cpp
)
list(TRANSFORM ACTIONS_DRAW_POLYGON_FILES PREPEND "${ACTIONS_DRAW_POLYGON_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_POLYGON_FILES})

# Group 10: actions drawing draw spline
set(ACTIONS_DRAW_SPLINE_PATH "librecad/src/actions/drawing/draw/spline/")
set(ACTIONS_DRAW_SPLINE_FILES
    lc_actiondrawsplinepoints.cpp
    lc_actiondrawsplinepoints.h
    rs_actiondrawspline.cpp
    rs_actiondrawspline.h
    lc_actionremovesplinepoints.h
    lc_actionremovesplinepoints.cpp
    lc_actionsplineappendpoint.h
    lc_actionsplineappendpoint.cpp
    lc_actionsplineaddpoint.h
    lc_actionsplineaddpoint.cpp
    lc_actionsplinemodifybase.h
    lc_actionsplinemodifybase.cpp
    lc_actionsplineexplode.h
    lc_actionsplineexplode.cpp
    lc_actionsplinefrompolyline.h
    lc_actionsplinefrompolyline.cpp
    lc_actionsplineremovebetween.h
    lc_actionsplineremovebetween.cpp
)
list(TRANSFORM ACTIONS_DRAW_SPLINE_FILES PREPEND "${ACTIONS_DRAW_SPLINE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_SPLINE_FILES})

# Group 11: actions file
set(ACTIONS_FILE_PATH "librecad/src/actions/file/")
set(ACTIONS_FILE_FILES
    lc_actionfileexportmakercam.cpp
    lc_actionfileexportmakercam.h
)
list(TRANSFORM ACTIONS_FILE_FILES PREPEND "${ACTIONS_FILE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_FILE_FILES})

# Group 12: actions dock_widgets layer
set(ACTIONS_DOCK_LAYER_PATH "librecad/src/actions/dock_widgets/layer/")
set(ACTIONS_DOCK_LAYER_FILES
    lc_actionlayersexport.cpp
    lc_actionlayersexport.h
    lc_actionlayerstoggleconstruction.cpp
    lc_actionlayerstoggleconstruction.h
    lc_actionentitylayerbase.h
    lc_actionentitylayerbase.cpp
    lc_actionentitylayertoggle.cpp
    lc_actionentitylayertoggle.cpp
    rs_actionlayersadd.cpp
    rs_actionlayersadd.h
    lc_actionlayerscmd.h
    lc_actionlayerscmd.cpp
    rs_actionlayersedit.cpp
    rs_actionlayersedit.h
    rs_actionlayersfreezeall.cpp
    rs_actionlayersfreezeall.h
    rs_actionlayerslockall.cpp
    rs_actionlayerslockall.h
    rs_actionlayersremove.cpp
    rs_actionlayersremove.h
    rs_actionlayerstogglelock.cpp
    rs_actionlayerstogglelock.h
    rs_actionlayerstoggleprint.cpp
    rs_actionlayerstoggleprint.h
    rs_actionlayerstoggleview.cpp
    rs_actionlayerstoggleview.h
)
list(TRANSFORM ACTIONS_DOCK_LAYER_FILES PREPEND "${ACTIONS_DOCK_LAYER_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DOCK_LAYER_FILES})

# Group 13: actions dock_widgets block
set(ACTIONS_DOCK_BLOCK_PATH "librecad/src/actions/dock_widgets/block/")
set(ACTIONS_DOCK_BLOCK_FILES
    rs_actionblocksadd.cpp
    rs_actionblocksadd.h
    rs_actionblocksattributes.cpp
    rs_actionblocksattributes.h
    rs_actionblockscreate.cpp
    rs_actionblockscreate.h
    rs_actionblocksedit.cpp
    rs_actionblocksedit.h
    rs_actionblocksfreezeall.cpp
    rs_actionblocksfreezeall.h
    rs_actionblocksinsert.cpp
    rs_actionblocksinsert.h
    rs_actionblocksremove.cpp
    rs_actionblocksremove.h
    rs_actionblockssave.cpp
    rs_actionblockssave.h
    rs_actionblockstoggleview.cpp
    rs_actionblockstoggleview.h
)
list(TRANSFORM ACTIONS_DOCK_BLOCK_FILES PREPEND "${ACTIONS_DOCK_BLOCK_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DOCK_BLOCK_FILES})

# Group 14: actions drawing modify
set(ACTIONS_DRAW_MODIFY_PATH "librecad/src/actions/drawing/modify/")
set(ACTIONS_DRAW_MODIFY_FILES
    rs_actionblocksexplode.cpp
    rs_actionblocksexplode.h
    rs_actionmodifyattributes.cpp
    rs_actionmodifyattributes.h
    rs_actionmodifybevel.cpp
    rs_actionmodifybevel.h
    rs_actionmodifycut.cpp
    rs_actionmodifycut.h
    rs_actionmodifydelete.cpp
    rs_actionmodifydelete.h
    rs_actionmodifydeletefree.cpp
    rs_actionmodifydeletefree.h
    rs_actionmodifyentity.cpp
    rs_actionmodifyentity.h
    rs_actionmodifyexplodetext.cpp
    rs_actionmodifyexplodetext.h
    rs_actionmodifymirror.cpp
    rs_actionmodifymirror.h
    rs_actionmodifymove.cpp
    rs_actionmodifymove.h
    rs_actionmodifymoverotate.cpp
    rs_actionmodifymoverotate.h
    rs_actionmodifyoffset.cpp
    rs_actionmodifyoffset.h
    rs_actionmodifyrevertdirection.cpp
    rs_actionmodifyrevertdirection.h
    rs_actionmodifyrotate.cpp
    rs_actionmodifyrotate.h
    rs_actionmodifyrotate2.cpp
    rs_actionmodifyrotate2.h
    rs_actionmodifyround.cpp
    rs_actionmodifyround.h
    rs_actionmodifyscale.cpp
    rs_actionmodifyscale.h
    rs_actionmodifystretch.cpp
    rs_actionmodifystretch.h
    rs_actionmodifytrim.cpp
    rs_actionmodifytrim.h
    rs_actionmodifytrimamount.cpp
    rs_actionmodifytrimamount.h
    lc_actionmodifymoveadjust.h
    lc_actionmodifymoveadjust.cpp
    rs_actionorder.cpp
    rs_actionorder.h
    lc_actionmodifylinejoin.cpp
    lc_actionmodifyduplicate.h
    lc_actionmodifyduplicate.cpp
    lc_actionmodifybreakdivide.cpp
    lc_actionmodifybreakdivide.h
    lc_actionmodifylinegap.cpp
    lc_actionmodifyalign.h
    lc_actionmodifyalign.cpp
    lc_actionmodifyalignsingle.h
    lc_actionmodifyalignsingle.cpp
    lc_actionmodifyalignref.h
    lc_actionmodifyalignref.cpp
    lc_actionmodifyselectionbase.h
    lc_actionmodifyselectionbase.cpp
    lc_actionmodifybase.cpp
    lc_actionmodifybase.h
)
list(TRANSFORM ACTIONS_DRAW_MODIFY_FILES PREPEND "${ACTIONS_DRAW_MODIFY_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_MODIFY_FILES})

# Group 15: actions drawing draw curve
set(ACTIONS_DRAW_CURVE_PATH "librecad/src/actions/drawing/draw/curve/")
set(ACTIONS_DRAW_CURVE_FILES
    rs_actiondrawarc.cpp
    rs_actiondrawarc.h
    rs_actiondrawarc3p.cpp
    rs_actiondrawarc3p.h
    rs_actiondrawarctangential.cpp
    rs_actiondrawarctangential.h
    rs_actiondrawlinefree.cpp
    rs_actiondrawlinefree.h
    lc_actiondrawparabola4points.cpp
    lc_actiondrawparabolaFD.cpp
    lc_actiondrawarc2pointsbase.h
    lc_actiondrawarc2pointsbase.cpp
    lc_actiondrawarc2pointsradius.h
    lc_actiondrawarc2pointsradius.cpp
    lc_actiondrawarc2pointsangle.h
    lc_actiondrawarc2pointsangle.cpp
    lc_actiondrawarc2pointslength.h
    lc_actiondrawarc2pointslength.cpp
    lc_actiondrawarc2pointsheight.h
    lc_actiondrawarc2pointsheight.cpp
)
list(TRANSFORM ACTIONS_DRAW_CURVE_FILES PREPEND "${ACTIONS_DRAW_CURVE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_CURVE_FILES})

# Group 16: actions drawing draw hatch
set(ACTIONS_DRAW_HATCH_PATH "librecad/src/actions/drawing/draw/hatch/")
set(ACTIONS_DRAW_HATCH_FILES
    rs_actiondrawhatch.cpp
    rs_actiondrawhatch.h
)
list(TRANSFORM ACTIONS_DRAW_HATCH_FILES PREPEND "${ACTIONS_DRAW_HATCH_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_HATCH_FILES})

# Group 17: actions drawing draw image
set(ACTIONS_DRAW_IMAGE_PATH "librecad/src/actions/drawing/draw/image/")
set(ACTIONS_DRAW_IMAGE_FILES
    rs_actiondrawimage.cpp
    rs_actiondrawimage.h
)
list(TRANSFORM ACTIONS_DRAW_IMAGE_FILES PREPEND "${ACTIONS_DRAW_IMAGE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_IMAGE_FILES})

# Group 18: actions drawing draw line
set(ACTIONS_DRAW_LINE_PATH "librecad/src/actions/drawing/draw/line/")
set(ACTIONS_DRAW_LINE_FILES
    rs_actiondrawline.cpp
    rs_actiondrawline.h
    rs_actiondrawlineangle.cpp
    rs_actiondrawlineangle.h
    rs_actiondrawlinebisector.cpp
    rs_actiondrawlinebisector.h
    rs_actiondrawlinehorvert.cpp
    rs_actiondrawlinehorvert.h
    rs_actiondrawlineorthtan.cpp
    rs_actiondrawlineorthtan.h
    rs_actiondrawlineparallel.cpp
    rs_actiondrawlineparallel.h
    rs_actiondrawlineparallelthrough.cpp
    rs_actiondrawlineparallelthrough.h
    rs_actiondrawlinerelangle.cpp
    rs_actiondrawlinerelangle.h
    rs_actiondrawlinetangent1.cpp
    rs_actiondrawlinetangent1.h
    rs_actiondrawlinetangent2.cpp
    rs_actiondrawlinetangent2.h
    lc_actiondrawlinesnake.h
    lc_actiondrawlinesnake.cpp
    lc_actiondrawlineanglerel.h
    lc_actiondrawlineanglerel.cpp
    lc_actiondrawslicedivide.cpp
    lc_abstractactiondrawline.cpp
    lc_actiondrawlinefrompointtoline.cpp
)
list(TRANSFORM ACTIONS_DRAW_LINE_FILES PREPEND "${ACTIONS_DRAW_LINE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_LINE_FILES})

# Group 19: actions drawing draw rect
set(ACTIONS_DRAW_RECT_PATH "librecad/src/actions/drawing/draw/rect/")
set(ACTIONS_DRAW_RECT_FILES
    rs_actiondrawlinerectangle.cpp
    rs_actiondrawlinerectangle.h
    lc_actiondrawrectangle3points.cpp
    lc_actiondrawrectangle3points.h
    lc_actiondrawrectangle1point.cpp
    lc_abstractactiondrawrectangle.h
    lc_abstractactiondrawrectangle.cpp
    lc_actiondrawrectangle2points.h
    lc_actiondrawrectangle2points.cpp
)
list(TRANSFORM ACTIONS_DRAW_RECT_FILES PREPEND "${ACTIONS_DRAW_RECT_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_RECT_FILES})

# Group 20: actions drawing draw text
set(ACTIONS_DRAW_TEXT_PATH "librecad/src/actions/drawing/draw/text/")
set(ACTIONS_DRAW_TEXT_FILES
    rs_actiondrawmtext.cpp
    rs_actiondrawmtext.h
    rs_actiondrawtext.cpp
    rs_actiondrawtext.h
)
list(TRANSFORM ACTIONS_DRAW_TEXT_FILES PREPEND "${ACTIONS_DRAW_TEXT_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_TEXT_FILES})

# Group 21: actions drawing draw point
set(ACTIONS_DRAW_POINT_PATH "librecad/src/actions/drawing/draw/point/")
set(ACTIONS_DRAW_POINT_FILES
    rs_actiondrawpoint.cpp
    rs_actiondrawpoint.h
    lc_actiondrawlinepoints.h
    lc_actiondrawlinepoints.cpp
    lc_actionselectpoints.h
    lc_actionselectpoints.cpp
    lc_actiondrawpointslattice.h
    lc_actiondrawpointslattice.cpp
    lc_actionpastetopoints.h
    lc_actionpastetopoints.cpp
)
list(TRANSFORM ACTIONS_DRAW_POINT_FILES PREPEND "${ACTIONS_DRAW_POINT_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_POINT_FILES})

# Group 22: actions drawing draw polyline
set(ACTIONS_DRAW_POLYLINE_PATH "librecad/src/actions/drawing/draw/polyline/")
set(ACTIONS_DRAW_POLYLINE_FILES
    rs_actiondrawpolyline.cpp
    rs_actiondrawpolyline.h
    rs_actionpolylineadd.cpp
    rs_actionpolylineadd.h
    rs_actionpolylineappend.cpp
    rs_actionpolylineappend.h
    rs_actionpolylinedel.cpp
    rs_actionpolylinedel.h
    rs_actionpolylinedelbetween.cpp
    rs_actionpolylinedelbetween.h
    rs_actionpolylineequidistant.cpp
    rs_actionpolylineequidistant.h
    rs_actionpolylinesegment.cpp
    rs_actionpolylinesegment.h
    rs_actionpolylinetrim.cpp
    rs_actionpolylinetrim.h
    lc_actionpolylinedeletebase.cpp
    lc_actionpolylinechangesegmenttype.h
    lc_actionpolylinechangesegmenttype.cpp
    lc_actionpolylinearcstolines.h
    lc_actionpolylinearcstolines.cpp
)
list(TRANSFORM ACTIONS_DRAW_POLYLINE_FILES PREPEND "${ACTIONS_DRAW_POLYLINE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_POLYLINE_FILES})

# Group 23: actions drawing draw ellipse
set(ACTIONS_DRAW_ELLIPSE_PATH "librecad/src/actions/drawing/draw/ellipse/")
set(ACTIONS_DRAW_ELLIPSE_FILES
    rs_actiondrawellipse4points.cpp
    rs_actiondrawellipse4points.h
    rs_actiondrawellipseaxis.cpp
    rs_actiondrawellipseaxis.h
    rs_actiondrawellipsecenter3points.cpp
    rs_actiondrawellipsecenter3points.h
    rs_actiondrawellipsefocipoint.cpp
    rs_actiondrawellipsefocipoint.h
    rs_actiondrawellipseinscribe.cpp
    rs_actiondrawellipseinscribe.h
    lc_actiondrawellipse1point.h
    lc_actiondrawellipse1point.cpp
)
list(TRANSFORM ACTIONS_DRAW_ELLIPSE_FILES PREPEND "${ACTIONS_DRAW_ELLIPSE_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_ELLIPSE_FILES})

# Group 24: actions drawing edit
set(ACTIONS_DRAW_EDIT_PATH "librecad/src/actions/drawing/edit/")
set(ACTIONS_DRAW_EDIT_FILES
    rs_actioneditcopy.cpp
    rs_actioneditcopy.h
    rs_actioneditundo.cpp
    rs_actioneditundo.h
    lc_actioneditpastetransform.cpp
    lc_actioneditpastetransform.h
)
list(TRANSFORM ACTIONS_DRAW_EDIT_FILES PREPEND "${ACTIONS_DRAW_EDIT_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_EDIT_FILES})

# Group 25: actions drawing info
set(ACTIONS_DRAW_INFO_PATH "librecad/src/actions/drawing/info/")
set(ACTIONS_DRAW_INFO_FILES
    rs_actioninfoangle.cpp
    rs_actioninfoangle.h
    rs_actioninfoarea.cpp
    rs_actioninfoarea.h
    rs_actioninfodist.cpp
    rs_actioninfodist.h
    rs_actioninfodist2.cpp
    rs_actioninfodist2.h
    rs_actioninfoinside.cpp
    rs_actioninfoinside.h
    rs_actioninfototallength.cpp
    rs_actioninfototallength.h
    lc_actioninfopoint.h
    lc_actioninfopoint.cpp
    lc_actioninfoproperties.cpp
    lc_actioninfo3pointsangle.h
    lc_actioninfo3pointsangle.cpp
)
list(TRANSFORM ACTIONS_DRAW_INFO_FILES PREPEND "${ACTIONS_DRAW_INFO_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_INFO_FILES})

# Group 26: actions drawing pick
set(ACTIONS_DRAW_PICK_PATH "librecad/src/actions/drawing/pick/")
set(ACTIONS_DRAW_PICK_FILES
    lc_actioninteractivepickbase.h
    lc_actioninteractivepickbase.cpp
    lc_actioninteractivepickangle.h
    lc_actioninteractivepickangle.cpp
    lc_actioninteractivepickdistance.h
    lc_actioninteractivepickdistance.cpp
    lc_actioninteractivepickposition.h
    lc_actioninteractivepickposition.cpp
)
list(TRANSFORM ACTIONS_DRAW_PICK_FILES PREPEND "${ACTIONS_DRAW_PICK_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_PICK_FILES})

# Group 27: actions dock_widgets library
set(ACTIONS_DOCK_LIBRARY_PATH "librecad/src/actions/dock_widgets/library/")
set(ACTIONS_DOCK_LIBRARY_FILES
    rs_actionlibraryinsert.cpp
    rs_actionlibraryinsert.h
)
list(TRANSFORM ACTIONS_DOCK_LIBRARY_FILES PREPEND "${ACTIONS_DOCK_LIBRARY_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DOCK_LIBRARY_FILES})

# Group 28: actions drawing rel_zero
set(ACTIONS_DRAW_RELZERO_PATH "librecad/src/actions/drawing/rel_zero/")
set(ACTIONS_DRAW_RELZERO_FILES
    rs_actionlockrelativezero.cpp
    rs_actionlockrelativezero.h
    rs_actionsetrelativezero.cpp
    rs_actionsetrelativezero.h
)
list(TRANSFORM ACTIONS_DRAW_RELZERO_FILES PREPEND "${ACTIONS_DRAW_RELZERO_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_RELZERO_FILES})

# Group 29: actions options
set(ACTIONS_OPTIONS_PATH "librecad/src/actions/options/")
set(ACTIONS_OPTIONS_FILES
    rs_actionoptionsdrawing.cpp
    rs_actionoptionsdrawing.h
)
list(TRANSFORM ACTIONS_OPTIONS_FILES PREPEND "${ACTIONS_OPTIONS_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_OPTIONS_FILES})

# Group 30: actions print_preview
set(ACTIONS_PRINT_PREVIEW_PATH "librecad/src/actions/print_preview/")
set(ACTIONS_PRINT_PREVIEW_FILES
    rs_actionprintpreview.cpp
    rs_actionprintpreview.h
)
list(TRANSFORM ACTIONS_PRINT_PREVIEW_FILES PREPEND "${ACTIONS_PRINT_PREVIEW_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_PRINT_PREVIEW_FILES})

# Group 31: actions drawing selection
set(ACTIONS_DRAW_SELECTION_PATH "librecad/src/actions/drawing/selection/")
set(ACTIONS_DRAW_SELECTION_FILES
    rs_actionselectall.cpp
    rs_actionselectall.h
    lc_actionsingleentityselectbase.h
    lc_actionsingleentityselectbase.cpp
    rs_actionselectcontour.cpp
    rs_actionselectcontour.h
    rs_actionselectintersected.cpp
    rs_actionselectintersected.h
    rs_actionselectinvert.cpp
    rs_actionselectinvert.h
    rs_actionselectlayer.cpp
    rs_actionselectlayer.h
    rs_actionselectsingle.cpp
    rs_actionselectsingle.h
    rs_actionselectwindow.cpp
    rs_actionselectwindow.h
)
list(TRANSFORM ACTIONS_DRAW_SELECTION_FILES PREPEND "${ACTIONS_DRAW_SELECTION_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_SELECTION_FILES})

# Group 32: actions drawing snap
set(ACTIONS_DRAW_SNAP_PATH "librecad/src/actions/drawing/snap/")
set(ACTIONS_DRAW_SNAP_FILES
    rs_actionsnapintersectionmanual.cpp
    rs_actionsnapintersectionmanual.h
    lc_actionsnapmiddlemanual.cpp
)
list(TRANSFORM ACTIONS_DRAW_SNAP_FILES PREPEND "${ACTIONS_DRAW_SNAP_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_SNAP_FILES})

# Group 33: actions drawing zoom
set(ACTIONS_DRAW_ZOOM_PATH "librecad/src/actions/drawing/zoom/")
set(ACTIONS_DRAW_ZOOM_FILES
    rs_actionzoomauto.cpp
    rs_actionzoomauto.h
    rs_actionzoomin.cpp
    rs_actionzoomin.h
    rs_actionzoompan.cpp
    rs_actionzoompan.h
    rs_actionzoomprevious.cpp
    rs_actionzoomprevious.h
    rs_actionzoomredraw.cpp
    rs_actionzoomredraw.h
    rs_actionzoomscroll.cpp
    rs_actionzoomscroll.h
    rs_actionzoomwindow.cpp
    rs_actionzoomwindow.h
)
list(TRANSFORM ACTIONS_DRAW_ZOOM_FILES PREPEND "${ACTIONS_DRAW_ZOOM_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_ZOOM_FILES})

# Group 34: cmd
set(CMD_PATH "librecad/src/cmd/")
set(CMD_FILES
    rs_commands.cpp
    rs_commands.h
    lc_commandItems.h
)
list(TRANSFORM CMD_FILES PREPEND "${CMD_PATH}")
list(APPEND LIB_SOURCES ${CMD_FILES})

# Group 35: lib actions
set(LIB_ACTIONS_PATH "librecad/src/lib/actions/")
set(LIB_ACTIONS_FILES
    rs_actioninterface.cpp
    rs_actioninterface.h
    rs_previewactioninterface.cpp
    rs_previewactioninterface.h
    rs_snapper.cpp
    rs_snapper.h
    rs_actionselectbase.cpp
    rs_actionselectbase.h
    lc_actioninfomessagebuilder.h
    lc_actioninfomessagebuilder.cpp
    lc_modifiersinfo.h
    lc_modifiersinfo.cpp
    lc_actionpreselectionawarebase.h
    lc_actionpreselectionawarebase.cpp
    lc_actioncontext.h
    lc_actioncontext.cpp
    lc_overlayboxaction.h
    lc_overlayboxaction.cpp
)
list(TRANSFORM LIB_ACTIONS_FILES PREPEND "${LIB_ACTIONS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ACTIONS_FILES})

# Group 36: lib creation
set(LIB_CREATION_PATH "librecad/src/lib/creation/")
set(LIB_CREATION_FILES
    rs_creation.cpp
    rs_creation.h
)
list(TRANSFORM LIB_CREATION_FILES PREPEND "${LIB_CREATION_PATH}")
list(APPEND LIB_SOURCES ${LIB_CREATION_FILES})

# Group 37: lib debug
set(LIB_DEBUG_PATH "librecad/src/lib/debug/")
set(LIB_DEBUG_FILES
    rs_debug.cpp
    rs_debug.h
)
list(TRANSFORM LIB_DEBUG_FILES PREPEND "${LIB_DEBUG_PATH}")
list(APPEND LIB_SOURCES ${LIB_DEBUG_FILES})

# Group 38: lib engine
set(LIB_ENGINE_PATH "librecad/src/lib/engine/")
set(LIB_ENGINE_FILES
    rs.cpp
    rs.h
    rs_color.cpp
    rs_color.h
    rs_flags.cpp
    rs_flags.h
    rs_pen.h
    rs_pen.cpp
    rs_system.cpp
    rs_system.h
    rs_units.cpp
    rs_units.h
    rs_vector.cpp
    rs_vector.h
    lc_defaults.h
    lc_drawable.h
)
list(TRANSFORM LIB_ENGINE_FILES PREPEND "${LIB_ENGINE_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_FILES})

# Group 39: lib engine document
set(LIB_ENGINE_DOCUMENT_PATH "librecad/src/lib/engine/document/")
set(LIB_ENGINE_DOCUMENT_FILES
    rs_document.cpp
    rs_document.h
    rs_graphic.cpp
    rs_graphic.h
    dxf_format.h
    lc_graphicvariables.h
    lc_graphicvariables.cpp
)
list(TRANSFORM LIB_ENGINE_DOCUMENT_FILES PREPEND "${LIB_ENGINE_DOCUMENT_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_DOCUMENT_FILES})

# Group 40: lib engine document entities
set(LIB_ENGINE_ENTITIES_PATH "librecad/src/lib/engine/document/entities/")
set(LIB_ENGINE_ENTITIES_FILES
    lc_dimarc.cpp
    lc_dimarc.h
    lc_hyperbola.cpp
    lc_hyperbola.h
    lc_rect.cpp
    lc_rect.h
    lc_splinepoints.cpp
    lc_splinepoints.h
    rs_arc.cpp
    rs_arc.h
    rs_atomicentity.cpp
    rs_atomicentity.h
    rs_circle.cpp
    rs_circle.h
    rs_constructionline.cpp
    rs_constructionline.h
    rs_dimaligned.cpp
    rs_dimaligned.h
    rs_dimangular.cpp
    rs_dimangular.h
    rs_dimdiametric.cpp
    rs_dimdiametric.h
    rs_dimension.cpp
    rs_dimension.h
    rs_dimlinear.cpp
    rs_dimlinear.h
    lc_dimordinate.h
    lc_dimordinate.cpp
    lc_tolerance.h
    lc_tolerance.cpp
    rs_dimradial.cpp
    rs_dimradial.h
    rs_ellipse.cpp
    rs_ellipse.h
    rs_entity.cpp
    rs_entity.h
    rs_hatch.cpp
    rs_hatch.h
    rs_image.cpp
    rs_image.h
    rs_insert.cpp
    rs_insert.h
    rs_leader.cpp
    rs_leader.h
    rs_line.cpp
    rs_line.h
    rs_mtext.cpp
    rs_mtext.h
    rs_point.cpp
    rs_point.h
    rs_polyline.cpp
    rs_polyline.h
    rs_solid.cpp
    rs_solid.h
    rs_spline.cpp
    rs_spline.h
    rs_text.cpp
    rs_text.h
    lc_parabola.cpp
    lc_cachedlengthentity.h
    lc_cachedlengthentity.cpp
    lc_extentitydata.h
    lc_extentitydata.cpp
)
list(TRANSFORM LIB_ENGINE_ENTITIES_FILES PREPEND "${LIB_ENGINE_ENTITIES_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_ENTITIES_FILES})

# Group 41: lib engine document entities support
set(LIB_ENGINE_ENTITIES_SUPPORT_PATH "librecad/src/lib/engine/document/entities/support/")
set(LIB_ENGINE_ENTITIES_SUPPORT_FILES
    lc_arrow_box.h
    lc_arrow_box.cpp
    lc_arrow_none.h
    lc_arrow_none.cpp
    lc_arrow_circle.h
    lc_arrow_circle.cpp
    lc_arrow_datum.h
    lc_arrow_datum.cpp
    lc_arrow_headclosed.h
    lc_arrow_headclosed.cpp
    lc_arrow_headclosed_blank.h
    lc_arrow_headclosed_blank.cpp
    lc_arrow_headclosed2.h
    lc_arrow_headclosed2.cpp
    lc_arrow_headopen.h
    lc_arrow_headopen.cpp
    lc_arrow_tick.h
    lc_arrow_tick.cpp
    lc_arrow_dot.h
    lc_arrow_dot.cpp
    lc_dimarrowblock.h
    lc_dimarrowblock.cpp
    lc_dimarrowblockpoly.h
    lc_dimarrowblockpoly.cpp
    lc_arrow_integral.h
    lc_arrow_integral.cpp
)
list(TRANSFORM LIB_ENGINE_ENTITIES_SUPPORT_FILES PREPEND "${LIB_ENGINE_ENTITIES_SUPPORT_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_ENTITIES_SUPPORT_FILES})

# Group 42: lib engine document container
set(LIB_ENGINE_CONTAINER_PATH "librecad/src/lib/engine/document/container/")
set(LIB_ENGINE_CONTAINER_FILES
    lc_looputils.cpp
    lc_looputils.h
    lc_containertraverser.cpp
    lc_containertraverser.h
    rs_entitycontainer.cpp
    rs_entitycontainer.h
)
list(TRANSFORM LIB_ENGINE_CONTAINER_FILES PREPEND "${LIB_ENGINE_CONTAINER_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_CONTAINER_FILES})

# Group 43: lib engine utils
set(LIB_ENGINE_UTILS_PATH "librecad/src/lib/engine/utils/")
set(LIB_ENGINE_UTILS_FILES
    lc_rtree.cpp
    lc_rtree.h
    rs_utility.cpp
    rs_utility.h
    lc_rectregion.h
    lc_rectregion.cpp
)
list(TRANSFORM LIB_ENGINE_UTILS_FILES PREPEND "${LIB_ENGINE_UTILS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_UTILS_FILES})

# Group 44: lib engine undo
set(LIB_ENGINE_UNDO_PATH "librecad/src/lib/engine/undo/")
set(LIB_ENGINE_UNDO_FILES
    lc_undosection.cpp
    lc_undosection.h
    rs_undo.cpp
    rs_undo.h
    rs_undoable.cpp
    rs_undoable.h
    rs_undocycle.cpp
    rs_undocycle.h
    lc_undoablerelzero.h
    lc_undoablerelzero.cpp
)
list(TRANSFORM LIB_ENGINE_UNDO_FILES PREPEND "${LIB_ENGINE_UNDO_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_UNDO_FILES})

# Group 45: lib engine document blocks
set(LIB_ENGINE_BLOCKS_PATH "librecad/src/lib/engine/document/blocks/")
set(LIB_ENGINE_BLOCKS_FILES
    rs_block.cpp
    rs_block.h
    rs_blocklist.cpp
    rs_blocklist.h
    rs_blocklistlistener.h
)
list(TRANSFORM LIB_ENGINE_BLOCKS_FILES PREPEND "${LIB_ENGINE_BLOCKS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_BLOCKS_FILES})

# Group 46: lib engine clipboard
set(LIB_ENGINE_CLIPBOARD_PATH "librecad/src/lib/engine/clipboard/")
set(LIB_ENGINE_CLIPBOARD_FILES
    rs_clipboard.cpp
    rs_clipboard.h
)
list(TRANSFORM LIB_ENGINE_CLIPBOARD_FILES PREPEND "${LIB_ENGINE_CLIPBOARD_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_CLIPBOARD_FILES})

# Group 47: lib engine document fonts
set(LIB_ENGINE_FONTS_PATH "librecad/src/lib/engine/document/fonts/")
set(LIB_ENGINE_FONTS_FILES
    rs_font.cpp
    rs_font.h
    rs_fontchar.h
    rs_fontlist.cpp
    rs_fontlist.h
)
list(TRANSFORM LIB_ENGINE_FONTS_FILES PREPEND "${LIB_ENGINE_FONTS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_FONTS_FILES})

# Group 48: lib engine document layers
set(LIB_ENGINE_LAYERS_PATH "librecad/src/lib/engine/document/layers/")
set(LIB_ENGINE_LAYERS_FILES
    rs_layer.cpp
    rs_layer.h
    rs_layerlist.cpp
    rs_layerlist.h
    rs_layerlistlistener.h
)
list(TRANSFORM LIB_ENGINE_LAYERS_FILES PREPEND "${LIB_ENGINE_LAYERS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_LAYERS_FILES})

# Group 49: lib engine document dimstyles
set(LIB_ENGINE_DIMSTYLES_PATH "librecad/src/lib/engine/document/dimstyles/")
set(LIB_ENGINE_DIMSTYLES_FILES
    lc_dimstyle.h
    lc_dimstyle.cpp
    lc_dimstyleslist.h
    lc_dimstyleslist.cpp
    lc_dimarrowregistry.h
    lc_dimarrowregistry.cpp
    lc_dimstyletovariablesmapper.h
    lc_dimstyletovariablesmapper.cpp
)
list(TRANSFORM LIB_ENGINE_DIMSTYLES_FILES PREPEND "${LIB_ENGINE_DIMSTYLES_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_DIMSTYLES_FILES})

# Group 50: lib engine document patterns
set(LIB_ENGINE_PATTERNS_PATH "librecad/src/lib/engine/document/patterns/")
set(LIB_ENGINE_PATTERNS_FILES
    rs_pattern.cpp
    rs_pattern.h
    rs_patternlist.cpp
    rs_patternlist.h
)
list(TRANSFORM LIB_ENGINE_PATTERNS_FILES PREPEND "${LIB_ENGINE_PATTERNS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_PATTERNS_FILES})

# Group 51: lib engine settings
set(LIB_ENGINE_SETTINGS_PATH "librecad/src/lib/engine/settings/")
set(LIB_ENGINE_SETTINGS_FILES
    rs_settings.cpp
    rs_settings.h
    lc_settingsexporter.h
    lc_settingsexporter.cpp
)
list(TRANSFORM LIB_ENGINE_SETTINGS_FILES PREPEND "${LIB_ENGINE_SETTINGS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_SETTINGS_FILES})

# Group 52: lib engine document variables
set(LIB_ENGINE_VARIABLES_PATH "librecad/src/lib/engine/document/variables/")
set(LIB_ENGINE_VARIABLES_FILES
    rs_variable.h
    rs_variabledict.cpp
    rs_variabledict.h
)
list(TRANSFORM LIB_ENGINE_VARIABLES_FILES PREPEND "${LIB_ENGINE_VARIABLES_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_VARIABLES_FILES})

# Group 53: lib fileio
set(LIB_FILEIO_PATH "librecad/src/lib/fileio/")
set(LIB_FILEIO_FILES
    rs_fileio.cpp
    rs_fileio.h
    lc_filenameselectionservice.h
    lc_filenameselectionservice.cpp
)
list(TRANSFORM LIB_FILEIO_FILES PREPEND "${LIB_FILEIO_PATH}")
list(APPEND LIB_SOURCES ${LIB_FILEIO_FILES})

# Group 54: lib filters
set(LIB_FILTERS_PATH "librecad/src/lib/filters/")
set(LIB_FILTERS_FILES
    rs_filtercxf.cpp
    rs_filterdxf1.cpp
    rs_filterdxf1.h
    rs_filterdxfrw.cpp
    rs_filterdxfrw.h
    rs_filterinterface.h
    rs_filterjww.cpp
    rs_filterjww.h
    rs_filterlff.cpp
    rs_filterlff.h
)
list(TRANSFORM LIB_FILTERS_FILES PREPEND "${LIB_FILTERS_PATH}")
list(APPEND LIB_SOURCES ${LIB_FILTERS_FILES})

# Group 55: lib generators makercamsvg
set(LIB_GENERATORS_MAKERCAMSVG_PATH "librecad/src/lib/generators/makercamsvg/")
set(LIB_GENERATORS_MAKERCAMSVG_FILES
    lc_makercamsvg.cpp
    lc_makercamsvg.h
    lc_xmlwriterinterface.h
    lc_xmlwriterqxmlstreamwriter.cpp
    lc_xmlwriterqxmlstreamwriter.h
)
list(TRANSFORM LIB_GENERATORS_MAKERCAMSVG_FILES PREPEND "${LIB_GENERATORS_MAKERCAMSVG_PATH}")
list(APPEND LIB_SOURCES ${LIB_GENERATORS_MAKERCAMSVG_FILES})

# Group 56: lib generators layers
set(LIB_GENERATORS_LAYERS_PATH "librecad/src/lib/generators/layers/")
set(LIB_GENERATORS_LAYERS_FILES
    lc_layersexporter.h
    lc_layersexporter.cpp
)
list(TRANSFORM LIB_GENERATORS_LAYERS_FILES PREPEND "${LIB_GENERATORS_LAYERS_PATH}")
list(APPEND LIB_SOURCES ${LIB_GENERATORS_LAYERS_FILES})

# Group 57: lib generators image
set(LIB_GENERATORS_IMAGE_PATH "librecad/src/lib/generators/image/")
set(LIB_GENERATORS_IMAGE_FILES
    lc_imageexporter.h
    lc_imageexporter.cpp
)
list(TRANSFORM LIB_GENERATORS_IMAGE_FILES PREPEND "${LIB_GENERATORS_IMAGE_PATH}")
list(APPEND LIB_SOURCES ${LIB_GENERATORS_IMAGE_FILES})

# Group 58: lib gui
set(LIB_GUI_PATH "librecad/src/lib/gui/")
set(LIB_GUI_FILES
    rs_commandevent.h
    rs_coordinateevent.h
    rs_dialogfactory.cpp
    rs_dialogfactory.h
    rs_dialogfactoryinterface.h
    lc_eventhandler.h
    lc_eventhandler.cpp
    rs_graphicview.cpp
    rs_graphicview.h
    rs_linetypepattern.cpp
    rs_linetypepattern.h
    rs_mainwindowinterface.h
    lc_coordinates_mapper.cpp
    lc_coordinates_mapper.h
    lc_coordinates_parser.h
    lc_coordinates_parser.cpp
    lc_graphicviewport.h
    lc_graphicviewport.cpp
    lc_latecompletionrequestor.h
    lc_latecompletionrequestor.cpp
    lc_graphicviewportlistener.h
    lc_graphicviewportlistener.cpp
)
list(TRANSFORM LIB_GUI_FILES PREPEND "${LIB_GUI_PATH}")
list(APPEND LIB_SOURCES ${LIB_GUI_FILES})

# Group 59: lib gui grid
set(LIB_GUI_GRID_PATH "librecad/src/lib/gui/grid/")
set(LIB_GUI_GRID_FILES
    rs_grid.cpp
    rs_grid.h
    lc_lattice.h
    lc_lattice.cpp
    lc_orthogonalgrid.h
    lc_orthogonalgrid.cpp
    lc_gridsystem.h
    lc_gridsystem.cpp
    lc_isometricgrid.cpp
)
list(TRANSFORM LIB_GUI_GRID_FILES PREPEND "${LIB_GUI_GRID_PATH}")
list(APPEND LIB_SOURCES ${LIB_GUI_GRID_FILES})

# Group 60: lib gui render
set(LIB_GUI_RENDER_PATH "librecad/src/lib/gui/render/")
set(LIB_GUI_RENDER_FILES
    rs_painter.cpp
    rs_painter.h
    lc_graphicviewportrenderer.h
    lc_graphicviewportrenderer.cpp
)
list(TRANSFORM LIB_GUI_RENDER_FILES PREPEND "${LIB_GUI_RENDER_PATH}")
list(APPEND LIB_SOURCES ${LIB_GUI_RENDER_FILES})

# Group 61: lib gui render widget
set(LIB_GUI_RENDER_WIDGET_PATH "librecad/src/lib/gui/render/widget/")
set(LIB_GUI_RENDER_WIDGET_FILES
    lc_printpreviewviewrenderer.h
    lc_printpreviewviewrenderer.cpp
    lc_graphicviewrenderer.h
    lc_graphicviewrenderer.cpp
    lc_widgetviewportrenderer.h
    lc_widgetviewportrenderer.cpp
)
list(TRANSFORM LIB_GUI_RENDER_WIDGET_FILES PREPEND "${LIB_GUI_RENDER_WIDGET_PATH}")
list(APPEND LIB_SOURCES ${LIB_GUI_RENDER_WIDGET_FILES})

# Group 62: lib gui render headless
set(LIB_GUI_RENDER_HEADLESS_PATH "librecad/src/lib/gui/render/headless/")
set(LIB_GUI_RENDER_HEADLESS_FILES
    lc_printviewportrenderer.h
    lc_printviewportrenderer.cpp
)
list(TRANSFORM LIB_GUI_RENDER_HEADLESS_FILES PREPEND "${LIB_GUI_RENDER_HEADLESS_PATH}")
list(APPEND LIB_SOURCES ${LIB_GUI_RENDER_HEADLESS_FILES})

# Group 63: lib information
set(LIB_INFORMATION_PATH "librecad/src/lib/information/")
set(LIB_INFORMATION_FILES
    rs_infoarea.cpp
    rs_infoarea.h
    rs_information.cpp
    rs_information.h
    rs_locale.cpp
    rs_locale.h
)
list(TRANSFORM LIB_INFORMATION_FILES PREPEND "${LIB_INFORMATION_PATH}")
list(APPEND LIB_SOURCES ${LIB_INFORMATION_FILES})

# Group 64: lib math
set(LIB_MATH_PATH "librecad/src/lib/math/")
set(LIB_MATH_FILES
    lc_quadratic.cpp
    lc_quadratic.h
    rs_math.cpp
    rs_math.h
    lc_linemath.h
    lc_linemath.cpp
    lc_convert.h
    lc_convert.cpp
)
list(TRANSFORM LIB_MATH_FILES PREPEND "${LIB_MATH_PATH}")
list(APPEND LIB_SOURCES ${LIB_MATH_FILES})

# Group 65: lib modification
set(LIB_MODIFICATION_PATH "librecad/src/lib/modification/")
set(LIB_MODIFICATION_FILES
    rs_modification.cpp
    rs_modification.h
    rs_selection.cpp
    rs_selection.h
    lc_align.h
    lc_align.cpp
    lc_division.h
    lc_division.cpp
)
list(TRANSFORM LIB_MODIFICATION_FILES PREPEND "${LIB_MODIFICATION_PATH}")
list(APPEND LIB_SOURCES ${LIB_MODIFICATION_FILES})

# Group 66: lib printing
set(LIB_PRINTING_PATH "librecad/src/lib/printing/")
set(LIB_PRINTING_FILES
    lc_printing.cpp
    lc_printing.h
)
list(TRANSFORM LIB_PRINTING_FILES PREPEND "${LIB_PRINTING_PATH}")
list(APPEND LIB_SOURCES ${LIB_PRINTING_FILES})

# Group 67: lib scripting
set(LIB_SCRIPTING_PATH "librecad/src/lib/scripting/")
set(LIB_SCRIPTING_FILES
    rs_python.cpp
    rs_python.h
    rs_python_wrappers.cpp
    rs_python_wrappers.h
    rs_script.cpp
    rs_script.h
    rs_scriptlist.cpp
    rs_scriptlist.h
    rs_simplepython.cpp
    rs_simplepython.h
)
list(TRANSFORM LIB_SCRIPTING_FILES PREPEND "${LIB_SCRIPTING_PATH}")
list(APPEND LIB_SOURCES ${LIB_SCRIPTING_FILES})

# Group 68: main console_dxf2pdf
set(MAIN_CONSOLE_DXF2PDF_PATH "librecad/src/main/console_dxf2pdf/")
set(MAIN_CONSOLE_DXF2PDF_FILES
    console_dxf2pdf.cpp
    console_dxf2pdf.h
    pdf_print_loop.cpp
    pdf_print_loop.h
)
list(TRANSFORM MAIN_CONSOLE_DXF2PDF_FILES PREPEND "${MAIN_CONSOLE_DXF2PDF_PATH}")
list(APPEND LIB_SOURCES ${MAIN_CONSOLE_DXF2PDF_FILES})

# Group 69: main
set(MAIN_PATH "librecad/src/main/")
set(MAIN_FILES
    console_dxf2png.cpp
    console_dxf2png.h
    doc_plugin_interface.cpp
    doc_plugin_interface.h
    lc_application.cpp
    lc_application.h
    qc_dialogfactory.cpp
    qc_dialogfactory.h
)
list(TRANSFORM MAIN_FILES PREPEND "${MAIN_PATH}")
list(APPEND LIB_SOURCES ${MAIN_FILES})

# Group 70: plugins intern
set(PLUGINS_INTERN_PATH "librecad/src/plugins/intern/")
set(PLUGINS_INTERN_FILES
    qc_actiongetent.cpp
    qc_actiongetent.h
    qc_actiongetpoint.cpp
    qc_actiongetpoint.h
    qc_actiongetselect.cpp
    qc_actiongetselect.h
)
list(TRANSFORM PLUGINS_INTERN_FILES PREPEND "${PLUGINS_INTERN_PATH}")
list(APPEND LIB_SOURCES ${PLUGINS_INTERN_FILES})

# Group 71: plugins
set(PLUGINS_PATH "librecad/src/plugins/")
set(PLUGINS_FILES
    document_interface.h
    qc_plugininterface.h
    lc_plugininvoker.h
    lc_plugininvoker.cpp
)
list(TRANSFORM PLUGINS_FILES PREPEND "${PLUGINS_PATH}")
list(APPEND LIB_SOURCES ${PLUGINS_FILES})

# Group 72: ui dialog entity
set(UI_DIALOG_ENTITY_PATH "librecad/src/ui/dialogs/entity/")
set(UI_DIALOG_ENTITY_FILES
    lc_dlgtolerance.h
    lc_dlgtolerance.cpp
    qg_blockdialog.cpp
    qg_blockdialog.h
    lc_pointpickbutton.h
    lc_pointpickbutton.cpp
    qg_dimensionlabeleditor.cpp
    qg_dimensionlabeleditor.h
    qg_dlgattributes.cpp
    qg_dlgattributes.h
    qg_dlghatch.cpp
    qg_dlghatch.h
    qg_dlgmtext.cpp
    qg_dlgmtext.h
    qg_dlgtext.cpp
    qg_dlgtext.h
    lc_entitypropertiesdlg.h
    lc_entitypropertiesdlg.cpp
    lc_entitypropertieseditor.h
    lc_entitypropertieseditor.cpp
    lc_entitypropertieseditorwidget.h
    lc_entitypropertieseditorwidget.cpp
    lc_entitypropertieseditorsupport.h
    lc_entitypropertieseditorsupport.cpp
    lc_pointpropertieseditingwidget.h
    lc_pointpropertieseditingwidget.cpp
    lc_linepropertieseditingwidget.h
    lc_linepropertieseditingwidget.cpp
    lc_arcpropertieseditingwidget.h
    lc_arcpropertieseditingwidget.cpp
    lc_circlepropertieseditingwidget.h
    lc_circlepropertieseditingwidget.cpp
    lc_ellipsepropertieseditingwidget.h
    lc_ellipsepropertieseditingwidget.cpp
    lc_polylinepropertieseditingwidget.h
    lc_polylinepropertieseditingwidget.cpp
    lc_splinepropertieseditingwidget.h
    lc_splinepropertieseditingwidget.cpp
    lc_splinepointspropertieseditingwidget.h
    lc_splinepointspropertieseditingwidget.cpp
    lc_imagepropertieseditingwidget.h
    lc_imagepropertieseditingwidget.cpp
    lc_insertpropertieseditingwidget.h
    lc_insertpropertieseditingwidget.cpp
    lc_parabolapropertieseditingwidget.h
    lc_parabolapropertieseditingwidget.cpp
    lc_dlgentityproperties.h
    lc_dlgentityproperties.cpp
    lc_dlgdimension.h
    lc_dlgdimension.cpp
)
list(TRANSFORM UI_DIALOG_ENTITY_FILES PREPEND "${UI_DIALOG_ENTITY_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_ENTITY_FILES})

# Group 73: ui dialog settings options_widget
set(UI_DIALOG_SETTINGS_OPTIONS_WIDGET_PATH "librecad/src/ui/dialogs/settings/options_widget/")
set(UI_DIALOG_SETTINGS_OPTIONS_WIDGET_FILES
    lc_widgetoptionsdialog.cpp
    lc_widgetoptionsdialog.h
    lc_iconcolorsoptions.h
    lc_iconcolorsoptions.cpp
    lc_iconengineshared.h
    lc_iconengineshared.cpp
    lc_dlgiconssetup.cpp
)
list(TRANSFORM UI_DIALOG_SETTINGS_OPTIONS_WIDGET_FILES PREPEND "${UI_DIALOG_SETTINGS_OPTIONS_WIDGET_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_SETTINGS_OPTIONS_WIDGET_FILES})

# Group 74: ui components status_bar
set(UI_COMPONENTS_STATUS_BAR_PATH "librecad/src/ui/components/status_bar/")
set(UI_COMPONENTS_STATUS_BAR_FILES
    qg_activelayername.cpp
    qg_activelayername.h
    qg_coordinatewidget.cpp
    qg_coordinatewidget.h
    qg_mousewidget.cpp
    qg_mousewidget.h
    qg_selectionwidget.cpp
    qg_selectionwidget.h
    twostackedlabels.cpp
    twostackedlabels.h
    lc_relzerocoordinateswidget.h
    lc_relzerocoordinateswidget.cpp
    lc_anglesbasiswidget.h
    lc_anglesbasiswidget.cpp
    lc_qtstatusbarmanager.h
    lc_qtstatusbarmanager.cpp
    lc_ucsstatewidget.h
    lc_ucsstatewidget.cpp
)
list(TRANSFORM UI_COMPONENTS_STATUS_BAR_FILES PREPEND "${UI_COMPONENTS_STATUS_BAR_PATH}")
list(APPEND LIB_SOURCES ${UI_COMPONENTS_STATUS_BAR_FILES})

# Group 75: ui action_options curve
set(UI_ACTION_OPTIONS_CURVE_PATH "librecad/src/ui/action_options/curve/")
set(UI_ACTION_OPTIONS_CURVE_FILES
    qg_arcoptions.cpp
    qg_arcoptions.h
    qg_arctangentialoptions.cpp
    qg_arctangentialoptions.h
    lc_ellipsearcoptions.cpp
    lc_actiondrawarc2poptions.h
    lc_actiondrawarc2poptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_CURVE_FILES PREPEND "${UI_ACTION_OPTIONS_CURVE_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_CURVE_FILES})

# Group 76: ui action_options modify
set(UI_ACTION_OPTIONS_MODIFY_PATH "librecad/src/ui/action_options/modify/")
set(UI_ACTION_OPTIONS_MODIFY_FILES
    qg_beveloptions.cpp
    qg_beveloptions.h
    qg_modifyoffsetoptions.cpp
    qg_modifyoffsetoptions.h
    qg_moverotateoptions.cpp
    qg_moverotateoptions.h
    qg_roundoptions.cpp
    qg_roundoptions.h
    qg_trimamountoptions.cpp
    qg_trimamountoptions.h
    lc_linejoinoptions.cpp
    lc_duplicateoptions.cpp
    lc_modifybreakdivideoptions.h
    lc_modifybreakdivideoptions.cpp
    lc_modifygapoptions.h
    lc_modifygapoptions.cpp
    lc_modifymirroroptions.cpp
    lc_modifystretchoptions.cpp
    lc_modifystretchoptions.h
    lc_moveoptions.h
    lc_moveoptions.cpp
    lc_rotate2options.cpp
    lc_rotate2options.h
    lc_modifyscaleoptions.h
    lc_modifyscaleoptions.cpp
    lc_modifyrotateoptions.cpp
    lc_modifyalignoptions.h
    lc_modifyalignoptions.cpp
    lc_modifyalignrefoptions.h
    lc_modifyalignrefoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_MODIFY_FILES PREPEND "${UI_ACTION_OPTIONS_MODIFY_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_MODIFY_FILES})

# Group 77: ui action_options circle
set(UI_ACTION_OPTIONS_CIRCLE_PATH "librecad/src/ui/action_options/circle/")
set(UI_ACTION_OPTIONS_CIRCLE_FILES
    qg_circleoptions.cpp
    qg_circleoptions.h
    qg_circletan2options.cpp
    qg_circletan2options.h
    lc_circlebyarcoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_CIRCLE_FILES PREPEND "${UI_ACTION_OPTIONS_CIRCLE_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_CIRCLE_FILES})

# Group 78: ui dock_widgets command_line
set(UI_DOCK_COMMAND_LINE_PATH "librecad/src/ui/dock_widgets/command_line/")
set(UI_DOCK_COMMAND_LINE_FILES
    qg_commandwidget.cpp
    qg_commandwidget.h
    qg_commandedit.cpp
    qg_commandedit.h
    qg_commandhistory.cpp
    qg_commandhistory.h
)
list(TRANSFORM UI_DOCK_COMMAND_LINE_FILES PREPEND "${UI_DOCK_COMMAND_LINE_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_COMMAND_LINE_FILES})

# Group 79: ui action_options dimensions
set(UI_ACTION_OPTIONS_DIMENSIONS_PATH "librecad/src/ui/action_options/dimensions/")
set(UI_ACTION_OPTIONS_DIMENSIONS_FILES
    qg_dimoptions.cpp
    qg_dimoptions.h
)
list(TRANSFORM UI_ACTION_OPTIONS_DIMENSIONS_FILES PREPEND "${UI_ACTION_OPTIONS_DIMENSIONS_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_DIMENSIONS_FILES})

# Group 80: ui dialog file export image
set(UI_DIALOG_FILE_EXPORT_IMAGE_PATH "librecad/src/ui/dialogs/file/export/image/")
set(UI_DIALOG_FILE_EXPORT_IMAGE_FILES
    qg_dlgimageoptions.cpp
    qg_dlgimageoptions.h
    lc_exporttoimageservice.h
    lc_exporttoimageservice.cpp
)
list(TRANSFORM UI_DIALOG_FILE_EXPORT_IMAGE_FILES PREPEND "${UI_DIALOG_FILE_EXPORT_IMAGE_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_FILE_EXPORT_IMAGE_FILES})

# Group 81: ui dialog main
set(UI_DIALOG_MAIN_PATH "librecad/src/ui/dialogs/main/")
set(UI_DIALOG_MAIN_FILES
    qg_exitdialog.cpp
    qg_exitdialog.h
    lc_dlgnewversionavailable.h
    lc_dlgnewversionavailable.cpp
    lc_dlgabout.cpp
)
list(TRANSFORM UI_DIALOG_MAIN_FILES PREPEND "${UI_DIALOG_MAIN_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_MAIN_FILES})

# Group 82: ui dialog actions modify
set(UI_DIALOG_ACTIONS_MODIFY_PATH "librecad/src/ui/dialogs/actions/modify/")
set(UI_DIALOG_ACTIONS_MODIFY_FILES
    qg_dlgmirror.cpp
    qg_dlgmirror.h
    qg_dlgmove.cpp
    qg_dlgmove.h
    qg_dlgmoverotate.cpp
    qg_dlgmoverotate.h
    qg_dlgrotate.cpp
    qg_dlgrotate.h
    qg_dlgrotate2.cpp
    qg_dlgrotate2.h
    qg_dlgscale.cpp
    qg_dlgscale.h
)
list(TRANSFORM UI_DIALOG_ACTIONS_MODIFY_FILES PREPEND "${UI_DIALOG_ACTIONS_MODIFY_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_ACTIONS_MODIFY_FILES})

# Group 83: ui dialog settings dimstyles dimstyle_manager
set(UI_DIALOG_DIMSTYLE_MANAGER_PATH "librecad/src/ui/dialogs/settings/dimstyles/dimstyle_manager/")
set(UI_DIALOG_DIMSTYLE_MANAGER_FILES
    lc_dlgdimstylemanager.h
    lc_dlgdimstylemanager.cpp
)
list(TRANSFORM UI_DIALOG_DIMSTYLE_MANAGER_FILES PREPEND "${UI_DIALOG_DIMSTYLE_MANAGER_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_DIMSTYLE_MANAGER_FILES})

# Group 84: ui dialog settings dimstyles dimstyle_manager support
set(UI_DIALOG_DIMSTYLE_MANAGER_SUPPORT_PATH "librecad/src/ui/dialogs/settings/dimstyles/dimstyle_manager/support/")
set(UI_DIALOG_DIMSTYLE_MANAGER_SUPPORT_FILES
    lc_tabproxywidget.h
    lc_tabproxywidget.cpp
    lc_dimstylepreviewgraphicview.h
    lc_dimstylepreviewgraphicview.cpp
    lc_dimstylepreviewpanel.h
    lc_dimstylepreviewpanel.cpp
)
list(TRANSFORM UI_DIALOG_DIMSTYLE_MANAGER_SUPPORT_FILES PREPEND "${UI_DIALOG_DIMSTYLE_MANAGER_SUPPORT_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_DIMSTYLE_MANAGER_SUPPORT_FILES})

# Group 85: ui dialog settings options_drawing
set(UI_DIALOG_OPTIONS_DRAWING_PATH "librecad/src/ui/dialogs/settings/options_drawing/")
set(UI_DIALOG_OPTIONS_DRAWING_FILES
    qg_dlgoptionsdrawing.cpp
    qg_dlgoptionsdrawing.h
    lc_dimstylesexporter.h
    lc_dimstylesexporter.cpp
    lc_dlgnewcustomvariable.h
    lc_dlgnewcustomvariable.cpp
    lg_dimzerosbox.cpp
    lg_dimzerosbox.h
    lc_dlgnewdimstyle.h
    lc_dlgnewdimstyle.cpp
)
list(TRANSFORM UI_DIALOG_OPTIONS_DRAWING_FILES PREPEND "${UI_DIALOG_OPTIONS_DRAWING_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_OPTIONS_DRAWING_FILES})

# Group 86: ui dialog settings dimstyles
set(UI_DIALOG_DIMSTYLES_PATH "librecad/src/ui/dialogs/settings/dimstyles/")
set(UI_DIALOG_DIMSTYLES_FILES
    lc_dimstyleslistmodel.h
    lc_dimstyleslistmodel.cpp
    lc_dimstylestreemodel.h
    lc_dimstylestreemodel.cpp
    lc_dimstyleitem.cpp
)
list(TRANSFORM UI_DIALOG_DIMSTYLES_FILES PREPEND "${UI_DIALOG_DIMSTYLES_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_DIMSTYLES_FILES})

# Group 87: ui dialog settings options_general
set(UI_DIALOG_OPTIONS_GENERAL_PATH "librecad/src/ui/dialogs/settings/options_general/")
set(UI_DIALOG_OPTIONS_GENERAL_FILES
    qg_dlgoptionsgeneral.cpp
    qg_dlgoptionsgeneral.h
)
list(TRANSFORM UI_DIALOG_OPTIONS_GENERAL_FILES PREPEND "${UI_DIALOG_OPTIONS_GENERAL_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_OPTIONS_GENERAL_FILES})

# Group 88: ui dialog file export makercam
set(UI_DIALOG_EXPORT_MAKERCAM_PATH "librecad/src/ui/dialogs/file/export/makercam/")
set(UI_DIALOG_EXPORT_MAKERCAM_FILES
    qg_dlgoptionsmakercam.cpp
    qg_dlgoptionsmakercam.h
)
list(TRANSFORM UI_DIALOG_EXPORT_MAKERCAM_FILES PREPEND "${UI_DIALOG_EXPORT_MAKERCAM_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_EXPORT_MAKERCAM_FILES})

# Group 89: ui action_options image
set(UI_ACTION_OPTIONS_IMAGE_PATH "librecad/src/ui/action_options/image/")
set(UI_ACTION_OPTIONS_IMAGE_FILES
    qg_imageoptions.cpp
    qg_imageoptions.h
)
list(TRANSFORM UI_ACTION_OPTIONS_IMAGE_FILES PREPEND "${UI_ACTION_OPTIONS_IMAGE_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_IMAGE_FILES})

# Group 90: ui action_options insert
set(UI_ACTION_OPTIONS_INSERT_PATH "librecad/src/ui/action_options/insert/")
set(UI_ACTION_OPTIONS_INSERT_FILES
    qg_insertoptions.cpp
    qg_insertoptions.h
    qg_libraryinsertoptions.cpp
    qg_libraryinsertoptions.h
)
list(TRANSFORM UI_ACTION_OPTIONS_INSERT_FILES PREPEND "${UI_ACTION_OPTIONS_INSERT_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_INSERT_FILES})

# Group 91: ui dialog actions
set(UI_DIALOG_ACTIONS_PATH "librecad/src/ui/dialogs/actions/")
set(UI_DIALOG_ACTIONS_FILES
    qg_layerdialog.cpp
    qg_layerdialog.h
)
list(TRANSFORM UI_DIALOG_ACTIONS_FILES PREPEND "${UI_DIALOG_ACTIONS_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_ACTIONS_FILES})

# Group 92: ui dock_widgets layers_tree
set(UI_DOCK_LAYERS_TREE_PATH "librecad/src/ui/dock_widgets/layers_tree/")
set(UI_DOCK_LAYERS_TREE_FILES
    lc_layerdialog_ex.cpp
    lc_layerdialog_ex.h
    lc_layertreeitem.cpp
    lc_layertreeitem.h
    lc_layertreemodel.cpp
    lc_layertreemodel.h
    lc_layertreewidget.cpp
    lc_layertreewidget.h
    lc_layertreeview.cpp
    lc_layertreeview.h
    lc_layertreeoptionsdialog.cpp
    lc_layertreemodel_options.cpp
)
list(TRANSFORM UI_DOCK_LAYERS_TREE_FILES PREPEND "${UI_DOCK_LAYERS_TREE_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_LAYERS_TREE_FILES})

# Group 93: ui dialog file export layers
set(UI_DIALOG_EXPORT_LAYERS_PATH "librecad/src/ui/dialogs/file/export/layers/")
set(UI_DIALOG_EXPORT_LAYERS_FILES
    lc_layerexportoptions.h
    lc_layerexportoptions.cpp
    lc_filedialogservice.cpp
    lc_filedialogservice.h
    lc_exportlayersdialogservice.h
    lc_exportlayersdialogservice.cpp
)
list(TRANSFORM UI_DIALOG_EXPORT_LAYERS_FILES PREPEND "${UI_DIALOG_EXPORT_LAYERS_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_EXPORT_LAYERS_FILES})

# Group 94: ui action_options line
set(UI_ACTION_OPTIONS_LINE_PATH "librecad/src/ui/action_options/line/")
set(UI_ACTION_OPTIONS_LINE_FILES
    qg_lineangleoptions.cpp
    qg_lineangleoptions.h
    qg_linebisectoroptions.cpp
    qg_linebisectoroptions.h
    qg_lineoptions.cpp
    qg_lineoptions.h
    qg_lineparalleloptions.cpp
    qg_lineparalleloptions.h
    qg_lineparallelthroughoptions.cpp
    qg_lineparallelthroughoptions.h
    qg_linerelangleoptions.cpp
    qg_linerelangleoptions.h
    lc_lineoptions.cpp
    lc_lineanglereloptions.cpp
    lc_slicedivideoptions.cpp
    lc_linefrompointtolineoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_LINE_FILES PREPEND "${UI_ACTION_OPTIONS_LINE_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_LINE_FILES})

# Group 95: ui action_options polygon
set(UI_ACTION_OPTIONS_POLYGON_PATH "librecad/src/ui/action_options/polygon/")
set(UI_ACTION_OPTIONS_POLYGON_FILES
    qg_linepolygonoptions.cpp
    qg_linepolygonoptions.h
    lc_staroptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_POLYGON_FILES PREPEND "${UI_ACTION_OPTIONS_POLYGON_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_POLYGON_FILES})

# Group 96: ui action_options text
set(UI_ACTION_OPTIONS_TEXT_PATH "librecad/src/ui/action_options/text/")
set(UI_ACTION_OPTIONS_TEXT_FILES
    qg_mtextoptions.cpp
    qg_mtextoptions.h
    qg_textoptions.cpp
    qg_textoptions.h
)
list(TRANSFORM UI_ACTION_OPTIONS_TEXT_FILES PREPEND "${UI_ACTION_OPTIONS_TEXT_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_TEXT_FILES})

# Group 97: ui action_options polyline
set(UI_ACTION_OPTIONS_POLYLINE_PATH "librecad/src/ui/action_options/polyline/")
set(UI_ACTION_OPTIONS_POLYLINE_FILES
    qg_polylineequidistantoptions.cpp
    qg_polylineequidistantoptions.h
    qg_polylineoptions.cpp
    qg_polylineoptions.h
)
list(TRANSFORM UI_ACTION_OPTIONS_POLYLINE_FILES PREPEND "${UI_ACTION_OPTIONS_POLYLINE_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_POLYLINE_FILES})

# Group 98: ui action_options print_preview
set(UI_ACTION_OPTIONS_PRINT_PREVIEW_PATH "librecad/src/ui/action_options/print_preview/")
set(UI_ACTION_OPTIONS_PRINT_PREVIEW_FILES
    qg_printpreviewoptions.cpp
    qg_printpreviewoptions.h
)
list(TRANSFORM UI_ACTION_OPTIONS_PRINT_PREVIEW_FILES PREPEND "${UI_ACTION_OPTIONS_PRINT_PREVIEW_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_PRINT_PREVIEW_FILES})

# Group 99: ui action_options snap
set(UI_ACTION_OPTIONS_SNAP_PATH "librecad/src/ui/action_options/snap/")
set(UI_ACTION_OPTIONS_SNAP_FILES
    qg_snapdistoptions.cpp
    qg_snapdistoptions.h
    qg_snapmiddleoptions.cpp
    qg_snapmiddleoptions.h
)
list(TRANSFORM UI_ACTION_OPTIONS_SNAP_FILES PREPEND "${UI_ACTION_OPTIONS_SNAP_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_SNAP_FILES})

# Group 100: ui components toolbars
set(UI_COMPONENTS_TOOLBARS_PATH "librecad/src/ui/components/toolbars/")
set(UI_COMPONENTS_TOOLBARS_FILES
    qg_snaptoolbar.cpp
    qg_snaptoolbar.h
    qg_pentoolbar.cpp
    qg_pentoolbar.h
)
list(TRANSFORM UI_COMPONENTS_TOOLBARS_FILES PREPEND "${UI_COMPONENTS_TOOLBARS_PATH}")
list(APPEND LIB_SOURCES ${UI_COMPONENTS_TOOLBARS_FILES})

# Group 101: ui action_options spline
set(UI_ACTION_OPTIONS_SPLINE_PATH "librecad/src/ui/action_options/spline/")
set(UI_ACTION_OPTIONS_SPLINE_FILES
    qg_splineoptions.cpp
    qg_splineoptions.h
    lc_splineexplodeoptions.h
    lc_splineexplodeoptions.cpp
    lc_splinefrompolylineoptions.h
    lc_splinefrompolylineoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_SPLINE_FILES PREPEND "${UI_ACTION_OPTIONS_SPLINE_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_SPLINE_FILES})

# Group 102: ui components pen
set(UI_COMPONENTS_PEN_PATH "librecad/src/ui/components/pen/")
set(UI_COMPONENTS_PEN_FILES
    qg_widgetpen.cpp
    qg_widgetpen.h
)
list(TRANSFORM UI_COMPONENTS_PEN_FILES PREPEND "${UI_COMPONENTS_PEN_PATH}")
list(APPEND LIB_SOURCES ${UI_COMPONENTS_PEN_FILES})

# Group 103: ui components creators
set(UI_COMPONENTS_CREATORS_PATH "librecad/src/ui/components/creators/")
set(UI_COMPONENTS_CREATORS_FILES
    actionlist.cpp
    actionlist.h
    lc_creatorinvoker.h
    lc_creatorinvoker.cpp
    lc_menuactivator.h
    lc_menuactivator.cpp
)
list(TRANSFORM UI_COMPONENTS_CREATORS_FILES PREPEND "${UI_COMPONENTS_CREATORS_PATH}")
list(APPEND LIB_SOURCES ${UI_COMPONENTS_CREATORS_FILES})

# Group 104: ui dialog creators
set(UI_DIALOG_CREATORS_PATH "librecad/src/ui/dialogs/creators/")
set(UI_DIALOG_CREATORS_FILES
    lc_dlgmenuassigner.h
    lc_dlgmenuassigner.cpp
    lc_dlgwidgetcreator.h
    lc_dlgwidgetcreator.cpp
    lc_dlgnewwidget.h
    lc_dlgnewwidget.cpp
)
list(TRANSFORM UI_DIALOG_CREATORS_FILES PREPEND "${UI_DIALOG_CREATORS_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_CREATORS_FILES})

# Group 105: ui dock_widgets pen_wizard
set(UI_DOCK_PEN_WIZARD_PATH "librecad/src/ui/dock_widgets/pen_wizard/")
set(UI_DOCK_PEN_WIZARD_FILES
    colorcombobox.cpp
    colorcombobox.h
    colorwizard.cpp
    colorwizard.h
    lc_penwizard.cpp
    lc_penwizard.h
)
list(TRANSFORM UI_DOCK_PEN_WIZARD_FILES PREPEND "${UI_DOCK_PEN_WIZARD_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_PEN_WIZARD_FILES})

# Group 106: ui components comboboxes
set(UI_COMPONENTS_COMBOBOXES_PATH "librecad/src/ui/components/comboboxes/")
set(UI_COMPONENTS_COMBOBOXES_FILES
    comboboxoption.cpp
    comboboxoption.h
    qg_colorbox.cpp
    qg_colorbox.h
    qg_fontbox.cpp
    qg_fontbox.h
    qg_layerbox.cpp
    qg_layerbox.h
    qg_linetypebox.cpp
    qg_linetypebox.h
    qg_patternbox.cpp
    qg_patternbox.h
    qg_widthbox.cpp
    qg_widthbox.h
)
list(TRANSFORM UI_COMPONENTS_COMBOBOXES_FILES PREPEND "${UI_COMPONENTS_COMBOBOXES_PATH}")
list(APPEND LIB_SOURCES ${UI_COMPONENTS_COMBOBOXES_FILES})

# Group 107: ui components
set(UI_COMPONENTS_PATH "librecad/src/ui/components/")
set(UI_COMPONENTS_FILES
    textfileviewer.cpp
    textfileviewer.h
    lc_plaintextedit.h
    qg_scrollbar.h
    containers/lc_optionswidgetsholder.cpp
    containers/lc_optionswidgetsholder.h
    containers/lc_snapoptionswidgetsholder.cpp
    containers/lc_snapoptionswidgetsholder.h
)
list(TRANSFORM UI_COMPONENTS_FILES PREPEND "${UI_COMPONENTS_PATH}")
list(APPEND LIB_SOURCES ${UI_COMPONENTS_FILES})

# Group 108: ui main init
set(UI_MAIN_INIT_PATH "librecad/src/ui/main/init/")
set(UI_MAIN_INIT_FILES
    lc_actionfactory.cpp
    lc_actionfactory.h
    lc_widgetfactory.cpp
    lc_widgetfactory.h
    lc_toolbarfactory.h
    lc_toolbarfactory.cpp
    lc_menufactory.cpp
    lc_menufactory.h
    lc_applicationwindowinitializer.h
    lc_applicationwindowinitializer.cpp
)
list(TRANSFORM UI_MAIN_INIT_FILES PREPEND "${UI_MAIN_INIT_PATH}")
list(APPEND LIB_SOURCES ${UI_MAIN_INIT_FILES})

set(UI_MAIN_INIT_PERSISTENCE_PATH "librecad/src/ui/main/persistence/")
set(UI_MAIN_INIT_PERSISTENCE_FILES
lc_documentsstorage.h
lc_documentsstorage.cpp
)
list(TRANSFORM UI_MAIN_INIT_PERSISTENCE_FILES PREPEND "${UI_MAIN_INIT_PERSISTENCE_PATH}")
list(APPEND LIB_SOURCES ${UI_MAIN_INIT_PERSISTENCE_FILES})

# Group 109: ui actions
set(UI_ACTIONS_PATH "librecad/src/ui/actions/")
set(UI_ACTIONS_FILES
    lc_actiongroupmanager.cpp
    lc_actiongroupmanager.h
    lc_actionfactorybase.h
    lc_actionfactorybase.cpp
    lc_shortcutinfo.h
    lc_shortcutsstorage.h
    lc_shortcutsstorage.cpp
    lc_actiongroup.h
    lc_actiongroup.cpp
    lc_shortcutinfo.cpp
    lc_shortcuts_manager.cpp
    lc_shortcuts_manager.h
)
list(TRANSFORM UI_ACTIONS_FILES PREPEND "${UI_ACTIONS_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTIONS_FILES})

# Group 110: ui view
set(UI_VIEW_PATH "librecad/src/ui/view/")
set(UI_VIEW_FILES
    lc_centralwidget.cpp
    lc_centralwidget.h
    qg_graphicview.cpp
    qg_graphicview.h
    lc_printpreviewview.h
    lc_printpreviewview.cpp
)
list(TRANSFORM UI_VIEW_FILES PREPEND "${UI_VIEW_PATH}")
list(APPEND LIB_SOURCES ${UI_VIEW_FILES})

# Group 111: ui dialog settings options_device
set(UI_DIALOG_OPTIONS_DEVICE_PATH "librecad/src/ui/dialogs/settings/options_device/")
set(UI_DIALOG_OPTIONS_DEVICE_FILES
    lc_deviceoptions.cpp
    lc_deviceoptions.h
)
list(TRANSFORM UI_DIALOG_OPTIONS_DEVICE_FILES PREPEND "${UI_DIALOG_OPTIONS_DEVICE_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_OPTIONS_DEVICE_FILES})

# Group 112: ui dock_widgets
set(UI_DOCK_WIDGETS_PATH "librecad/src/ui/dock_widgets/")
set(UI_DOCK_WIDGETS_FILES
    lc_dockwidget.h
    lc_dockwidget.cpp
    lc_graphicviewawarewidget.h
    lc_graphicviewawarewidget.cpp
)
list(TRANSFORM UI_DOCK_WIDGETS_FILES PREPEND "${UI_DOCK_WIDGETS_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_WIDGETS_FILES})

# Group 113: ui dock_widgets cad
set(UI_DOCK_CAD_PATH "librecad/src/ui/dock_widgets/cad/")
set(UI_DOCK_CAD_FILES
    lc_caddockwidget.h
    lc_caddockwidget.cpp
)
list(TRANSFORM UI_DOCK_CAD_FILES PREPEND "${UI_DOCK_CAD_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_CAD_FILES})

# Group 114: ui main
set(UI_MAIN_PATH "librecad/src/ui/main/")
set(UI_MAIN_FILES
    mainwindowx.cpp
    mainwindowx.h
    qc_applicationwindow.h
    qc_applicationwindow.cpp
    lc_appwindowaware.h
    lc_appwindowaware.cpp
    qc_mdiwindow.cpp
    qc_mdiwindow.h
    lc_defaultactioncontext.h
    lc_defaultactioncontext.cpp
    lc_mdiapplicationwindow.h
    lc_mdiapplicationwindow.cpp
)
list(TRANSFORM UI_MAIN_FILES PREPEND "${UI_MAIN_PATH}")
list(APPEND LIB_SOURCES ${UI_MAIN_FILES})

# Group 115: ui main fontviewer
set(UI_MAIN_FONTVIEWER_PATH "librecad/src/ui/main/fontviewer/")
set(UI_MAIN_FONTVIEWER_FILES
    lc_fontfileviewer.h
    lc_fontfileviewer.cpp
)
list(TRANSFORM UI_MAIN_FONTVIEWER_FILES PREPEND "${UI_MAIN_FONTVIEWER_PATH}")
list(APPEND LIB_SOURCES ${UI_MAIN_FONTVIEWER_FILES})

# Group 116: ui
set(UI_PATH "librecad/src/ui/")
set(UI_FILES
    qg_actionhandler.cpp
    qg_actionhandler.h
    lc_snapmanager.h
    lc_snapmanager.cpp
    lc_uiutils.h
    lc_uiutils.cpp
    lc_actionhandlerfactory.h
    lc_actionhandlerfactory.cpp
)
list(TRANSFORM UI_FILES PREPEND "${UI_PATH}")
list(APPEND LIB_SOURCES ${UI_FILES})

# Group 117: ui dock_widgets block_widget
set(UI_DOCK_BLOCK_WIDGET_PATH "librecad/src/ui/dock_widgets/block_widget/")
set(UI_DOCK_BLOCK_WIDGET_FILES
    qg_blockwidget.cpp
    qg_blockwidget.h
)
list(TRANSFORM UI_DOCK_BLOCK_WIDGET_FILES PREPEND "${UI_DOCK_BLOCK_WIDGET_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_BLOCK_WIDGET_FILES})

# Group 118: ui dialog
set(UI_DIALOG_PATH "librecad/src/ui/dialogs/")
set(UI_DIALOG_FILES
    qg_dialogfactory.cpp
    qg_dialogfactory.h
    lc_inputtextdialog.h
    lc_inputtextdialog.cpp
    lc_dialog.h
    lc_dialog.cpp
)
list(TRANSFORM UI_DIALOG_FILES PREPEND "${UI_DIALOG_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_FILES})

# Group 119: ui dialog file
set(UI_DIALOG_FILE_PATH "librecad/src/ui/dialogs/file/")
set(UI_DIALOG_FILE_FILES
    qg_filedialog.cpp
    qg_filedialog.h
)
list(TRANSFORM UI_DIALOG_FILE_FILES PREPEND "${UI_DIALOG_FILE_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_FILE_FILES})

# Group 120: ui dock_widgets layer_widget
set(UI_DOCK_LAYER_WIDGET_PATH "librecad/src/ui/dock_widgets/layer_widget/")
set(UI_DOCK_LAYER_WIDGET_FILES
    qg_layerwidget.cpp
    qg_layerwidget.h
)
list(TRANSFORM UI_DOCK_LAYER_WIDGET_FILES PREPEND "${UI_DOCK_LAYER_WIDGET_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_LAYER_WIDGET_FILES})

# Group 121: ui dock_widgets library_widget
set(UI_DOCK_LIBRARY_WIDGET_PATH "librecad/src/ui/dock_widgets/library_widget/")
set(UI_DOCK_LIBRARY_WIDGET_FILES
    qg_librarywidget.cpp
    qg_librarywidget.h
)
list(TRANSFORM UI_DOCK_LIBRARY_WIDGET_FILES PREPEND "${UI_DOCK_LIBRARY_WIDGET_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_LIBRARY_WIDGET_FILES})

# Group 122: ui main support
set(UI_MAIN_SUPPORT_PATH "librecad/src/ui/main/support/")
set(UI_MAIN_SUPPORT_FILES
    qg_recentfiles.cpp
    qg_recentfiles.h
    lc_lastopenfilesopener.h
    lc_lastopenfilesopener.cpp
    lc_appwindowdialogsinvoker.h
    lc_appwindowdialogsinvoker.cpp
    lc_gridviewinvoker.h
    lc_gridviewinvoker.cpp
    lc_infocursorsettingsmanager.h
    lc_infocursorsettingsmanager.cpp
    lc_customstylehelper.h
    lc_customstylehelper.cpp
)
list(TRANSFORM UI_MAIN_SUPPORT_FILES PREPEND "${UI_MAIN_SUPPORT_PATH}")
list(APPEND LIB_SOURCES ${UI_MAIN_SUPPORT_FILES})

# Group 123: ui dock_widgets pen_palette
set(UI_DOCK_PEN_PALETTE_PATH "librecad/src/ui/dock_widgets/pen_palette/")
set(UI_DOCK_PEN_PALETTE_FILES
    lc_penitem.cpp
    lc_penitem.h
    lc_penpalettewidget.cpp
    lc_penpalettewidget.h
    lc_peninforegistry.h
    lc_peninforegistry.cpp
    lc_penpalettemodel.cpp
    lc_penpaletteoptions.cpp
    lc_penpalettedata.h
    lc_penpalettedata.cpp
    lc_penpaletteoptionsdialog.cpp
)
list(TRANSFORM UI_DOCK_PEN_PALETTE_FILES PREPEND "${UI_DOCK_PEN_PALETTE_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_PEN_PALETTE_FILES})

# Group 124: actions drawing pen
set(ACTIONS_DRAW_PEN_PATH "librecad/src/actions/drawing/pen/")
set(ACTIONS_DRAW_PEN_FILES
    lc_actionpenapply.cpp
    lc_actionpenpick.cpp
    lc_actionpensyncactivebylayer.h
    lc_actionpensyncactivebylayer.cpp
)
list(TRANSFORM ACTIONS_DRAW_PEN_FILES PREPEND "${ACTIONS_DRAW_PEN_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_PEN_FILES})

# Group 125: actions drawing draw line misc
set(ACTIONS_DRAW_LINE_MISC_PATH "librecad/src/actions/drawing/draw/line/misc/")
set(ACTIONS_DRAW_LINE_MISC_FILES
    lc_actiondrawcross.cpp
    lc_actiondrawcross.h
    lc_actiondrawmidline.h
    lc_actiondrawmidline.cpp
    lc_actiondrawboundingbox.h
    lc_actiondrawboundingbox.cpp
)
list(TRANSFORM ACTIONS_DRAW_LINE_MISC_FILES PREPEND "${ACTIONS_DRAW_LINE_MISC_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAW_LINE_MISC_FILES})

# Group 126: ui action_options misc
set(UI_ACTION_OPTIONS_MISC_PATH "librecad/src/ui/action_options/misc/")
set(UI_ACTION_OPTIONS_MISC_FILES
    lc_crossoptions.cpp
    lc_midlineoptions.h
    lc_midlineoptions.cpp
    lc_drawboundingboxoptions.h
    lc_drawboundingboxoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_MISC_FILES PREPEND "${UI_ACTION_OPTIONS_MISC_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_MISC_FILES})

# Group 127: ui action_options rect
set(UI_ACTION_OPTIONS_RECT_PATH "librecad/src/ui/action_options/rect/")
set(UI_ACTION_OPTIONS_RECT_FILES
    lc_rectangle1pointoptions.cpp
    lc_rectangle2pointsoptions.h
    lc_rectangle2pointsoptions.cpp
    lc_rectangle3pointsoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_RECT_FILES PREPEND "${UI_ACTION_OPTIONS_RECT_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_RECT_FILES})

# Group 128: ui action_options point
set(UI_ACTION_OPTIONS_POINT_PATH "librecad/src/ui/action_options/point/")
set(UI_ACTION_OPTIONS_POINT_FILES
    lc_linepointsoptions.h
    lc_linepointsoptions.cpp
    lc_pointslatticeoptions.h
    lc_pointslatticeoptions.cpp
    lc_pastetopointsoptions.h
    lc_pastetopointsoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_POINT_FILES PREPEND "${UI_ACTION_OPTIONS_POINT_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_POINT_FILES})

# Group 129: actions drawing
set(ACTIONS_DRAWING_PATH "librecad/src/actions/drawing/")
set(ACTIONS_DRAWING_FILES
    rs_actiondefault.h
    rs_actiondefault.cpp
    lc_abstractactionwithpreview.h
    lc_abstractactionwithpreview.cpp
)
list(TRANSFORM ACTIONS_DRAWING_FILES PREPEND "${ACTIONS_DRAWING_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DRAWING_FILES})

# Group 130: ui dock_widgets entity_info
set(UI_DOCK_ENTITY_INFO_PATH "librecad/src/ui/dock_widgets/entity_info/")
set(UI_DOCK_ENTITY_INFO_FILES
    lc_quickinfowidget.cpp
    lc_quickinfopointsdata.h
    lc_quickinfopointsdata.cpp
    lc_quickinfobasedata.cpp
    lc_quickinfoentitydata.cpp
    lc_quickinfowidgetoptionsdialog.cpp
    lc_quickinfowidgetoptions.h
    lc_quickinfowidgetoptions.cpp
)
list(TRANSFORM UI_DOCK_ENTITY_INFO_FILES PREPEND "${UI_DOCK_ENTITY_INFO_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_ENTITY_INFO_FILES})

# Group 131: actions dock_widgets entity_info
set(ACTIONS_DOCK_ENTITY_INFO_PATH "librecad/src/actions/dock_widgets/entity_info/")
set(ACTIONS_DOCK_ENTITY_INFO_FILES
    lc_actioninfopickcoordinates.cpp
)
list(TRANSFORM ACTIONS_DOCK_ENTITY_INFO_FILES PREPEND "${ACTIONS_DOCK_ENTITY_INFO_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DOCK_ENTITY_INFO_FILES})

# Group 132: lib engine overlays highlight
set(LIB_ENGINE_OVERLAYS_HIGHLIGHT_PATH "librecad/src/lib/engine/overlays/highlight/")
set(LIB_ENGINE_OVERLAYS_HIGHLIGHT_FILES
    lc_highlight.cpp
)
list(TRANSFORM LIB_ENGINE_OVERLAYS_HIGHLIGHT_FILES PREPEND "${LIB_ENGINE_OVERLAYS_HIGHLIGHT_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_OVERLAYS_HIGHLIGHT_FILES})

set(UI_ACTION_OPTIONS_PATH "librecad/src/ui/action_options/")
set(UI_ACTION_OPTIONS_FILES
    lc_actionoptionswidget.h
    lc_actionoptionswidget.cpp
    lc_actionoptionsmanager.h
    lc_actionoptionsmanager.cpp
    lc_actionoptionswidgetbase.cpp
    lc_actionoptionswidgetbase.h
)

# Group 133: ui action_options info
set(UI_ACTION_OPTIONS_INFO_PATH "librecad/src/ui/action_options/info/")
set(UI_ACTION_OPTIONS_INFO_FILES
    lc_infodist2options.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_INFO_FILES PREPEND "${UI_ACTION_OPTIONS_INFO_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_INFO_FILES})

# Group 134: lib engine overlays references
set(LIB_ENGINE_OVERLAYS_REFERENCES_PATH "librecad/src/lib/engine/overlays/references/")
set(LIB_ENGINE_OVERLAYS_REFERENCES_FILES
    lc_refpoint.cpp
    lc_refpoint.h
    lc_refline.h
    lc_refline.cpp
    lc_refcircle.h
    lc_refcircle.cpp
    lc_refarc.cpp
    lc_refarc.h
    lc_refellipse.cpp
    lc_refconstructionline.h
    lc_refconstructionline.cpp
)
list(TRANSFORM LIB_ENGINE_OVERLAYS_REFERENCES_FILES PREPEND "${LIB_ENGINE_OVERLAYS_REFERENCES_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_OVERLAYS_REFERENCES_FILES})

# Group 135: ui action_options selection
set(UI_ACTION_OPTIONS_SELECTION_PATH "librecad/src/ui/action_options/selection/")
set(UI_ACTION_OPTIONS_SELECTION_FILES
    lc_selectwindowoptions.h
    lc_selectwindowoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_SELECTION_FILES PREPEND "${UI_ACTION_OPTIONS_SELECTION_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_SELECTION_FILES})

# Group 136: ui action_options edit
set(UI_ACTION_OPTIONS_EDIT_PATH "librecad/src/ui/action_options/edit/")
set(UI_ACTION_OPTIONS_EDIT_FILES
    lc_pastetransformoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_EDIT_FILES PREPEND "${UI_ACTION_OPTIONS_EDIT_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_EDIT_FILES})

# Group 137: ui dialog settings shortcuts
set(UI_DIALOG_SETTINGS_SHORTCUTS_PATH "librecad/src/ui/dialogs/settings/shortcuts/")
set(UI_DIALOG_SETTINGS_SHORTCUTS_FILES
    lc_shortcutbutton.cpp
    lc_shortcutbutton.h
    lc_actionsshortcutsdialog.cpp
    lc_actionsshortcutsdialog.h
    lc_shortcutstreemodel.h
    lc_shortcutstreemodel.cpp
    lc_shortcuttreeitem.cpp
    lc_shortcuttreeitem.h
    lc_shortcutstreeview.h
    lc_shortcutstreeview.cpp
)
list(TRANSFORM UI_DIALOG_SETTINGS_SHORTCUTS_FILES PREPEND "${UI_DIALOG_SETTINGS_SHORTCUTS_PATH}")
list(APPEND LIB_SOURCES ${UI_DIALOG_SETTINGS_SHORTCUTS_FILES})

# Group 138: ui action_options ellipse
set(UI_ACTION_OPTIONS_ELLIPSE_PATH "librecad/src/ui/action_options/ellipse/")
set(UI_ACTION_OPTIONS_ELLIPSE_FILES
    lc_ellipse1pointoptions.h
    lc_ellipse1pointoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_ELLIPSE_FILES PREPEND "${UI_ACTION_OPTIONS_ELLIPSE_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_ELLIPSE_FILES})

# Group 140: ui main release_check
set(UI_MAIN_RELEASE_CHECK_PATH "librecad/src/ui/main/release_check/")
set(UI_MAIN_RELEASE_CHECK_FILES
    lc_releasechecker.h
    lc_releasechecker.cpp
)
list(TRANSFORM UI_MAIN_RELEASE_CHECK_FILES PREPEND "${UI_MAIN_RELEASE_CHECK_PATH}")
list(APPEND LIB_SOURCES ${UI_MAIN_RELEASE_CHECK_FILES})

# Group 141: lib engine document views
set(LIB_ENGINE_VIEWS_PATH "librecad/src/lib/engine/document/views/")
set(LIB_ENGINE_VIEWS_FILES
    lc_view.h
    lc_view.cpp
    lc_viewslist.h
    lc_viewslist.cpp
)
list(TRANSFORM LIB_ENGINE_VIEWS_FILES PREPEND "${LIB_ENGINE_VIEWS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_VIEWS_FILES})

# Group 142: ui dock_widgets views_list
set(UI_DOCK_VIEWS_LIST_PATH "librecad/src/ui/dock_widgets/views_list/")
set(UI_DOCK_VIEWS_LIST_FILES
    lc_namedviewslistwidget.h
    lc_namedviewslistwidget.cpp
    lc_namedviewsmodel.h
    lc_namedviewsmodel.cpp
    lc_namedviewslistoptions.cpp
    lc_dlgnamedviewslistoptions.h
    lc_dlgnamedviewslistoptions.cpp
    lc_namedviewsbutton.h
    lc_namedviewsbutton.cpp
)
list(TRANSFORM UI_DOCK_VIEWS_LIST_FILES PREPEND "${UI_DOCK_VIEWS_LIST_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_VIEWS_LIST_FILES})

# Group 143: lib engine overlays ucs_mark
set(LIB_ENGINE_OVERLAYS_UCS_MARK_PATH "librecad/src/lib/engine/overlays/ucs_mark/")
set(LIB_ENGINE_OVERLAYS_UCS_MARK_FILES
    lc_ucs_mark.h
    lc_ucs_mark.cpp
    lc_overlayrelativezero.h
    lc_overlayrelativezero.cpp
    lc_overlayucszero.h
    lc_overlayucszero.cpp
)
list(TRANSFORM LIB_ENGINE_OVERLAYS_UCS_MARK_FILES PREPEND "${LIB_ENGINE_OVERLAYS_UCS_MARK_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_OVERLAYS_UCS_MARK_FILES})

# Group 144: lib engine document ucs
set(LIB_ENGINE_UCS_PATH "librecad/src/lib/engine/document/ucs/")
set(LIB_ENGINE_UCS_FILES
    lc_ucs.h
    lc_ucs.cpp
    lc_ucslist.h
    lc_ucslist.cpp
)
list(TRANSFORM LIB_ENGINE_UCS_FILES PREPEND "${LIB_ENGINE_UCS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_UCS_FILES})

# Group 145: actions dock_widgets ucs_list
set(ACTIONS_DOCK_UCS_LIST_PATH "librecad/src/actions/dock_widgets/ucs_list/")
set(ACTIONS_DOCK_UCS_LIST_FILES
    lc_actionucscreate.h
    lc_actionucscreate.cpp
    lc_actionucsbydimordinate.h
    lc_actionucsbydimordinate.cpp
)
list(TRANSFORM ACTIONS_DOCK_UCS_LIST_FILES PREPEND "${ACTIONS_DOCK_UCS_LIST_PATH}")
list(APPEND LIB_SOURCES ${ACTIONS_DOCK_UCS_LIST_FILES})

# Group 146: ui action_options other
set(UI_ACTION_OPTIONS_OTHER_PATH "librecad/src/ui/action_options/other/")
set(UI_ACTION_OPTIONS_OTHER_FILES
    lc_ucssetoptions.h
    lc_ucssetoptions.cpp
)
list(TRANSFORM UI_ACTION_OPTIONS_OTHER_FILES PREPEND "${UI_ACTION_OPTIONS_OTHER_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_OTHER_FILES})

# Group 147: ui dock_widgets ucs_list
set(UI_DOCK_UCS_LIST_PATH "librecad/src/ui/dock_widgets/ucs_list/")
set(UI_DOCK_UCS_LIST_FILES
    lc_ucslistwidget.h
    lc_ucslistwidget.cpp
    lc_ucslistoptions.h
    lc_ucslistoptions.cpp
    lc_ucslistmodel.h
    lc_ucslistmodel.cpp
    lc_dlgucslistoptions.h
    lc_dlgucslistoptions.cpp
    lc_ucslistbutton.h
    lc_ucslistbutton.cpp
    lc_dlgucsproperties.h
    lc_dlgucsproperties.cpp
)
list(TRANSFORM UI_DOCK_UCS_LIST_FILES PREPEND "${UI_DOCK_UCS_LIST_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_UCS_LIST_FILES})

# Group 148: lib engine overlays info_cursor
set(LIB_ENGINE_OVERLAYS_INFO_CURSOR_PATH "librecad/src/lib/engine/overlays/info_cursor/")
set(LIB_ENGINE_OVERLAYS_INFO_CURSOR_FILES
    lc_cursoroverlayinfo.h
    lc_cursoroverlayinfo.cpp
)
list(TRANSFORM LIB_ENGINE_OVERLAYS_INFO_CURSOR_FILES PREPEND "${LIB_ENGINE_OVERLAYS_INFO_CURSOR_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_OVERLAYS_INFO_CURSOR_FILES})

# Group 149: lib engine overlays crosshair
set(LIB_ENGINE_OVERLAYS_CROSSHAIR_PATH "librecad/src/lib/engine/overlays/crosshair/")
set(LIB_ENGINE_OVERLAYS_CROSSHAIR_FILES
    lc_crosshair.cpp
)
list(TRANSFORM LIB_ENGINE_OVERLAYS_CROSSHAIR_FILES PREPEND "${LIB_ENGINE_OVERLAYS_CROSSHAIR_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_OVERLAYS_CROSSHAIR_FILES})

# Group 150: lib engine overlays
set(LIB_ENGINE_OVERLAYS_PATH "librecad/src/lib/engine/overlays/")
set(LIB_ENGINE_OVERLAYS_FILES
    preview/rs_preview.cpp
    preview/rs_preview.h
    overlay_box/rs_overlaybox.cpp
    overlay_box/rs_overlaybox.h
    rs_overlayline.cpp
    rs_overlayline.h
    lc_overlaysmanager.h
    lc_overlaysmanager.cpp
    lc_overlayentity.h
    lc_overlayentitiescontainer.h
    lc_overlayentitiescontainer.cpp
)
list(TRANSFORM LIB_ENGINE_OVERLAYS_FILES PREPEND "${LIB_ENGINE_OVERLAYS_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_OVERLAYS_FILES})

# Group 151: lib engine overlays angles_base
set(LIB_ENGINE_OVERLAYS_ANGLES_BASE_PATH "librecad/src/lib/engine/overlays/angles_base/")
set(LIB_ENGINE_OVERLAYS_ANGLES_BASE_FILES
    lc_overlayanglesbasemark.h
    lc_overlayanglesbasemark.cpp
)
list(TRANSFORM LIB_ENGINE_OVERLAYS_ANGLES_BASE_FILES PREPEND "${LIB_ENGINE_OVERLAYS_ANGLES_BASE_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_OVERLAYS_ANGLES_BASE_FILES})

# Group 152: lib engine document textstyles
set(LIB_ENGINE_TEXTSTYLES_PATH "librecad/src/lib/engine/document/textstyles/")
set(LIB_ENGINE_TEXTSTYLES_FILES
    lc_textstylelist.h
    lc_textstylelist.cpp
    lc_textstyle.h
    lc_textstyle.cpp
)
list(TRANSFORM LIB_ENGINE_TEXTSTYLES_FILES PREPEND "${LIB_ENGINE_TEXTSTYLES_PATH}")
list(APPEND LIB_SOURCES ${LIB_ENGINE_TEXTSTYLES_FILES})

# Group 153: ui main workspaces
set(UI_MAIN_WORKSPACES_PATH "librecad/src/ui/main/workspaces/")
set(UI_MAIN_WORKSPACES_FILES
    lc_workspacesmanager.h
    lc_workspacesmanager.cpp
    lc_workspacesinvoker.h
    lc_workspacesinvoker.cpp
)
list(TRANSFORM UI_MAIN_WORKSPACES_FILES PREPEND "${UI_MAIN_WORKSPACES_PATH}")
list(APPEND LIB_SOURCES ${UI_MAIN_WORKSPACES_FILES})

# Group 154: ui dock_widgets workspaces
set(UI_DOCK_WORKSPACES_PATH "librecad/src/ui/dock_widgets/workspaces/")
set(UI_DOCK_WORKSPACES_FILES
    lc_workspacelistbutton.h
    lc_workspacelistbutton.cpp
)
list(TRANSFORM UI_DOCK_WORKSPACES_FILES PREPEND "${UI_DOCK_WORKSPACES_PATH}")
list(APPEND LIB_SOURCES ${UI_DOCK_WORKSPACES_FILES})

# Group 155: flexlayout
set(UI_COMPONENTS_LAYOUTS_PATH "librecad/src/ui/components/layouts/")
set(UI_COMPONENTS_LAYOUTS_FILES
    lc_flexlayout.h
    lc_flexlayout.cpp
)
list(TRANSFORM UI_COMPONENTS_LAYOUTS_FILES PREPEND "${UI_COMPONENTS_LAYOUTS_PATH}")
list(APPEND LIB_SOURCES ${UI_COMPONENTS_LAYOUTS_FILES})

# Unnumbered group: ui action_options
set(UI_ACTION_OPTIONS_PATH "librecad/src/ui/action_options/")
set(UI_ACTION_OPTIONS_FILES
    lc_actionoptionswidget.h
    lc_actionoptionswidget.cpp
    lc_actionoptionsmanager.h
    lc_actionoptionsmanager.cpp
    lc_actionoptionswidgetbase.cpp
    lc_actionoptionswidgetbase.h
)
list(TRANSFORM UI_ACTION_OPTIONS_FILES PREPEND "${UI_ACTION_OPTIONS_PATH}")
list(APPEND LIB_SOURCES ${UI_ACTION_OPTIONS_FILES})

set(LIBRECAD_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
    librecad/src
    libraries
    libraries/jwwlib/src/
    libraries/libdxfrw/src/
    libraries/libdxfrw/src/intern/
    libraries/muparser/include/
    libraries/muparser/src/
    librecad/src/actions/drawing/draw/dimensions/
    librecad/src/actions/drawing/draw/circle/
    librecad/src/actions/drawing/draw/polygon/
    librecad/src/actions/drawing/draw/spline/
    librecad/src/actions/file/
    librecad/src/actions/dock_widgets/layer/
    librecad/src/actions/dock_widgets/block/
    librecad/src/actions/drawing/modify/
    librecad/src/actions/drawing/draw/curve/
    librecad/src/actions/drawing/draw/hatch/
    librecad/src/actions/drawing/draw/image/
    librecad/src/actions/drawing/draw/line/
    librecad/src/actions/drawing/draw/rect/
    librecad/src/actions/drawing/draw/text/
    librecad/src/actions/drawing/draw/point/
    librecad/src/actions/drawing/draw/polyline/
    librecad/src/actions/drawing/draw/ellipse/
    librecad/src/actions/drawing/edit/
    librecad/src/actions/drawing/info/
    librecad/src/actions/drawing/pick/
    librecad/src/actions/dock_widgets/library/
    librecad/src/actions/drawing/rel_zero/
    librecad/src/actions/options/
    librecad/src/actions/print_preview/
    librecad/src/actions/drawing/selection/
    librecad/src/actions/drawing/snap/
    librecad/src/actions/drawing/zoom/
    librecad/src/cmd/
    librecad/src/lib/actions/
    librecad/src/lib/creation/
    librecad/src/lib/debug/
    librecad/src/lib/engine/
    librecad/src/lib/engine/document/
    librecad/src/lib/engine/document/entities/
    librecad/src/lib/engine/document/entities/support/
    librecad/src/lib/engine/document/container/
    librecad/src/lib/engine/utils/
    librecad/src/lib/engine/undo/
    librecad/src/lib/engine/document/blocks/
    librecad/src/lib/engine/clipboard/
    librecad/src/lib/engine/document/fonts/
    librecad/src/lib/engine/document/layers/
    librecad/src/lib/engine/document/dimstyles/
    librecad/src/lib/engine/document/patterns/
    librecad/src/lib/engine/settings/
    librecad/src/lib/engine/document/variables/
    librecad/src/lib/fileio/
    librecad/src/lib/filters/
    librecad/src/lib/generators/makercamsvg/
    librecad/src/lib/generators/layers/
    librecad/src/lib/generators/image/
    librecad/src/lib/gui/
    librecad/src/lib/gui/grid/
    librecad/src/lib/gui/render/
    librecad/src/lib/gui/render/widget/
    librecad/src/lib/gui/render/headless/
    librecad/src/lib/information/
    librecad/src/lib/math/
    librecad/src/lib/modification/
    librecad/src/lib/printing/
    librecad/src/lib/scripting/
    librecad/src/main/console_dxf2pdf/
    librecad/src/main/
    librecad/src/plugins/intern/
    librecad/src/plugins/
    librecad/src/ui/dialogs/entity/
    librecad/src/ui/dialogs/settings/options_widget/
    librecad/src/ui/components/status_bar/
    librecad/src/ui/action_options/
    librecad/src/ui/action_options/curve/
    librecad/src/ui/action_options/modify/
    librecad/src/ui/action_options/circle/
    librecad/src/ui/dock_widgets/command_line/
    librecad/src/ui/action_options/dimensions/
    librecad/src/ui/dialogs/file/export/image/
    librecad/src/ui/dialogs/main/
    librecad/src/ui/dialogs/actions/modify/
    librecad/src/ui/dialogs/settings/dimstyles/dimstyle_manager/
    librecad/src/ui/dialogs/settings/dimstyles/dimstyle_manager/support/
    librecad/src/ui/dialogs/settings/options_drawing/
    librecad/src/ui/dialogs/settings/dimstyles/
    librecad/src/ui/dialogs/settings/options_general/
    librecad/src/ui/dialogs/file/export/makercam/
    librecad/src/ui/action_options/image/
    librecad/src/ui/action_options/insert/
    librecad/src/ui/dialogs/actions/
    librecad/src/ui/dock_widgets/layers_tree/
    librecad/src/ui/dialogs/file/export/layers/
    librecad/src/ui/action_options/line/
    librecad/src/ui/action_options/polygon/
    librecad/src/ui/action_options/text/
    librecad/src/ui/action_options/polyline/
    librecad/src/ui/action_options/print_preview/
    librecad/src/ui/action_options/snap/
    librecad/src/ui/components/toolbars/
    librecad/src/ui/action_options/spline/
    librecad/src/ui/components/pen/
    librecad/src/ui/components/creators/
    librecad/src/ui/dialogs/creators/
    librecad/src/ui/dock_widgets/pen_wizard/
    librecad/src/ui/components/comboboxes/
    librecad/src/ui/components/
    librecad/src/ui/components/layouts/
    librecad/src/ui/main/init/
    librecad/src/ui/actions/
    librecad/src/ui/view/
    librecad/src/ui/dialogs/settings/options_device/
    librecad/src/ui/dock_widgets/
    librecad/src/ui/dock_widgets/cad/
    librecad/src/ui/main/
    librecad/src/ui/main/fontviewer/
    librecad/src/ui/main/persistence/
    librecad/src/ui/
    librecad/src/ui/dock_widgets/block_widget/
    librecad/src/ui/dialogs/
    librecad/src/ui/dialogs/file/
    librecad/src/ui/dock_widgets/layer_widget/
    librecad/src/ui/dock_widgets/library_widget/
    librecad/src/ui/main/support/
    librecad/src/ui/dock_widgets/pen_palette/
    librecad/src/actions/drawing/pen/
    librecad/src/actions/drawing/draw/line/misc/
    librecad/src/ui/action_options/misc/
    librecad/src/ui/action_options/rect/
    librecad/src/ui/action_options/point/
    librecad/src/actions/drawing/
    librecad/src/ui/dock_widgets/entity_info/
    librecad/src/actions/dock_widgets/entity_info/
    librecad/src/lib/engine/overlays/highlight/
    librecad/src/lib/engine/overlays/overlay_box/
    librecad/src/ui/action_options/info/
    librecad/src/ui/components/containers/
    librecad/src/lib/engine/overlays/references/
    librecad/src/ui/action_options/selection/
    librecad/src/ui/action_options/edit/
    librecad/src/ui/dialogs/settings/shortcuts/
    librecad/src/ui/action_options/ellipse/
    librecad/src/ui/main/release_check/
    librecad/src/lib/engine/document/views/
    librecad/src/ui/dock_widgets/views_list/
    librecad/src/lib/engine/overlays/ucs_mark/
    librecad/src/lib/engine/document/ucs/
    librecad/src/actions/dock_widgets/ucs_list/
    librecad/src/ui/action_options/other/
    librecad/src/ui/dock_widgets/ucs_list/
    librecad/src/lib/engine/overlays/info_cursor/
    librecad/src/lib/engine/overlays/crosshair/
    librecad/src/lib/engine/overlays/
    librecad/src/lib/engine/overlays/angles_base/
    librecad/src/lib/engine/document/textstyles/
    librecad/src/lib/engine/overlays/preview
    librecad/src/ui/main/workspaces/
    librecad/src/ui/dock_widgets/workspaces/
)

add_library(librecad_core STATIC)
target_compile_features(librecad_core PRIVATE cxx_std_17)
target_sources(librecad_core PRIVATE ${LIB_SOURCES})
target_include_directories(librecad_core PRIVATE
    ${LIBRECAD_INCLUDE_DIRS}
)
qt_import_plugins(librecad_core TARGET_INCLUDE
  libraries/lciconengine/lciconengine)

set(LC_SOURCES librecad/src/main/main.cpp
    librecad/src/ui/dialogs/main/qg_dlginitial.cpp
    librecad/src/ui/dialogs/main/qg_dlginitial.h
    licenses/licenses.qrc
    librecad/res/icons/icons.qrc
    librecad/res/controls/controls.qrc
    librecad/res/images/images.qrc
    librecad/res/arrows/arrows.qrc
    librecad/res/dxf/dxf.qrc
    librecad/res/gdt/gdt.qrc
  )

qt_add_executable(librecad 
  ${LC_SOURCES}
)

target_include_directories(librecad PRIVATE
    ${LIBRECAD_INCLUDE_DIRS}
)

target_link_libraries(librecad PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::PrintSupport Qt6::Svg Qt6::Network ${Boost_LIBRARIES})
target_link_libraries(librecad_core PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui
  Qt6::PrintSupport Qt6::Svg Qt6::Network ${Boost_LIBRARIES} )
target_link_libraries(librecad PRIVATE librecad_core)

if(FREETYPE_FOUND)
    target_link_libraries(librecad PRIVATE ${FREETYPE_LIBRARIES})
endif()

install(TARGETS librecad RUNTIME DESTINATION bin)

set(TS_DIR ${PROJECT_SOURCE_DIR}/librecad/ts)

set(TS_FILES
    ${TS_DIR}/librecad_ar.ts
    ${TS_DIR}/librecad_ca.ts
    ${TS_DIR}/librecad_cs.ts
    ${TS_DIR}/librecad_da.ts
    ${TS_DIR}/librecad_de.ts
    ${TS_DIR}/librecad_el.ts
    ${TS_DIR}/librecad_en_au.ts
    ${TS_DIR}/librecad_en.ts
    ${TS_DIR}/librecad_es_ar.ts
    ${TS_DIR}/librecad_es_bo.ts
    ${TS_DIR}/librecad_es_cl.ts
    ${TS_DIR}/librecad_es_co.ts
    ${TS_DIR}/librecad_es_cr.ts
    ${TS_DIR}/librecad_es_do.ts
    ${TS_DIR}/librecad_es_ec.ts
    ${TS_DIR}/librecad_es_gt.ts
    ${TS_DIR}/librecad_es_hn.ts
    ${TS_DIR}/librecad_es_mx.ts
    ${TS_DIR}/librecad_es_ni.ts
    ${TS_DIR}/librecad_es_pa.ts
    ${TS_DIR}/librecad_es_pe.ts
    ${TS_DIR}/librecad_es_pr.ts
    ${TS_DIR}/librecad_es_py.ts
    ${TS_DIR}/librecad_es_sv.ts
    ${TS_DIR}/librecad_es.ts
    ${TS_DIR}/librecad_es_us.ts
    ${TS_DIR}/librecad_es_uy.ts
    ${TS_DIR}/librecad_es_ve.ts
    ${TS_DIR}/librecad_et.ts
    ${TS_DIR}/librecad_eu.ts
    ${TS_DIR}/librecad_fi.ts
    ${TS_DIR}/librecad_fr.ts
    ${TS_DIR}/librecad_gl.ts
    ${TS_DIR}/librecad_he.ts
    ${TS_DIR}/librecad_hi.ts
    ${TS_DIR}/librecad_hu.ts
    ${TS_DIR}/librecad_id_ID.ts
    ${TS_DIR}/librecad_it.ts
    ${TS_DIR}/librecad_ja.ts
    ${TS_DIR}/librecad_ka.ts
    ${TS_DIR}/librecad_ko.ts
    ${TS_DIR}/librecad_lv.ts
    ${TS_DIR}/librecad_mk.ts
    ${TS_DIR}/librecad_nl.ts
    ${TS_DIR}/librecad_no.ts
    ${TS_DIR}/librecad_pa.ts
    ${TS_DIR}/librecad_pl.ts
    ${TS_DIR}/librecad_pt_br.ts
    ${TS_DIR}/librecad_pt_pt.ts
    ${TS_DIR}/librecad_ro_ro.ts
    ${TS_DIR}/librecad_ru.ts
    ${TS_DIR}/librecad_sk.ts
    ${TS_DIR}/librecad_sl.ts
    ${TS_DIR}/librecad_sq_al.ts
    ${TS_DIR}/librecad_sr.ts
    ${TS_DIR}/librecad_sv.ts
    ${TS_DIR}/librecad_ta.ts
    ${TS_DIR}/librecad_th.ts
    ${TS_DIR}/librecad_tr.ts
    ${TS_DIR}/librecad_uk.ts
    ${TS_DIR}/librecad_zh_cn.ts
    ${TS_DIR}/librecad_zh_tw.ts
)

qt_add_translations(librecad TS_FILES ${TS_FILES})

if(FREETYPE_FOUND AND NOT WIN32)
    add_executable(ttf2lff tools/ttf2lff/main.cpp)
    target_link_libraries(ttf2lff PRIVATE ${FREETYPE_LIBRARIES})
    target_include_directories(ttf2lff PRIVATE ${FREETYPE_INCLUDE_DIRS})
endif()


option(BUILD_TESTS "Build unit tests for LibreCAD" OFF)
if(BUILD_TESTS)
    add_subdirectory(externals/Catch2)
    add_executable(librecad_tests
      ${LC_SOURCES}
        librecad/src/lib/math/tests/rs_math_tests.cpp
        librecad/src/lib/engine/document/entities/tests/rs_ellipse_tests.cpp
    )
    target_include_directories(librecad_tests PRIVATE
        ${LIBRECAD_INCLUDE_DIRS}
    )
    target_link_libraries(librecad_tests PRIVATE librecad_core Catch2::Catch2WithMain Qt6::Core Qt6::Widgets Qt6::Gui Qt6::PrintSupport Qt6::Svg Qt6::Network ${Boost_LIBRARIES})
    target_compile_features(librecad_tests PRIVATE cxx_std_17)
    target_compile_definitions(librecad_tests PRIVATE BUILD_TESTS=1)
endif()

add_subdirectory(libraries/lciconengine)
add_subdirectory(plugins)


