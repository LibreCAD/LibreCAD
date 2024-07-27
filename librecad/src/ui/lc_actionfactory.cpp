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
#include "rs_debug.h"

LC_ActionFactory::LC_ActionFactory(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler)
    : LC_ActionFactoryBase(parent, a_handler){
}
// todo - add explanations for commands for actions (probably mix with commandItems) as it was mentioned in issue #570
// todo fixme - review and add proper support of shortcuts, add shortcut management UI

// fixme - cleanup c action handler slots if no issues with ActionType based setup will be discovered, and get rid of template method

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

    foreach (QAction* value, a_map){
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


    setDefaultShortcuts(a_map);
    setupCreatedActions(a_map);

    //    action = new QAction(tr("Regenerate Dimension Entities"), disable_group);
//    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotToolRegenerateDimensions()));
//    action->setObjectName("ToolRegenerateDimensions");
//    a_map["ToolRegenerateDimensions"] = action;
}



void LC_ActionFactory::createDrawLineActions(QMap<QString, QAction*>& map, QActionGroup* group){
    createActionHandlerActions(map, group,{
        {"DrawPoint",                RS2::ActionDrawPoint,               "&Points",                ":/icons/points.svg"},
        {"DrawLine",                 RS2::ActionDrawLine,                "&2 Points",              ":/icons/line_2p.svg"},
        {"DrawLineAngle",            RS2::ActionDrawLineAngle,           "&Angle",                 ":/icons/line_angle.svg"},
        {"DrawLineHorizontal",       RS2::ActionDrawLineHorizontal,      "&Horizontal",            ":/icons/line_horizontal.svg"},
        {"DrawLineVertical",         RS2::ActionDrawLineVertical,        "Vertical",               ":/icons/line_vertical.svg"},
        {"DrawLineFree",             RS2::ActionDrawLineFree,            "&Freehand Line",         ":/icons/line_freehand.svg"},
        {"DrawLineParallel",         RS2::ActionDrawLineParallel,        "&Parallel",              ":/icons/line_parallel.svg"},
        {"DrawLineParallelThrough",  RS2::ActionDrawLineParallelThrough, "Parallel through point", ":/icons/line_parallel_p.svg"},
        {"DrawLineRectangle",        RS2::ActionDrawLineRectangle,       "Rectangle",              ":/icons/line_rectangle.svg"},
        {"DrawLineBisector",         RS2::ActionDrawLineBisector,        "Bisector",               ":/icons/line_bisector.svg"},
        {"DrawLineTangent1",         RS2::ActionDrawLineTangent1,        "Tangent (P,C)",          ":/icons/line_tangent_pc.svg"},
        {"DrawLineTangent2",         RS2::ActionDrawLineTangent2,        "Tangent (C,C)",          ":/icons/line_tangent_cc.svg"},
        {"DrawLineOrthTan",          RS2::ActionDrawLineOrthTan,         "Tangent &Orthogonal",    ":/icons/line_tangent_perpendicular.svg"},
        {"DrawLineOrthogonal",       RS2::ActionDrawLineOrthogonal,      "Orthogonal",             ":/icons/line_perpendicular.svg"},
        {"DrawLineRelAngle",         RS2::ActionDrawLineRelAngle,        "Relative angle",         ":/icons/line_relative_angle.svg"},
        {"DrawLinePolygonCenCor",    RS2::ActionDrawLinePolygonCenCor,   "Pol&ygon (Cen,Cor)",     ":/icons/line_polygon_cen_cor.svg"},
        {"DrawLinePolygonCenTan",    RS2::ActionDrawLinePolygonCenTan,   "Pol&ygon (Cen,Tan)",     ":/icons/line_polygon_cen_tan.svg"},
        {"DrawLinePolygonCorCor",    RS2::ActionDrawLinePolygonCorCor,   "Polygo&n (Cor,Cor)",     ":/icons/line_polygon_cor_cor.svg"},
        {"DrawLineRel",              RS2::ActionDrawSnakeLine,           "Snake",                  ":/icons/line_rel.svg"},
        {"DrawLineRelX",             RS2::ActionDrawSnakeLineX,          "Snake (X)",              ":/icons/line_rel_x.svg"},
        {"DrawLineRelY",             RS2::ActionDrawSnakeLineY,          "Snake (Y)",              ":/icons/line_rel_y.svg"},
        {"DrawLineRectangle1Point",  RS2::ActionDrawRectangle1Point,     "Rectangle (1 Point)",    ":/icons/rectangle_1_point.svg"},
        {"DrawLineRectangle2Points", RS2::ActionDrawRectangle2Points,    "Rectangle (2 Points)",   ":/icons/rectangle_2_points.svg"},
        {"DrawLineRectangle3Points", RS2::ActionDrawRectangle3Points,    "Rectangle (3 Points)",   ":/icons/rectangle_3_points.svg"},
        {"DrawStar",                 RS2::ActionDrawStar,                "Star",                   ":/icons/line_polygon_star.svg"},
        {"DrawLineAngleRel",         RS2::ActionDrawLineAngleRel,        "Angle From Line",        ":/icons/line_angle_rel.svg"},
        {"DrawLineOrthogonalRel",    RS2::ActionDrawLineOrthogonalRel,   "Orthogonal From Line",   ":/icons/line_ortho_rel.svg"},
        {"DrawLineFromPointToLine",  RS2::ActionDrawLineFromPointToLine, "From Point To Line",     ":/icons/line_to_ortho.svg"},
        {"DrawCross",                RS2::ActionDrawCross,               "Cross",                  ":/icons/cross_circle1.svg"},
        {"DrawSliceDivideLine",      RS2::ActionDrawSliceDivideLine,     "Slice/Divide Line",      ":/icons/slice_divide.svg"},
        {"DrawSliceDivideCircle",    RS2::ActionDrawSliceDivideCircle,   "Slice/Divide Circle",    ":/icons/slice_divide_circle.svg"},
        {"DrawLinePoints",           RS2::ActionDrawLinePoints,          "Line of Points",         ":/icons/line_points.svg"}
    });
}

