/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

// This file was first published at: github.com/r-a-v-a-s/LibreCAD.git

// lc_actionfactory is a rewrite of qg_actionfactory; some copied content remains.
// qg_actionfactory contributors:
// Andrew Mustun, Claude Sylvain, R. van Twisk, Dongxu Li, Rallaz, Armin Stebich, ravas, korhadris

#include <QAction>
#include <QActionGroup>

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_settings.h"
#include "lc_shortcutinfo.h"
#include "lc_shortcuts_manager.h"

LC_ActionFactory::LC_ActionFactory(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler)
    : LC_ActionFactoryBase(parent, a_handler){
}
// todo - add explanations for commands for actions (probably mix with commandItems) as it was mentioned in issue #570

void LC_ActionFactory::fillActionContainer(QMap<QString, QAction*>& a_map, LC_ActionGroupManager* agm, bool useTheme){
    using_theme = useTheme;
    createSelectActions(a_map, agm->select);
    createDrawLineActions(a_map, agm->line);
    createDrawCircleActions(a_map, agm->circle);
    createDrawCurveActions(a_map, agm->curve);
    createDrawEllipseActions(a_map, agm->ellipse);
    createDrawPolylineActions(a_map, agm->polyline);
    createDrawOtherActions(a_map, agm->other);
    createDrawDimensionsActions(a_map, agm->dimension);
    createModifyActions(a_map, agm->modify);
    createPenActions(a_map, agm->pen);
    createInfoActions(a_map, agm->info);
    createViewActions(a_map, agm->view);
    createWidgetActions(a_map, agm->widgets);
    createFileActions(a_map, agm->file);

    createSnapActions(a_map, agm->snap);
    createSnapExtraActions(a_map, agm->snap_extras);
    createRestrictActions(a_map, agm->restriction);
    createOtherActions(a_map, agm->other);

    for (QAction* value: a_map){
       value->setCheckable(true);
    }

    // not checkable actions
    createPenActionsUncheckable(a_map, agm->pen);
    createOrderActionsUncheckable(a_map, agm->modify);
    createLayerActionsUncheckable(a_map, agm->layer);
    createBlockActionsUncheckable(a_map, agm->block);
    createOptionsActionsUncheckable(a_map, agm->options);
    createSelectActionsUncheckable(a_map, agm->select);
    createFileActionsUncheckable(a_map, agm->file);
    createViewActionsUncheckable(a_map, agm->view);
    createWidgetActionsUncheckable(a_map, agm->widgets);
    createEditActionsUncheckable(a_map, agm->edit);


    setupCreatedActions(a_map);
    setDefaultShortcuts(a_map, agm);

    agm->loadShortcuts(a_map);

    // todo - may we report errors somehow there?

    markNotEditableActionsShortcuts(a_map);

    // fixme - review why this action is not used, is it really necessary or may be removed?
    //    action = new QAction(tr("Regenerate Dimension Entities"), disable_group);
//    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotToolRegenerateDimensions()));
//    action->setObjectName("ToolRegenerateDimensions");
//    a_map["ToolRegenerateDimensions"] = action;
}



