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

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include <QAction>
#include <QActionGroup>

LC_ActionFactory::LC_ActionFactory(QObject* parent, QObject* a_handler)
    : QObject(parent)
    , using_theme(false)
    , main_window(parent)
    , action_handler(a_handler)
{
}

void LC_ActionFactory::fillActionContainer(QMap<QString, QAction*>& a_map, LC_ActionGroupManager* agm)
{
    QAction* action;

    // <[~ Zoom ~]>

    action = new QAction(tr("Zoom &Panning"), agm->other);
    action->setIcon(QIcon(":/icons/zoom_pan.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomPan()));
    action->setObjectName("ZoomPan");
    a_map["ZoomPan"] = action;

    // <[~ Select ~]>

    action = new QAction(tr("Select Entity"), agm->select);
    action->setIcon(QIcon(":/icons/select_entity.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectSingle()));
    action->setObjectName("SelectSingle");
    a_map["SelectSingle"] = action;

    action = new QAction(tr("Select Window"), agm->select);
    action->setIcon(QIcon(":/icons/select_window.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectWindow()));
    action->setObjectName("SelectWindow");
    a_map["SelectWindow"] = action;

    action = new QAction(tr("Deselect Window"), agm->select);
    action->setIcon(QIcon(":/icons/deselect_window.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectWindow()));
    action->setObjectName("DeselectWindow");
    a_map["DeselectWindow"] = action;

    action = new QAction(tr("(De-)Select &Contour"), agm->select);
    action->setIcon(QIcon(":/icons/deselect_contour.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectContour()));
    action->setObjectName("SelectContour");
    a_map["SelectContour"] = action;

    action = new QAction(tr("Select Intersected Entities"), agm->select);
    action->setIcon(QIcon(":/icons/select_intersected_entities.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectIntersected()));
    action->setObjectName("SelectIntersected");
    a_map["SelectIntersected"] = action;

    action = new QAction(tr("Deselect Intersected Entities"), agm->select);
    action->setIcon(QIcon(":/icons/deselect_intersected_entities.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectIntersected()));
    action->setObjectName("DeselectIntersected");
    a_map["DeselectIntersected"] = action;

    action = new QAction(tr("(De-)Select Layer"), agm->select);
    action->setIcon(QIcon(":/icons/deselect_layer.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectLayer()));
    action->setObjectName("SelectLayer");
    a_map["SelectLayer"] = action;

    // <[~ Draw ~]>

    action = new QAction(tr("&Points"), agm->other);
    action->setIcon(QIcon(":/icons/points.svg"));

    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawPoint()));
    action->setObjectName("DrawPoint");
    a_map["DrawPoint"] = action;

    // <[~ Line ~]>

    action = new QAction(tr("&2 Points"), agm->line);
    action->setIcon(QIcon(":/icons/line_2p.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLine()));
    action->setObjectName("DrawLine");
    a_map["DrawLine"] = action;

    action = new QAction(tr("&Angle"), agm->line);
    action->setIcon(QIcon(":/icons/line_angle.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineAngle()));
    action->setObjectName("DrawLineAngle");
    a_map["DrawLineAngle"] = action;

    action = new QAction(tr("&Horizontal"), agm->line);
    action->setIcon(QIcon(":/icons/line_horizontal.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineHorizontal()));
    action->setObjectName("DrawLineHorizontal");
    a_map["DrawLineHorizontal"] = action;

    action = new QAction(tr("Vertical"), agm->line);
    action->setIcon(QIcon(":/icons/line_vertical.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineVertical()));
    action->setObjectName("DrawLineVertical");
    a_map["DrawLineVertical"] = action;

//    action = new QAction(tr("Vertical"), agm->line);
//    connect(action, SIGNAL(triggered()),
//    action_handler, SLOT(slotDrawLineHorVert()));
//    action->setObjectName("DrawLineHorVert");
//    a_map["DrawLineHorVert"] = action;

    action = new QAction(tr("&Freehand Line"), agm->line);
    action->setIcon(QIcon(":/icons/line_freehand.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineFree()));
    action->setObjectName("DrawLineFree");
    a_map["DrawLineFree"] = action;

    action = new QAction(tr("&Parallel"), agm->line);
    action->setIcon(QIcon(":/icons/line_parallel.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineParallel()));
    action->setObjectName("DrawLineParallel");
    a_map["DrawLineParallel"] = action;

    action = new QAction(tr("Parallel through point"), agm->line);
    action->setIcon(QIcon(":/icons/line_parallel_p.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineParallelThrough()));
    action->setObjectName("DrawLineParallelThrough");
    a_map["DrawLineParallelThrough"] = action;

    action = new QAction(tr("Rectangle"), agm->line);
    action->setIcon(QIcon(":/icons/line_rectangle.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineRectangle()));
    action->setObjectName("DrawLineRectangle");
    a_map["DrawLineRectangle"] = action;

    action = new QAction(tr("Bisector"), agm->line);
    action->setIcon(QIcon(":/icons/line_bisector.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineBisector()));
    action->setObjectName("DrawLineBisector");
    a_map["DrawLineBisector"] = action;

    action = new QAction(tr("Tangent (P,C)"), agm->line);
    action->setIcon(QIcon(":/icons/line_tangent_pc.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineTangent1()));
    action->setObjectName("DrawLineTangent1");
    a_map["DrawLineTangent1"] = action;

    action = new QAction(tr("Tangent (C,C)"), agm->line);
    action->setIcon(QIcon(":/icons/line_tangent_cc.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineTangent2()));
    action->setObjectName("DrawLineTangent2");
    a_map["DrawLineTangent2"] = action;

    action = new QAction(tr("Tangent &Orthogonal"), agm->line);
    action->setIcon(QIcon(":/icons/line_tangent_perpendicular.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineOrthTan()));
    action->setObjectName("DrawLineOrthTan");
    a_map["DrawLineOrthTan"] = action;

    action = new QAction(tr("Orthogonal"), agm->line);
    action->setIcon(QIcon(":/icons/line_perpendicular.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineOrthogonal()));
    action->setObjectName("DrawLineOrthogonal");
    a_map["DrawLineOrthogonal"] = action;

    action = new QAction(tr("Relative angle"), agm->line);
    action->setIcon(QIcon(":/icons/line_relative_angle.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineRelAngle()));
    action->setObjectName("DrawLineRelAngle");
    a_map["DrawLineRelAngle"] = action;

    action = new QAction(tr("Pol&ygon (Cen,Cor)"), agm->line);
    action->setIcon(QIcon(":/icons/line_polygon_cen_cor.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLinePolygon()));
    action->setObjectName("DrawLinePolygonCenCor");
    a_map["DrawLinePolygonCenCor"] = action;

    action = new QAction(tr("Pol&ygon (Cen,Tan)"), agm->line);  //20161223 added by txmy
    action->setIcon(QIcon(":/icons/line_polygon_cen_tan.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLinePolygon3()));
    action->setObjectName("DrawLinePolygonCenTan");
    a_map["DrawLinePolygonCenTan"] = action;

    action = new QAction(tr("Polygo&n (Cor,Cor)"), agm->line);
    action->setIcon(QIcon(":/icons/line_polygon_cor_cor.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLinePolygon2()));
    action->setObjectName("DrawLinePolygonCorCor");
    a_map["DrawLinePolygonCorCor"] = action;

    // <[~ Circle ~]>

    action = new QAction(tr("Center, &Point"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_center_point.svg"));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotDrawCircle()));
    action->setObjectName("DrawCircle");
    a_map["DrawCircle"] = action;

    action = new QAction(tr("Center, &Radius"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_center_radius.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleCR()));
    action->setObjectName("DrawCircleCR");
    a_map["DrawCircleCR"] = action;

    action = new QAction(tr("2 Points"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_2_points.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle2P()));
    action->setObjectName("DrawCircle2P");
    a_map["DrawCircle2P"] = action;

    action = new QAction(tr("2 Points, Radius"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_2_points_radius.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle2PR()));
    action->setObjectName("DrawCircle2PR");
    a_map["DrawCircle2PR"] = action;

    action = new QAction(tr("3 Points"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_3_points.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle3P()));
    action->setObjectName("DrawCircle3P");
    a_map["DrawCircle3P"] = action;

    action = new QAction(tr("&Concentric"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_concentric.svg"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleParallel()));
    action->setObjectName("DrawCircleParallel");
    a_map["DrawCircleParallel"] = action;

    action = new QAction(tr("Circle &Inscribed"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_inscribed.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleInscribe()));
    action->setObjectName("DrawCircleInscribe");
    a_map["DrawCircleInscribe"] = action;

    action = new QAction(tr("Tangential 2 Circles, Radius",  "circle tangential with two circles, and given radius"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_tangential_2circles_radius.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan2()));
    action->setObjectName("DrawCircleTan2");
    a_map["DrawCircleTan2"] = action;

    action = new QAction(tr("Tangential 2 Circles, 1 Point"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_tangential_2circles_point.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan2_1P()));
    action->setObjectName("DrawCircleTan2_1P");
    a_map["DrawCircleTan2_1P"] = action;

    action = new QAction(tr("Tangential &3 Circles"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_tangential_3entities.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan3()));
    action->setObjectName("DrawCircleTan3");
    a_map["DrawCircleTan3"] = action;

    action = new QAction(tr("Tangential, 2 P&oints"), agm->circle);
    action->setIcon(QIcon(":/icons/circle_tangential_2points.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan1_2P()));
    action->setObjectName("DrawCircleTan1_2P");
    a_map["DrawCircleTan1_2P"] = action;

    // <[~ Arc ~]>

    action = new QAction(tr("&Center, Point, Angles"), agm->curve);
    action->setIcon(QIcon(":/icons/arc_center_point_angle.svg"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArc()));
    action->setObjectName("DrawArc");
    a_map["DrawArc"] = action;

    action = new QAction(tr("&3 Points"), agm->curve);
    action->setIcon(QIcon(":/icons/arc_3_points.svg"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArc3P()));
    action->setObjectName("DrawArc3P");
    a_map["DrawArc3P"] = action;

    action = new QAction(tr("&Concentric"), agm->curve);
    action->setIcon(QIcon(":/icons/arc_concentric.svg"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArcParallel()));
    action->setObjectName("DrawArcParallel");
    a_map["DrawArcParallel"] = action;

    action = new QAction(tr("Arc &Tangential"), agm->curve);
    action->setIcon(QIcon(":/icons/arc_continuation.svg"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArcTangential()));
    action->setObjectName("DrawArcTangential");
    a_map["DrawArcTangential"] = action;

    // <[~ Ellipse ~]>

    action = new QAction(tr("&Ellipse (Axis)"), agm->ellipse);
    action->setIcon(QIcon(":/icons/ellipse_axis.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseAxis()));
    action->setObjectName("DrawEllipseAxis");
    a_map["DrawEllipseAxis"] = action;

    action = new QAction(tr("Ellipse &Arc (Axis)"), agm->ellipse);
    action->setIcon(QIcon(":/icons/ellipse_arc_axis.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseArcAxis()));
    action->setObjectName("DrawEllipseArcAxis");
    a_map["DrawEllipseArcAxis"] = action;

    action = new QAction(tr("Ellipse &Foci Point"), agm->ellipse);
    action->setIcon(QIcon(":/icons/ellipse_foci_point.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseFociPoint()));
    action->setObjectName("DrawEllipseFociPoint");
    a_map["DrawEllipseFociPoint"] = action;

    action = new QAction(tr("Ellipse &4 Point"), agm->ellipse);
    action->setIcon(QIcon(":/icons/ellipse_4_points.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipse4Points()));
    action->setObjectName("DrawEllipse4Points");
    a_map["DrawEllipse4Points"] = action;

    action = new QAction(tr("Ellipse Center and &3 Points"), agm->ellipse);
    action->setIcon(QIcon(":/icons/ellipse_center_3_points.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseCenter3Points()));
    action->setObjectName("DrawEllipseCenter3Points");
    a_map["DrawEllipseCenter3Points"] = action;

    action = new QAction(tr("Ellipse &Inscribed"), agm->ellipse);
    action->setIcon(QIcon(":/icons/ellipse_inscribed.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseInscribe()));
    action->setObjectName("DrawEllipseInscribe");
    a_map["DrawEllipseInscribe"] = action;

    // <[~ Spline ~]>

    action = new QAction(tr("&Spline"), agm->curve);
    action->setIcon(QIcon(":/icons/spline.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawSpline()));
    action->setObjectName("DrawSpline");
    a_map["DrawSpline"] = action;

    action = new QAction(tr("&Spline through points"), agm->curve);
    action->setIcon(QIcon(":/icons/spline_points.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawSplinePoints()));
    action->setObjectName("DrawSplinePoints");
    a_map["DrawSplinePoints"] = action;

    // <[~ Polyline ~]>

    action = new QAction(tr("&Polyline"), agm->polyline);
    action->setIcon(QIcon(":/icons/polylines_polyline.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawPolyline()));
    action->setObjectName("DrawPolyline");
    a_map["DrawPolyline"] = action;

    action = new QAction(tr("&Add node"), agm->polyline);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/icons/insert_node.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineAdd()));
    action->setObjectName("PolylineAdd");
    a_map["PolylineAdd"] = action;

    action = new QAction(tr("A&ppend node"), agm->polyline);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/icons/append_node.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineAppend()));
    action->setObjectName("PolylineAppend");
    a_map["PolylineAppend"] = action;

    action = new QAction(tr("&Delete node"), agm->polyline);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/icons/delete_node.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineDel()));
    action->setObjectName("PolylineDel");
    a_map["PolylineDel"] = action;

    action = new QAction(tr("Delete &between two nodes"), agm->polyline);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/icons/delete_between_nodes.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineDelBetween()));
    action->setObjectName("PolylineDelBetween");
    a_map["PolylineDelBetween"] = action;

    action = new QAction(tr("&Trim segments"), agm->polyline);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/icons/trim.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineTrim()));
    action->setObjectName("PolylineTrim");
    a_map["PolylineTrim"] = action;

    action = new QAction(tr("Create &Equidistant Polylines"), agm->polyline);
    action->setIcon(QIcon(":/icons/create_equidistant_polyline.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineEquidistant()));
    action->setObjectName("PolylineEquidistant");
    a_map["PolylineEquidistant"] = action;

    action = new QAction(tr("Create Polyline from Existing &Segments"), agm->polyline);
    action->setIcon(QIcon(":/icons/create_polyline_from_existing_segments.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineSegment()));
    action->setObjectName("PolylineSegment");
    a_map["PolylineSegment"] = action;

    // <[~ Misc ~]>

    action = new QAction(QIcon(":/icons/text.svg"), tr("&MText"), agm->other);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawMText()));
    action->setObjectName("DrawMText");
    a_map["DrawMText"] = action;

    action = new QAction(tr("&Text"), agm->other);
    action->setIcon(QIcon(":/icons/text.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawText()));
    action->setObjectName("DrawText");
    a_map["DrawText"] = action;

    action = new QAction(tr("&Hatch"), agm->other);
    action->setIcon(QIcon(":/icons/hatch.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawHatch()));
    action->setObjectName("DrawHatch");
    a_map["DrawHatch"] = action;

    action = new QAction(tr("Insert &Image"), agm->other);
    action->setIcon(QIcon(":/icons/camera.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawImage()));
    action->setObjectName("DrawImage");
    a_map["DrawImage"] = action;

    // <[~ Dimension ~]>

    action = new QAction(tr("&Aligned"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_aligned.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimAligned()));
    action->setObjectName("DimAligned");
    a_map["DimAligned"] = action;

    action = new QAction(tr("&Linear"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_linear.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinear()));
    action->setObjectName("DimLinear");
    a_map["DimLinear"] = action;

    action = new QAction(tr("&Horizontal"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_horizontal.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinearHor()));
    action->setObjectName("DimLinearHor");
    a_map["DimLinearHor"] = action;

    action = new QAction(tr("&Vertical"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_vertical.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinearVer()));
    action->setObjectName("DimLinearVer");
    a_map["DimLinearVer"] = action;

    action = new QAction(tr("&Radial"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_radial.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimRadial()));
    action->setObjectName("DimRadial");
    a_map["DimRadial"] = action;

    action = new QAction(tr("&Diametric"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_diametric.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimDiametric()));
    action->setObjectName("DimDiametric");
    a_map["DimDiametric"] = action;

    action = new QAction(tr("&Angular"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_angular.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimAngular()));
    action->setObjectName("DimAngular");
    a_map["DimAngular"] = action;

    action = new QAction(tr("&Leader"), agm->dimension);
    action->setIcon(QIcon(":/icons/dim_leader.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLeader()));
    action->setObjectName("DimLeader");
    a_map["DimLeader"] = action;

    // <[~ Modify ~]>

    action = new QAction(tr("&Attributes"), agm->modify);
    action->setIcon(QIcon(":/icons/attributes.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyAttributes()));
    action->setObjectName("ModifyAttributes");
    action->setData("modifyattr, attr, ma");
    a_map["ModifyAttributes"] = action;

    action = new QAction(tr("&Delete"), agm->modify);
    action->setIcon(QIcon(":/icons/delete.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDelete()));
    action->setObjectName("ModifyDelete");
    a_map["ModifyDelete"] = action;

    action = new QAction(tr("Delete Freehand"), agm->modify);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDeleteFree()));
    action->setObjectName("ModifyDeleteFree");
    a_map["ModifyDeleteFree"] = action;

    action = new QAction(tr("&Move / Copy"), agm->modify);
    action->setIcon(QIcon(":/icons/move_copy.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMove()));
    action->setObjectName("ModifyMove");
    action->setData("move, mv");
    a_map["ModifyMove"] = action;

    action = new QAction(tr("Re&vert direction"), agm->modify);
    action->setIcon(QIcon(":/icons/revert_direction.svg"));
    action->setShortcut(QKeySequence(tr("Ctrl+R")));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotModifyRevertDirection()));
    action->setObjectName("ModifyRevertDirection");
    action->setData("revert, rev");
    a_map["ModifyRevertDirection"] = action;

    action = new QAction(tr("&Rotate"), agm->modify);
    action->setIcon(QIcon(":/icons/rotate.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRotate()));
    action->setObjectName("ModifyRotate");
    action->setData("rotate, ro");
    a_map["ModifyRotate"] = action;

    action = new QAction(tr("&Scale"), agm->modify);
    action->setIcon(QIcon(":/icons/scale.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyScale()));
    action->setObjectName("ModifyScale");
    action->setData("scale, sz");
    a_map["ModifyScale"] = action;

    action = new QAction(tr("&Mirror"), agm->modify);
    action->setIcon(QIcon(":/icons/mirror.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMirror()));
    action->setObjectName("ModifyMirror");
    action->setData("mirror, mi");
    a_map["ModifyMirror"] = action;

    action = new QAction(tr("Mo&ve and Rotate"), agm->modify);
    action->setIcon(QIcon(":/icons/move_rotate.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMoveRotate()));
    action->setObjectName("ModifyMoveRotate");
    a_map["ModifyMoveRotate"] = action;

    action = new QAction(tr("Rotate T&wo"), agm->modify);
    action->setIcon(QIcon(":/icons/rotate2.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRotate2()));
    action->setObjectName("ModifyRotate2");
    a_map["ModifyRotate2"] = action;

    action = new QAction(tr("&Properties"), agm->modify);
    action->setIcon(QIcon(":/icons/properties.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyEntity()));
    action->setObjectName("ModifyEntity");
    action->setData("properties, prop");
    a_map["ModifyEntity"] = action;

    action = new QAction(tr("&Trim"), agm->modify);
    action->setIcon(QIcon(":/icons/trim.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrim()));
    action->setObjectName("ModifyTrim");
    action->setData("trim, tm");
    a_map["ModifyTrim"] = action;

    action = new QAction(tr("Tr&im Two"), agm->modify);
    action->setIcon(QIcon(":/icons/trim2.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrim2()));
    action->setObjectName("ModifyTrim2");
    action->setData("trim2, tm2");
    a_map["ModifyTrim2"] = action;

    action = new QAction(tr("&Lengthen"), agm->modify);
    action->setIcon(QIcon(":/icons/trim_value.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrimAmount()));
    action->setObjectName("ModifyTrimAmount");
    action->setData("lengthen, le");
    a_map["ModifyTrimAmount"] = action;

    action = new QAction(tr("O&ffset"),agm->modify);
    action->setIcon(QIcon(":/icons/offset.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyOffset()));
    action->setObjectName("ModifyOffset");
    action->setData("offset, o");
    a_map["ModifyOffset"] = action;

    action = new QAction(tr("&Divide"), agm->modify);
    action->setIcon(QIcon(":/icons/divide.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyCut()));
    action->setObjectName("ModifyCut");
    action->setData("divide, cut, div");
    a_map["ModifyCut"] = action;

    action = new QAction(tr("&Stretch"), agm->modify);
    action->setIcon(QIcon(":/icons/stretch.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyStretch()));
    action->setObjectName("ModifyStretch");
    action->setData("stretch, ss");
    a_map["ModifyStretch"] = action;

    action = new QAction(tr("&Bevel"), agm->modify);
    action->setIcon(QIcon(":/icons/bevel.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyBevel()));
    action->setObjectName("ModifyBevel");
    action->setData("bevel, bev, ch");
    a_map["ModifyBevel"] = action;

    action = new QAction(tr("&Fillet"), agm->modify);
    action->setIcon(QIcon(":/icons/fillet.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRound()));
    action->setObjectName("ModifyRound");
    action->setData("fillet, fi");
    a_map["ModifyRound"] = action;

    action = new QAction(tr("&Explode Text into Letters"), agm->modify);
    action->setIcon(QIcon(":/icons/explode_text_to_letters.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyExplodeText()));
    action->setObjectName("ModifyExplodeText");
    a_map["ModifyExplodeText"] = action;

    action = new QAction(tr("Ex&plode"), agm->modify);
    action->setIcon(QIcon(":/icons/explode.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksExplode()));
    action->setObjectName("BlocksExplode");
    a_map["BlocksExplode"] = action;

    // <[~ Info ~]>

    action = new QAction(tr("Point inside contour"), agm->info);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoInside()));
    action->setObjectName("InfoInside");
    a_map["InfoInside"] = action;

    action = new QAction(tr("&Distance Point to Point"), agm->info);
    action->setIcon(QIcon(":/icons/distance_point_to_point.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoDist()));
    action->setObjectName("InfoDist");
    a_map["InfoDist"] = action;

    action = new QAction(tr("&Distance Entity to Point"), agm->info);
    action->setIcon(QIcon(":/icons/distance_point_to_entity.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoDist2()));
    action->setObjectName("InfoDist2");
    a_map["InfoDist2"] = action;

    action = new QAction(tr("An&gle between two lines"), agm->info);
    action->setIcon(QIcon(":/icons/angle_line_to_line.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoAngle()));
    action->setObjectName("InfoAngle");
    a_map["InfoAngle"] = action;

    action = new QAction(tr("&Total length of selected entities"), agm->info);
    action->setIcon(QIcon(":/icons/total_length_selected_entities.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoTotalLength()));
    action->setObjectName("InfoTotalLength");
    a_map["InfoTotalLength"] = action;

    action = new QAction(tr("Polygonal &Area"), agm->info);
    action->setIcon(QIcon(":/icons/polygonal_area.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoArea()));
    action->setObjectName("InfoArea");
    a_map["InfoArea"] = action;

    foreach (QAction* value, a_map)
    {
        value->setCheckable(true);
    }

    // =============================
    // <[~ not checkable actions ~]>
    // =============================


    // <[~ Order ~]>

    action = new QAction(tr("move to bottom"), agm->modify);
    action->setShortcut(QKeySequence(Qt::Key_End));
    action->setIcon(QIcon(":/icons/downmost.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderBottom()));
    action->setObjectName("OrderBottom");
    a_map["OrderBottom"] = action;

    action = new QAction(tr("lower after entity"), agm->modify);
    action->setShortcut(QKeySequence(Qt::Key_PageDown));
    action->setIcon(QIcon(":/icons/down.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderLower()));
    action->setObjectName("OrderLower");
    a_map["OrderLower"] = action;

    action = new QAction(tr("raise over entity"), agm->modify);
    action->setShortcut(QKeySequence(Qt::Key_PageUp));
    action->setIcon(QIcon(":/icons/up.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderRaise()));
    action->setObjectName("OrderRaise");
    a_map["OrderRaise"] = action;

    action = new QAction(tr("move to top"), agm->modify);
    action->setShortcut(QKeySequence(Qt::Key_Home));
    action->setIcon(QIcon(":/icons/upmost.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderTop()));
    action->setObjectName("OrderTop");
    a_map["OrderTop"] = action;

    // <[~ Layer ~]>

    action = new QAction(tr("&Show all"), agm->layer);
    action->setIcon(QIcon(":/ui/visibleblock.png"));
    connect(action, SIGNAL(triggered()),
            action_handler, SLOT(slotLayersDefreezeAll()));
    action->setObjectName("LayersDefreezeAll");
    a_map["LayersDefreezeAll"] = action;

    action = new QAction(tr("&Hide all"), agm->layer);
    action->setIcon(QIcon(":/ui/hiddenblock.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersFreezeAll()));
    action->setObjectName("LayersFreezeAll");
    a_map["LayersFreezeAll"] = action;

    action = new QAction(tr("&Unlock all"), agm->layer);
    action->setIcon(QIcon(":/ui/unlockedlayer.png"));
    connect(action, SIGNAL(triggered()),
            action_handler, SLOT(slotLayersUnlockAll()));
    action->setObjectName("LayersUnlockAll");
    a_map["LayersUnlockAll"] = action;

    action = new QAction(tr("&Lock all"), agm->layer);
    action->setIcon(QIcon(":/ui/lockedlayer.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersLockAll()));
    action->setObjectName("LayersLockAll");
    a_map["LayersLockAll"] = action;

    action = new QAction(tr("&Add Layer"), agm->layer);
    action->setIcon(QIcon(":/icons/add.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersAdd()));
    action->setObjectName("LayersAdd");
    a_map["LayersAdd"] = action;

    action = new QAction(tr("&Remove Layer"), agm->layer);
    action->setIcon(QIcon(":/icons/remove.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersRemove()));
    action->setObjectName("LayersRemove");
    a_map["LayersRemove"] = action;

    action = new QAction(tr("&Edit Layer"), agm->layer);
    action->setIcon(QIcon(":/icons/attributes.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersEdit()));
    action->setObjectName("LayersEdit");
    a_map["LayersEdit"] = action;

    action = new QAction(tr("Toggle Layer Loc&k"), agm->layer);
    action->setIcon(QIcon(":/icons/locked.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersToggleLock()));
    action->setObjectName("LayersToggleLock");
    a_map["LayersToggleLock"] = action;

    action = new QAction(tr("&Toggle Layer Visibility"), agm->layer);
    action->setIcon(QIcon(":/icons/visible.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersToggleView()));
    action->setObjectName("LayersToggleView");
    a_map["LayersToggleView"] = action;

    action = new QAction(tr("Toggle Layer &Print"), agm->layer);
    action->setIcon(QIcon(":/icons/print.svg"));
    connect(action, SIGNAL(triggered()), action_handler,
            SLOT(slotLayersTogglePrint()));
    action->setObjectName("LayersTogglePrint");
    a_map["LayersTogglePrint"] = action;

    action = new QAction(tr("Toggle &Construction Layer"), agm->layer);
    action->setIcon(QIcon(":/icons/construction_layer.svg"));
    connect(action, SIGNAL(triggered()),
            action_handler, SLOT(slotLayersToggleConstruction()));
    action->setObjectName("LayersToggleConstruction");
    a_map["LayersToggleConstruction"] = action;

    // <[~ Block ~]>

    action = new QAction(tr("&Show all"), agm->block);
    action->setIcon(QIcon(":/ui/blockdefreeze.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksDefreezeAll()));
    action->setObjectName("BlocksDefreezeAll");
    a_map["BlocksDefreezeAll"] = action;

    action= new QAction(tr("&Hide all"), agm->block);
    action->setIcon(QIcon(":/ui/blockfreeze.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksFreezeAll()));
    action->setObjectName("BlocksFreezeAll");
    a_map["BlocksFreezeAll"] = action;

    action = new QAction(tr("&Add Block"), agm->block);
    action->setIcon(QIcon(":/icons/add.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksAdd()));
    action->setObjectName("BlocksAdd");
    a_map["BlocksAdd"] = action;

    action = new QAction(tr("&Remove Block"), agm->block);
    action->setIcon(QIcon(":/icons/remove.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksRemove()));
    action->setObjectName("BlocksRemove");
    a_map["BlocksRemove"] = action;

    action = new QAction(tr("&Rename Block"), agm->block);
    action->setIcon(QIcon(":/icons/rename_active_block.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksAttributes()));
    action->setObjectName("BlocksAttributes");
    a_map["BlocksAttributes"] = action;

    action = new QAction(tr("&Edit Block"), agm->block);
    action->setIcon(QIcon(":/icons/properties.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksEdit()));
    action->setObjectName("BlocksEdit");
    a_map["BlocksEdit"] = action;

    action = new QAction( tr("&Save Block"), agm->block);
    action->setIcon(QIcon(":/icons/save.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksSave()));
    action->setObjectName("BlocksSave");
    a_map["BlocksSave"] = action;

    action = new QAction(tr("&Insert Block"), agm->block);
    action->setIcon(QIcon(":/icons/insert_active_block.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksInsert()));
    action->setObjectName("BlocksInsert");
    a_map["BlocksInsert"] = action;

    action = new QAction(tr("Toggle Block &Visibility"), agm->block);
    action->setIcon(QIcon(":/ui/layertoggle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksToggleView()));
    action->setObjectName("BlocksToggleView");
    a_map["BlocksToggleView"] = action;

    action = new QAction(tr("&Create Block"), agm->block);
    action->setIcon(QIcon(":/icons/create_block.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksCreate()));
    action->setObjectName("BlocksCreate");
    a_map["BlocksCreate"] = action;

    // <[~ Options ~]>

    action = new QAction(tr("&Application Preferences"), agm->options);
    action->setIcon(QIcon(":/icons/settings.svg"));
    connect(action, SIGNAL(triggered()),
    main_window, SLOT(slotOptionsGeneral()));
    action->setMenuRole(QAction::NoRole);
    action->setObjectName("OptionsGeneral");
    a_map["OptionsGeneral"] = action;

    action = new QAction(tr("Current &Drawing Preferences"), agm->options);
    action->setIcon(QIcon(":/icons/drawing_settings.svg"));
    action->setShortcut(QKeySequence::Preferences);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOptionsDrawing()));
    action->setObjectName("OptionsDrawing");
    a_map["OptionsDrawing"] = action;

    action = new QAction(tr("Widget Options"), agm->options);
    action->setObjectName("WidgetOptions");
    a_map["WidgetOptions"] = action;
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(widgetOptionsDialog()));

    action = new QAction(tr("Device Options"), agm->options);
    action->setObjectName("DeviceOptions");
    a_map["DeviceOptions"] = action;
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(showDeviceOptions()));

    // <[~ Modify ~]>

    action = new QAction(tr("&Delete selected"), agm->edit);
    action->setIcon(QIcon(":/icons/delete.svg"));
    action->setShortcuts(QList<QKeySequence>() << QKeySequence::Delete << QKeySequence(Qt::Key_Backspace));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDeleteQuick()));
    action->setObjectName("ModifyDeleteQuick");
    a_map["ModifyDeleteQuick"] = action;

    action = new QAction(tr("Select &All"), agm->select);
    action->setShortcut(QKeySequence::SelectAll);
    action->setIcon(QIcon(":/icons/select_all.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectAll()));
    action->setObjectName("SelectAll");
    a_map["SelectAll"] = action;

    // <[~ Select ~]>

    action = new QAction(tr("Deselect &all"), agm->select);
    // RVT April 29, 2011 - Added esc key to de-select all entities
    action->setShortcuts(QList<QKeySequence>() << QKeySequence(tr("Ctrl+K")));
    action->setIcon(QIcon(":/icons/deselect_all.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectAll()));
    action->setObjectName("DeselectAll");
    a_map["DeselectAll"] = action;

    action = new QAction(tr("Invert Selection"), agm->select);
    action->setIcon(QIcon(":/icons/select_inverted.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectInvert()));
    action->setObjectName("SelectInvert");
    a_map["SelectInvert"] = action;

    // <[~ Misc ~]>

    action = new QAction(tr("Export as CA&M/plain SVG..."), agm->file);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotFileExportMakerCam()));
    action->setObjectName("FileExportMakerCam");
    a_map["FileExportMakerCam"] = action;

//    action = new QAction(tr("Regenerate Dimension Entities"), disable_group);
//    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotToolRegenerateDimensions()));
//    action->setObjectName("ToolRegenerateDimensions");
//    a_map["ToolRegenerateDimensions"] = action;


    // ===========================
    // <[~ Main Window Actions ~]>
    // ===========================


    action = new QAction(tr("&Export as image"), agm->file);
    action->setIcon(QIcon(":/icons/export.svg"));
    connect(action, SIGNAL( triggered()), main_window, SLOT(slotFileExport()));
    action->setObjectName("FileExport");
    a_map["FileExport"] = action;

    action = new QAction(tr("&Close"), agm->file);
    action->setIcon(QIcon(":/icons/close.svg"));
    action->setShortcut(QKeySequence::Close);
    action->setShortcutContext(Qt::WidgetShortcut);
    action->setObjectName("FileClose");
    a_map["FileClose"] = action;

    action = new QAction(tr("Export as PDF"), agm->file);
    action->setIcon(QIcon(":/icons/export_pdf.svg"));
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFilePrintPDF()));
    action->setObjectName("FilePrintPDF");
    a_map["FilePrintPDF"] = action;

    action = new QAction(tr("&Block"), agm->file);
    action->setIcon(QIcon(":/icons/insert_active_block.svg"));
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotImportBlock()));
    action->setObjectName("BlocksImport");
    a_map["BlocksImport"] = action;

    // <[~ View ~]>

    action = new QAction(tr("&Fullscreen"), agm->view);
    #if defined(Q_OS_LINUX)
        action->setShortcut(QKeySequence("F11"));
    #else
        action->setShortcut(QKeySequence::FullScreen);
    #endif
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(toggleFullscreen(bool)));
    action->setObjectName("Fullscreen");
    a_map["Fullscreen"] = action;

    action = new QAction(tr("&Grid"), agm->view);
    action->setIcon(QIcon(":/icons/grid.svg"));
    action->setShortcut(QKeySequence(tr("Ctrl+G", "Toggle Grid")));
    action->setCheckable(true);
    action->setChecked(true);
    connect(main_window, SIGNAL(gridChanged(bool)), action, SLOT(setChecked(bool)));
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewGrid(bool)));
    action->setObjectName("ViewGrid");
    a_map["ViewGrid"] = action;

    action = new QAction(tr("&Draft"), agm->view);
    action->setIcon(QIcon(":/icons/draft.svg"));
    action->setCheckable(true);
    action->setShortcut(QKeySequence(tr("Ctrl+D", "Toggle Draft Mode")));
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewDraft(bool)));
    connect(main_window, SIGNAL(draftChanged(bool)), action, SLOT(setChecked(bool)));
    action->setObjectName("ViewDraft");
    a_map["ViewDraft"] = action;

    action = new QAction(tr("&Statusbar"), agm->view);
    action->setCheckable(true);
    action->setChecked(true);
    action->setShortcut(QKeySequence(tr("Ctrl+I", "Hide Statusbar")));
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewStatusBar(bool)));
    action->setObjectName("ViewStatusBar");
    a_map["ViewStatusBar"] = action;

    action = new QAction(tr("Focus on &Command Line"), agm->view);
    action->setIcon(QIcon(":/main/editclear.png"));
    QList<QKeySequence> commandLineShortcuts;
    commandLineShortcuts<<QKeySequence(Qt::CTRL + Qt::Key_M)<<QKeySequence(Qt::Key_Colon)<<QKeySequence(Qt::Key_Space);
    action->setShortcuts(commandLineShortcuts);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFocusCommandLine()));
    action->setObjectName("FocusCommand");
    a_map["FocusCommand"] = action;

    action = new QAction(tr("Left"), agm->widgets);
    action->setIcon(QIcon(":/icons/dockwidgets_left"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleLeftDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("LeftDockAreaToggle");
    a_map["LeftDockAreaToggle"] = action;

    action = new QAction(tr("Right"), agm->widgets);
    action->setIcon(QIcon(":/icons/dockwidgets_right"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleRightDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(true);
    action->setObjectName("RightDockAreaToggle");
    a_map["RightDockAreaToggle"] = action;

    action = new QAction(tr("Top"), agm->widgets);
    action->setIcon(QIcon(":/icons/dockwidgets_top"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleTopDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("TopDockAreaToggle");
    a_map["TopDockAreaToggle"] = action;

    action = new QAction(tr("Bottom"), agm->widgets);
    action->setIcon(QIcon(":/icons/dockwidgets_bottom"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleBottomDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("BottomDockAreaToggle");
    a_map["BottomDockAreaToggle"] = action;

    action = new QAction(tr("Floating"), agm->widgets);
    action->setIcon(QIcon(":/icons/dockwidgets_floating"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleFloatingDockwidgets(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("FloatingDockwidgetsToggle");
    a_map["FloatingDockwidgetsToggle"] = action;

    action = new QAction(tr("Reload Style Sheet"), agm->options);
    action->setShortcut(QKeySequence("Ctrl+T"));
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(reloadStyleSheet()));
    action->setObjectName("ReloadStyleSheet");
    a_map["ReloadStyleSheet"] = action;

    action = new QAction(tr("Menu Creator"), agm->widgets);
    action->setIcon(QIcon(":/icons/create_menu.svg"));
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(invokeMenuCreator()));
    action->setObjectName("InvokeMenuCreator");
    a_map["InvokeMenuCreator"] = action;

    action = new QAction(tr("Toolbar Creator"), agm->widgets);
    action->setIcon(QIcon(":/icons/create_toolbar.svg"));
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(invokeToolbarCreator()));
    action->setObjectName("InvokeToolbarCreator");
    a_map["InvokeToolbarCreator"] = action;

    commonActions(a_map, agm);
}

void LC_ActionFactory::commonActions(QMap<QString, QAction*>& a_map, LC_ActionGroupManager* agm)
{
    QAction* action;

    // <[~ Edit ~]>

    action = new QAction(tr("&Selection pointer"), agm->edit);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("go-previous-view", QIcon(":/icons/cursor.svg")));
    else
        action->setIcon(QIcon(":/icons/cursor.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditKillAllActions()));
    action->setObjectName("EditKillAllActions");
    a_map["EditKillAllActions"] = action;

    action = new QAction(tr("&Undo"), agm->edit);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/icons/undo.svg")));
    else
        action->setIcon(QIcon(":/icons/undo.svg"));
    action->setShortcut(QKeySequence::Undo);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditUndo()));
    action->setObjectName("EditUndo");
    a_map["EditUndo"] = action;

    action = new QAction(tr("&Redo"), agm->edit);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/icons/redo.svg")));
    else
        action->setIcon(QIcon(":/icons/redo.svg"));
    action->setShortcut(QKeySequence::Redo);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditRedo()));
    action->setObjectName("EditRedo");
    a_map["EditRedo"] = action;

    action = new QAction(tr("Cu&t"), agm->edit);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/icons/cut.svg")));
    else
        action->setIcon(QIcon(":/icons/cut.svg"));
    action->setShortcut(QKeySequence::Cut);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditCut()));
    action->setObjectName("EditCut");
    a_map["EditCut"] = action;

    action = new QAction(tr("&Copy"), agm->edit);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/icons/copy.svg")));
    else
        action->setIcon(QIcon(":/icons/copy.svg"));
    action->setShortcut(QKeySequence::Copy);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditCopy()));
    action->setObjectName("EditCopy");
    a_map["EditCopy"] = action;

    action = new QAction(tr("&Paste"), agm->edit);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/icons/paste.svg")));
    else
        action->setIcon(QIcon(":/icons/paste.svg"));
    action->setShortcut(QKeySequence::Paste);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditPaste()));
    action->setObjectName("EditPaste");
    a_map["EditPaste"] = action;

    // <[~ Zoom ~]>

    action = new QAction(tr("Zoom &In"), agm->view);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/icons/zoom_in.svg")));
    else
        action->setIcon(QIcon(":/icons/zoom_in.svg"));
    action->setShortcut(QKeySequence::ZoomIn);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomIn()));
    action->setObjectName("ZoomIn");
    a_map["ZoomIn"] = action;

    action = new QAction(tr("Zoom &Out"), agm->view);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/icons/zoom_out.svg")));
    else
        action->setIcon(QIcon(":/icons/zoom_out.svg"));
    action->setShortcut(QKeySequence::ZoomOut);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomOut()));
    action->setObjectName("ZoomOut");
    a_map["ZoomOut"] = action;

    action = new QAction(tr("&Auto Zoom"), agm->view);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/icons/zoom_auto.svg")));
    else
        action->setIcon(QIcon(":/icons/zoom_auto.svg"));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomAuto()));
    action->setObjectName("ZoomAuto");
    a_map["ZoomAuto"] = action;

    action = new QAction(tr("Previous &View"), agm->view);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("zoom-previous", QIcon(":/icons/zoom_previous.svg")));
    else
        action->setIcon(QIcon(":/icons/zoom_previous.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomPrevious()));
    action->setEnabled(false);
    action->setObjectName("ZoomPrevious");
    a_map["ZoomPrevious"] = action;

    action = new QAction(tr("&Redraw"), agm->view);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/icons/redraw.svg")));
    else
        action->setIcon(QIcon(":/icons/redraw.svg"));
    action->setShortcut(QKeySequence::Refresh);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomRedraw()));
    action->setObjectName("ZoomRedraw");
    a_map["ZoomRedraw"] = action;

    action = new QAction(tr("&Window Zoom"), agm->other);
    action->setCheckable(true);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("zoom-select", QIcon(":/icons/zoom_window.svg")));
    else
        action->setIcon(QIcon(":/icons/zoom_window.svg"));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomWindow()));
    action->setObjectName("ZoomWindow");
    a_map["ZoomWindow"] = action;

    // <[~ File ~]>

    action = new QAction(tr("&New"), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("document-new", QIcon(":/icons/new.svg")));
    else
        action->setIcon(QIcon(":/icons/new.svg"));
    action->setShortcut(QKeySequence::New);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileNewNew()));
    action->setObjectName("FileNew");
    a_map["FileNew"] = action;

    action = new QAction(tr("New From &Template"), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("document-new", QIcon(":/icons/new_from_template.svg")));
    else
        action->setIcon(QIcon(":/icons/new_from_template.svg"));
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileNewTemplate()));
    action->setObjectName("FileNewTemplate");
    a_map["FileNewTemplate"] = action;

    action = new QAction(tr("&Open..."), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("document-open", QIcon(":/icons/open.svg")));
    else
        action->setIcon(QIcon(":/icons/open.svg"));
    action->setShortcut(QKeySequence::Open);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileOpen()));
    action->setObjectName("FileOpen");
    a_map["FileOpen"] = action;

    action = new QAction(tr("&Save"), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("document-save", QIcon(":/icons/save.svg")));
    else
        action->setIcon(QIcon(":/icons/save.svg"));
    action->setShortcut(QKeySequence::Save);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileSave()));
    action->setObjectName("FileSave");
    a_map["FileSave"] = action;

    action = new QAction(tr("Save &as..."), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/icons/save_as.svg")));
    else
        action->setIcon(QIcon(":/icons/save_as.svg"));
    action->setShortcut(QKeySequence::SaveAs);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileSaveAs()));
    action->setObjectName("FileSaveAs");
    a_map["FileSaveAs"] = action;

    action = new QAction(tr("&Print..."), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("document-print", QIcon(":/icons/print.svg")));
    else
        action->setIcon(QIcon(":/icons/print.svg"));
    action->setShortcut(QKeySequence::Print);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFilePrint()));
    connect(main_window, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    action->setObjectName("FilePrint");
    a_map["FilePrint"] = action;

    action = new QAction(tr("Print Pre&view"), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("document-print-preview", QIcon(":/icons/print_preview.svg")));
    else
        action->setIcon(QIcon(":/icons/print_preview.svg"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), main_window, SLOT(slotFilePrintPreview(bool)));
    connect(main_window, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    action->setObjectName("FilePrintPreview");
    a_map["FilePrintPreview"] = action;

    action = new QAction(tr("&Quit"), agm->file);
    if (using_theme)
        action->setIcon(QIcon::fromTheme("application-exit", QIcon(":/icons/quit.svg")));
    else
        action->setIcon(QIcon(":/icons/quit.svg"));
    action->setShortcut(QKeySequence::Quit);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileQuit()));
    action->setObjectName("FileQuit");
    a_map["FileQuit"] = action;
}