void LC_ActionFactory::createSelectActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group,{
        {"SelectSingle",        RS2::ActionSelectSingle,        "Select Entity",                 ":/icons/select_entity.svg"},
        {"SelectWindow",        RS2::ActionSelectWindow,        "Select Window",                 ":/icons/select_window.svg"},
        {"DeselectWindow",      RS2::ActionDeselectWindow,      "Deselect Window",               ":/icons/deselect_window.svg"},
        {"SelectContour",       RS2::ActionSelectContour,       "(De-)Select &Contour",          ":/icons/deselect_contour.svg"},
        {"SelectIntersected",   RS2::ActionSelectIntersected,   "Select Intersected Entities",   ":/icons/select_intersected_entities.svg"},
        {"DeselectIntersected", RS2::ActionDeselectIntersected, "Deselect Intersected Entities", ":/icons/deselect_intersected_entities.svg"},
        {"SelectLayer",         RS2::ActionSelectLayer,         "(De-)Select Layer",             ":/icons/deselect_layer.svg"}
    });
}

void LC_ActionFactory::createDrawCircleActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group,{
        {"DrawCircle",         RS2::ActionDrawCircle,         "Center, &Point",                ":/icons/circle_center_point.svg"},
        {"DrawCircleByArc",    RS2::ActionDrawCircleByArc,    "By Arc",                        ":/icons/circle_by_arc.svg"},
        {"DrawCircleCR",       RS2::ActionDrawCircleCR,       "Center, &Radius",               ":/icons/circle_center_radius.svg"},
        {"DrawCircle2P",       RS2::ActionDrawCircle2P,       "2 Points",                      ":/icons/circle_2_points.svg"},
        {"DrawCircle2PR",      RS2::ActionDrawCircle2PR,      "2 Points, Radius",              ":/icons/circle_2_points_radius.svg"},
        {"DrawCircle3P",       RS2::ActionDrawCircle3P,       "3 Points",                      ":/icons/circle_3_points.svg"},
        {"DrawCircleParallel", RS2::ActionDrawCircleParallel, "&Concentric",                   ":/icons/circle_concentric.svg"},
        {"DrawCircleInscribe", RS2::ActionDrawCircleInscribe, "Circle &Inscribed",             ":/icons/circle_inscribed.svg"},
        {"DrawCircleTan2",     RS2::ActionDrawCircleTan2,     "Tangential 2 Circles, Radius",  ":/icons/circle_tangential_2circles_radius.svg"},
        {"DrawCircleTan2_1P",  RS2::ActionDrawCircleTan2_1P,  "Tangential 2 Circles, 1 Point", ":/icons/circle_tangential_2circles_point.svg"},
        {"DrawCircleTan3",     RS2::ActionDrawCircleTan3,     "Tangential &3 Circles",         ":/icons/circle_tangential_3entities.svg"},
        {"DrawCircleTan1_2P",  RS2::ActionDrawCircleTan1_2P,  "Tangential, 2 P&oints",         ":/icons/circle_tangential_2points.svg"}
    });
}