void LC_ActionFactory::createDrawLineActions(QMap<QString, QAction*>& map, QActionGroup* group){
    createActionHandlerActions(map, group,{
        {"DrawPoint",                RS2::ActionDrawPoint,               tr("&Points"),                ":/icons/points.svg"},
        {"DrawLine",                 RS2::ActionDrawLine,                tr("&2 Points"),              ":/icons/line_2p.svg"},
        {"DrawLineAngle",            RS2::ActionDrawLineAngle,           tr("&Angle"),                 ":/icons/line_angle.svg"},
        {"DrawLineHorizontal",       RS2::ActionDrawLineHorizontal,      tr("&Horizontal"),            ":/icons/line_horizontal.svg"},
        {"DrawLineVertical",         RS2::ActionDrawLineVertical,        tr("Vertical"),               ":/icons/line_vertical.svg"},
        {"DrawLineFree",             RS2::ActionDrawLineFree,            tr("&Freehand Line"),         ":/icons/line_freehand.svg"},
        {"DrawLineParallel",         RS2::ActionDrawLineParallel,        tr("&Parallel"),              ":/icons/line_parallel.svg"},
        {"DrawLineParallelThrough",  RS2::ActionDrawLineParallelThrough, tr("Parallel through point"), ":/icons/line_parallel_p.svg"},
        {"DrawLineRectangle",        RS2::ActionDrawLineRectangle,       tr("Rectangle"),              ":/icons/line_rectangle.svg"},
        {"DrawLineBisector",         RS2::ActionDrawLineBisector,        tr("Bisector"),               ":/icons/line_bisector.svg"},
        {"DrawLineTangent1",         RS2::ActionDrawLineTangent1,        tr("Tangent (P,C)"),          ":/icons/line_tangent_pc.svg"},
        {"DrawLineTangent2",         RS2::ActionDrawLineTangent2,        tr("Tangent (C,C)"),          ":/icons/line_tangent_cc.svg"},
        {"DrawLineOrthTan",          RS2::ActionDrawLineOrthTan,         tr("Tangent &Orthogonal"),    ":/icons/line_tangent_perpendicular.svg"},
        {"DrawLineOrthogonal",       RS2::ActionDrawLineOrthogonal,      tr("Orthogonal"),             ":/icons/line_perpendicular.svg"},
        {"DrawLineRelAngle",         RS2::ActionDrawLineRelAngle,        tr("Relative angle"),         ":/icons/line_relative_angle.svg"},
        {"DrawLinePolygonCenCor",    RS2::ActionDrawLinePolygonCenCor,   tr("Pol&ygon (Cen,Cor)"),     ":/icons/line_polygon_cen_cor.svg"},
        {"DrawLinePolygonCenTan",    RS2::ActionDrawLinePolygonCenTan,   tr("Pol&ygon (Cen,Tan)"),     ":/icons/line_polygon_cen_tan.svg"},
        {"DrawLinePolygonCorCor",    RS2::ActionDrawLinePolygonCorCor,   tr("Polygo&n (Cor,Cor)"),     ":/icons/line_polygon_cor_cor.svg"},
        {"DrawLineRel",              RS2::ActionDrawSnakeLine,           tr("Snake"),                  ":/icons/line_rel.svg"},
        {"DrawLineRelX",             RS2::ActionDrawSnakeLineX,          tr("Snake (X)"),              ":/icons/line_rel_x.svg"},
        {"DrawLineRelY",             RS2::ActionDrawSnakeLineY,          tr("Snake (Y)"),              ":/icons/line_rel_y.svg"},
        {"DrawLineRectangle1Point",  RS2::ActionDrawRectangle1Point,     tr("Rectangle (1 Point)"),    ":/icons/rectangle_1_point.svg"},
        {"DrawLineRectangle2Points", RS2::ActionDrawRectangle2Points,    tr("Rectangle (2 Points)"),   ":/icons/rectangle_2_points.svg"},
        {"DrawLineRectangle3Points", RS2::ActionDrawRectangle3Points,    tr("Rectangle (3 Points)"),   ":/icons/rectangle_3_points.svg"},
        {"DrawStar",                 RS2::ActionDrawStar,                tr("Star"),                   ":/icons/line_polygon_star.svg"},
        {"DrawLineAngleRel",         RS2::ActionDrawLineAngleRel,        tr("Angle From Line"),        ":/icons/line_angle_rel.svg"},
        {"DrawLineOrthogonalRel",    RS2::ActionDrawLineOrthogonalRel,   tr("Orthogonal From Line"),   ":/icons/line_ortho_rel.svg"},
        {"DrawLineFromPointToLine",  RS2::ActionDrawLineFromPointToLine, tr("From Point To Line"),     ":/icons/line_to_ortho.svg"},
        {"DrawCross",                RS2::ActionDrawCross,               tr("Cross"),                  ":/icons/cross_circle1.svg"},
        {"DrawSliceDivideLine",      RS2::ActionDrawSliceDivideLine,     tr("Slice/Divide Line"),      ":/icons/slice_divide.svg"},
        {"DrawSliceDivideCircle",    RS2::ActionDrawSliceDivideCircle,   tr("Slice/Divide Circle"),    ":/icons/slice_divide_circle.svg"},
        {"DrawLinePoints",           RS2::ActionDrawLinePoints,          tr("Line of Points"),         ":/icons/line_points.svg"}
    });
}

void LC_ActionFactory::createSelectActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group,{
        {"SelectSingle",        RS2::ActionSelectSingle,        tr("Select Entity"),                 ":/icons/select_entity.svg"},
        {"SelectWindow",        RS2::ActionSelectWindow,        tr("Select Window"),                 ":/icons/select_window.svg"},
        {"DeselectWindow",      RS2::ActionDeselectWindow,      tr("Deselect Window"),               ":/icons/deselect_window.svg"},
        {"SelectContour",       RS2::ActionSelectContour,       tr("(De-)Select &Contour"),          ":/icons/deselect_contour.svg"},
        {"SelectIntersected",   RS2::ActionSelectIntersected,   tr("Select Intersected Entities"),   ":/icons/select_intersected_entities.svg"},
        {"DeselectIntersected", RS2::ActionDeselectIntersected, tr("Deselect Intersected Entities"), ":/icons/deselect_intersected_entities.svg"},
        {"SelectLayer",         RS2::ActionSelectLayer,         tr("(De-)Select Layer"),             ":/icons/deselect_layer.svg"}
    });
}

void LC_ActionFactory::createDrawCircleActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group,{
        {"DrawCircle",         RS2::ActionDrawCircle,         tr("Center, &Point"),                ":/icons/circle_center_point.svg"},
        {"DrawCircleByArc",    RS2::ActionDrawCircleByArc,    tr("By Arc"),                        ":/icons/circle_by_arc.svg"},
        {"DrawCircleCR",       RS2::ActionDrawCircleCR,       tr("Center, &Radius"),               ":/icons/circle_center_radius.svg"},
        {"DrawCircle2P",       RS2::ActionDrawCircle2P,       tr("2 Points"),                      ":/icons/circle_2_points.svg"},
        {"DrawCircle2PR",      RS2::ActionDrawCircle2PR,      tr("2 Points, Radius"),              ":/icons/circle_2_points_radius.svg"},
        {"DrawCircle3P",       RS2::ActionDrawCircle3P,       tr("3 Points"),                      ":/icons/circle_3_points.svg"},
        {"DrawCircleParallel", RS2::ActionDrawCircleParallel, tr("&Concentric"),                   ":/icons/circle_concentric.svg"},
        {"DrawCircleInscribe", RS2::ActionDrawCircleInscribe, tr("Circle &Inscribed"),             ":/icons/circle_inscribed.svg"},
        {"DrawCircleTan2",     RS2::ActionDrawCircleTan2,     tr("Tangential 2 Circles, Radius"),  ":/icons/circle_tangential_2circles_radius.svg"},
        {"DrawCircleTan2_1P",  RS2::ActionDrawCircleTan2_1P,  tr("Tangential 2 Circles, 1 Point"), ":/icons/circle_tangential_2circles_point.svg"},
        {"DrawCircleTan3",     RS2::ActionDrawCircleTan3,     tr("Tangential &3 Circles"),         ":/icons/circle_tangential_3entities.svg"},
        {"DrawCircleTan1_2P",  RS2::ActionDrawCircleTan1_2P,  tr("Tangential, 2 P&oints"),         ":/icons/circle_tangential_2points.svg"}
    });
}

