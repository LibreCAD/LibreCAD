/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

// This file was first published at: github.com/r-a-v-a-s/LibreCAD.git

// lc_actionfactory is a rewrite of qg_actionfactory; some copied content remains.
// qg_actionfactory contributors:
// Andrew Mustun, Claude Sylvain, R. van Twisk, Dongxu Li, Rallaz, Armin Stebich, ravas, korhadris

#include <QAction>
#include <QKeySequence>
#include "lc_actionfactory.h"

#include "lc_actiongroup.h"
#include "lc_actiongroupmanager.h"
#include "lc_infocursorsettingsmanager.h"
#include "qc_applicationwindow.h"
#include "rs_settings.h"
#include "lc_shortcutinfo.h"

LC_ActionFactory::LC_ActionFactory(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler)
    : LC_ActionFactoryBase(parent, a_handler){
}

void LC_ActionFactory::initActions(LC_ActionGroupManager* agm, bool useTheme) {
   initActionGroupManager(agm);
   fillActionContainer(agm, useTheme);
}

void LC_ActionFactory::initActionGroupManager(LC_ActionGroupManager* agm) {
    createActionGroups(
    {
       {"block", tr("Block"), tr("Block related operations"), ":/icons/create_block.lci"},
        {"circle", tr("Circle"), tr("Circle drawing commands"), ":/icons/circle.lci"},
        {"curve", tr("Arc"), tr("Arc drawing commands"), ":/icons/arc_center_point_angle.lci"},
        {"spline", tr("Spline"), tr("Spline drawing commands"), ":/icons/spline_points.lci"},
        {"edit", tr("Edit"), tr("Editing operations"), ":/icons/rename_active_block.lci"},
        {"ellipse", tr("Ellipse"), tr("Ellipse drawing commands"), ":/icons/ellipses.lci"},
        {"file", tr("File"), tr("File Operations"), ":/icons/save.lci"},
        {"dimension", tr("Dimension"), tr("Dimensions creation commands"), ":/icons/dim_horizontal.lci"},
        {"info", tr("Info"), tr("Informational commands"), ":/icons/measure.lci"},
        {"layer", tr("Layer"), tr("Layers operations"), ":/icons/deselect_layer.lci", false},
        {"line", tr("Line"), tr("Line drawing commands"), ":/icons/line.lci"},
        {"point", tr("Point"), tr("Point drawing commands"), ":/icons/points.lci"},
        {"shape", tr("Polygon"), tr("Polygon drawing commands"), ":/icons/rectangle_2_points.lci"},
        {"modify", tr("Modify"), tr("Modification operations"), ":/icons/move_rotate.lci"},
        {"options", tr("Options"), tr("Options management"), ":/icons/settings.lci"},
        {"other", tr("Other"), tr("Other operations"), ":/icons/text.lci"},
        {"relZero", tr("Relative Zero"), tr("Relative Zero"), ":/icons/set_rel_zero.lci", false},
        {"polyline", tr("Polyline"), tr("Polyline drawing commands"), ":/icons/polylines_polyline.lci"},
        {"pen", tr("PenTB"), tr("Pen related operations"), ":/icons/pen_apply.lci"},
        {"restriction", tr("Restriction"), tr("Snap restrictions"), ":/icons/restr_ortho.lci", false},
        {"select", tr("Select"), tr("Entity selection operations"), ":/icons/select.lci"},
        {"snap", tr("Snap"), tr("Snapping operations"), ":/icons/snap_intersection.lci", false},
        {"snap_extras", tr("Snap Extras"), tr("Additional Snaps"), ":/icons/snap_free.lci", false},
        {"view", tr("View"), tr("View related operations"), ":/icons/zoom_in.lci", false},
        {"namedViews", tr("Named Views"), tr("Persistent Views operations"), ":/icons/visible.lci", false},
        {"workspaces", tr("Workspaces"), tr("Workspaces operations"), ":/icons/workspace.lci", false},
        {"ucs", tr("UCS"), tr("UCS operations"), ":/icons/set_ucs.lci"},
        {"widgets", tr("Widgets"), tr("Widgets management"), ":/icons/dockwidgets_bottom.lci", false},
        {"infoCursor", tr("InfoCursor"), tr("Informational Cursor"), ":/icons/info_cursor_enable.lci", false},
        {"entity_layer", tr("Entity Layer"), tr("Entity's Layer"), ":/icons/layer_list.lci", false},
        {"interactive_pick", tr("Interactive"), tr("Interactive Pick"), ":/icons/interactive_pick_point.lci", true}
    },agm);

    auto fileGroup = agm->getGroupByName("file");
    auto optionsGroup = agm->getGroupByName("options");

    for (auto const& actionGroup : agm->findChildren<LC_ActionGroup*>()) {
        actionGroup->setExclusive(false);
        if (fileGroup != actionGroup && optionsGroup != actionGroup) {
            connect(m_appWin, &QC_ApplicationWindow::windowsChanged, actionGroup, &LC_ActionGroup::setEnabled);
        }
    }

    /*auto toolGroups = agm->toolGroups();
    for (auto actionGroup : toolGroups) {
        connect(actionGroup, &LC_ActionGroup::triggered, m_appWin, &QC_ApplicationWindow::relayAction);
    }*/
}

void LC_ActionFactory::createEntityLayerActions(QMap<QString, QAction*>& map, LC_ActionGroup* group) {
    createActionHandlerActions(map, group,{
     {"EntityLayerActivate",     RS2::ActionLayerEntityActivate,          tr("Activate Entity's Layer"),           ":/icons/select_entity.lci"},
     {"EntityLayerView",         RS2::ActionLayerEntityToggleView,        tr("Hide Entity's Layer"),  ":/icons/not_visible.lci"},
     {"EntityLayerHideOthers",   RS2::ActionLayerEntityHideOthers,        tr("Hide Other Layers than Entity's"),  ":/icons/not_visible_all.lci"},
     {"EntityLayerLock",         RS2::ActionLayerEntityToggleLock,        tr("Lock Entity's Layer"),        ":/icons/locked.lci"},
     {"EntityLayerConstruction", RS2::ActionLayerEntityToggleConstruction,tr("Toggle Entity's Layer Construction"),":/icons/construction_layer.lci"},
     {"EntityLayerPrint",        RS2::ActionLayerEntityTogglePrint,       tr("Toggle Entity's Layer Printing"),     ":/icons/print.lci"}
 });
}

// todo - add explanations for commands for actions (probably mix with commandItems) as it was mentioned in issue #570

void LC_ActionFactory::fillActionContainer(LC_ActionGroupManager* agm, bool useTheme){
    m_usingTheme = useTheme;
    QMap<QString, QAction *> &a_map = agm->getActionsMap();
    createSelectActions(a_map, agm->getGroupByName("select"));
    createDrawLineActions(a_map, agm->getGroupByName("line"));
    createDrawPointsActions(a_map, agm->getGroupByName("point"));
    createDrawShapeActions(a_map, agm->getGroupByName("shape"));
    createDrawCircleActions(a_map, agm->getGroupByName("circle"));
    createDrawCurveActions(a_map, agm->getGroupByName("curve"));
    createDrawSplineActions(a_map, agm->getGroupByName("spline"));
    createDrawEllipseActions(a_map, agm->getGroupByName("ellipse"));
    createDrawPolylineActions(a_map, agm->getGroupByName("polyline"));
    createDrawOtherActions(a_map, agm->getGroupByName("other"));
    createDrawDimensionsActions(a_map, agm->getGroupByName("dimension"));
    createModifyActions(a_map, agm->getGroupByName("modify"));
    createPenActions(a_map, agm->getGroupByName("pen"));
    createInfoActions(a_map, agm->getGroupByName("info"));
    createViewActions(a_map, agm->getGroupByName("view"));
    createWidgetActions(a_map, agm->getGroupByName("widgets"));
    createFileActions(a_map, agm->getGroupByName("file"));

    createSnapActions(a_map, agm->getGroupByName("snap"));
    createInfoCursorActions(a_map, agm->getGroupByName("infoCursor"));
    createSnapExtraActions(a_map, agm->getGroupByName("snap_extras"));
    createRestrictActions(a_map, agm->getGroupByName("restriction"));
    createRelZeroActions(a_map, agm->getGroupByName("other"));
    createUCSActions(a_map, agm->getGroupByName("ucs"));
    createEditActions(a_map, agm->getGroupByName("edit"));

    createEntityLayerActions(a_map, agm->getGroupByName("entity_layer"));
    createInteractivePickActions(a_map, agm->getGroupByName("interactive_pick"));

    for (QAction* value: std::as_const(a_map)){
        if (value != nullptr) {
            value->setCheckable(true);
        }
    }

    // not checkable actions
    createPenActionsUncheckable(a_map, agm->getGroupByName("pen"));
    createOrderActionsUncheckable(a_map, agm->getGroupByName("modify"));
    createLayerActionsUncheckable(a_map, agm->getGroupByName("layer"));
    createBlockActionsUncheckable(a_map, agm->getGroupByName("block"));
    createOptionsActionsUncheckable(a_map, agm->getGroupByName("options"));
    createSelectActionsUncheckable(a_map, agm->getGroupByName("select"));
    createFileActionsUncheckable(a_map, agm->getGroupByName("file"));
    createViewActionsUncheckable(a_map, agm->getGroupByName("view"));
    createNamedViewActionsUncheckable(a_map, agm->getGroupByName("namedViews"));
    createWorkspacesActionsUncheckable(a_map, agm->getGroupByName("workspaces"));
    createWidgetActionsUncheckable(a_map, agm->getGroupByName("widgets"));
    createEditActionsUncheckable(a_map, agm->getGroupByName("edit"));
    createDrawDimensionsUncheckable(a_map, agm->getGroupByName("dimension"));

    setupCreatedActions(a_map);
    setDefaultShortcuts(a_map, agm);

    agm->loadShortcuts(a_map);

    // todo - may we report errors somehow there?
    markNotEditableActionsShortcuts(a_map);

    agm->completeInit();

    fillActionLists(a_map);
    addActionsToMainWindow(a_map);

    prepareActionsToDisableInPrintPreview(m_appWin->m_actionsToDisableInPrintPreviewList, a_map);
}