void LC_ActionFactory::createDrawCurveActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group,{
        {"DrawArc",             RS2::ActionDrawArc,             "&Center, Point, Angles",    ":/icons/arc_center_point_angle.svg"},
        {"DrawArc3P",           RS2::ActionDrawArc3P,           "&3 Points",                 ":/icons/arc_3_points.svg"},
        {"DrawArcParallel",     RS2::ActionDrawArcParallel,     "&Concentric",               ":/icons/arc_concentric.svg"},     // fixme - why this action is not in list?
        {"DrawArcTangential",   RS2::ActionDrawArcTangential,   "Arc &Tangential",           ":/icons/arc_continuation.svg"},
        {"DrawParabola4Points", RS2::ActionDrawParabola4Points, "Para&bola 4 points",        ":/icons/parabola_4_points.svg"},
        {"DrawParabolaFD",      RS2::ActionDrawParabolaFD,      "Parabola &Focus Directrix", ":/icons/parabola_focus_directrix.svg"},
        {"DrawSpline",          RS2::ActionDrawSpline,          "&Spline",                   ":/icons/spline.svg"},
        {"DrawSplinePoints",    RS2::ActionDrawSplinePoints,    "&Spline through points",    ":/icons/spline_points.svg"}
    });
}

void LC_ActionFactory::createDrawEllipseActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group,{
        {"DrawEllipseAxis",          RS2::ActionDrawEllipseAxis,          "&Ellipse (Axis)",              ":/icons/ellipse_axis.svg"},
        {"DrawEllipseArcAxis",       RS2::ActionDrawEllipseArcAxis,       "Ellipse &Arc (Axis)",          ":/icons/ellipse_arc_axis.svg"},
        {"DrawEllipseFociPoint",     RS2::ActionDrawEllipseFociPoint,     "Ellipse &Foci Point",          ":/icons/ellipse_foci_point.svg"},
        {"DrawEllipse4Points",       RS2::ActionDrawEllipse4Points,       "Ellipse &4 Point",             ":/icons/ellipse_4_points.svg"},
        {"DrawEllipseCenter3Points", RS2::ActionDrawEllipseCenter3Points, "Ellipse Center and &3 Points", ":/icons/ellipse_center_3_points.svg"},
        {"DrawEllipseInscribe",      RS2::ActionDrawEllipseInscribe,      "Ellipse &Inscribed",           ":/icons/ellipse_inscribed.svg"}
    });
}

void LC_ActionFactory::createDrawPolylineActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group,{
        {"DrawPolyline",        RS2::ActionDrawPolyline,        "&Polyline",                               ":/icons/polylines_polyline.svg"},
        {"PolylineAdd",         RS2::ActionPolylineAdd,         "&Add node",                               ":/icons/insert_node.svg"},
        {"PolylineAppend",      RS2::ActionPolylineAppend,      "A&ppend node",                            ":/icons/append_node.svg"},
        {"PolylineDel",         RS2::ActionPolylineDel,         "&Delete node",                            ":/icons/delete_node.svg"},
        {"PolylineDelBetween",  RS2::ActionPolylineDelBetween,  "Delete &between two nodes",               ":/icons/delete_between_nodes.svg"},
        {"PolylineTrim",        RS2::ActionPolylineTrim,        "&Trim segments",                          ":/icons/trim.svg"},
        {"PolylineEquidistant", RS2::ActionPolylineEquidistant, "Create &Equidistant Polylines",           ":/icons/create_equidistant_polyline.svg"},
        {"PolylineSegment",     RS2::ActionPolylineSegment,     "Create Polyline from Existing &Segments", ":/icons/create_polyline_from_existing_segments.svg"}
    });
}