void LC_ActionFactory::createDrawCurveActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group,{
        {"DrawArc",             RS2::ActionDrawArc,             tr("&Center, Point, Angles"),    ":/icons/arc_center_point_angle.svg"},
        {"DrawArc3P",           RS2::ActionDrawArc3P,           tr("&3 Points"),                 ":/icons/arc_3_points.svg"},
        {"DrawArcParallel",     RS2::ActionDrawArcParallel,     tr("&Concentric"),               ":/icons/arc_concentric.svg"},     // fixme - why this action is not in list?
        {"DrawArcTangential",   RS2::ActionDrawArcTangential,   tr("Arc &Tangential"),           ":/icons/arc_continuation.svg"},
        {"DrawParabola4Points", RS2::ActionDrawParabola4Points, tr("Para&bola 4 points"),        ":/icons/parabola_4_points.svg"},
        {"DrawParabolaFD",      RS2::ActionDrawParabolaFD,      tr("Parabola &Focus Directrix"), ":/icons/parabola_focus_directrix.svg"},
        {"DrawSpline",          RS2::ActionDrawSpline,          tr("&Spline"),                   ":/icons/spline.svg"},
        {"DrawSplinePoints",    RS2::ActionDrawSplinePoints,    tr("&Spline through points"),    ":/icons/spline_points.svg"}
    });
}

void LC_ActionFactory::createDrawEllipseActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"DrawEllipse1Point",        RS2::ActionDrawEllipse1Point,        tr("&Ellipse (1 Point)"),           ":/icons/ellipse_1_point.svg"},
        {"DrawEllipseArc1Point",     RS2::ActionDrawEllipseArc1Point,     tr("&Ellipse Arc (1 Point)"),       ":/icons/ellipse_arc_1_point.svg"},
        {"DrawEllipseAxis",          RS2::ActionDrawEllipseAxis,          tr("&Ellipse (Axis)"),              ":/icons/ellipse_axis.svg"},
        {"DrawEllipseArcAxis",       RS2::ActionDrawEllipseArcAxis,       tr("Ellipse &Arc (Axis)"),          ":/icons/ellipse_arc_axis.svg"},
        {"DrawEllipseFociPoint",     RS2::ActionDrawEllipseFociPoint,     tr("Ellipse &Foci Point"),          ":/icons/ellipse_foci_point.svg"},
        {"DrawEllipse4Points",       RS2::ActionDrawEllipse4Points,       tr("Ellipse &4 Point"),             ":/icons/ellipse_4_points.svg"},
        {"DrawEllipseCenter3Points", RS2::ActionDrawEllipseCenter3Points, tr("Ellipse Center and &3 Points"), ":/icons/ellipse_center_3_points.svg"},
        {"DrawEllipseInscribe",      RS2::ActionDrawEllipseInscribe,      tr("Ellipse &Inscribed"),           ":/icons/ellipse_inscribed.svg"}
    });
}

void LC_ActionFactory::createDrawPolylineActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group,{
        {"DrawPolyline",        RS2::ActionDrawPolyline,        tr("&Polyline"),                               ":/icons/polylines_polyline.svg"},
        {"PolylineAdd",         RS2::ActionPolylineAdd,         tr("&Add node"),                               ":/icons/insert_node.svg"},
        {"PolylineAppend",      RS2::ActionPolylineAppend,      tr("A&ppend node"),                            ":/icons/append_node.svg"},
        {"PolylineDel",         RS2::ActionPolylineDel,         tr("&Delete node"),                            ":/icons/delete_node.svg"},
        {"PolylineDelBetween",  RS2::ActionPolylineDelBetween,  tr("Delete &between two nodes"),               ":/icons/delete_between_nodes.svg"},
        {"PolylineTrim",        RS2::ActionPolylineTrim,        tr("&Trim segments"),                          ":/icons/trim.svg"},
        {"PolylineEquidistant", RS2::ActionPolylineEquidistant, tr("Create &Equidistant Polylines"),           ":/icons/create_equidistant_polyline.svg"},
        {"PolylineSegment",     RS2::ActionPolylineSegment,     tr("Polyline from Existing &Segments"), ":/icons/create_polyline_from_existing_segments.svg"}
    });
}

void LC_ActionFactory::createDrawOtherActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group,{
        {"ZoomPan",   RS2::ActionZoomPan,   tr("Zoom &Panning"), ":/icons/zoom_pan.svg"},
        {"DrawMText", RS2::ActionDrawMText, tr("&MText"),        ":/icons/mtext.svg"},
        {"DrawText",  RS2::ActionDrawText,  tr("&Text"),         ":/icons/text.svg"},
        {"DrawHatch", RS2::ActionDrawHatch, tr("&Hatch"),        ":/icons/hatch.svg"},
        {"DrawImage", RS2::ActionDrawImage, tr("Insert &Image"), ":/icons/camera.svg"}
    });
}