void LC_ActionFactory::createDrawShapeActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group, {
        {"DrawLineRectangle",        RS2::ActionDrawLineRectangle,       tr("Rectangle"),              ":/icons/line_rectangle.lci"},
        {"DrawLinePolygonCenCor",    RS2::ActionDrawLinePolygonCenCor,   tr("Pol&ygon (Cen,Cor)"),     ":/icons/line_polygon_cen_cor.lci"},
        {"DrawLinePolygonCenTan",    RS2::ActionDrawLinePolygonCenTan,   tr("Pol&ygon (Cen,Tan)"),     ":/icons/line_polygon_cen_tan.lci"},
        {"DrawLinePolygonCorCor",    RS2::ActionDrawLinePolygonCorCor,   tr("Polygo&n (Cor,Cor)"),     ":/icons/line_polygon_cor_cor.lci"},
        {"DrawLinePolygonSideSide",  RS2::ActionDrawLinePolygonSideSide, tr("Polygo&n (Tan,Tan)"),     ":/icons/line_polygon_size_size.lci"},
        {"DrawStar",                 RS2::ActionDrawStar,                tr("Star"),                   ":/icons/line_polygon_star.lci"},
        {"DrawLineRectangle1Point",  RS2::ActionDrawRectangle1Point,     tr("Rectangle (1 Point)"),    ":/icons/rectangle_1_point.lci"},
        {"DrawLineRectangle2Points", RS2::ActionDrawRectangle2Points,    tr("Rectangle (2 Points)"),   ":/icons/rectangle_2_points.lci"},
        {"DrawLineRectangle3Points", RS2::ActionDrawRectangle3Points,    tr("Rectangle (3 Points)"),   ":/icons/rectangle_3_points.lci"}
    });
}

void LC_ActionFactory::createDrawLineActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group,{
        {"DrawLine",                 RS2::ActionDrawLine,                tr("&2 Points"),              ":/icons/line_2p.lci"},
        {"DrawLineAngle",            RS2::ActionDrawLineAngle,           tr("&Angle"),                 ":/icons/line_angle.lci"},
        {"DrawLineHorizontal",       RS2::ActionDrawLineHorizontal,      tr("&Horizontal"),            ":/icons/line_horizontal.lci"},
        {"DrawLineVertical",         RS2::ActionDrawLineVertical,        tr("Vertical"),               ":/icons/line_vertical.lci"},
        {"DrawLineFree",             RS2::ActionDrawLineFree,            tr("&Freehand Line"),         ":/icons/line_freehand.lci"},
        {"DrawLineParallel",         RS2::ActionDrawLineParallel,        tr("&Parallel"),              ":/icons/line_parallel.lci"},
        {"DrawLineParallelThrough",  RS2::ActionDrawLineParallelThrough, tr("Parallel through point"), ":/icons/line_parallel_p.lci"},
        {"DrawLineBisector",         RS2::ActionDrawLineBisector,        tr("Bisector"),               ":/icons/line_bisector.lci"},
        {"DrawLineTangent1",         RS2::ActionDrawLineTangent1,        tr("Tangent (P,C)"),          ":/icons/line_tangent_pc.lci"},
        {"DrawLineTangent2",         RS2::ActionDrawLineTangent2,        tr("Tangent (C,C)"),          ":/icons/line_tangent_cc.lci"},
        {"DrawLineOrthTan",          RS2::ActionDrawLineOrthTan,         tr("Tangent &Orthogonal"),    ":/icons/line_tangent_perpendicular.lci"},
        {"DrawLineOrthogonal",       RS2::ActionDrawLineOrthogonal,      tr("Orthogonal"),             ":/icons/line_perpendicular.lci"},
        {"DrawLineRelAngle",         RS2::ActionDrawLineRelAngle,        tr("Relative angle"),         ":/icons/line_relative_angle.lci"},
        {"DrawLineRel",              RS2::ActionDrawSnakeLine,           tr("Snake"),                  ":/icons/line_rel.lci"},
        {"DrawLineRelX",             RS2::ActionDrawSnakeLineX,          tr("Snake (X)"),              ":/icons/line_rel_x.lci"},
        {"DrawLineRelY",             RS2::ActionDrawSnakeLineY,          tr("Snake (Y)"),              ":/icons/line_rel_y.lci"},
        {"DrawLineAngleRel",         RS2::ActionDrawLineAngleRel,        tr("Angle From Line"),        ":/icons/line_angle_rel.lci"},
        {"DrawLineOrthogonalRel",    RS2::ActionDrawLineOrthogonalRel,   tr("Orthogonal From Line"),   ":/icons/line_ortho_rel.lci"},
        {"DrawLineFromPointToLine",  RS2::ActionDrawLineFromPointToLine, tr("From Point To Line"),     ":/icons/line_to_ortho.lci"},
        {"DrawLineMiddle",           RS2::ActionDrawLineMiddle,          tr("Centerline"),            ":/icons/line_middle.lci"},
        {"DrawCross",                RS2::ActionDrawCross,               tr("Center Mark"),              ":/icons/cross_circle1.lci"},
        {"DrawSliceDivideLine",      RS2::ActionDrawSliceDivideLine,     tr("Slice/Divide Line"),      ":/icons/slice_divide.lci"},
        {"DrawSliceDivideCircle",    RS2::ActionDrawSliceDivideCircle,   tr("Slice/Divide Circle"),    ":/icons/slice_divide_circle.lci"}
    });
}

void LC_ActionFactory::createDrawPointsActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group,{
        {"DrawPoint",                RS2::ActionDrawPoint,               tr("&Points"),                ":/icons/points.lci"},
        {"DrawLinePoints",           RS2::ActionDrawLinePoints,          tr("Line of Points"),         ":/icons/line_points.lci"},
        {"DrawPointsMiddle",         RS2::ActionDrawPointsMiddle,        tr("Middle Points"),          ":/icons/points_middle.lci"},
        {"DrawPointLattice",         RS2::ActionDrawPointsLattice,       tr("Lattice of Points"),      ":/icons/points_lattice.lci"},
        {"SelectPoints",             RS2::ActionSelectPoints,            tr("Select Points"),          ":/icons/select_points.lci"},
        {"PasteToPoints",            RS2::ActionPasteToPoints,           tr("Paste to Points"),        ":/icons/paste_to_points.lci"}
    });
}

void LC_ActionFactory::createSelectActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group,{
        {"SelectSingle",        RS2::ActionSelectSingle,        tr("Select Entity"),                 ":/icons/select_entity.lci"},
        {"SelectWindow",        RS2::ActionSelectWindow,        tr("Select Window"),                 ":/icons/select_window.lci"},
        {"DeselectWindow",      RS2::ActionDeselectWindow,      tr("Deselect Window"),               ":/icons/deselect_window.lci"},
        {"SelectContour",       RS2::ActionSelectContour,       tr("(De-)Select &Contour"),          ":/icons/deselect_contour.lci"},
        {"SelectIntersected",   RS2::ActionSelectIntersected,   tr("Select Intersected Entities"),   ":/icons/select_intersected_entities.lci"},
        {"DeselectIntersected", RS2::ActionDeselectIntersected, tr("Deselect Intersected Entities"), ":/icons/deselect_intersected_entities.lci"},
        {"SelectLayer",         RS2::ActionSelectLayer,         tr("(De-)Select Layer"),             ":/icons/deselect_layer.lci"}
    });
}