void LC_ActionFactory::createDrawOtherActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group,{
        {"ZoomPan",   RS2::ActionZoomPan,   "Zoom &Panning", ":/icons/zoom_pan.svg"},
        {"DrawMText", RS2::ActionDrawMText, "&MText",        ":/icons/mtext.svg"},
        {"DrawText",  RS2::ActionDrawText,  "&Text",         ":/icons/text.svg"},
        {"DrawHatch", RS2::ActionDrawHatch, "&Hatch",        ":/icons/hatch.svg"},
        {"DrawImage", RS2::ActionDrawImage, "Insert &Image", ":/icons/camera.svg"}
    });
}

void LC_ActionFactory::createDrawDimensionsActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group,{
        {"DimAligned",   RS2::ActionDimAligned,   "&Aligned",    ":/icons/dim_aligned.svg"},
        {"DimLinear",    RS2::ActionDimLinear,    "&Linear",     ":/icons/dim_linear.svg"},
        {"DimLinearHor", RS2::ActionDimLinearHor, "&Horizontal", ":/icons/dim_horizontal.svg"},
        {"DimLinearVer", RS2::ActionDimLinearVer, "&Vertical",   ":/icons/dim_vertical.svg"},
        {"DimRadial",    RS2::ActionDimRadial,    "&Radial",     ":/icons/dim_radial.svg"},
        {"DimDiametric", RS2::ActionDimDiametric, "&Diametric",  ":/icons/dim_diametric.svg"},
        {"DimAngular",   RS2::ActionDimAngular,   "&Angular",    ":/icons/dim_angular.svg"},
        {"DimArc",       RS2::ActionDimArc,       "&Arc",        ":/icons/dim_arc.svg"},
        {"DimLeader",    RS2::ActionDimLeader,    "&Leader",     ":/icons/dim_leader.svg"}
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
        {"ModifyAttributes",      RS2::ActionModifyAttributes,      "&Attributes",                ":/icons/attributes.svg"},
        {"ModifyDelete",          RS2::ActionModifyDelete,          "&Delete",                    ":/icons/delete.svg"},
        {"ModifyMove",            RS2::ActionModifyMove,            "&Move / Copy",               ":/icons/move_copy.svg"},
        {"ModifyRevertDirection", RS2::ActionModifyRevertDirection, "Re&vert direction",          ":/icons/revert_direction.svg"},
        {"ModifyRotate",          RS2::ActionModifyRotate,          "&Rotate",                    ":/icons/rotate.svg"},
        {"ModifyScale",           RS2::ActionModifyScale,           "&Scale",                     ":/icons/scale.svg"},
        {"ModifyMirror",          RS2::ActionModifyMirror,          "&Mirror",                    ":/icons/mirror.svg"},
        {"ModifyMoveRotate",      RS2::ActionModifyMoveRotate,      "Mo&ve and Rotate",           ":/icons/move_rotate.svg"},
        {"ModifyRotate2",         RS2::ActionModifyRotate2,         "Rotate T&wo",                ":/icons/rotate2.svg"},
        {"ModifyEntity",          RS2::ActionModifyEntity,          "&Properties",                ":/icons/properties.svg"},
        {"ModifyTrim",            RS2::ActionModifyTrim,            "&Trim",                      ":/icons/trim.svg"},
        {"ModifyTrim2",           RS2::ActionModifyTrim2,           "Tr&im Two",                  ":/icons/trim2.svg"},
        {"ModifyTrimAmount",      RS2::ActionModifyTrimAmount,      "&Lengthen",                  ":/icons/trim_value.svg"},
        {"ModifyOffset",          RS2::ActionModifyOffset,          "O&ffset",                    ":/icons/offset.svg"},
        {"ModifyCut",             RS2::ActionModifyCut,             "&Divide",                    ":/icons/divide.svg"},
        {"ModifyStretch",         RS2::ActionModifyStretch,         "&Stretch",                   ":/icons/stretch.svg"},
        {"ModifyBevel",           RS2::ActionModifyBevel,           "&Bevel",                     ":/icons/bevel.svg"},
        {"ModifyRound",           RS2::ActionModifyRound,           "&Fillet",                    ":/icons/fillet.svg"},
        {"ModifyExplodeText",     RS2::ActionModifyExplodeText,     "&Explode Text into Letters", ":/icons/explode_text_to_letters.svg"},
        {"BlocksExplode",         RS2::ActionBlocksExplode,         "Ex&plode",                   ":/icons/explode.svg"},
        {"ModifyBreakDivide",     RS2::ActionModifyBreakDivide,     "Break/Divide",               ":/icons/break_out_trim.svg"},
        {"ModifyLineGap",         RS2::ActionModifyLineGap,         "Line Gap",                   ":/icons/line_gap.svg"},
        {"ModifyLineJoin",        RS2::ActionModifyLineJoin,        "Line Join",                  ":/icons/line_join.svg"},
        {"ModifyDuplicate",       RS2::ActionModifyDuplicate,       "Duplicate",                  ":/icons/duplicate.svg"}
    });
}