void LC_ActionFactory::createDrawDimensionsActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group,{
        {"DimAligned",   RS2::ActionDimAligned,   tr("&Aligned"),    ":/icons/dim_aligned.svg"},
        {"DimLinear",    RS2::ActionDimLinear,    tr("&Linear"),     ":/icons/dim_linear.svg"},
        {"DimLinearHor", RS2::ActionDimLinearHor, tr("&Horizontal"), ":/icons/dim_horizontal.svg"},
        {"DimLinearVer", RS2::ActionDimLinearVer, tr("&Vertical"),   ":/icons/dim_vertical.svg"},
        {"DimRadial",    RS2::ActionDimRadial,    tr("&Radial"),     ":/icons/dim_radial.svg"},
        {"DimDiametric", RS2::ActionDimDiametric, tr("&Diametric"),  ":/icons/dim_diametric.svg"},
        {"DimAngular",   RS2::ActionDimAngular,   tr("&Angular"),    ":/icons/dim_angular.svg"},
        {"DimArc",       RS2::ActionDimArc,       tr("&Arc"),        ":/icons/dim_arc.svg"},
        {"DimLeader",    RS2::ActionDimLeader,    tr("&Leader"),     ":/icons/dim_leader.svg"},
        {"DimBaseline",  RS2::ActionDimBaseline,  tr("&Baseline"),  ":/icons/dim_baseline.svg"},
        {"DimContinue",  RS2::ActionDimContinue,  tr("&Continue"),  ":/icons/dim_continue.svg"}
    });
}

void LC_ActionFactory::createModifyActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    /* action = new QAction(tr("Delete Freehand"), agm->modify);
     action->setIcon(QIcon(":/icons/delete_freehand.svg"));
     connect(action, SIGNAL(triggered()),
     action_handler, SLOT(slotModifyDeleteFree()));
     action->setObjectName("ModifyDeleteFree");
     a_map["ModifyDeleteFree"] = action;*/
    createActionHandlerActions(map, group,{
        {"ModifyAttributes",      RS2::ActionModifyAttributes,      tr("&Attributes"),                ":/icons/attributes.svg"},
        {"ModifyDelete",          RS2::ActionModifyDelete,          tr("&Delete"),                    ":/icons/delete.svg"},
        {"ModifyMove",            RS2::ActionModifyMove,            tr("&Move / Copy"),               ":/icons/move_copy.svg"},
        {"ModifyRevertDirection", RS2::ActionModifyRevertDirection, tr("Re&vert direction"),          ":/icons/revert_direction.svg"},
        {"ModifyRotate",          RS2::ActionModifyRotate,          tr("&Rotate"),                    ":/icons/rotate.svg"},
        {"ModifyScale",           RS2::ActionModifyScale,           tr("&Scale"),                     ":/icons/scale.svg"},
        {"ModifyMirror",          RS2::ActionModifyMirror,          tr("&Mirror"),                    ":/icons/mirror.svg"},
        {"ModifyMoveRotate",      RS2::ActionModifyMoveRotate,      tr("Mo&ve and Rotate"),           ":/icons/move_rotate.svg"},
        {"ModifyRotate2",         RS2::ActionModifyRotate2,         tr("Rotate T&wo"),                ":/icons/rotate2.svg"},
        {"ModifyEntity",          RS2::ActionModifyEntity,          tr("&Properties"),                ":/icons/properties.svg"},
        {"ModifyTrim",            RS2::ActionModifyTrim,            tr("&Trim"),                      ":/icons/trim.svg"},
        {"ModifyTrim2",           RS2::ActionModifyTrim2,           tr("Tr&im Two"),                  ":/icons/trim2.svg"},
        {"ModifyTrimAmount",      RS2::ActionModifyTrimAmount,      tr("&Lengthen"),                  ":/icons/trim_value.svg"},
        {"ModifyOffset",          RS2::ActionModifyOffset,          tr("O&ffset"),                    ":/icons/offset.svg"},
        {"ModifyCut",             RS2::ActionModifyCut,             tr("&Divide"),                    ":/icons/divide.svg"},
        {"ModifyStretch",         RS2::ActionModifyStretch,         tr("&Stretch"),                   ":/icons/stretch.svg"},
        {"ModifyBevel",           RS2::ActionModifyBevel,           tr("&Bevel"),                     ":/icons/bevel.svg"},
        {"ModifyRound",           RS2::ActionModifyRound,           tr("&Fillet"),                    ":/icons/fillet.svg"},
        {"ModifyExplodeText",     RS2::ActionModifyExplodeText,     tr("&Explode Text into Letters"), ":/icons/explode_text_to_letters.svg"},
        {"BlocksExplode",         RS2::ActionBlocksExplode,         tr("Ex&plode"),                   ":/icons/explode.svg"},
        {"ModifyBreakDivide",     RS2::ActionModifyBreakDivide,     tr("Break/Divide"),               ":/icons/break_out_trim.svg"},
        {"ModifyLineGap",         RS2::ActionModifyLineGap,         tr("Line Gap"),                   ":/icons/line_gap.svg"},
        {"ModifyLineJoin",        RS2::ActionModifyLineJoin,        tr("Line Join"),                  ":/icons/line_join.svg"},
        {"ModifyDuplicate",       RS2::ActionModifyDuplicate,       tr("Duplicate"),                  ":/icons/duplicate.svg"}
    });
}

void LC_ActionFactory::createPenActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"PenSyncFromLayer", RS2::ActionPenSyncFromLayer, tr("Update Current Pen by Active Layer' Pen"), ":/extui/back.png"}
    });
}