void LC_ActionFactory::createDrawCircleActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group,{
        {"DrawCircle",         RS2::ActionDrawCircle,         tr("Center, &Point"),                ":/icons/circle_center_point.lci"},
        {"DrawCircleByArc",    RS2::ActionDrawCircleByArc,    tr("By Arc"),                        ":/icons/circle_by_arc.lci"},
        {"DrawCircleCR",       RS2::ActionDrawCircleCR,       tr("Center, &Radius"),               ":/icons/circle_center_radius.lci"},
        {"DrawCircle2P",       RS2::ActionDrawCircle2P,       tr("2 Points"),                      ":/icons/circle_2_points.lci"},
        {"DrawCircle2PR",      RS2::ActionDrawCircle2PR,      tr("2 Points, Radius"),              ":/icons/circle_2_points_radius.lci"},
        {"DrawCircle3P",       RS2::ActionDrawCircle3P,       tr("3 Points"),                      ":/icons/circle_3_points.lci"},
        {"DrawCircleParallel", RS2::ActionDrawCircleParallel, tr("&Concentric"),                   ":/icons/circle_concentric.lci"},
        {"DrawCircleInscribe", RS2::ActionDrawCircleInscribe, tr("Circle &Inscribed"),             ":/icons/circle_inscribed.lci"},
        {"DrawCircleTan2",     RS2::ActionDrawCircleTan2,     tr("Tangential 2 Entities, Radius"),  ":/icons/circle_tangential_2circles_radius.lci"},
        {"DrawCircleTan2_1P",  RS2::ActionDrawCircleTan2_1P,  tr("Tangential 2 Entities, 1 Point"), ":/icons/circle_tangential_2circles_point.lci"},
        {"DrawCircleTan3",     RS2::ActionDrawCircleTan3,     tr("Tangential &3 Entities"),         ":/icons/circle_tangential_3entities.lci"},
        {"DrawCircleTan1_2P",  RS2::ActionDrawCircleTan1_2P,  tr("Tangential, 2 P&oints"),         ":/icons/circle_tangential_2points.lci"}
    });
}

void LC_ActionFactory::createDrawCurveActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group, {
        {"DrawArc",                RS2::ActionDrawArc,               tr("&Center, Point, Angles"),    ":/icons/arc_center_point_angle.lci"},
        {"DrawArcChord",           RS2::ActionDrawArcChord,          tr("&Center, Point, Chord"),     ":/icons/arc_center_point_chord.lci"},
        {"DrawArcAngleLen",        RS2::ActionDrawArcAngleLen,       tr("&Center, Point, Arc Angle"), ":/icons/arc_center_point_anglelen.lci"},
        {"DrawArc3P",              RS2::ActionDrawArc3P,             tr("&3 Points"),                 ":/icons/arc_3_points.lci"},
        {"DrawArc2PAngle",         RS2::ActionDrawArc2PAngle,        tr("&2 Points, Angle"),          ":/icons/arc_2p_angle.lci"},
        {"DrawArc2PRadius",        RS2::ActionDrawArc2PRadius,       tr("&2 Points, Radius"),         ":/icons/arc_2p_radius.lci"},
        {"DrawArc2PLength",        RS2::ActionDrawArc2PLength,       tr("&2 Points, Length"),         ":/icons/arc_2p_length.lci"},
        {"DrawArc2PHeight",        RS2::ActionDrawArc2PHeight,       tr("&2 Points, Height"),         ":/icons/arc_2p_height.lci"},
        {"DrawArcParallel",        RS2::ActionDrawArcParallel,       tr("&Concentric"),               ":/icons/arc_concentric.lci"},     // fixme - why this action is not in list?
        {"DrawArcTangential",      RS2::ActionDrawArcTangential,     tr("Arc &Tangential"),           ":/icons/arc_continuation.lci"}
    });
}

void LC_ActionFactory::createDrawSplineActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group, {
        {"DrawParabola4Points",    RS2::ActionDrawParabola4Points,   tr("Para&bola 4 points"),        ":/icons/parabola_4_points.lci"},
        {"DrawParabolaFD",         RS2::ActionDrawParabolaFD,        tr("Parabola &Focus Directrix"), ":/icons/parabola_focus_directrix.lci"},
        {"DrawHyperbolaFP",        RS2::ActionDrawHyperbolaFP,       tr("&Hyperbola &Foci Point"),    ":/icons/hyperbola_foci_point.lci"},
        {"DrawSpline",             RS2::ActionDrawSpline,            tr("&Spline"),                   ":/icons/spline.lci"},
        {"DrawSplinePoints",       RS2::ActionDrawSplinePoints,      tr("&Spline through points"),    ":/icons/spline_points.lci"},
        {"DrawSplinePointsAppend", RS2::ActionDrawSplinePointAppend, tr("&Append spline point"),      ":/icons/spline_points_add.lci"},
        {"DrawSplinePointsRemove", RS2::ActionDrawSplinePointRemove, tr("&Remove spline points"),     ":/icons/spline_points_remove.lci"},
        {"DrawSplinePointsAdd",    RS2::ActionDrawSplinePointAdd,    tr("&Insert spline points"),     ":/icons/spline_points_insert.lci"},
        {"DrawSplineExplode",      RS2::ActionDrawSplineExplode,     tr("&Explode spline to lines"),  ":/icons/spline_explode.lci"},
        {"DrawSplineFromPolyline", RS2::ActionDrawSplineFromPolyline,tr("&Spline from polyline"),     ":/icons/spline_from_polyline.lci"},
        {"DrawSplinePointsDelTwo", RS2::ActionDrawSplinePointDelTwo, tr("&Remove between two points"),":/icons/spline_points_remove_two.lci"}
    });
}

void LC_ActionFactory::createDrawEllipseActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"DrawEllipse1Point",        RS2::ActionDrawEllipse1Point,        tr("&Ellipse (1 Point)"),           ":/icons/ellipse_1_point.lci"},
        {"DrawEllipseArc1Point",     RS2::ActionDrawEllipseArc1Point,     tr("&Ellipse Arc (1 Point)"),       ":/icons/ellipse_arc_1_point.lci"},
        {"DrawEllipseAxis",          RS2::ActionDrawEllipseAxis,          tr("&Ellipse (Axis)"),              ":/icons/ellipse_axis.lci"},
        {"DrawEllipseArcAxis",       RS2::ActionDrawEllipseArcAxis,       tr("Ellipse &Arc (Axis)"),          ":/icons/ellipse_arc_axis.lci"},
        {"DrawEllipseFociPoint",     RS2::ActionDrawEllipseFociPoint,     tr("Ellipse &Foci Point"),          ":/icons/ellipse_foci_point.lci"},
        {"DrawEllipse4Points",       RS2::ActionDrawEllipse4Points,       tr("Ellipse &4 Point"),             ":/icons/ellipse_4_points.lci"},
        {"DrawEllipseCenter3Points", RS2::ActionDrawEllipseCenter3Points, tr("Ellipse Center and &3 Points"), ":/icons/ellipse_center_3_points.lci"},
        {"DrawEllipseInscribe",      RS2::ActionDrawEllipseInscribe,      tr("Ellipse &Inscribed"),           ":/icons/ellipse_inscribed.lci"}
    });
}

void LC_ActionFactory::createDrawPolylineActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group,{
        {"DrawPolyline",        RS2::ActionDrawPolyline,        tr("&Polyline"),                               ":/icons/polylines_polyline.lci"},
        {"PolylineAdd",         RS2::ActionPolylineAdd,         tr("&Add node"),                               ":/icons/insert_node.lci"},
        {"PolylineAppend",      RS2::ActionPolylineAppend,      tr("A&ppend node"),                            ":/icons/append_node.lci"},
        {"PolylineDel",         RS2::ActionPolylineDel,         tr("&Delete node"),                            ":/icons/delete_node.lci"},
        {"PolylineDelBetween",  RS2::ActionPolylineDelBetween,  tr("Delete &between two nodes"),               ":/icons/delete_between_nodes.lci"},
        {"PolylineTrim",        RS2::ActionPolylineTrim,        tr("&Trim segments"),                          ":/icons/trim.lci"},
        {"PolylineEquidistant", RS2::ActionPolylineEquidistant, tr("Create &Equidistant Polylines"),           ":/icons/create_equidistant_polyline.lci"},
        {"PolylineSegment",     RS2::ActionPolylineSegment,     tr("Polyline from Existing &Segments"),        ":/icons/create_polyline_from_existing_segments.lci"},
        {"PolylineArcToLines",  RS2::ActionPolylineArcsToLines, tr("Polyline Arcs to Chords"),                 ":/icons/polyline_arc_to_lines.lci"},
        {"PolylineSegmentType", RS2::ActionPolylineChangeSegmentType,    tr("Polyline Change Segment Type"),            ":/icons/polyline_segment_type.lci"}
    });
}

void LC_ActionFactory::createDrawOtherActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"ZoomPan",         RS2::ActionZoomPan,         tr("Zoom &Panning"), ":/icons/zoom_pan.lci"},
        {"DrawMText",       RS2::ActionDrawMText,       tr("&MText"),        ":/icons/mtext.lci"},
        {"DrawText",        RS2::ActionDrawText,        tr("&Text"),         ":/icons/text.lci"},
        {"DrawHatch",       RS2::ActionDrawHatch,       tr("&Hatch"),        ":/icons/hatch.lci"},
        {"DrawImage",       RS2::ActionDrawImage,       tr("Insert &Image"), ":/icons/camera.lci"},
        {"DrawBoundingBox", RS2::ActionDrawBoundingBox, tr("Bounding &Box"), ":/icons/bounding_box.lci"}
    });
}