void LC_ActionFactory::createPenActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"PenSyncFromLayer", RS2::ActionPenSyncFromLayer, "Update Current Pen by Active Layer' Pen", ":/extui/back.png"}
    });
}

void LC_ActionFactory::createPenActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"PenPick",         RS2::ActionPenPick,         "&Pick Pen From Entity",            ":/extui/selectsingle.png"},
        {"PenPickResolved", RS2::ActionPenPickResolved, "&Pick Pen From Entity (Resolved)", ":/extui/relzeromove.png"},
        {"PenApply",        RS2::ActionPenApply,        "Apply Pen to Entity",              ":/icons/pen_apply.svg"},
        {"PenCopy",         RS2::ActionPenCopy,         "Copy Pen",                         ":/icons/pen_copy.svg"}
    });
}

void LC_ActionFactory::createOrderActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {

    createActionHandlerActions(map, group, {
        {"OrderBottom", RS2::ActionOrderBottom, "move to bottom", ":/icons/downmost.svg"},
        {"OrderLower",  RS2::ActionOrderLower, "lower after entity", ":/icons/down.svg"},
        {"OrderRaise",  RS2::ActionOrderRaise, "raise over entity", ":/icons/up.svg"},
        {"OrderTop",    RS2::ActionOrderTop, "move to top", ":/icons/upmost.svg"}
    });
}

void LC_ActionFactory::createInfoActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group, {
        {"InfoInside",      RS2::ActionInfoInside,           "Point inside contour",               ""},
        {"InfoDist",        RS2::ActionInfoDistPoint2Point,  "&Distance Point to Point",           ":/icons/distance_point_to_point.svg"},
        {"InfoDist2",       RS2::ActionInfoDistEntity2Point, "&Distance Entity to Point",          ":/icons/distance_entity_to_point.svg"},
        {"InfoDist3",       RS2::ActionInfoDistPoint2Entity, "&Distance Point to Entity",          ":/icons/distance_point_to_entity.svg"},
        {"InfoAngle",       RS2::ActionInfoAngle,            "An&gle between two lines",           ":/icons/angle_line_to_line.svg"},
        {"InfoTotalLength", RS2::ActionInfoTotalLength,      "&Total length of selected entities", ":/icons/total_length_selected_entities.svg"},
        {"InfoArea",        RS2::ActionInfoArea,             "Polygonal &Area",                    ":/icons/polygonal_area.svg"},
        {"EntityInfo",      RS2::ActionInfoProperties,       "Entity Properties",                  ":/extui/menuselect.png"},
        {"PickCoordinates", RS2::ActionInfoPickCoordinates,  "Collect Coordinates",                ":/extui/menupoint.png"}
    });
}

void LC_ActionFactory::createViewActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group, {
        {"ZoomWindow",RS2::ActionZoomWindow, "&Window Zoom", ":/icons/zoom_window.svg","zoom-select"}});

    createMainWindowActions(map, group, {
        {"Fullscreen",    SLOT(toggleFullscreen(bool)),  "&Fullscreen"},
        {"ViewGrid",      SLOT(slotViewGrid(bool)),      "&Grid",  ":/icons/grid.svg"},
        {"ViewDraft",     SLOT(slotViewDraft(bool)),     "&Draft", ":/icons/draft.svg"},
        {"ViewStatusBar", SLOT(slotViewStatusBar(bool)), "&Statusbar"}
    }, true);
}