void LC_ActionFactory::createPenActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"PenPick",         RS2::ActionPenPick,         tr("&Pick Pen From Entity"),            ":/extui/selectsingle.png"},
        {"PenPickResolved", RS2::ActionPenPickResolved, tr("&Pick Pen From Entity (Resolved)"), ":/extui/relzeromove.png"},
        {"PenApply",        RS2::ActionPenApply,        tr("Apply Pen to Entity"),              ":/icons/pen_apply.svg"},
        {"PenCopy",         RS2::ActionPenCopy,         tr("Copy Pen"),                         ":/icons/pen_copy.svg"}
    });
}

void LC_ActionFactory::createSnapActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActions(map, group, {
        {"SnapGrid",         tr("Snap on grid"),       ":/icons/snap_grid.svg"},
        {"SnapMiddleManual", tr("Snap Middle Manual"), ":/icons/snap_middle_manual.svg"},
        {"SnapEnd",          tr("Snap on Endpoints"),  ":/icons/snap_endpoints.svg"},
        {"SnapEntity",       tr("Snap on Entity"),     ":/icons/snap_entity.svg"},
        {"SnapCenter",       tr("Snap Center"),        ":/icons/snap_center.svg"},
        {"SnapMiddle",       tr("Snap Middle"),        ":/icons/snap_middle.svg"},
        {"SnapDistance",     tr("Snap Distance"),      ":/icons/snap_distance.svg"},
        {"SnapIntersection", tr("Snap Intersection"),  ":/icons/snap_intersection.svg"},
    });
}

void LC_ActionFactory::createRestrictActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActions(map, group, {
        {"RestrictHorizontal", tr("Restrict Horizontal"), ":/icons/restr_hor.svg"},
        {"RestrictVertical",   tr("Restrict Vertical"),   ":/icons/restr_ver.svg"},
        {"RestrictOrthogonal", tr("Restrict Orthogonal"), ":/icons/restr_ortho.svg"},
        {"RestrictNothing",    tr("Restrict Nothing"),    ":/extui/restrictnothing.png"},
    });
}

void LC_ActionFactory::createOtherActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActions(map, group, {
        {"SetRelativeZero",    tr("Set relative zero position"),  ":/icons/set_rel_zero.svg"},
        {"LockRelativeZero",   tr("Lock relative zero position"), ":/icons/lock_rel_zero.svg"}
        // todo - add action for hiding/showing related zero
       //{"RestrictOrthogonal", tr("Restrict Orthogonal"),         ":/icons/restr_ortho.svg"}
    });
}


void LC_ActionFactory::createSnapExtraActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActions(map, group, {
        {"ExclusiveSnapMode", tr("Exclusive Snap Mode"), ":/icons/exclusive.svg"},
        {"SnapFree",          tr("Free Snap"),           ":/icons/snap_free.svg"}
    });
}

void LC_ActionFactory::createOrderActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {

    createActionHandlerActions(map, group, {
        {"OrderBottom", RS2::ActionOrderBottom, tr("move to bottom"), ":/icons/downmost.svg"},
        {"OrderLower",  RS2::ActionOrderLower, tr("lower after entity"), ":/icons/down.svg"},
        {"OrderRaise",  RS2::ActionOrderRaise, tr("raise over entity"), ":/icons/up.svg"},
        {"OrderTop",    RS2::ActionOrderTop, tr("move to top"), ":/icons/upmost.svg"}
    });
}

void LC_ActionFactory::createInfoActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"InfoInside",       RS2::ActionInfoInside,           tr("Point inside contour"),               ""},
        {"InfoDist",         RS2::ActionInfoDistPoint2Point,  tr("&Distance Point to Point"),           ":/icons/distance_point_to_point.svg"},
        {"InfoDist2",        RS2::ActionInfoDistEntity2Point, tr("Distance &Entity to Point"),          ":/icons/distance_entity_to_point.svg"},
        {"InfoDist3",        RS2::ActionInfoDistPoint2Entity, tr("Distance &Point to Entity"),          ":/icons/distance_point_to_entity.svg"},
        {"InfoAngle",        RS2::ActionInfoAngle,            tr("An&gle between two lines"),           ":/icons/angle_line_to_line.svg"},
        {"InfoTotalLength",  RS2::ActionInfoTotalLength,      tr("Total &length of selected entities"), ":/icons/total_length_selected_entities.svg"},
        {"InfoArea",         RS2::ActionInfoArea,             tr("Polygonal &Area"),                    ":/icons/polygonal_area.svg"},
        {"EntityInfo",       RS2::ActionInfoProperties,       tr("Entity Pro&perties"),                 ":/extui/menuselect.png"},
        {"PickCoordinates",  RS2::ActionInfoPickCoordinates,  tr("Collect &Coordinates"),               ":/extui/menupoint.png"},
        {"InfoAngle3Points", RS2::ActionInfoAngle3Points,     tr("Ang&le between 3 points"),            ":/icons/angle_3_points.svg"}
    });
}

void LC_ActionFactory::createViewActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group, {
        {"ZoomWindow",RS2::ActionZoomWindow, tr("&Window Zoom"), ":/icons/zoom_window.svg","zoom-select"}});

    createMainWindowActions(map, group, {
        {"Fullscreen",    SLOT(toggleFullscreen(bool)),  tr("&Fullscreen")},
        {"ViewGrid",      SLOT(slotViewGrid(bool)),      tr("&Grid"),  ":/icons/grid.svg"},
        {"ViewDraft",     SLOT(slotViewDraft(bool)),     tr("&Draft"), ":/icons/draft.svg"},
        {"ViewStatusBar", SLOT(slotViewStatusBar(bool)), tr("&Statusbar")}
    }, true);
}