void LC_ActionFactory::createDrawDimensionsActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group,{
        {"DimAligned",        RS2::ActionDimAligned,   tr("&Aligned"),    ":/icons/dim_aligned.lci"},
        {"DimLinear",         RS2::ActionDimLinear,    tr("&Linear"),     ":/icons/dim_linear.lci"},
        {"DimLinearHor",      RS2::ActionDimLinearHor, tr("&Horizontal"), ":/icons/dim_horizontal.lci"},
        {"DimLinearVer",      RS2::ActionDimLinearVer, tr("&Vertical"),   ":/icons/dim_vertical.lci"},
        {"DimRadial",         RS2::ActionDimRadial,    tr("&Radial"),     ":/icons/dim_radial.lci"},
        {"DimDiametric",      RS2::ActionDimDiametric, tr("&Diametric"),  ":/icons/dim_diametric.lci"},
        {"DimAngular",        RS2::ActionDimAngular,   tr("&Angular"),    ":/icons/dim_angular.lci"},
        {"DimArc",            RS2::ActionDimArc,       tr("&Arc"),        ":/icons/dim_arc.lci"},
        {"DimLeader",         RS2::ActionDimLeader,    tr("&Leader"),     ":/icons/dim_leader.lci"},
        {"DimBaseline",       RS2::ActionDimBaseline,  tr("&Baseline"),   ":/icons/dim_baseline.lci"},
        {"DimContinue",       RS2::ActionDimContinue,  tr("&Continue"),   ":/icons/dim_continue.lci"},
        {"DimOrdinate",       RS2::ActionDimOrdinate,  tr("&Ordinate"),   ":/icons/dim_ordinate.lci"},
        {"DimOrdinateForBase",RS2::ActionDimOrdByOriginSelect,  tr("Select Ordinates by base"),  ":/icons/dim_ordinate_by_origin.lci"},
        {"DimOrdinateReBase", RS2::ActionDimOrdRebase, tr("Ordinates Re-base"),  ":/icons/dim_ordinate_rebase.lci"},
        {"GTDFeatureFrame",   RS2::ActionGTDFCFrame,   tr("Feature Control Frame"),  ":/icons/gdt_featurecontrolframe.lci"},
        {"DimPickApply",      RS2::ActionDimStyleApply,tr("Copy Style"),  ":/icons/dim_apply_style.lci"},
        {"DimModify",         RS2::ActionDimModify,    tr("Modify Style"),":/icons/dim_modify_style.lci"}
    });
}


void LC_ActionFactory::createModifyActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    /* action = new QAction(tr("Delete Freehand"), agm->modify);
     action->setIcon(QIcon(":/icons/delete_freehand.lci"));
     connect(action, SIGNAL(triggered()),
     action_handler, SLOT(slotModifyDeleteFree()));
     action->setObjectName("ModifyDeleteFree");
     a_map["ModifyDeleteFree"] = action;*/
    createActionHandlerActions(map, group,{
        {"ModifyAttributes",      RS2::ActionModifyAttributes,      tr("&Attributes"),                ":/icons/attributes.lci"},
        {"ModifyDelete",          RS2::ActionModifyDelete,          tr("&Delete"),                    ":/icons/delete.lci"},
        {"ModifyMove",            RS2::ActionModifyMove,            tr("&Move / Copy"),               ":/icons/move_copy.lci"},
        {"ModifyRevertDirection", RS2::ActionModifyRevertDirection, tr("Re&vert direction"),          ":/icons/revert_direction.lci"},
        {"ModifyRotate",          RS2::ActionModifyRotate,          tr("&Rotate"),                    ":/icons/rotate.lci"},
        {"ModifyScale",           RS2::ActionModifyScale,           tr("&Scale"),                     ":/icons/scale.lci"},
        {"ModifyMirror",          RS2::ActionModifyMirror,          tr("&Mirror"),                    ":/icons/mirror.lci"},
        {"ModifyMoveRotate",      RS2::ActionModifyMoveRotate,      tr("Mo&ve and Rotate"),           ":/icons/move_rotate.lci"},
        {"ModifyRotate2",         RS2::ActionModifyRotate2,         tr("Rotate T&wo"),                ":/icons/rotate2.lci"},
        {"ModifyEntity",          RS2::ActionModifyEntity,          tr("&Properties"),                ":/icons/properties.lci"},
        {"ModifyTrim",            RS2::ActionModifyTrim,            tr("&Trim"),                      ":/icons/trim.lci"},
        {"ModifyTrim2",           RS2::ActionModifyTrim2,           tr("Tr&im Two"),                  ":/icons/trim2.lci"},
        {"ModifyTrimAmount",      RS2::ActionModifyTrimAmount,      tr("&Lengthen"),                  ":/icons/trim_value.lci"},
        {"ModifyOffset",          RS2::ActionModifyOffset,          tr("O&ffset"),                    ":/icons/offset.lci"},
        {"ModifyCut",             RS2::ActionModifyCut,             tr("&Divide"),                    ":/icons/divide.lci"},
        {"ModifyStretch",         RS2::ActionModifyStretch,         tr("&Stretch"),                   ":/icons/stretch.lci"},
        {"ModifyBevel",           RS2::ActionModifyBevel,           tr("&Bevel"),                     ":/icons/bevel.lci"},
        {"ModifyRound",           RS2::ActionModifyRound,           tr("&Fillet"),                    ":/icons/fillet.lci"},
        {"ModifyExplodeText",     RS2::ActionModifyExplodeText,     tr("&Explode Text into Letters"), ":/icons/explode_text_to_letters.lci"},
        {"BlocksExplode",         RS2::ActionBlocksExplode,         tr("Ex&plode"),                   ":/icons/explode.lci"},
        {"ModifyBreakDivide",     RS2::ActionModifyBreakDivide,     tr("Break/Divide"),               ":/icons/break_out_trim.lci"},
        {"ModifyLineGap",         RS2::ActionModifyLineGap,         tr("Line Gap"),                   ":/icons/line_gap.lci"},
        {"ModifyLineJoin",        RS2::ActionModifyLineJoin,        tr("Line Join"),                  ":/icons/line_join.lci"},
        {"ModifyDuplicate",       RS2::ActionModifyDuplicate,       tr("Duplicate"),                  ":/icons/duplicate.lci"},
        {"ModifyAlign",           RS2::ActionModifyAlign,           tr("Align"),                      ":/icons/halign_left.lci"},
        {"ModifyAlignOne",        RS2::ActionModifyAlignOne,        tr("Align One"),                  ":/icons/align_one.lci"},
        {"ModifyAlignRef",        RS2::ActionModifyAlignRef,        tr("Align Reference Points"),     ":/icons/align_ref.lci"}
    });
}

void LC_ActionFactory::createPenActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"PenSyncFromLayer", RS2::ActionPenSyncFromLayer, tr("Update Current Pen by Active Layer' Pen"), ":/icons/back.lci"}
    });
}

void LC_ActionFactory::createPenActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"PenPick",         RS2::ActionPenPick,         tr("&Pick Pen From Entity"),            ":/icons/pen_pick_entity.lci"},
        {"PenPickResolved", RS2::ActionPenPickResolved, tr("&Pick Pen From Entity (Resolved)"), ":/icons/pen_pick_resolved.lci"},
        {"PenApply",        RS2::ActionPenApply,        tr("Apply Pen to Entity"),              ":/icons/pen_apply.lci"},
        {"PenCopy",         RS2::ActionPenCopy,         tr("Copy Pen"),                         ":/icons/pen_copy.lci"}
    });
}

void LC_ActionFactory::createInfoCursorActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"EntityDescriptionInfo", &QC_ApplicationWindow::slotShowEntityDescriptionOnHover, tr("Show Entity Description"), ":/icons/entity_description_info.lci"}
    });
    createActions(map, group, {
        {"InfoCursorEnable",        tr("Enable Info Cursor"), ":/icons/info_cursor_enable.lci"},
        {"InfoCursorAbs",           tr("Absolute Pos"),       ":/icons/info_cursor_zone1.lci"},
        {"InfoCursorSnap",          tr("Snap"),               ":/icons/info_cursor_zone2.lci"},
        {"InfoCursorRel",           tr("Relative"),           ":/icons/info_cursor_zone3.lci"},
        {"InfoCursorPrompt",        tr("Prompt"),             ":/icons/info_cursor_zone4.lci"},
        {"InfoCursorCatchedEntity", tr("Caught Entity"),      ":/icons/info_cursor_zone2_entity.lci"},
    });
}