void LC_ActionFactory::createLayerActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"LayersDefreezeAll",        RS2::ActionLayersDefreezeAll,        "&Show all layers",           ":/ui/visibleblock.png"},
        {"LayersFreezeAll",          RS2::ActionLayersFreezeAll,          "&Hide all layers",           ":/ui/hiddenblock.png"},
        {"LayersUnlockAll",          RS2::ActionLayersUnlockAll,          "&Unlock all",                ":/ui/unlockedlayer.png"},
        {"LayersLockAll",            RS2::ActionLayersLockAll,            "&Lock all",                  ":/ui/lockedlayer.png"},
        {"LayersAdd",                RS2::ActionLayersAdd,                "&Add Layer",                 ":/icons/add.svg"},
        {"LayersRemove",             RS2::ActionLayersRemove,             "&Remove Layer",              ":/icons/remove.svg"},
        {"LayersEdit",               RS2::ActionLayersEdit,               "&Edit Layer",                ":/icons/attributes.svg"},
        {"LayersToggleLock",         RS2::ActionLayersToggleLock,         "Toggle Layer Loc&k",         ":/icons/locked.svg"},
        {"LayersToggleView",         RS2::ActionLayersToggleView,         "&Toggle Layer Visibility",   ":/icons/visible.svg"},
        {"LayersTogglePrint",        RS2::ActionLayersTogglePrint,        "Toggle Layer &Print",        ":/icons/print.svg"},
        {"LayersToggleConstruction", RS2::ActionLayersToggleConstruction, "Toggle &Construction Layer", ":/icons/construction_layer.svg"},
        {"LayersExportSelected",     RS2::ActionLayersExportSelected,     "&Export Selected Layer(s)"},
        {"LayersExportVisible",      RS2::ActionLayersExportVisible,      "Export &Visible Layer(s)"}
    });
}

void LC_ActionFactory::createBlockActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"BlocksDefreezeAll", RS2::ActionBlocksDefreezeAll, "&Show all blocks",         ":/ui/blockdefreeze.png"},
        {"BlocksFreezeAll",   RS2::ActionBlocksFreezeAll,   "&Hide all blocks",         ":/ui/blockfreeze.png"},
        {"BlocksAdd",         RS2::ActionBlocksAdd,         "&Add Block",               ":/icons/add.svg"},
        {"BlocksRemove",      RS2::ActionBlocksRemove,      "&Remove Block",            ":/icons/remove.svg"},
        {"BlocksAttributes",  RS2::ActionBlocksAttributes,  "&Rename Block",            ":/icons/rename_active_block.svg"},
        {"BlocksEdit",        RS2::ActionBlocksEdit,        "&Edit Block",              ":/icons/properties.svg"},
        {"BlocksSave",        RS2::ActionBlocksSave,        "&Save Block",              ":/icons/save.svg"},
        {"BlocksInsert",      RS2::ActionBlocksInsert,      "&Insert Block",            ":/icons/insert_active_block.svg"},
        {"BlocksToggleView",  RS2::ActionBlocksToggleView,  "Toggle Block &Visibility", ":/ui/layertoggle.png"},
        {"BlocksCreate",      RS2::ActionBlocksCreate,      "&Create Block",            ":/icons/create_block.svg"}
    });
}

void LC_ActionFactory::createOptionsActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"OptionsDrawing",RS2::ActionOptionsDrawing, "Current &Drawing Preferences", ":/icons/drawing_settings.svg"}
    });

    createMainWindowActions(map, group, {
        {"OptionsGeneral",   SLOT(slotOptionsGeneral()), "&Application Preferences", ":/icons/settings.svg"},
        {"WidgetOptions",    SLOT(widgetOptionsDialog()),    "Widget Options"},
        {"DeviceOptions",    SLOT(showDeviceOptions()),      "Device Options"},
        {"ReloadStyleSheet", SLOT(reloadStyleSheet()),   "Reload Style Sheet"}
    });
}

void LC_ActionFactory::createFileActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"FilePrintPreview", SLOT(slotFilePrintPreview(bool)),  "Print Pre&view",     ":/icons/print_preview.svg",     "document-print-preview"},
    });
}