void LC_ActionFactory::createLayerActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"LayersDefreezeAll",        RS2::ActionLayersDefreezeAll,        tr("&Show all layers"),           ":/ui/visibleblock.png"},
        {"LayersFreezeAll",          RS2::ActionLayersFreezeAll,          tr("&Hide all layers"),           ":/ui/hiddenblock.png"},
        {"LayersUnlockAll",          RS2::ActionLayersUnlockAll,          tr("&Unlock all"),                ":/ui/unlockedlayer.png"},
        {"LayersLockAll",            RS2::ActionLayersLockAll,            tr("&Lock all"),                  ":/ui/lockedlayer.png"},
        {"LayersAdd",                RS2::ActionLayersAdd,                tr("&Add Layer"),                 ":/icons/add.svg"},
        {"LayersRemove",             RS2::ActionLayersRemove,             tr("&Remove Layer"),              ":/icons/remove.svg"},
        {"LayersEdit",               RS2::ActionLayersEdit,               tr("&Edit Layer"),                ":/icons/attributes.svg"},
        {"LayersToggleLock",         RS2::ActionLayersToggleLock,         tr("Toggle Layer Loc&k"),         ":/icons/locked.svg"},
        {"LayersToggleView",         RS2::ActionLayersToggleView,         tr("&Toggle Layer Visibility"),   ":/icons/visible.svg"},
        {"LayersTogglePrint",        RS2::ActionLayersTogglePrint,        tr("Toggle Layer &Print"),        ":/icons/print.svg"},
        {"LayersToggleConstruction", RS2::ActionLayersToggleConstruction, tr("Toggle &Construction Layer"), ":/icons/construction_layer.svg"},
        {"LayersExportSelected",     RS2::ActionLayersExportSelected,     tr("&Export Selected Layer(s)")},
        {"LayersExportVisible",      RS2::ActionLayersExportVisible,      tr("Export &Visible Layer(s)")}
    });
}

void LC_ActionFactory::createBlockActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"BlocksDefreezeAll", RS2::ActionBlocksDefreezeAll, tr("&Show all blocks"),         ":/ui/blockdefreeze.png"},
        {"BlocksFreezeAll",   RS2::ActionBlocksFreezeAll,   tr("&Hide all blocks"),         ":/ui/blockfreeze.png"},
        {"BlocksAdd",         RS2::ActionBlocksAdd,         tr("&Add Block"),               ":/icons/add.svg"},
        {"BlocksRemove",      RS2::ActionBlocksRemove,      tr("&Remove Block"),            ":/icons/remove.svg"},
        {"BlocksAttributes",  RS2::ActionBlocksAttributes,  tr("&Rename Block"),            ":/icons/rename_active_block.svg"},
        {"BlocksEdit",        RS2::ActionBlocksEdit,        tr("&Edit Block"),              ":/icons/properties.svg"},
        {"BlocksSave",        RS2::ActionBlocksSave,        tr("&Save Block"),              ":/icons/save.svg"},
        {"BlocksInsert",      RS2::ActionBlocksInsert,      tr("&Insert Block"),            ":/icons/insert_active_block.svg"},
        {"BlocksToggleView",  RS2::ActionBlocksToggleView,  tr("Toggle Block &Visibility"), ":/ui/layertoggle.png"},
        {"BlocksCreate",      RS2::ActionBlocksCreate,      tr("&Create Block"),            ":/icons/create_block.svg"}
    });
}

void LC_ActionFactory::createOptionsActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"OptionsDrawing",RS2::ActionOptionsDrawing, "Current &Drawing Preferences", ":/icons/drawing_settings.svg"}
    });

    createMainWindowActions(map, group, {
        {"OptionsGeneral",   SLOT(slotOptionsGeneral()), tr("&Application Preferences"), ":/icons/settings.svg"},
        {"WidgetOptions",    SLOT(widgetOptionsDialog()),    tr("Widget Options")},
        {"ShortcutsOptions",    SLOT(slotOptionsShortcuts()),    tr("Keyboard Shortcuts"), ":/icons/shortcuts_settings.svg"},
        {"DeviceOptions",    SLOT(showDeviceOptions()),      tr("Device Options")},
        {"ReloadStyleSheet", SLOT(reloadStyleSheet()),   tr("Reload Style Sheet")}
    });
}

void LC_ActionFactory::createFileActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"FilePrintPreview", SLOT(slotFilePrintPreview(bool)),  "Print Pre&view",     ":/icons/print_preview.svg",     "document-print-preview"},
    });
}