void LC_ActionFactory::createSnapActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActions(map, group, {
        {"SnapGrid",         tr("Snap on grid"),       ":/icons/snap_grid.lci"},
        {"SnapMiddleManual", tr("Snap Middle Manual"), ":/icons/snap_middle_manual.lci"},
        {"SnapEnd",          tr("Snap on Endpoints"),  ":/icons/snap_endpoints.lci"},
        {"SnapEntity",       tr("Snap on Entity"),     ":/icons/snap_entity.lci"},
        {"SnapCenter",       tr("Snap Center"),        ":/icons/snap_center.lci"},
        {"SnapMiddle",       tr("Snap Middle"),        ":/icons/snap_middle.lci"},
        {"SnapDistance",     tr("Snap Distance"),      ":/icons/snap_distance.lci"},
        {"SnapIntersection", tr("Snap Intersection"),  ":/icons/snap_intersection.lci"},
    });
}

void LC_ActionFactory::createRestrictActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActions(map, group, {
        {"RestrictHorizontal", tr("Restrict Horizontal"), ":/icons/restr_hor.lci"},
        {"RestrictVertical",   tr("Restrict Vertical"),   ":/icons/restr_ver.lci"},
        {"RestrictOrthogonal", tr("Restrict Orthogonal"), ":/icons/restr_ortho.lci"},
        {"RestrictNothing",    tr("Restrict Nothing"),    ":/icons/restrict_nothing.lci"}
    });
}

void LC_ActionFactory::createUCSActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActions(map, group, {
        {"UCSSetWCS",   RS2::ActionUCSCreate,   tr("To WCS"),  ":/icons/ucs_set_wcs.lci"}
        // todo - add action for hiding/showing related zero
    });

    createActionHandlerActions(map, group, {
        {"UCSCreate",   RS2::ActionUCSCreate,   tr("Create UCS"),  ":/icons/ucs_add.lci"},
        {"UCSSetByDimOrdinate",   RS2::ActionUCSSetByDimOrdinate,   tr("Set UCS By Ordinate Dimension"),  ":/icons/ucs_set_by_dimordinate.lci"}
    });
}

void LC_ActionFactory::createWorkspacesActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group){
    createMainWindowActions(map, group, {
        {"WorkspaceCreate",  &QC_ApplicationWindow::saveWorkspace,    tr("Save Workspace"),    ":/icons/workspace_save.lci"},
        {"WorkspaceRemove",  &QC_ApplicationWindow::removeWorkspace,  tr("Remove Workspace"),  ":/icons/workspace_remove.lci"},
        {"WorkspaceRestore", &QC_ApplicationWindow::restoreWorkspace, tr("Restore Workspace"), ":/icons/workspace.lci"}
    });
}

void LC_ActionFactory::createRelZeroActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"SetRelativeZero",    RS2::ActionSetRelativeZero, tr("Set relative zero position"),  ":/icons/set_rel_zero.lci"},
    });
    createActions(map, group, {
        {"LockRelativeZero",   tr("Lock relative zero position"), ":/icons/lock_rel_zero.lci"}
        // todo - add action for hiding/showing related zero
       //{"RestrictOrthogonal", tr("Restrict Orthogonal"),         ":/icons/restr_ortho.lci"}
    });
}

void LC_ActionFactory::createSnapExtraActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActions(map, group, {
        {"ExclusiveSnapMode", tr("Exclusive Snap Mode"), ":/icons/exclusive.lci"},
        {"SnapFree",          tr("Free Snap"),           ":/icons/snap_free.lci"}
    });
}

void LC_ActionFactory::createOrderActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"OrderBottom", RS2::ActionOrderBottom, tr("Move to Bottom"), ":/icons/downmost.lci"},
        {"OrderLower",  RS2::ActionOrderLower, tr("Lower After Entity"), ":/icons/down.lci"},
        {"OrderRaise",  RS2::ActionOrderRaise, tr("Raise Over Entity"), ":/icons/up.lci"},
        {"OrderTop",    RS2::ActionOrderTop, tr("Move to Top"), ":/icons/upmost.lci"}
    });
}

void LC_ActionFactory::createInfoActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        // {"InfoInside",       RS2::ActionInfoInside,           tr("Point inside contour"),               ""},
        {"InfoPoint",        RS2::ActionInfoPoint,            tr("&Point Coordinates"),                 ":/icons/info_point.lci"},
        {"InfoDist",         RS2::ActionInfoDistPoint2Point,  tr("&Distance Point to Point"),           ":/icons/distance_point_to_point.lci"},
        {"InfoDist2",        RS2::ActionInfoDistEntity2Point, tr("Distance &Entity to Point"),          ":/icons/distance_entity_to_point.lci"},
        {"InfoDist3",        RS2::ActionInfoDistPoint2Entity, tr("Distance &Point to Entity"),          ":/icons/distance_point_to_entity.lci"},
        {"InfoAngle",        RS2::ActionInfoAngle,            tr("An&gle between two lines"),           ":/icons/angle_line_to_line.lci"},
        {"InfoTotalLength",  RS2::ActionInfoTotalLength,      tr("Total &length of selected entities"), ":/icons/total_length_selected_entities.lci"},
        {"InfoArea",         RS2::ActionInfoArea,             tr("Polygonal &Area"),                    ":/icons/polygonal_area.lci"},
        {"EntityInfo",       RS2::ActionInfoProperties,       tr("Entity Pro&perties"),                 ":/icons/entity_properties_select.lci"},
        {"PickCoordinates",  RS2::ActionInfoPickCoordinates,  tr("Collect &Coordinates"),               ":/icons/pick_coordinates.lci"},
        {"InfoAngle3Points", RS2::ActionInfoAngle3Points,     tr("Ang&le between 3 points"),            ":/icons/angle_3_points.lci"}
    });
}

void LC_ActionFactory::createViewActions(QMap<QString, QAction*>& map, QActionGroup* group) {
    createActionHandlerActions(map, group, {
        {"ZoomWindow",RS2::ActionZoomWindow, tr("&Window Zoom"), ":/icons/zoom_window.lci","zoom-select"}});

    createMainWindowActions(map, group, {
        {"Fullscreen",       &QC_ApplicationWindow::toggleFullscreen,     tr("&Fullscreen"),           ":/icons/fullscreen.lci"},
        {"ViewGrid",         &QC_ApplicationWindow::slotViewGrid,         tr("&Grid"),                 ":/icons/grid.lci"},
        {"ViewDraft",        &QC_ApplicationWindow::slotViewDraft,        tr("&Draft"),                ":/icons/draft.lci"},
        {"ViewLinesDraft",   &QC_ApplicationWindow::slotViewDraftLines,   tr("&Draft Lines"),          ":/icons/draftLineWidth.lci"},
        {"ViewAntialiasing", &QC_ApplicationWindow::slotViewAntialiasing, tr("&Antialiasing"),         ":/icons/anti_aliasing.lci"},
        {"ViewStatusBar",    &QC_ApplicationWindow::slotViewStatusBar,    tr("&Statusbar")},
        {"ViewGridOrtho",    &QC_ApplicationWindow::slotViewGridOrtho,    tr("&Orthogonal Grid"),      ":/icons/grid_ortho.lci"},
        {"ViewGridIsoLeft",  &QC_ApplicationWindow::slotViewGridIsoLeft,  tr("&Isometric Left Grid"),  ":/icons/grid_iso_left.lci"},
        {"ViewGridIsoTop",   &QC_ApplicationWindow::slotViewGridIsoTop,   tr("&Isometric Top Grid"),   ":/icons/grid_iso_top.lci"},
        {"ViewGridIsoRight", &QC_ApplicationWindow::slotViewGridIsoRight, tr("&Isometric Right Grid"), ":/icons/grid_iso_right.lci"},
    }, true);
}

void LC_ActionFactory::createLayerActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"LayersDefreezeAll",        RS2::ActionLayersDefreezeAll,        tr("&Show all layers"),           ":/icons/visible_all.lci"},
        {"LayersFreezeAll",          RS2::ActionLayersFreezeAll,          tr("&Hide all layers"),           ":/icons/not_visible_all.lci"},
        {"LayersUnlockAll",          RS2::ActionLayersUnlockAll,          tr("&Unlock all"),                ":/icons/unlocked.lci"},
        {"LayersLockAll",            RS2::ActionLayersLockAll,            tr("&Lock all"),                  ":/icons/locked.lci"},
        {"LayersAdd",                RS2::ActionLayersAdd,                tr("&Add Layer"),                 ":/icons/add.lci"},
        {"LayersRemove",             RS2::ActionLayersRemove,             tr("&Remove Layer"),              ":/icons/remove.lci"},
        {"LayersEdit",               RS2::ActionLayersEdit,               tr("&Edit Layer"),                ":/icons/attributes.lci"},
        {"LayersToggleLock",         RS2::ActionLayersToggleLock,         tr("Toggle Layer Loc&k"),         ":/icons/locked.lci"},
        {"LayersToggleView",         RS2::ActionLayersToggleView,         tr("&Toggle Layer Visibility"),   ":/icons/visible.lci"},
        {"LayersTogglePrint",        RS2::ActionLayersTogglePrint,        tr("Toggle Layer &Print"),        ":/icons/print.lci"},
        {"LayersToggleConstruction", RS2::ActionLayersToggleConstruction, tr("Toggle &Construction Layer"), ":/icons/construction_layer.lci"},
        {"LayersExportSelected",     RS2::ActionLayersExportSelected,     tr("&Export Selected Layer(s)"),  ":/icons/layer_export_selected.lci"},
        {"LayersExportVisible",      RS2::ActionLayersExportVisible,      tr("Export &Visible Layer(s)"),   ":/icons/layer_export_visible.lci"}
    });
}