void LC_ActionFactory::createFileActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"FileExport",       SLOT(slotFileExport()),       "&Export as image",   ":/icons/export.svg"},
        {"FileClose", nullptr,                             "&Close",             ":/icons/close.svg"},
        {"FileCloseAll",     SLOT(slotFileCloseAll()),     "Close All",          ":/icons/close_all.svg"},
        {"FilePrintPDF",     SLOT(slotFilePrintPDF()),     "Export as PDF",      ":/icons/export_pdf.svg"},
        {"BlocksImport",     SLOT(slotImportBlock()),      "&Block",             ":/icons/insert_active_block.svg"},
        {"FileNew",          SLOT(slotFileNewNew()),       "&New",               ":/icons/new.svg",               "document-new"},
        {"FileNewTemplate",  SLOT(slotFileNewTemplate()),  "New From &Template", ":/icons/new_from_template.svg", "document-new"},// fixme
        {"FileOpen",         SLOT(slotFileOpen()),         "&Open...",           ":/icons/open.svg",              "document-open"},
        {"FileSave",         SLOT(slotFileSave()),         "&Save",              ":/icons/save.svg",              "document-save"},
        {"FileSaveAs",       SLOT(slotFileSaveAs()),       "Save &as...",        ":/icons/save_as.svg",           "document-save-as"},
        {"FileSaveAll",      SLOT(slotFileSaveAll()),      "Save A&ll...",       ":/icons/save_all.svg"},
        {"FilePrint",        SLOT(slotFilePrint()),        "&Print...",          ":/icons/print.svg",             "document-print"},
        {"FileQuit",         SLOT(slotFileQuit()),         "&Quit",              ":/icons/quit.svg",              "application-exit"},
    });

    createAction_AH("FileExportMakerCam",RS2::ActionFileExportMakerCam,
                    "Export as CA&M/plain SVG...", nullptr, nullptr, group, map);
}

void LC_ActionFactory::createWidgetActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"LeftDockAreaToggle",        SLOT(toggleLeftDockArea(bool)),        "Left",     ":/icons/dockwidgets_left.svg"},
        {"RightDockAreaToggle",       SLOT(toggleRightDockArea(bool)),       "Right",    ":/icons/dockwidgets_right.svg"},
        {"TopDockAreaToggle",         SLOT(toggleTopDockArea(bool)),         "Top",      ":/icons/dockwidgets_top.svg"},
        {"BottomDockAreaToggle",      SLOT(toggleBottomDockArea(bool)),      "Bottom",   ":/icons/dockwidgets_bottom.svg"},
        {"FloatingDockwidgetsToggle", SLOT(toggleFloatingDockwidgets(bool)), "Floating", ":/icons/dockwidgets_floating.svg"}
    }, true);
}

void LC_ActionFactory::createWidgetActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"RedockWidgets",        SLOT(slotRedockWidgets()),    "Re-dock Widgets"},
        {"InvokeMenuCreator",    SLOT(invokeMenuCreator()),    "Menu Creator"},
        {"InvokeToolbarCreator", SLOT(invokeToolbarCreator()), "Toolbar Creator"}
    });
}

void LC_ActionFactory::createViewActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createAction_MW("FocusCommand",SLOT(slotFocusCommandLine()), "Focus on &Command Line", ":/main/editclear.png", nullptr, group, map);

    createActionHandlerActions(map, group, {
        {"ZoomIn",       RS2::ActionZoomIn,       "Zoom &In",       ":/icons/zoom_in.svg",       "zoom-in"},
        {"ZoomOut",      RS2::ActionZoomOut,      "Zoom &Out",      ":/icons/zoom_out.svg",      "zoom-out"},
        {"ZoomAuto",     RS2::ActionZoomAuto,     "&Auto Zoom",     ":/icons/zoom_auto.svg",     "zoom-fit-best"},
        {"ZoomPrevious", RS2::ActionZoomPrevious, "Previous &View", ":/icons/zoom_previous.svg", "zoom-previous"},
        {"ZoomRedraw",   RS2::ActionZoomRedraw,   "&Redraw",        ":/icons/redraw.svg",        "view-refresh"}
    });
}