void LC_ActionFactory::createFileActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"FileExport",       SLOT(slotFileExport()),       tr("&Export as image"),   ":/icons/export.svg"},
        {"FileClose", nullptr,                             tr("&Close"),             ":/icons/close.svg"},
        {"FileCloseAll",     SLOT(slotFileCloseAll()),     tr("Close All"),          ":/icons/close_all.svg"},
        {"FilePrintPDF",     SLOT(slotFilePrintPDF()),     tr("Export as PDF"),      ":/icons/export_pdf.svg"},
        {"BlocksImport",     SLOT(slotImportBlock()),      tr("&Block"),             ":/icons/insert_active_block.svg"},
        {"FileNew",          SLOT(slotFileNewNew()),       tr("&New"),               ":/icons/new.svg",               "document-new"},
        {"FileNewTemplate",  SLOT(slotFileNewTemplate()),  tr("New From &Template"), ":/icons/new_from_template.svg", "document-new"},// fixme - check
        {"FileOpen",         SLOT(slotFileOpen()),         tr("&Open..."),           ":/icons/open.svg",              "document-open"},
        {"FileSave",         SLOT(slotFileSave()),         tr("&Save"),              ":/icons/save.svg",              "document-save"},
        {"FileSaveAs",       SLOT(slotFileSaveAs()),       tr("Save &as..."),        ":/icons/save_as.svg",           "document-save-as"},
        {"FileSaveAll",      SLOT(slotFileSaveAll()),      tr("Save A&ll..."),       ":/icons/save_all.svg"},
        {"FilePrint",        SLOT(slotFilePrint()),        tr("&Print..."),          ":/icons/print.svg",             "document-print"},
        {"FileQuit",         SLOT(slotFileQuit()),         tr("&Quit"),              ":/icons/quit.svg",              "application-exit"},
    });

    createAction_AH("FileExportMakerCam",RS2::ActionFileExportMakerCam,  tr("Export as CA&M/plain SVG..."), nullptr, nullptr, group, map);
}

void LC_ActionFactory::createWidgetActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"LeftDockAreaToggle",        SLOT(toggleLeftDockArea(bool)),        tr("Left"),     ":/icons/dockwidgets_left.svg"},
        {"RightDockAreaToggle",       SLOT(toggleRightDockArea(bool)),       tr("Right"),    ":/icons/dockwidgets_right.svg"},
        {"TopDockAreaToggle",         SLOT(toggleTopDockArea(bool)),         tr("Top"),      ":/icons/dockwidgets_top.svg"},
        {"BottomDockAreaToggle",      SLOT(toggleBottomDockArea(bool)),      tr("Bottom"),   ":/icons/dockwidgets_bottom.svg"},
        {"FloatingDockwidgetsToggle", SLOT(toggleFloatingDockwidgets(bool)), tr("Floating"), ":/icons/dockwidgets_floating.svg"}
    }, true);
}

void LC_ActionFactory::createWidgetActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"RedockWidgets",        SLOT(slotRedockWidgets()),    tr("Re-dock Widgets")},
        {"InvokeMenuCreator",    SLOT(invokeMenuCreator()),    tr("Menu Creator")},
        {"InvokeToolbarCreator", SLOT(invokeToolbarCreator()), tr("Toolbar Creator")}
    });
}

void LC_ActionFactory::createViewActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createAction_MW("FocusCommand",SLOT(slotFocusCommandLine()), tr("Focus on &Command Line"), ":/main/editclear.png", nullptr, group, map);
    createAction_MW("FocusOptions",SLOT(slotFocusOptionsWidget()), tr("Focus on &Options Widget"), ":/main/contents.png", nullptr, group, map);

    createActionHandlerActions(map, group, {
        {"ZoomIn",       RS2::ActionZoomIn,       tr("Zoom &In"),       ":/icons/zoom_in.svg",       "zoom-in"},
        {"ZoomOut",      RS2::ActionZoomOut,      tr("Zoom &Out"),      ":/icons/zoom_out.svg",      "zoom-out"},
        {"ZoomAuto",     RS2::ActionZoomAuto,     tr("&Auto Zoom"),     ":/icons/zoom_auto.svg",     "zoom-fit-best"},
        {"ZoomPrevious", RS2::ActionZoomPrevious, tr("Previous &View"), ":/icons/zoom_previous.svg", "zoom-previous"},
        {"ZoomRedraw",   RS2::ActionZoomRedraw,   tr("&Redraw"),        ":/icons/redraw.svg",        "view-refresh"}
    });
}

void LC_ActionFactory::createSelectActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"SelectAll",    RS2::ActionSelectAll,    tr("Select &All"),      ":/icons/select_all.svg"},
        {"DeselectAll",  RS2::ActionDeselectAll,  tr("Deselect &all"),    ":/icons/deselect_all.svg"},
        {"SelectInvert", RS2::ActionSelectInvert, tr("Invert Selection"), ":/icons/select_inverted.svg"}
    });
}

void LC_ActionFactory::createEditActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"EditUndo",           RS2::ActionEditUndo,           tr("&Undo"),              ":/icons/undo.svg",            "edit-undo"},
        {"EditRedo",           RS2::ActionEditRedo,           tr("&Redo"),              ":/icons/redo.svg",            "edit-redo"},
        {"EditCut",            RS2::ActionEditCut,            tr("Cu&t"),               ":/icons/cut.svg",             "edit-cut"},
        {"EditCutQuick",       RS2::ActionEditCutQuick,       tr("Cut Quic&k"),         ":/icons/cut.svg",             "edit-cut"},
        {"EditCopy",           RS2::ActionEditCopy,           tr("&Copy"),              ":/icons/copy.svg",            "edit-copy"},
        {"EditCopyQuick",      RS2::ActionEditCopyQuick,      tr("Copy &Quick"),        ":/icons/copy.svg",            "edit-copy"},
        {"EditPaste",          RS2::ActionEditPaste,          tr("&Paste"),             ":/icons/paste.svg",           "edit-paste"},
        {"EditPasteTransform", RS2::ActionEditPasteTransform, tr("Paste &Transform"),   ":/icons/paste_transform.svg", "edit-paste"},
        {"ModifyDeleteQuick",  RS2::ActionModifyDelete,       tr("&Delete Selected"),   ":/icons/delete.svg"},
        {"EditKillAllActions", RS2::ActionEditKillAllActions, tr("&Selection Pointer"), ":/icons/cursor.svg",          "go-previous-view"}
    });
}

