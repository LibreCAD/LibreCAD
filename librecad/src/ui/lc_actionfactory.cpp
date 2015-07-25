/*
**********************************************************************************
**
** This file is part of the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (ravas@outlook.com)
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

// github.com/r-a-v-a-s/LibreCAD.git

// lc_actionfactory is a rewrite of qg_actionfactory; some copied content remains.
// qg_actionfactory contributors:
// Andrew Mustun, Claude Sylvain, R. van Twisk, Dongxu Li, Rallaz, Armin Stebich, ravas, korhadris

#include "lc_actionfactory.h"

LC_ActionFactory::LC_ActionFactory(QG_ActionHandler* ah, QMainWindow* mw, QActionGroup* group):
    action_handler(ah),
    main_window(mw),
    tools(group)
{}

std::map<std::string, QAction*> LC_ActionFactory::action_map()
{

    std::map<std::string, QAction*> a_map;
    QAction* action;

    // <[~ Zoom ~]>

    action = new QAction(tr("&Window Zoom"), tools);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("zoom-select", QIcon(":/actions/zoomwindow.png")));
    #else
    action->setIcon(QIcon(":/actions/zoomwindow.png"));
    #endif
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomWindow()));
    a_map["ZoomWindow"] = action;

    action = new QAction(tr("Zoom &Panning"), tools);
    action->setIcon(QIcon(":/actions/zoompan.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomPan()));
    a_map["ZoomPan"] = action;

    // <[~ Select ~]>

    action = new QAction(tr("Select Entity"), tools);
    action->setIcon(QIcon(":/extui/selectsingle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectSingle()));
    a_map["SelectSingle"] = action;

    action = new QAction(tr("Select Window"), tools);
    action->setIcon(QIcon(":/extui/selectwindow.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectWindow()));
    a_map["SelectWindow"] = action;

    action = new QAction(tr("Deselect Window"), tools);
    action->setIcon(QIcon(":/extui/deselectwindow.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectWindow()));
    a_map["DeselectWindow"] = action;

    action = new QAction(tr("(De-)Select &Contour"), tools);
    action->setIcon(QIcon(":/extui/selectcontour.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectContour()));
    a_map["SelectContour"] = action;

    action = new QAction(tr("Select Intersected Entities"), tools);
    action->setIcon(QIcon(":/extui/selectinters.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectIntersected()));
    a_map["SelectIntersected"] = action;

    action = new QAction(tr("Deselect Intersected Entities"), tools);
    action->setIcon(QIcon(":/extui/deselectinters.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectIntersected()));
    a_map["DeselectIntersected"] = action;

    action = new QAction(tr("(De-)Select Layer"), tools);
    action->setIcon(QIcon(":/extui/selectlayer.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectLayer()));
    a_map["SelectLayer"] = action;

    // <[~ Draw ~]>

    action = new QAction(tr("&Points"), tools);
    action->setIcon(QIcon(":/extui/points.png"));
    action->setData(RS2::ActionDrawPoint);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawPoint()));
    a_map["DrawPoint"] = action;

    // <[~ Line ~]>

    action = new QAction(tr("&2 Points"), tools);
    action->setIcon(QIcon(":/extui/linesnormal.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLine()));
    a_map["DrawLine"] = action;

    action = new QAction(QIcon(":/extui/linesangle.png"), tr("&Angle"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineAngle()));
    a_map["DrawLineAngle"] = action;

    action = new QAction(QIcon(":/extui/lineshor.png"), tr("&Horizontal"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineHorizontal()));
    a_map["DrawLineHorizontal"] = action;

    new QAction(QIcon(":/extui/linesver.png"), tr("Vertical"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineVertical()));
    a_map["DrawLineVertical"] = action;

    action = new QAction(tr("Vertical"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineHorVert()));
    a_map["DrawLineHorVert"] = action;

    action = new QAction(tr("&Freehand Line"), tools);
    action->setIcon(QIcon(":/extui/linesfree.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineFree()));
    a_map["DrawLineFree"] = action;

    action = new QAction(tr("&Parallel"), tools);
    action->setIcon(QIcon(":/extui/linespara.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineParallel()));
    a_map["DrawLineParallel"] = action;

    action = new QAction(tr("Parallel through point"), tools);
    action->setIcon(QIcon(":/extui/linesparathrough.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineParallelThrough()));
    a_map["DrawLineParallelThrough"] = action;

    action = new QAction(tr("Rectangle"), tools);
    action->setIcon(QIcon(":/extui/linesrect.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineRectangle()));
    a_map["DrawLineRectangle"] = action;

    action = new QAction(QIcon(":/extui/linesbisector.png"),tr("Bisector"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineBisector()));
    a_map["DrawLineBisector"] = action;

    action = new QAction(QIcon(":/extui/linestan1.png"), tr("Tangent (P,C)"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineTangent1()));
    a_map["DrawLineTangent1"] = action;

    action = new QAction(QIcon(":/extui/linestan2.png"), tr("Tangent (C,C)"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineTangent2()));
    a_map["DrawLineTangent2"] = action;

    action = new QAction(tr("Tangent &Orthogonal"), tools);
    action->setIcon(QIcon(":/extui/linesorthtan.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineOrthTan()));
    a_map["DrawLineOrthTan"] = action;

    action = new QAction(tr("Orthogonal"), tools);
    action->setIcon(QIcon(":/extui/linesorthogonal.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineOrthogonal()));
    a_map["DrawLineOrthogonal"] = action;

    action = new QAction(tr("Relative angle"), tools);
    action->setIcon(QIcon(":/extui/linesrelativeangle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineRelAngle()));
    a_map["DrawLineRelAngle"] = action;

    action = new QAction(tr("Pol&ygon (Cen,Cor)"), tools);
    action->setIcon(QIcon(":/extui/linespolygon.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLinePolygon()));
    a_map["DrawLinePolygonCenCor"] = action;

    action = new QAction(tr("Polygo&n (Cor,Cor)"), tools);
    action->setIcon(QIcon(":/extui/linespolygon2.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLinePolygon2()));
    a_map["DrawLinePolygonCorCor"] = action;

    // <[~ Circle ~]>

    action = new QAction(QIcon(":/extui/circles.png"), tr("Center, &Point"), tools);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotDrawCircle()));
    a_map["DrawCircle"] = action;

    action = new QAction(tr("Center, &Radius"), tools);
    action->setIcon(QIcon(":/extui/circlescr.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleCR()));
    a_map["DrawCircleCR"] = action;

    action = new QAction(tr("2 Points"), tools);
    action->setIcon(QIcon(":/extui/circles2p.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle2P()));
    a_map["DrawCircle2P"] = action;

    action = new QAction(QIcon(":/extui/circle2pr.png"), tr("2 Points, Radius"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle2PR()));
    a_map["DrawCircle2PR"] = action;

    action = new QAction(QIcon(":/extui/circles3p.png"), tr("3 Points"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle3P()));
    a_map["DrawCircle3P"] = action;

    action = new QAction(tr("&Concentric"), tools);
    action->setIcon(QIcon(":/extui/circlespara.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleParallel()));
    a_map["DrawCircleParallel"] = action;

    action = new QAction(QIcon(":/extui/circleinscribe.png"), tr("Circle &Inscribed"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleInscribe()));
    a_map["DrawCircleInscribe"] = action;

    action = new QAction(tr("Tangential 2 Circles, Radius",  "circle tangential with two circles, and given radius"), tools);
    action->setIcon(QIcon(":/extui/circletan2.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan2()));
    a_map["DrawCircleTan2"] = action;

    action = new QAction(tr("Tangential 2 Circles, 1 Point"), tools);
    action->setIcon(QIcon(":/extui/circletan2_1p.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan2_1P()));
    a_map["DrawCircleTan2_1P"] = action;

    action = new QAction(tr("Tangential &3 Circles"), tools);
    action->setIcon(QIcon(":/extui/circletan3.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan3()));
    a_map["DrawCircleTan3"] = action;

    action = new QAction(tr("Tangential, 2 P&oints"), tools);
    action->setIcon(QIcon(":/extui/circletan1_2p.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan1_2P()));
    a_map["DrawCircleTan1_2P"] = action;

    // <[~ Arc ~]>

    action = new QAction(tr("&Center, Point, Angles"),tools);
    action->setIcon(QIcon(":/extui/arcscraa.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArc()));
    a_map["DrawArc"] = action;

    action = new QAction(tr("&3 Points"), tools);
    action->setIcon(QIcon(":/extui/arcs3p.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArc3P()));
    a_map["DrawArc3P"] = action;

    action = new QAction(tr("&Concentric"), tools);
    action->setIcon(QIcon(":/extui/arcspara.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArcParallel()));
    a_map["DrawArcParallel"] = action;

    action = new QAction(QIcon(":/extui/arcstangential.png"), tr("Arc &Tangential"), tools);
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArcTangential()));
    a_map["DrawArcTangential"] = action;

    // <[~ Ellipse ~]>

    action = new QAction(QIcon(":/extui/ellipsesaxes.png"), tr("&Ellipse (Axis)"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseAxis()));
    a_map["DrawEllipseAxis"] = action;

    action = new QAction(QIcon(":/extui/ellipsearcsaxes.png"), tr("Ellipse &Arc (Axis)"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseArcAxis()));
    a_map["DrawEllipseArcAxis"] = action;

    action = new QAction(QIcon(":/extui/ellipsefocipoint.png"), tr("Ellipse &Foci Point"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseFociPoint()));
    a_map["DrawEllipseFociPoint"] = action;

    action = new QAction(tr("Ellipse &4 Point"), tools);
    action->setIcon(QIcon(":/extui/ellipse4points.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipse4Points()));
    a_map["DrawEllipse4Points"] = action;

    action = new QAction(QIcon(":/extui/ellipsecenter3points.png"), tr("Ellipse Center and &3 Points"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseCenter3Points()));
    a_map["DrawEllipseCenter3Points"] = action;

    action = new QAction(QIcon(":/extui/ellipseinscribed.png"), tr("Ellipse &Inscribed"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseInscribe()));
    a_map["DrawEllipseInscribe"] = action;

    // <[~ Spline ~]>

    action = new QAction(QIcon(":/extui/menuspline.png"), tr("&Spline"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawSpline()));
    a_map["DrawSpline"] = action;

    action = new QAction(QIcon(":/extui/menusplinepoints.png"), tr("&Spline through points"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawSplinePoints()));
    a_map["DrawSplinePoints"] = action;

    // <[~ Polyline ~]>

    action = new QAction(QIcon(":/extui/polyline.png"), tr("&Polyline"), tools);
    action->setStatusTip(tr("Draw polylines"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawPolyline()));
    a_map["DrawPolyline"] = action;

    action = new QAction(tr("&Add node"), tools);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylineadd.png"));
    action->setStatusTip(tr("Add polyline's node"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineAdd()));
    a_map["PolylineAdd"] = action;

    action = new QAction(tr("A&ppend node"), tools);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylineappend.png"));
    action->setStatusTip(tr("Append polyline's node"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineAppend()));
    a_map["PolylineAppend"] = action;

    action = new QAction(tr("&Delete node"), tools);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinedel.png"));
    action->setStatusTip(tr("Delete polyline's node"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineDel()));
    a_map["PolylineDel"] = action;

    action = new QAction(tr("Delete &between two nodes"), tools);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinedelbetween.png"));
    action->setStatusTip(tr("Delete between two nodes"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineDelBetween()));
    a_map["PolylineDelBetween"] = action;

    action = new QAction(tr("&Trim segments"), tools);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinetrim.png"));
    action->setStatusTip(tr("Trim polyline's segments"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineTrim()));
    a_map["PolylineTrim"] = action;

    action = new QAction(QIcon(":/extui/polylineequidstant.png"), tr("Create &Equidistant Polylines"), tools);
    action->setStatusTip(tr("Create Equidistant Polylines"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineEquidistant()));
    a_map["PolylineEquidistant"] = action;

    action = new QAction(tr("Create Polyline from Existing &Segments"), tools);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinesegment.png"));
    action->setStatusTip(tr("Create Polyline from Existing Segments"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineSegment()));
    a_map["PolylineSegment"] = action;

    // <[~ Misc ~]>

    action = new QAction(QIcon(":/extui/menutext.png"), tr("&MText"), tools);
    action->setData(RS2::ActionDrawMText);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawMText()));
    a_map["DrawMText"] = action;

    action = new QAction(tr("&Text"), tools);
    action->setIcon(QIcon(":/extui/menutext.png"));
    action->setData(RS2::ActionDrawText);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawText()));
    a_map["DrawText"] = action;

    action = new QAction(QIcon(":/extui/menuhatch.png"), tr("&Hatch"), tools);
    action->setData(RS2::ActionDrawHatch);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawHatch()));
    a_map["DrawHatch"] = action;

    action = new QAction(QIcon(":/extui/menuimage.png"), tr("Insert &Image"), tools);
    action->setData(RS2::ActionDrawImage);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawImage()));
    a_map["DrawImage"] = action;

    // <[~ Dimension ~]>

    action = new QAction(tr("&Aligned"), tools);
    action->setIcon(QIcon(":/extui/dimaligned.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimAligned()));
    a_map["DimAligned"] = action;

    action = new QAction(tr("&Linear"), tools);
    action->setIcon(QIcon(":/extui/dimlinear.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinear()));
    a_map["DimLinear"] = action;

    action = new QAction(tr("&Horizontal"), tools);
    action->setIcon(QIcon(":/extui/dimhor.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinearHor()));
    a_map["DimLinearHor"] = action;

    action = new QAction(tr("&Vertical"), tools);
    action->setIcon(QIcon(":/extui/dimver.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinearVer()));
    a_map["DimLinearVer"] = action;

    action = new QAction(tr("&Radial"), tools);
    action->setIcon(QIcon(":/extui/dimradial.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimRadial()));
    a_map["DimRadial"] = action;

    action = new QAction(tr("&Diametric"), tools);
    action->setIcon(QIcon(":/extui/dimdiametric.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimDiametric()));
    a_map["DimDiametric"] = action;

    action = new QAction(QIcon(":/extui/dimangular.png"), tr("&Angular"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimAngular()));
    a_map["DimAngular"] = action;

    action = new QAction(QIcon(":/extui/dimleader.png"), tr("&Leader"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLeader()));
    a_map["DimLeader"] = action;

    // <[~ Modify ~]>

    action = new QAction(tr("&Attributes"), tools);
    action->setIcon(QIcon(":/extui/modifyattributes.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyAttributes()));
    a_map["ModifyAttributes"] = action;

    action = new QAction(QIcon(":/extui/modifydelete.png"), tr("&Delete"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDelete()));
    a_map["ModifyDelete"] = action;

    action = new QAction(tr("Delete Freehand"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDeleteFree()));
    a_map["ModifyDeleteFree"] = action;

    action = new QAction(QIcon(":/extui/modifymove.png"), tr("&Move / Copy"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMove()));
    a_map["ModifyMove"] = action;

    action = new QAction(tr("Re&vert direction"), tools);
    action->setIcon(QIcon(":/extui/reverse.png"));
    action->setShortcut(QKeySequence(tr("Ctrl+R")));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotModifyRevertDirection()));
    a_map["ModifyRevertDirection"] = action;

    action = new QAction(QIcon(":/extui/modifyrotate.png"), tr("&Rotate"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRotate()));
    a_map["ModifyRotate"] = action;

    action = new QAction(QIcon(":/extui/modifyscale.png"), tr("&Scale"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyScale()));
    a_map["ModifyScale"] = action;

    action = new QAction(QIcon(":/extui/modifymirror.png"), tr("&Mirror"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMirror()));
    a_map["ModifyMirror"] = action;

    action = new QAction(QIcon(":/extui/modifymoverotate.png"), tr("M&ove and Rotate"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMoveRotate()));
    a_map["ModifyMoveRotate"] = action;

    action = new QAction(QIcon(":/extui/modifyrotate2.png"), tr("Rotate T&wo"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRotate2()));
    a_map["ModifyRotate2"] = action;

    action = new QAction(tr("&Properties"), tools);
    action->setIcon(QIcon(":/extui/modifyentity.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyEntity()));
    a_map["ModifyEntity"] = action;

    action = new QAction(tr("&Trim"), tools);
    action->setIcon(QIcon(":/extui/modifytrim.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrim()));
    a_map["ModifyTrim"] = action;

    action = new QAction(tr("&Trim Two"), tools);
    action->setIcon(QIcon(":/extui/modifytrim2.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrim2()));
    a_map["ModifyTrim2"] = action;

    action = new QAction(tr("&Lengthen"), tools);
    action->setIcon(QIcon(":/extui/modifytrimamount.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrimAmount()));
    a_map["ModifyTrimAmount"] = action;

    action = new QAction(QIcon(":/extui/arcspara.png"), tr("&Offset"),tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyOffset()));
    a_map["ModifyOffset"] = action;

    action = new QAction(tr("&Divide"), tools);
    action->setIcon(QIcon(":/extui/modifycut.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyCut()));
    a_map["ModifyCut"] = action;

    action = new QAction(QIcon(":/extui/modifystretch.png"), tr("&Stretch"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyStretch()));
    a_map["ModifyStretch"] = action;

    action = new QAction(QIcon(":/extui/modifybevel.png"), tr("&Bevel"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyBevel()));
    a_map["ModifyBevel"] = action;

    action = new QAction(QIcon(":/extui/modifyround.png"), tr("&Fillet"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRound()));
    a_map["ModifyRound"] = action;

    action = new QAction(tr("&Explode Text into Letters"), tools);
    action->setIcon(QIcon(":/extui/modifyexplodetext.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyExplodeText()));
    a_map["ModifyExplodeText"] = action;

    // <[~ Info ~]>

    action = new QAction(tr("Point inside contour"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoInside()));
    a_map["InfoInside"] = action;

    action = new QAction(QIcon(":/extui/restricthorizontal.png"), tr("&Distance Point to Point"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoDist()));
    a_map["InfoDist"] = action;

    action = new QAction(QIcon(":/extui/infodist2.png"), tr("&Distance Entity to Point"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoDist2()));
    a_map["InfoDist2"] = action;

    action = new QAction(QIcon(":/extui/infoangle.png"), tr("An&gle between two lines"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoAngle()));
    a_map["InfoAngle"] = action;

    action = new QAction(QIcon(":/extui/infototallength.png"), tr("&Total length of selected entities"), tools);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoTotalLength()));
    a_map["InfoTotalLength"] = action;

    action = new QAction(tr("Polygonal &Area"), tools);
    action->setIcon(QIcon(":/extui/infoarea.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoArea()));
    a_map["InfoArea"] = action;

    for (auto item : a_map)
    {
        item.second->setCheckable(true);
    }

    // =============================
    // <[~ not checkable actions ~]>
    // =============================

    // <[~ Edit ~]>

    action = new QAction(tr("&Selection pointer"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("go-previous-view", QIcon(":/actions/back.png")));
    #else
    action->setIcon(QIcon(":/actions/back.png"));
    #endif
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditKillAllActions()));
    a_map["EditKillAllActions"] = action;

    action = new QAction(tr("&Undo"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/actions/undo2.png")));
    #else
    action->setIcon(QIcon(":/actions/undo2.png"));
    #endif
    action->setShortcut(QKeySequence::Undo);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditUndo()));
    a_map["EditUndo"] = action;

    action = new QAction(tr("&Redo"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/actions/redo2.png")));
    #else
    action->setIcon(QIcon(":/actions/redo2.png"));
    #endif
    action->setShortcut(QKeySequence::Redo);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditRedo()));
    a_map["EditRedo"] = action;

    action = new QAction(tr("Cu&t"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/actions/editcut2.png")));
    #else
    action->setIcon(QIcon(":/actions/editcut2.png"));
    #endif
    action->setShortcut(QKeySequence::Cut);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditCut()));
    a_map["EditCut"] = action;

    action = new QAction(tr("&Copy"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/actions/editcopy2.png")));
    #else
    action->setIcon(QIcon(":/actions/editcopy2.png"));
    #endif
    action->setShortcut(QKeySequence::Copy);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditCopy()));
    a_map["EditCopy"] = action;

    action = new QAction(tr("&Paste"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/actions/editpaste2.png")));
    #else
    action->setIcon(QIcon(":/actions/editpaste2.png"));
    #endif
    action->setShortcut(QKeySequence::Paste);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditPaste()));
    a_map["EditPaste"] = action;

    action = new QAction(tr("move to bottom"), NULL);
    action->setIcon(QIcon(":/extui/order_bottom.png"));
    action->setStatusTip(tr("set to bottom"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderBottom()));
    a_map["OrderBottom"] = action;

    action = new QAction(tr("lower after entity"), NULL);
    action->setIcon(QIcon(":/extui/order_lower.png"));
    action->setStatusTip(tr("lower over entity"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderLower()));
    a_map["OrderLower"] = action;

    action = new QAction(tr("raise over entity"), NULL);
    action->setIcon(QIcon(":/extui/order_raise.png"));
    action->setStatusTip(tr("raise over entity"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderRaise()));
    a_map["OrderRaise"] = action;

    action = new QAction(tr("move to top"), NULL);
    action->setIcon(QIcon(":/extui/order_top.png"));
    action->setStatusTip(tr("set to top"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderTop()));
    a_map["OrderTop"] = action;

    // <[~ Layer ~]>

    action = new QAction(QIcon(":/ui/visibleblock.png"), tr("&Show all"), nullptr);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersDefreezeAll()));
    a_map["LayersDefreezeAll"] = action;

    action = new QAction(QIcon(":/ui/hiddenblock.png"), tr("&Hide all"), nullptr);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersFreezeAll()));
    a_map["LayersFreezeAll"] = action;

    action = new QAction(QIcon(":/ui/layeradd.png"), tr("&Add Layer"), NULL);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersAdd()));
    a_map["LayersAdd"] = action;

    action = new QAction(QIcon(":/ui/layerremove.png"), tr("&Remove Layer"), nullptr);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersRemove()));
    a_map["LayersRemove"] = action;

    action = new QAction(QIcon(":/ui/layeredit.png"), tr("&Edit Layer"), NULL);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersEdit()));
    a_map["LayersEdit"] = action;

    action = new QAction(QIcon(":/ui/lockedlayer.png"), tr("Toggle Layer Loc&k"), nullptr);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersToggleLock()));
    a_map["LayersToggleLock"] = action;

    action = new QAction(tr("&Toggle Layer Visibility"), NULL);
    action->setIcon(QIcon(":/ui/layertoggle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersToggleView()));
    a_map["LayersToggleView"] = action;

    action = new QAction(tr("Toggle Layer &Print"), NULL);
    action->setIcon(QIcon(":/ui/fileprint.png"));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotLayersTogglePrint()));
    a_map["LayersTogglePrint"] = action;

    action = new QAction(tr("Toggle &Construction Layer"), NULL);
    action->setIcon(QIcon(":/ui/constructionlayer.png"));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotLayersToggleConstruction()));
    a_map["LayersToggleConstruction"] = action;

    // <[~ Block ~]>

    action = new QAction(tr("&Show all"), NULL);
    action->setIcon(QIcon(":/ui/blockdefreeze.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksDefreezeAll()));
    a_map["BlocksDefreezeAll"] = action;

    action= new QAction(tr("&Hide all"), NULL);
    action->setIcon(QIcon(":/ui/blockfreeze.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksFreezeAll()));
    a_map["BlocksFreezeAll"] = action;

    action = new QAction(QIcon(":/ui/blockadd.png"), tr("&Add Block"), nullptr);
    action->setData(RS2::ActionBlocksAdd);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksAdd()));
    a_map["BlocksAdd"] = action;

    action = new QAction(tr("&Remove Block"), NULL);
    action->setIcon(QIcon(":/ui/blockremove.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksRemove()));
    a_map["BlocksRemove"] = action;

    action = new QAction(tr("&Rename Block"), NULL);
    action->setIcon(QIcon(":/ui/blockattributes.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksAttributes()));
    a_map["BlocksAttributes"] = action;

    action = new QAction( tr("&Edit Block"), NULL);
    action->setIcon(QIcon(":/ui/blockedit.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksEdit()));
    a_map["BlocksEdit"] = action;

    action = new QAction( tr("&Save Block"), NULL);
    action->setIcon(QIcon(":/main/filesave.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksSave()));
    a_map["BlocksSave"] = action;

    action = new QAction(tr("&Insert Block"), NULL);
    action->setIcon(QIcon(":/ui/blockinsert.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksInsert()));
    a_map["BlocksInsert"] = action;

    action = new QAction(QIcon(":/ui/layertoggle.png"), tr("Toggle Block &Visibility"), nullptr);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksToggleView()));
    a_map["BlocksToggleView"] = action;

    action = new QAction(QIcon(":/extui/menublock.png"), tr("&Create Block"), nullptr);
    action->setData(RS2::ActionBlocksCreate);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksCreate()));
    a_map["BlocksCreate"] = action;

    action = new QAction(tr("Ex&plode"), NULL);
    action->setIcon(QIcon(":/extui/modifyexplode.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksExplode()));
    a_map["BlocksExplode"] = action;

    // <[~ Option ~]>

    action = new QAction(QIcon(":/actions/configure.png"),
    #ifdef __APPLE__
     tr("&Preferences"),
    #else
     tr("&Application Preferences")+"...",
    #endif
     main_window);

    connect(action, SIGNAL(triggered()),
    main_window, SLOT(slotOptionsGeneral()));
    a_map["OptionsGeneral"] = action;

    action = new QAction( QIcon(":/actions/drawingprefs.png"), tr("Current &Drawing Preferences"), NULL);
    // Preferences shortcut was itroduced on 4.6
    #if QT_VERSION >= 0x040600
    action->setShortcut(QKeySequence::Preferences);
    #endif
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOptionsDrawing()));
    a_map["OptionsDrawing"] = action;

    action = new QAction(tr("Open IDE"), main_window);
    connect(action, SIGNAL(triggered()),
    main_window, SLOT(slotScriptOpenIDE()));
    a_map["ScriptOpenIDE"] = action;

    action = new QAction(tr("Run Script.."), main_window);
    connect(action, SIGNAL(triggered()),
    main_window, SLOT(slotScriptRun()));
    a_map["ScriptRun"] = action;

    // <[~ Modify ~]>

    action = new QAction(tr("&Delete selected"), main_window);
    action->setIcon(QIcon(":/extui/modifydelete.png"));
    action->setShortcut(QKeySequence::Delete);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDeleteQuick()));
    a_map["ModifyDeleteQuick"] = action;

    action = new QAction(tr("Select &All"), main_window);
    action->setShortcut(QKeySequence::SelectAll);
    action->setIcon(QIcon(":/extui/selectall.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectAll()));
    a_map["SelectAll"] = action;

    // <[~ Select ~]>

    action = new QAction(tr("Deselect &all"), main_window);
    // RVT April 29, 2011 - Added esc key to de-select all entities
    action->setShortcuts(QList<QKeySequence>() << QKeySequence(tr("Ctrl+K")));
    action->setIcon(QIcon(":/extui/selectnothing.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectAll()));
    a_map["DeselectAll"] = action;

    action = new QAction(tr("Invert Selection"), NULL);
    action->setIcon(QIcon(":/extui/selectinvert.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectInvert()));
    a_map["SelectInvert"] = action;

    // <[~ Misc ~]>

    action = new QAction(tr("Export as &MakerCAM SVG..."), main_window);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotFileExportMakerCam()));
    a_map["FileExportMakerCam"] = action;

    action = new QAction(tr("Regenerate Dimension Entities"), main_window);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotToolRegenerateDimensions()));
    a_map["ToolRegenerateDimensions"] = action;

    // <[~ Zoom ~]>

    action = new QAction(tr("Zoom &In"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/actions/zoomin.png")));
    #else
    action->setIcon(QIcon(":/actions/zoomin.png"));
    #endif
    action->setShortcut(QKeySequence::ZoomIn);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomIn()));
    a_map["ZoomIn"] = action;

    action = new QAction(tr("Zoom &Out"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/actions/zoomout.png")));
    #else
    action->setIcon(QIcon(":/actions/zoomout.png"));
    #endif
    action->setShortcut(QKeySequence::ZoomOut);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomOut()));
    a_map["ZoomOut"] = action;

    action = new QAction(tr("&Auto Zoom"), NULL);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/actions/zoomauto.png")));
    #else
    action->setIcon(QIcon(":/actions/zoomauto.png"));
    #endif
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomAuto()));
    a_map["ZoomAuto"] = action;

    action = new QAction(tr("Previous &View"), NULL);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("zoom-previous", QIcon(":/actions/zoomprevious.png")));
    #else
    action->setIcon(QIcon(":/actions/zoomprevious.png"));
    #endif
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomPrevious()));
    action->setEnabled(false);
    a_map["ZoomPrevious"] = action;

    action = new QAction(tr("&Redraw"), NULL);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/actions/zoomredraw.png")));
    #else
    action->setIcon(QIcon(":/actions/zoomredraw.png"));
    #endif
    action->setShortcut(QKeySequence::Refresh);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomRedraw()));
    a_map["ZoomRedraw"] = action;

    // ===========================
    // <[~ Main Window Actions ~]>
    // ===========================

    action = new QAction(tr("&New"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("document-new", QIcon(":/actions/filenew.png")));
    #else
    action->setIcon(QIcon(":/actions/filenew.png"));
    #endif
    action->setShortcut(QKeySequence::New);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileNewNew()));
    a_map["FileNew"] = action;

    action = new QAction(tr("New From &Template"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("document-new", QIcon(":/actions/filenew.png")));
    #else
    action->setIcon(QIcon(":/actions/filenew.png"));
    #endif
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileNewTemplate()));
    a_map["FileNewTemplate"] = action;

    action = new QAction(tr("&Open..."), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("document-open", QIcon(":/actions/fileopen2.png")));
    #else
    action->setIcon(QIcon(":/actions/fileopen2.png"));
    #endif
    action->setShortcut(QKeySequence::Open);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileOpen()));
    a_map["FileOpen"] = action;

    action = new QAction(tr("&Save"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("document-save", QIcon(":/actions/filesave2.png")));
    #else
    action->setIcon(QIcon(":/actions/filesave2.png"));
    #endif
    action->setShortcut(QKeySequence::Save);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileSave()));
    a_map["FileSave"] = action;

    action = new QAction(tr("Save &as..."), NULL);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/actions/filesaveas.png")));
    #else
    action->setIcon(QIcon(":/actions/filesaveas.png"));
    #endif
    // SaveAs was itroduces at 4.5 and later
    #if QT_VERSION >= 0x040500
    action->setShortcut(QKeySequence::SaveAs);
    #endif
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileSaveAs()));
    a_map["FileSaveAs"] = action;

    action = new QAction( QIcon(":/actions/fileexport.png"), tr("&Export as image"), NULL);
    connect(action, SIGNAL( triggered()), main_window, SLOT(slotFileExport()));
    a_map["FileExport"] = action;

    action = new QAction(tr("&Close"), main_window);
    action->setIcon(QIcon(":/actions/fileclose.png"));
    action->setShortcut(QKeySequence::Close);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileClose()));
    action->setShortcutContext(Qt::WidgetShortcut);
    a_map["FileClose"] = action;

    action = new QAction(tr("&Print..."), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("document-print", QIcon(":/actions/fileprint.png")));
    #else
    action->setIcon(QIcon(":/actions/fileprint.png"));
    #endif
    action->setShortcut(QKeySequence::Print);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFilePrint()));
    connect(main_window, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    a_map["FilePrint"] = action;

    action = new QAction(tr("Export as PDF"), main_window);
    action->setIcon(QIcon(":/actions/fileexportpdf.png"));
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFilePrintPDF()));
    a_map["FilePrintPDF"] = action;

    action = new QAction(tr("Print Pre&view"), NULL);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("document-print-preview", QIcon(":/actions/fileprintpreview.png")));
    #else
    action->setIcon(QIcon(":/actions/fileprintpreview.png"));
    #endif
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), main_window, SLOT(slotFilePrintPreview(bool)));
    connect(main_window, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    a_map["FilePrintPreview"] = action;

    action = new QAction(tr("&Quit"), main_window);
    #if QT_VERSION >= 0x040600
    action->setIcon(QIcon::fromTheme("application-exit", QIcon(":/actions/exit.png")));
    action->setShortcut(QKeySequence::Quit);
    #else
    action->setIcon(QIcon(":/actions/exit.png"));
    #endif
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileQuit()));
    a_map["FileQuit"] = action;

    action = new QAction(QIcon(":/ui/blockinsert.png"), tr("&Block"), main_window);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotImportBlock()));
    a_map["BlocksImport"] = action;

    // <[~ View ~]>

    action = new QAction(tr("&Grid"), main_window);
    action->setIcon(QIcon(":/actions/viewgrid.png"));
    action->setShortcut(QKeySequence(tr("Ctrl+G", "Toggle Grid")));
    action->setCheckable(true);
    action->setChecked(true);
    connect(main_window, SIGNAL(gridChanged(bool)), action, SLOT(setChecked(bool)));
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewGrid(bool)));
    a_map["ViewGrid"] = action;

    action = new QAction(tr("&Draft"), main_window);
    action->setIcon(QIcon(":/actions/viewdraft.png"));
    action->setChecked(true);
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewDraft(bool)));
    connect(main_window, SIGNAL(draftChanged(bool)), action, SLOT(setChecked(bool)));
    a_map["ViewDraft"] = action;

    action = new QAction(tr("&Statusbar"), main_window);
    action->setCheckable(true);
    action->setChecked(true);
    action->setShortcut(QKeySequence(tr("Ctrl+I", "Hide Statusbar")));
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewStatusBar(bool)));
    a_map["ViewStatusBar"] = action;

    return a_map;
}