void LC_ActionFactory::createSelectActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"SelectAll",    RS2::ActionSelectAll,    "Select &All",      ":/icons/select_all.svg"},
        {"DeselectAll",  RS2::ActionDeselectAll,  "Deselect &all",    ":/icons/deselect_all.svg"},
        {"SelectInvert", RS2::ActionSelectInvert, "Invert Selection", ":/icons/select_inverted.svg"}
    });
}

void LC_ActionFactory::createEditActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createActionHandlerActions(map, group, {
        {"EditUndo",           RS2::ActionEditUndo,           "&Undo",              ":/icons/undo.svg",            "edit-undo"},
        {"EditRedo",           RS2::ActionEditRedo,           "&Redo",              ":/icons/redo.svg",            "edit-redo"},
        {"EditCut",            RS2::ActionEditCut,            "Cu&t",               ":/icons/cut.svg",             "edit-cut"},
        {"EditCutQuick",       RS2::ActionEditCutQuick,       "Cut Quic&k",         ":/icons/cut.svg",             "edit-cut"},
        {"EditCopy",           RS2::ActionEditCopy,           "&Copy",              ":/icons/copy.svg",            "edit-copy"},
        {"EditCopyQuick",      RS2::ActionEditCopyQuick,      "Copy &Quick",        ":/icons/copy.svg",            "edit-copy"},
        {"EditPaste",          RS2::ActionEditPaste,          "&Paste",             ":/icons/paste.svg",           "edit-paste"},
        {"EditPasteTransform", RS2::ActionEditPasteTransform, "Paste &Transform",   ":/icons/paste_transform.svg", "edit-paste"},
        {"ModifyDeleteQuick",  RS2::ActionModifyDelete,       "&Delete Selected",   ":/icons/delete.svg"},
        {"EditKillAllActions", RS2::ActionEditKillAllActions, "&Selection Pointer", ":/icons/cursor.svg",          "go-previous-view"}
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

void LC_ActionFactory::setDefaultShortcuts(QMap<QString, QAction*>& map) {
    QList<QKeySequence> commandLineShortcuts;
    commandLineShortcuts << QKeySequence(Qt::CTRL | Qt::Key_M) << QKeySequence(Qt::Key_Colon);
    if (!RS_SETTINGS->readNumEntry("/Keyboard/ToggleFreeSnapOnSpace", false))
        commandLineShortcuts << QKeySequence(Qt::Key_Space);

    std::vector<ActionShortCutInfo> shortcutsList = {
        {"ModifyRevertDirection", QKeySequence(tr("Ctrl+R"))},
        {"ModifyDuplicate",QList<QKeySequence>() << QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D)<< QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_D)},
        {"OrderBottom", QKeySequence(Qt::Key_End)},
        {"OrderLower", QKeySequence(Qt::Key_PageDown)},
        {"OrderRaise", QKeySequence(Qt::Key_PageUp)},
        {"OrderTop", QKeySequence(Qt::Key_Home)},
        {"SelectAll", QKeySequence::SelectAll},
        // RVT April 29, 2011 - Added esc key to de-select all entities
        {"DeselectAll", QList<QKeySequence>() << QKeySequence(tr("Ctrl+K"))},
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
        {"ModifyDeleteQuick", QList<QKeySequence>() << QKeySequence::Delete << QKeySequence(Qt::Key_Backspace)},
        {"FileNew", QKeySequence::New},
        {"FileOpen", QKeySequence::Open},
        {"FileSave", QKeySequence::Save},
        {"FileSaveAs", QKeySequence::SaveAs},
        {"FilePrint", QKeySequence::Print},
        {"FileQuit", QKeySequence::Quit},
        {"FocusCommand", commandLineShortcuts},
#if defined(Q_OS_LINUX)
        {"Fullscreen", QKeySequence("F11")},
#else
        {"Fullscreen", QKeySequence::FullScreen},
#endif
    };

    map["FileClose"]->setShortcutContext(Qt::WidgetShortcut);

    QKeySequence shortcut = QKeySequence::SaveAs; //(Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    // only define this shortcut for platforms not already using it for save as
    if (shortcut != QKeySequence::SaveAs){
        shortcutsList.push_back({"FileSaveAll", shortcut});
    }

    assignShortcutsToActions(map, shortcutsList);
}