void LC_ActionFactory::setupCreatedActions(QMap<QString, QAction *> &map) {
    map["ZoomPrevious"]->setEnabled(false);
    map["RightDockAreaToggle"]->setChecked(true);
    map["ViewStatusBar"]->setChecked(true);

    map["OptionsGeneral"]->setMenuRole(QAction::NoRole);

    connect(main_window, SIGNAL(printPreviewChanged(bool)), map["FilePrint"], SLOT(setChecked(bool)));
    connect(main_window, SIGNAL(printPreviewChanged(bool)), map["FilePrintPreview"], SLOT(setChecked(bool)));
    connect(main_window, SIGNAL(gridChanged(bool)), map["ViewGrid"], SLOT(setChecked(bool)));
    connect(main_window, SIGNAL(draftChanged(bool)), map["ViewDraft"], SLOT(setChecked(bool)));

    connect(main_window, &QC_ApplicationWindow::windowsChanged, map["OptionsDrawing"], &QAction::setEnabled);
}

void LC_ActionFactory::setDefaultShortcuts(QMap<QString, QAction*>& map, LC_ActionGroupManager* agm) {
    QList<QKeySequence> commandLineShortcuts;
    commandLineShortcuts << QKeySequence(Qt::CTRL | Qt::Key_M) << QKeySequence(Qt::Key_Colon);
    if (LC_GET_BOOL("Keyboard/ToggleFreeSnapOnSpace"))
        commandLineShortcuts << QKeySequence(Qt::Key_Space);

    std::vector<LC_ShortcutInfo> shortcutsList = {
        {"ModifyRevertDirection", QKeySequence(tr("Ctrl+R"))},
        {"ModifyDuplicate",QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D)},
        {"OrderBottom", QKeySequence(Qt::Key_End)},
        {"OrderLower", QKeySequence(Qt::Key_PageDown)},
        {"OrderRaise", QKeySequence(Qt::Key_PageUp)},
        {"OrderTop", QKeySequence(Qt::Key_Home)},
        {"SelectAll", QKeySequence::SelectAll},
        // RVT April 29, 2011 - Added esc key to de-select all entities
        {"DeselectAll", QKeySequence(tr("Ctrl+K"))},
        {"EditUndo", QKeySequence::Undo},
        {"EditRedo", QKeySequence::Redo},
        {"EditCut", QKeySequence::Cut},
        {"EditCutQuick", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X)},
        {"EditCopy", QKeySequence::Copy},
        {"EditCopyQuick", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C)},
        {"EditPaste", QKeySequence::Paste},
        {"EditPasteTransform", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V)},
        {"ZoomIn", QKeySequence::ZoomIn},
        {"ZoomOut", QKeySequence::ZoomOut},
        {"ZoomAuto", QKeySequence(Qt::CTRL | Qt::Key_F)},
        {"ZoomRedraw", QKeySequence::Refresh},
        {"OptionsDrawing", QKeySequence::Preferences},
        {"ReloadStyleSheet", QKeySequence("Ctrl+T")},
        {"FileClose", QKeySequence::Close},
        {"FileCloseAll", QKeySequence("Shift+" + QKeySequence(QKeySequence::Close).toString())},
        {"ViewGrid", QKeySequence(tr("Ctrl+G", "Toggle Grid"))},
        // todo - it's better to replace to something different.... ctrl+d is rather for duplicate
        {"ViewDraft", QKeySequence(tr("Ctrl+D","Toggle Draft Mode"))},
        {"ViewStatusBar", QKeySequence(tr("Ctrl+I", "Hide Statusbar"))},
        {"ModifyDeleteQuick",QKeySequence::Delete  /*QList<QKeySequence>() << QKeySequence::Delete << QKeySequence(Qt::Key_Backspace)*/},
        {"FileNew", QKeySequence::New},
        {"FileOpen", QKeySequence::Open},
        {"FileSave", QKeySequence::Save},
        {"FileSaveAs", QKeySequence::SaveAs},
        {"FilePrint", QKeySequence::Print},
        {"FileQuit", QKeySequence::Quit},
        {"FocusCommand", QKeySequence(Qt::CTRL | Qt::Key_M)}, // commandLineShortcuts}, // fixme - restore shortcuts for focus command line!!!
#if defined(Q_OS_LINUX)
        {"Fullscreen", QKeySequence("F11")},
#else
        {"Fullscreen", QKeySequence::FullScreen},
        {"ExclusiveSnapMode", QKeySequence(Qt::ALT | Qt::Key_X)},


#endif
    };

    map["FileClose"]->setShortcutContext(Qt::WidgetShortcut);

    QKeySequence shortcut = QKeySequence::SaveAs; //(Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    // only define this shortcut for platforms not already using it for save as
    if (shortcut != QKeySequence::SaveAs){
        shortcutsList.push_back({"FileSaveAll", shortcut});
    }

    agm->assignShortcutsToActions(map, shortcutsList);
}

void LC_ActionFactory::markNotEditableActionsShortcuts(QMap<QString, QAction *> &map) {
    // placeholder for exclusion of some actions (by name) from editing in shortcuts mapping dialog
    makeActionsShortcutsNonEditable(map, {
        "RestrictNothing"
    });
}