void LC_ActionFactory::createBlockActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"BlocksDefreezeAll", RS2::ActionBlocksDefreezeAll, tr("&Show all blocks"),         ":/icons/visible_all.lci"},
        {"BlocksFreezeAll",   RS2::ActionBlocksFreezeAll,   tr("&Hide all blocks"),         ":/icons/not_visible_all.lci"},
        {"BlocksAdd",         RS2::ActionBlocksAdd,         tr("&Add Block"),               ":/icons/add.lci"},
        {"BlocksRemove",      RS2::ActionBlocksRemove,      tr("&Remove Block"),            ":/icons/remove.lci"},
        {"BlocksAttributes",  RS2::ActionBlocksAttributes,  tr("&Rename Block"),            ":/icons/rename_active_block.lci"},
        {"BlocksEdit",        RS2::ActionBlocksEdit,        tr("&Edit Block"),              ":/icons/properties.lci"},
        {"BlocksSave",        RS2::ActionBlocksSave,        tr("&Save Block"),              ":/icons/save.lci"},
        {"BlocksInsert",      RS2::ActionBlocksInsert,      tr("&Insert Block"),            ":/icons/insert_active_block.lci"},
        {"BlocksToggleView",  RS2::ActionBlocksToggleView,  tr("Toggle Block &Visibility"), ":/icons/visible.lci"},
        {"BlocksCreate",      RS2::ActionBlocksCreate,      tr("&Create Block"),            ":/icons/create_block.lci"}
    });
}

void LC_ActionFactory::createOptionsActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group){
    createActionHandlerActions(map, group, {
        {"OptionsDrawing",RS2::ActionOptionsDrawing, tr("Current &Drawing Preferences"), ":/icons/drawing_settings.lci"}
    });

    createMainWindowActions(map, group, {
        {"OptionsGeneral", &QC_ApplicationWindow::slotOptionsGeneral, tr("&Application Preferences"), ":/icons/settings.lci"},
        {"WidgetOptions", &QC_ApplicationWindow::widgetOptionsDialog, tr("Widget Options")},
        {"ShortcutsOptions", &QC_ApplicationWindow::slotOptionsShortcuts, tr("Keyboard Shortcuts"), ":/icons/shortcuts_settings.lci"},
        {"DeviceOptions", &QC_ApplicationWindow::showDeviceOptions, tr("Device Options")},
        {"ReloadStyleSheet", &QC_ApplicationWindow::reloadStyleSheet, tr("Reload Style Sheet")}
    });
}

void LC_ActionFactory::createFileActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"FilePrintPreview",&QC_ApplicationWindow::slotFilePrintPreview,  tr("Print Pre&view"), ":/icons/print_preview.lci", "document-print-preview"},
    });
}

void LC_ActionFactory::createFileActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group){
    createActions(map, group, {
        {"FileClose", tr("&Close"), ":/icons/close.lci"},
    });

    createMainWindowActions(map, group, {
        {"FileExport",      &QC_ApplicationWindow::slotFileExport,      tr("&Export as image"),   ":/icons/export.lci"},
        {"FileCloseAll",    &QC_ApplicationWindow::slotFileCloseAll,    tr("Close All"),          ":/icons/close_all.lci"},
        {"FilePrintPDF",    &QC_ApplicationWindow::slotFilePrintPDF,    tr("Export as PDF"),      ":/icons/export_pdf.lci"},
        {"BlocksImport",    &QC_ApplicationWindow::slotImportBlock,     tr("&Block"),             ":/icons/insert_active_block.lci"},
        {"FileNew",         &QC_ApplicationWindow::slotFileNewFromDefaultTemplate,         tr("&New"),               ":/icons/new.lci",               "document-new"},
        {"FileNewTemplate", &QC_ApplicationWindow::slotFileNewFromTemplate, tr("New From &Template"), ":/icons/new_from_template.lci", "document-new"},// fixme - check
        {"FileOpen",        &QC_ApplicationWindow::slotFileOpen,        tr("&Open..."),           ":/icons/open.lci",              "document-open"},
        {"FileSave",        &QC_ApplicationWindow::slotFileSave,        tr("&Save"),              ":/icons/save.lci",              "document-save"},
        {"FileSaveAs",      &QC_ApplicationWindow::slotFileSaveAs,      tr("Save &as..."),        ":/icons/save_as.lci",           "document-save-as"},
        {"FileSaveAll",     &QC_ApplicationWindow::slotFileSaveAll,     tr("Save A&ll..."),       ":/icons/save_all.lci"},
        {"FilePrint",       &QC_ApplicationWindow::slotFilePrint,       tr("&Print..."),          ":/icons/print.lci",             "document-print"},
        {"FileQuit",        &QC_ApplicationWindow::slotFileQuit,        tr("&Quit"),              ":/icons/quit.lci",              "application-exit"},
    });

    createAction_AH("FileExportMakerCam",RS2::ActionFileExportMakerCam,  tr("Export as CA&M/plain SVG..."), nullptr, nullptr, group, map);
}

void LC_ActionFactory::createWidgetActions(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"LeftDockAreaToggle",        &QC_ApplicationWindow::toggleLeftDockArea,        tr("Left"),     ":/icons/dockwidgets_left.lci"},
        {"RightDockAreaToggle",       &QC_ApplicationWindow::toggleRightDockArea,       tr("Right"),    ":/icons/dockwidgets_right.lci"},
        {"TopDockAreaToggle",         &QC_ApplicationWindow::toggleTopDockArea,         tr("Top"),      ":/icons/dockwidgets_top.lci"},
        {"BottomDockAreaToggle",      &QC_ApplicationWindow::toggleBottomDockArea,      tr("Bottom"),   ":/icons/dockwidgets_bottom.lci"},
        {"FloatingDockwidgetsToggle", &QC_ApplicationWindow::toggleFloatingDockwidgets, tr("Floating"), ":/icons/dockwidgets_floating.lci"}
    }, true);
}

void LC_ActionFactory::createWidgetActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"RedockWidgets",        &QC_ApplicationWindow::slotRedockWidgets,    tr("Re-dock Widgets")},
        {"InvokeMenuCreator",    &QC_ApplicationWindow::invokeMenuCreator,    tr("Custom Menu Creator"),    ":/icons/create_menu.lci"},
        {"InvokeToolbarCreator", &QC_ApplicationWindow::invokeToolbarCreator, tr("Custom Toolbar Creator"), ":/icons/create_toolbar.lci"}
    });
}

void LC_ActionFactory::createViewActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"FocusCommand", &QC_ApplicationWindow::slotFocusCommandLine,   tr("Focus on &Command Line"),   ":/icons/editclear.lci"},
        {"FocusOptions", &QC_ApplicationWindow::slotFocusOptionsWidget, tr("Focus on &Options Widget"), ":/icons/drawing_settings.lci"}
    });

    createActionHandlerActions(map, group, {
        {"ZoomIn",       RS2::ActionZoomIn,       tr("Zoom &In"),       ":/icons/zoom_in.lci",       "zoom-in"},
        {"ZoomOut",      RS2::ActionZoomOut,      tr("Zoom &Out"),      ":/icons/zoom_out.lci",      "zoom-out"},
        {"ZoomAuto",     RS2::ActionZoomAuto,     tr("&Auto Zoom"),     ":/icons/zoom_auto.lci",     "zoom-fit-best"},
        {"ZoomPrevious", RS2::ActionZoomPrevious, tr("Previous &View"), ":/icons/zoom_previous.lci", "zoom-previous"},
        {"ZoomRedraw",   RS2::ActionZoomRedraw,   tr("&Redraw"),        ":/icons/redraw.lci",        "view-refresh"}
    });
}

void LC_ActionFactory::createNamedViewActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) {
    createMainWindowActions(map, group, {
        {"ZoomViewSave",     &QC_ApplicationWindow::saveNamedView,           tr("&Save View"),           ":/icons/nview_add.lci"},
        // fixme - quite an ugly approach, think about direct invocation of action for views list?
        {"ZoomViewRestore",  &QC_ApplicationWindow::restoreNamedViewCurrent, tr("Restore Current View"), ":/icons/nview_visible.lci"},
        {"ZoomViewRestore1", &QC_ApplicationWindow::restoreNamedView1,       tr("Restore View 1"),       ":/icons/nview_visible.lci"},
        {"ZoomViewRestore2", &QC_ApplicationWindow::restoreNamedView2,       tr("Restore View 2"),       ":/icons/nview_visible.lci"},
        {"ZoomViewRestore3", &QC_ApplicationWindow::restoreNamedView3,       tr("Restore View 3"),       ":/icons/nview_visible.lci"},
        {"ZoomViewRestore4", &QC_ApplicationWindow::restoreNamedView4,       tr("Restore View 4"),       ":/icons/nview_visible.lci"},
        {"ZoomViewRestore5", &QC_ApplicationWindow::restoreNamedView5,       tr("Restore View 5"),       ":/icons/nview_visible.lci"},
    });
}

void LC_ActionFactory::createSelectActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"SelectAll",    RS2::ActionSelectAll,    tr("Select &All"),      ":/icons/select_all.lci"},
        {"DeselectAll",  RS2::ActionDeselectAll,  tr("Deselect &all"),    ":/icons/deselect_all.lci"},
        {"SelectInvert", RS2::ActionSelectInvert, tr("Invert Selection"), ":/icons/select_inverted.lci"}
    });
}

void LC_ActionFactory::createEditActionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"EditUndo",           RS2::ActionEditUndo,           tr("&Undo"),              ":/icons/undo.lci",            "edit-undo"},
        {"EditRedo",           RS2::ActionEditRedo,           tr("&Redo"),              ":/icons/redo.lci",            "edit-redo"},
        {"ModifyDeleteQuick",  RS2::ActionModifyDeleteQuick,  tr("&Delete Selected"),   ":/icons/delete.lci"},
        {"EditKillAllActions", RS2::ActionEditKillAllActions, tr("&Selection Pointer"), ":/icons/cursor.lci",          "go-previous-view"}
    });
}
void LC_ActionFactory::createDrawDimensionsUncheckable(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
       {"DimRegenerate",     RS2::ActionDimRegenerate,tr("Regenerate Dimensions"),  ":/icons/dim_regenerate.lci"}
    });

    auto dimSettingsAction = justCreateAction(map,"DimStyles",tr( "&Dimension Styles"),
       ":/icons/dim_style_manager.lci", "", group);

    connect(dimSettingsAction, &QAction::triggered, this, [this](bool){
        m_appWin->changeDrawingOptions(3);
    });
}

void LC_ActionFactory::createInteractivePickActions(QMap<QString, QAction *> &map, QActionGroup *group) const {
    createActionHandlerActions(map, group, {
        {"PickPoint",    RS2::ActionInteractivePickPoint,  tr("Pick Point"),   ":/icons/interactive_pick_point.lci"},
        {"PickDistance", RS2::ActionInteractivePickLength, tr("Pick Distance"),":/icons/interactive_pick_distance.lci"},
        {"PickAngle",    RS2::ActionInteractivePickAngle,  tr("Pick Angle"),   ":/icons/interactive_pick_angle.lci"}
    });
}

void LC_ActionFactory::createEditActions(QMap<QString, QAction*>& map, QActionGroup* group) const {
    createActionHandlerActions(map, group, {
        {"EditCut",            RS2::ActionEditCut,            tr("Cu&t"),               ":/icons/cut.lci",             "edit-cut"},
        {"EditCutQuick",       RS2::ActionEditCutQuick,       tr("Cut Quic&k"),         ":/icons/cut.lci",             "edit-cut"},
        {"EditCopy",           RS2::ActionEditCopy,           tr("&Copy"),              ":/icons/copy.lci",            "edit-copy"},
        {"EditCopyQuick",      RS2::ActionEditCopyQuick,      tr("Copy &Quick"),        ":/icons/copy.lci",            "edit-copy"},
        {"EditPaste",          RS2::ActionEditPaste,          tr("&Paste"),             ":/icons/paste.lci",           "edit-paste"},
        {"EditPasteTransform", RS2::ActionEditPasteTransform, tr("Paste &Transform"),   ":/icons/paste_transform.lci", "edit-paste"}
    });
}

void LC_ActionFactory::setupCreatedActions(QMap<QString, QAction *> &map) {
    map["ZoomPrevious"]->setEnabled(false);
    map["RightDockAreaToggle"]->setChecked(true);
    bool statusBarVisible = LC_GET_ONE_BOOL("Appearance", "StatusBarVisible", false);
    map["ViewStatusBar"]->setChecked(statusBarVisible);
    map["OptionsGeneral"]->setMenuRole(QAction::NoRole);

    connect(m_appWin, &QC_ApplicationWindow::printPreviewChanged, map["FilePrint"], &QAction::setChecked);
    connect(m_appWin, &QC_ApplicationWindow::printPreviewChanged, map["FilePrintPreview"], &QAction::setChecked);
    connect(m_appWin, &QC_ApplicationWindow::gridChanged, map["ViewGrid"], &QAction::setChecked);
    connect(m_appWin, &QC_ApplicationWindow::draftChanged, map["ViewDraft"], &QAction::setChecked);
    connect(m_appWin, &QC_ApplicationWindow::draftChanged, map["ViewLinesDraft"], &QAction::setDisabled);
    connect(m_appWin, &QC_ApplicationWindow::antialiasingChanged, map["ViewAntialiasing"], &QAction::setChecked);
    connect(m_appWin, &QC_ApplicationWindow::windowsChanged, map["OptionsDrawing"], &QAction::setEnabled);

    QAction *&entityInfoAction = map["EntityDescriptionInfo"];
    connect(m_appWin, &QC_ApplicationWindow::showEntityDescriptionOnHoverChanged, entityInfoAction, &QAction::setChecked);

    auto infoCursorSettingsManager = m_appWin->m_infoCursorSettingsManager.get();
    connect(infoCursorSettingsManager, &LC_InfoCursorSettingsManager::showInfoCursorSettingChanged, entityInfoAction, &QAction::setVisible);

    connect(map["InfoCursorAbs"], &QAction::triggered, infoCursorSettingsManager, &LC_InfoCursorSettingsManager::slotInfoCursorSetting);
    connect(infoCursorSettingsManager, &LC_InfoCursorSettingsManager::showInfoCursorSettingChanged, map["InfoCursorAbs"], &QAction::setEnabled);

    connect(map["InfoCursorSnap"], &QAction::triggered, infoCursorSettingsManager, &LC_InfoCursorSettingsManager::slotInfoCursorSetting);
    connect(infoCursorSettingsManager, &LC_InfoCursorSettingsManager::showInfoCursorSettingChanged, map["InfoCursorSnap"], &QAction::setEnabled);

    connect(map["InfoCursorRel"], &QAction::triggered,infoCursorSettingsManager, &LC_InfoCursorSettingsManager::slotInfoCursorSetting);
    connect(infoCursorSettingsManager, &LC_InfoCursorSettingsManager::showInfoCursorSettingChanged, map["InfoCursorRel"], &QAction::setEnabled);

    connect(map["InfoCursorPrompt"], &QAction::triggered, infoCursorSettingsManager, &LC_InfoCursorSettingsManager::slotInfoCursorSetting);
    connect(infoCursorSettingsManager, &LC_InfoCursorSettingsManager::showInfoCursorSettingChanged, map["InfoCursorPrompt"], &QAction::setEnabled);

    connect(map["InfoCursorCatchedEntity"], &QAction::triggered, infoCursorSettingsManager, &LC_InfoCursorSettingsManager::slotInfoCursorSetting);
    connect(infoCursorSettingsManager, &LC_InfoCursorSettingsManager::showInfoCursorSettingChanged, map["InfoCursorCatchedEntity"], &QAction::setEnabled);

    // with this setting, the action's icon will not be set as current item in action's options bar and status bar (in QC_ApplicationWindow::relayAction())
    map["DimStyles"]->setProperty("_SetAsCurrentActionInView", false);
    map["LockRelativeZero"]->setProperty("_SetAsCurrentActionInView", false);
}

void LC_ActionFactory::setDefaultShortcuts(QMap<QString, QAction*>& map, LC_ActionGroupManager* agm) {
    QList<QKeySequence> commandLineShortcuts;
    commandLineShortcuts << QKeySequence(Qt::CTRL | Qt::Key_M) << QKeySequence(Qt::Key_Colon);
    if (LC_GET_BOOL("Keyboard/ToggleFreeSnapOnSpace"))
        commandLineShortcuts << QKeySequence(Qt::Key_Space);

    std::vector<LC_ShortcutInfo> shortcutsList = {
        {"ModifyRevertDirection", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R)},
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

void LC_ActionFactory::markNotEditableActionsShortcuts(const QMap<QString, QAction *> &map) {
    // placeholder for exclusion of some actions (by name) from editing in shortcuts mapping dialog
    makeActionsShortcutsNonEditable(map, {
        "RestrictNothing",
        "PickPoint",
        "PickDistance",
        "PickAngle"
    });
}

void LC_ActionFactory::fillActionLists(QMap<QString, QAction *> &map){
    fillActionsList(file_actions,
                    {
                        "FileNew",
                        "FileNewTemplate",
                        "FileOpen",
                        "FileSave",
                        "FileSaveAs",
                        "FileSaveAll"
                    }, map);

    fillActionsList(shape_actions,
                    {
                        "DrawLineRectangle",
                        "DrawLineRectangle1Point",
                        "DrawLineRectangle2Points",
                        "DrawLineRectangle3Points",
                        "DrawLinePolygonCenCor",
                        "DrawLinePolygonCenTan",
                        "DrawLinePolygonCorCor",
                        "DrawLinePolygonSideSide",
                        "DrawStar"
                    }, map);

    fillActionsList(line_actions,
                    {
                        "DrawLine",
                        "DrawLineAngle",
                        "DrawLineHorizontal",
                        "DrawLineVertical",
                        "DrawLineParallelThrough",
                        "DrawLineParallel",
                        "DrawLineBisector",
                        "DrawLineTangent1",
                        "DrawLineTangent2",
                        "DrawLineOrthTan",
                        "DrawLineOrthogonal",
                        "DrawLineRelAngle",
                        "DrawLineRel",
                        "DrawLineRelX",
                        "DrawLineRelY",
                        "DrawLineAngleRel",
                        "DrawLineOrthogonalRel",
                        "DrawLineFromPointToLine",
                        "DrawSliceDivideLine",
                        "DrawSliceDivideCircle",
                        "DrawCross",
                        "DrawLineMiddle"
                    }, map);

    fillActionsList(point_actions, {
                        "DrawPoint",
                        "DrawLinePoints",
                        "DrawPointsMiddle",
                        "DrawPointLattice",
                        "SelectPoints",
                        "PasteToPoints"
                    }, map);

    fillActionsList(circle_actions, {
                        "DrawCircle",
                        "DrawCircle2P",
                        "DrawCircle2PR",
                        "DrawCircle3P",
                        "DrawCircleCR",
                        "DrawCircleTan2_1P",
                        "DrawCircleTan1_2P",
                        "DrawCircleTan2",
                        "DrawCircleTan3",
                        "DrawCircleInscribe",
                        "DrawCircleParallel",
                        "DrawCircleByArc"
                    }, map);

    fillActionsList(curve_actions, {
                        "DrawArc",
                        "DrawArcChord",
                        "DrawArcAngleLen",
                        "DrawArc3P",
                        "DrawArc2PAngle",
                        "DrawArc2PRadius",
                        "DrawArc2PLength",
                        "DrawArc2PHeight",
                        "DrawArcTangential",
                        "DrawEllipseArcAxis",
                        "DrawEllipseArc1Point"
                    }, map);

    fillActionsList(spline_actions, {
                        "DrawParabola4Points",
                        "DrawParabolaFD",
                        "DrawHyperbolaFP",
                        "DrawSpline",
                        "DrawSplinePoints",
                        "DrawSplineFromPolyline",
                        "DrawSplinePointsAppend",
                        "DrawSplinePointsAdd",
                        "DrawSplinePointsRemove",
                        "DrawSplinePointsDelTwo",
                        "DrawSplineExplode",
                        "DrawLineFree"
                    }, map);

    fillActionsList(ellipse_actions, {
                        "DrawEllipse1Point",
                        "DrawEllipseAxis",
                        "DrawEllipseFociPoint",
                        "DrawEllipse4Points",
                        "DrawEllipseCenter3Points",
                        "DrawEllipseInscribe"
                    }, map);

    fillActionsList(polyline_actions, {
                        "DrawPolyline",
                        "PolylineAdd",
                        "PolylineAppend",
                        "PolylineDel",
                        "PolylineDelBetween",
                        "PolylineTrim",
                        "PolylineEquidistant",
                        "PolylineSegment",
                        "PolylineArcToLines",
                        "PolylineSegmentType"
                    }, map);

    fillActionsList(select_actions, {
                        "DeselectAll",
                        "SelectAll",
                        "SelectSingle",
                        "SelectContour",
                        "SelectWindow",
                        "DeselectWindow",
                        "SelectIntersected",
                        "DeselectIntersected",
                        "SelectLayer",
                        "SelectInvert"
                    }, map);

    fillActionsList(dimension_actions, {
                        "DimAligned",
                        "DimLinear",
                        "DimLinearHor",
                        "DimLinearVer",
                        "DimBaseline",
                        "DimContinue",
                        "DimRadial",
                        "DimDiametric",
                        "DimAngular",
                        "DimArc",
                        "DimLeader",
                        "DimOrdinate",
                        "DimOrdinateForBase",
                        "DimOrdinateReBase",
        // fixme - sand - restore as GDT will be supported
                        // "GTDFeatureFrame",
                        // "DimModify",
                        "DimPickApply",
                        "DimRegenerate",
                        "DimStyles"
                    }, map);

    fillActionsList(other_drawing_actions, {
                        "DrawText",
                        "DrawMText",
                        "DrawHatch",
                        "DrawImage",
                        "DrawBoundingBox"
                    }, map);

    fillActionsList(modify_actions, {
                        "ModifyMove",
                        "ModifyDuplicate",
                        "ModifyAlign",
                        "ModifyAlignOne",
                        "ModifyAlignRef",
                        "ModifyRotate",
                        "ModifyScale",
                        "ModifyMirror",
                        "ModifyMoveRotate",
                        "ModifyRotate2",
                        "ModifyRevertDirection",
                        "ModifyTrim",
                        "ModifyTrim2",
                        "ModifyTrimAmount",
                        "ModifyLineJoin",
                        "ModifyBreakDivide",
                        "ModifyLineGap",
                        "ModifyOffset",
                        "ModifyBevel",
                        "ModifyRound",
                        "ModifyCut",
                        "ModifyStretch",
                        "ModifyEntity",
                        "ModifyAttributes",
                        "ModifyExplodeText",
                        "BlocksExplode",
                        "ModifyDelete"
                    }, map);

    fillActionsList(order_actions, {
                        "OrderTop",
                        "OrderBottom",
                        "OrderRaise",
                        "OrderLower"
                    }, map);

    fillActionsList(info_actions, {
                        "InfoPoint",
                        "InfoDist",
                        "InfoDist2",
                        "InfoDist3",
                        "InfoAngle",
                        "InfoAngle3Points",
                        "InfoTotalLength",
                        "InfoArea",
                        "EntityInfo"
                    }, map);

    fillActionsList(layer_actions, {
                        "LayersDefreezeAll",
                        "LayersFreezeAll",
                        "LayersUnlockAll",
                        "LayersLockAll",
                        "LayersAdd",
                        "LayersRemove",
                        "LayersEdit",
                        "LayersToggleLock",
                        "LayersToggleView",
                        "LayersTogglePrint",
                        "LayersToggleConstruction",
                        "LayersExportSelected",
                        "LayersExportVisible"
                    }, map);

    fillActionsList(block_actions, {
                        "BlocksDefreezeAll",
                        "BlocksFreezeAll",
                        "BlocksToggleView",
                        "BlocksAdd",
                        "BlocksRemove",
                        "BlocksAttributes",
                        "BlocksInsert",
                        "BlocksEdit",
                        "BlocksSave",
                        "BlocksCreate",
                        "BlocksExplode"
                    }, map);

    fillActionsList(pen_actions, {
                        "PenSyncFromLayer",
                        "PenPick",
                        "PenPickResolved",
                        "PenApply",
                        "PenCopy"
                    }, map);

    fillActionsList(entity_layer_actions,{
                        "EntityLayerActivate",
                        "EntityLayerView",
                        "EntityLayerHideOthers",
                        "EntityLayerLock",
                        "EntityLayerConstruction",
                        "EntityLayerPrint"
                    }, map);
}

void LC_ActionFactory::prepareActionsToDisableInPrintPreview(QList<QAction*>& actionsList, QMap<QString, QAction *> &map){
    fillActionsList(actionsList, {
        "EditCut",
        "EditCutQuick",
        "EditCopy",
        "EditCopyQuick",
        "EditPaste",
        "EditPasteTransform",
        "ViewGrid",
        "ViewDraft",
        "ViewLinesDraft",
        "ViewAntialiasing",
        "ModifyDeleteQuick",
        "EditKillAllActions",
        "ZoomIn",
        "ZoomOut",
        "ZoomAuto",
        "ZoomPrevious",
        "ZoomWindow",
        "ZoomPan",
        "OptionsDrawing",
        "ViewGridOrtho",
        "ViewGridIsoLeft",
        "ViewGridIsoTop",
        "ViewGridIsoRight",
        "UCSSetWCS",
        "UCSCreate",
        "UCSSetByDimOrdinate",
    }, map);

    actionsList.append(line_actions);
    actionsList.append(point_actions);
    actionsList.append(shape_actions);
    actionsList.append(circle_actions);
    actionsList.append(curve_actions);
    actionsList.append(spline_actions);
    actionsList.append(ellipse_actions);
    actionsList.append(polyline_actions);
    actionsList.append(select_actions);
    actionsList.append(dimension_actions);
    actionsList.append(other_drawing_actions);
    actionsList.append(modify_actions);
    actionsList.append(order_actions);
    actionsList.append(info_actions);
    actionsList.append(block_actions);
    actionsList.append(pen_actions);
    actionsList.append(layer_actions);
    actionsList.append(entity_layer_actions);
}
