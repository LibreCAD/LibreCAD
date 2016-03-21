/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
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

// This file was first published at: github.com/r-a-v-a-s/LibreCAD.git

// lc_actionfactory is a rewrite of qg_actionfactory; some copied content remains.
// qg_actionfactory contributors:
// Andrew Mustun, Claude Sylvain, R. van Twisk, Dongxu Li, Rallaz, Armin Stebich, ravas, korhadris

#include "lc_actionfactory.h"
#include <QAction>
#include <QActionGroup>

LC_ActionFactory::LC_ActionFactory(QObject* parent, QObject* a_handler)
    : QObject(parent)
    , tool_group(new QActionGroup(parent))
    , disable_group(new QActionGroup(parent))
    , main_window(parent)
    , action_handler(a_handler)
{
    disable_group->setExclusive(false);

    connect(parent, SIGNAL(windowsChanged(bool)),
            disable_group, SLOT(setEnabled(bool)));
    connect(parent, SIGNAL(windowsChanged(bool)),
            tool_group, SLOT(setEnabled(bool)));

    connect(tool_group, SIGNAL(triggered(QAction*)),
            parent, SLOT(relayAction(QAction*)));
}

void LC_ActionFactory::fillActionContainer(QMap<QString, QAction*>& a_map)
{
    QAction* action;

    // <[~ Zoom ~]>

    action = new QAction(tr("&Window Zoom"), tool_group);
    action->setIcon(QIcon::fromTheme("zoom-select", QIcon(":/icons/zoom_window.svg")));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomWindow()));
    action->setObjectName("ZoomWindow");
    a_map["ZoomWindow"] = action;

    action = new QAction(tr("Zoom &Panning"), tool_group);
    action->setIcon(QIcon(":/icons/zoom_pan_b.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomPan()));
    action->setObjectName("ZoomPan");
    a_map["ZoomPan"] = action;

    // <[~ Select ~]>

    action = new QAction(tr("Select Entity"), tool_group);
    action->setIcon(QIcon(":/extui/selectsingle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectSingle()));
    action->setObjectName("SelectSingle");
    a_map["SelectSingle"] = action;

    action = new QAction(tr("Select Window"), tool_group);
    action->setIcon(QIcon(":/extui/selectwindow.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectWindow()));
    action->setObjectName("SelectWindow");
    a_map["SelectWindow"] = action;

    action = new QAction(tr("Deselect Window"), tool_group);
    action->setIcon(QIcon(":/extui/deselectwindow.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectWindow()));
    action->setObjectName("DeselectWindow");
    a_map["DeselectWindow"] = action;

    action = new QAction(tr("(De-)Select &Contour"), tool_group);
    action->setIcon(QIcon(":/extui/selectcontour.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectContour()));
    action->setObjectName("SelectContour");
    a_map["SelectContour"] = action;

    action = new QAction(tr("Select Intersected Entities"), tool_group);
    action->setIcon(QIcon(":/extui/selectinters.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectIntersected()));
    action->setObjectName("SelectIntersected");
    a_map["SelectIntersected"] = action;

    action = new QAction(tr("Deselect Intersected Entities"), tool_group);
    action->setIcon(QIcon(":/extui/deselectinters.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectIntersected()));
    action->setObjectName("DeselectIntersected");
    a_map["DeselectIntersected"] = action;

    action = new QAction(tr("(De-)Select Layer"), tool_group);
    action->setIcon(QIcon(":/extui/selectlayer.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectLayer()));
    action->setObjectName("SelectLayer");
    a_map["SelectLayer"] = action;

    // <[~ Draw ~]>

    action = new QAction(tr("&Points"), tool_group);
    action->setIcon(QIcon(":/extui/points.png"));

    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawPoint()));
    action->setObjectName("DrawPoint");
    a_map["DrawPoint"] = action;

    // <[~ Line ~]>

    action = new QAction(tr("&2 Points"), tool_group);
    action->setIcon(QIcon(":/extui/linesnormal.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLine()));
    action->setObjectName("DrawLine");
    a_map["DrawLine"] = action;

    action = new QAction(QIcon(":/extui/linesangle.png"), tr("&Angle"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineAngle()));
    action->setObjectName("DrawLineAngle");
    a_map["DrawLineAngle"] = action;

    action = new QAction(QIcon(":/extui/lineshor.png"), tr("&Horizontal"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineHorizontal()));
    action->setObjectName("DrawLineHorizontal");
    a_map["DrawLineHorizontal"] = action;

    action = new QAction(QIcon(":/extui/linesver.png"), tr("Vertical"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineVertical()));
    action->setObjectName("DrawLineVertical");
    a_map["DrawLineVertical"] = action;

    action = new QAction(tr("Vertical"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineHorVert()));
    action->setObjectName("DrawLineHorVert");
    a_map["DrawLineHorVert"] = action;

    action = new QAction(tr("&Freehand Line"), tool_group);
    action->setIcon(QIcon(":/extui/linesfree.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineFree()));
    action->setObjectName("DrawLineFree");
    a_map["DrawLineFree"] = action;

    action = new QAction(tr("&Parallel"), tool_group);
    action->setIcon(QIcon(":/extui/linespara.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineParallel()));
    action->setObjectName("DrawLineParallel");
    a_map["DrawLineParallel"] = action;

    action = new QAction(tr("Parallel through point"), tool_group);
    action->setIcon(QIcon(":/extui/linesparathrough.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineParallelThrough()));
    action->setObjectName("DrawLineParallelThrough");
    a_map["DrawLineParallelThrough"] = action;

    action = new QAction(tr("Rectangle"), tool_group);
    action->setIcon(QIcon(":/extui/linesrect.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineRectangle()));
    action->setObjectName("DrawLineRectangle");
    a_map["DrawLineRectangle"] = action;

    action = new QAction(QIcon(":/extui/linesbisector.png"),tr("Bisector"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineBisector()));
    action->setObjectName("DrawLineBisector");
    a_map["DrawLineBisector"] = action;

    action = new QAction(QIcon(":/extui/linestan1.png"), tr("Tangent (P,C)"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineTangent1()));
    action->setObjectName("DrawLineTangent1");
    a_map["DrawLineTangent1"] = action;

    action = new QAction(QIcon(":/extui/linestan2.png"), tr("Tangent (C,C)"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineTangent2()));
    action->setObjectName("DrawLineTangent2");
    a_map["DrawLineTangent2"] = action;

    action = new QAction(tr("Tangent &Orthogonal"), tool_group);
    action->setIcon(QIcon(":/extui/linesorthtan.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineOrthTan()));
    action->setObjectName("DrawLineOrthTan");
    a_map["DrawLineOrthTan"] = action;

    action = new QAction(tr("Orthogonal"), tool_group);
    action->setIcon(QIcon(":/extui/linesorthogonal.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineOrthogonal()));
    action->setObjectName("DrawLineOrthogonal");
    a_map["DrawLineOrthogonal"] = action;

    action = new QAction(tr("Relative angle"), tool_group);
    action->setIcon(QIcon(":/extui/linesrelativeangle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLineRelAngle()));
    action->setObjectName("DrawLineRelAngle");
    a_map["DrawLineRelAngle"] = action;

    action = new QAction(tr("Pol&ygon (Cen,Cor)"), tool_group);
    action->setIcon(QIcon(":/extui/linespolygon.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLinePolygon()));
    action->setObjectName("DrawLinePolygonCenCor");
    a_map["DrawLinePolygonCenCor"] = action;

    action = new QAction(tr("Polygo&n (Cor,Cor)"), tool_group);
    action->setIcon(QIcon(":/extui/linespolygon2.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawLinePolygon2()));
    action->setObjectName("DrawLinePolygonCorCor");
    a_map["DrawLinePolygonCorCor"] = action;

    // <[~ Circle ~]>

    action = new QAction(QIcon(":/extui/circles.png"), tr("Center, &Point"), tool_group);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotDrawCircle()));
    action->setObjectName("DrawCircle");
    a_map["DrawCircle"] = action;

    action = new QAction(tr("Center, &Radius"), tool_group);
    action->setIcon(QIcon(":/extui/circlescr.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleCR()));
    action->setObjectName("DrawCircleCR");
    a_map["DrawCircleCR"] = action;

    action = new QAction(tr("2 Points"), tool_group);
    action->setIcon(QIcon(":/extui/circles2p.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle2P()));
    action->setObjectName("DrawCircle2P");
    a_map["DrawCircle2P"] = action;

    action = new QAction(QIcon(":/extui/circle2pr.png"), tr("2 Points, Radius"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle2PR()));
    action->setObjectName("DrawCircle2PR");
    a_map["DrawCircle2PR"] = action;

    action = new QAction(QIcon(":/extui/circles3p.png"), tr("3 Points"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircle3P()));
    action->setObjectName("DrawCircle3P");
    a_map["DrawCircle3P"] = action;

    action = new QAction(tr("&Concentric"), tool_group);
    action->setIcon(QIcon(":/extui/circlespara.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleParallel()));
    action->setObjectName("DrawCircleParallel");
    a_map["DrawCircleParallel"] = action;

    action = new QAction(QIcon(":/extui/circleinscribe.png"), tr("Circle &Inscribed"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleInscribe()));
    action->setObjectName("DrawCircleInscribe");
    a_map["DrawCircleInscribe"] = action;

    action = new QAction(tr("Tangential 2 Circles, Radius",  "circle tangential with two circles, and given radius"), tool_group);
    action->setIcon(QIcon(":/extui/circletan2.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan2()));
    action->setObjectName("DrawCircleTan2");
    a_map["DrawCircleTan2"] = action;

    action = new QAction(tr("Tangential 2 Circles, 1 Point"), tool_group);
    action->setIcon(QIcon(":/extui/circletan2_1p.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan2_1P()));
    action->setObjectName("DrawCircleTan2_1P");
    a_map["DrawCircleTan2_1P"] = action;

    action = new QAction(tr("Tangential &3 Circles"), tool_group);
    action->setIcon(QIcon(":/extui/circletan3.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan3()));
    action->setObjectName("DrawCircleTan3");
    a_map["DrawCircleTan3"] = action;

    action = new QAction(tr("Tangential, 2 P&oints"), tool_group);
    action->setIcon(QIcon(":/extui/circletan1_2p.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawCircleTan1_2P()));
    action->setObjectName("DrawCircleTan1_2P");
    a_map["DrawCircleTan1_2P"] = action;

    // <[~ Arc ~]>

    action = new QAction(tr("&Center, Point, Angles"), tool_group);
    action->setIcon(QIcon(":/extui/arcscraa.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArc()));
    action->setObjectName("DrawArc");
    a_map["DrawArc"] = action;

    action = new QAction(tr("&3 Points"), tool_group);
    action->setIcon(QIcon(":/extui/arcs3p.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArc3P()));
    action->setObjectName("DrawArc3P");
    a_map["DrawArc3P"] = action;

    action = new QAction(tr("&Concentric"), tool_group);
    action->setIcon(QIcon(":/extui/arcspara.png"));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArcParallel()));
    action->setObjectName("DrawArcParallel");
    a_map["DrawArcParallel"] = action;

    action = new QAction(QIcon(":/extui/arcstangential.png"), tr("Arc &Tangential"), tool_group);
    action->setCheckable(true);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawArcTangential()));
    action->setObjectName("DrawArcTangential");
    a_map["DrawArcTangential"] = action;

    // <[~ Ellipse ~]>

    action = new QAction(QIcon(":/extui/ellipsesaxes.png"), tr("&Ellipse (Axis)"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseAxis()));
    action->setObjectName("DrawEllipseAxis");
    a_map["DrawEllipseAxis"] = action;

    action = new QAction(QIcon(":/extui/ellipsearcsaxes.png"), tr("Ellipse &Arc (Axis)"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseArcAxis()));
    action->setObjectName("DrawEllipseArcAxis");
    a_map["DrawEllipseArcAxis"] = action;

    action = new QAction(QIcon(":/extui/ellipsefocipoint.png"), tr("Ellipse &Foci Point"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseFociPoint()));
    action->setObjectName("DrawEllipseFociPoint");
    a_map["DrawEllipseFociPoint"] = action;

    action = new QAction(tr("Ellipse &4 Point"), tool_group);
    action->setIcon(QIcon(":/extui/ellipse4points.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipse4Points()));
    action->setObjectName("DrawEllipse4Points");
    a_map["DrawEllipse4Points"] = action;

    action = new QAction(QIcon(":/extui/ellipsecenter3points.png"), tr("Ellipse Center and &3 Points"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseCenter3Points()));
    action->setObjectName("DrawEllipseCenter3Points");
    a_map["DrawEllipseCenter3Points"] = action;

    action = new QAction(QIcon(":/extui/ellipseinscribed.png"), tr("Ellipse &Inscribed"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawEllipseInscribe()));
    action->setObjectName("DrawEllipseInscribe");
    a_map["DrawEllipseInscribe"] = action;

    // <[~ Spline ~]>

    action = new QAction(QIcon(":/extui/menuspline.png"), tr("&Spline"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawSpline()));
    action->setObjectName("DrawSpline");
    a_map["DrawSpline"] = action;

    action = new QAction(QIcon(":/extui/menusplinepoints.png"), tr("&Spline through points"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawSplinePoints()));
    action->setObjectName("DrawSplinePoints");
    a_map["DrawSplinePoints"] = action;

    // <[~ Polyline ~]>

    action = new QAction(QIcon(":/extui/polyline.png"), tr("&Polyline"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawPolyline()));
    action->setObjectName("DrawPolyline");
    a_map["DrawPolyline"] = action;

    action = new QAction(tr("&Add node"), tool_group);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylineadd.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineAdd()));
    action->setObjectName("PolylineAdd");
    a_map["PolylineAdd"] = action;

    action = new QAction(tr("A&ppend node"), tool_group);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylineappend.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineAppend()));
    action->setObjectName("PolylineAppend");
    a_map["PolylineAppend"] = action;

    action = new QAction(tr("&Delete node"), tool_group);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinedel.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineDel()));
    action->setObjectName("PolylineDel");
    a_map["PolylineDel"] = action;

    action = new QAction(tr("Delete &between two nodes"), tool_group);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinedelbetween.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineDelBetween()));
    action->setObjectName("PolylineDelBetween");
    a_map["PolylineDelBetween"] = action;

    action = new QAction(tr("&Trim segments"), tool_group);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinetrim.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineTrim()));
    action->setObjectName("PolylineTrim");
    a_map["PolylineTrim"] = action;

    action = new QAction(QIcon(":/extui/polylineequidstant.png"), tr("Create &Equidistant Polylines"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineEquidistant()));
    action->setObjectName("PolylineEquidistant");
    a_map["PolylineEquidistant"] = action;

    action = new QAction(tr("Create Polyline from Existing &Segments"), tool_group);
    action->setShortcut(QKeySequence());
    action->setIcon(QIcon(":/extui/polylinesegment.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotPolylineSegment()));
    action->setObjectName("PolylineSegment");
    a_map["PolylineSegment"] = action;

    // <[~ Misc ~]>

    action = new QAction(QIcon(":/extui/menutext.png"), tr("&MText"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawMText()));
    action->setObjectName("DrawMText");
    a_map["DrawMText"] = action;

    action = new QAction(tr("&Text"), tool_group);
    action->setIcon(QIcon(":/extui/menutext.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawText()));
    action->setObjectName("DrawText");
    a_map["DrawText"] = action;

    action = new QAction(QIcon(":/extui/menuhatch.png"), tr("&Hatch"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawHatch()));
    action->setObjectName("DrawHatch");
    a_map["DrawHatch"] = action;

    action = new QAction(tr("Insert &Image"), tool_group);
    action->setIcon(QIcon(":/icons/camera.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDrawImage()));
    action->setObjectName("DrawImage");
    a_map["DrawImage"] = action;

    // <[~ Dimension ~]>

    action = new QAction(tr("&Aligned"), tool_group);
    action->setIcon(QIcon(":/extui/dimaligned.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimAligned()));
    action->setObjectName("DimAligned");
    a_map["DimAligned"] = action;

    action = new QAction(tr("&Linear"), tool_group);
    action->setIcon(QIcon(":/extui/dimlinear.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinear()));
    action->setObjectName("DimLinear");
    a_map["DimLinear"] = action;

    action = new QAction(tr("&Horizontal"), tool_group);
    action->setIcon(QIcon(":/extui/dimhor.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinearHor()));
    action->setObjectName("DimLinearHor");
    a_map["DimLinearHor"] = action;

    action = new QAction(tr("&Vertical"), tool_group);
    action->setIcon(QIcon(":/extui/dimver.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLinearVer()));
    action->setObjectName("DimLinearVer");
    a_map["DimLinearVer"] = action;

    action = new QAction(tr("&Radial"), tool_group);
    action->setIcon(QIcon(":/extui/dimradial.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimRadial()));
    action->setObjectName("DimRadial");
    a_map["DimRadial"] = action;

    action = new QAction(tr("&Diametric"), tool_group);
    action->setIcon(QIcon(":/extui/dimdiametric.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimDiametric()));
    action->setObjectName("DimDiametric");
    a_map["DimDiametric"] = action;

    action = new QAction(QIcon(":/extui/dimangular.png"), tr("&Angular"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimAngular()));
    action->setObjectName("DimAngular");
    a_map["DimAngular"] = action;

    action = new QAction(QIcon(":/extui/dimleader.png"), tr("&Leader"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDimLeader()));
    action->setObjectName("DimLeader");
    a_map["DimLeader"] = action;

    // <[~ Modify ~]>

    action = new QAction(tr("&Attributes"), tool_group);
    action->setIcon(QIcon(":/extui/modifyattributes.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyAttributes()));
    action->setObjectName("ModifyAttributes");
    action->setData("modifyattr, attr, ma");
    a_map["ModifyAttributes"] = action;

    action = new QAction(QIcon(":/extui/modifydelete.png"), tr("&Delete"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDelete()));
    action->setObjectName("ModifyDelete");
    a_map["ModifyDelete"] = action;

    action = new QAction(tr("Delete Freehand"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDeleteFree()));
    action->setObjectName("ModifyDeleteFree");
    a_map["ModifyDeleteFree"] = action;

    action = new QAction(QIcon(":/extui/modifymove.png"), tr("&Move / Copy"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMove()));
    action->setObjectName("ModifyMove");
    action->setData("move, mv");
    a_map["ModifyMove"] = action;

    action = new QAction(tr("Re&vert direction"), tool_group);
    action->setIcon(QIcon(":/extui/reverse.png"));
    action->setShortcut(QKeySequence(tr("Ctrl+R")));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotModifyRevertDirection()));
    action->setObjectName("ModifyRevertDirection");
    action->setData("revert, rev");
    a_map["ModifyRevertDirection"] = action;

    action = new QAction(QIcon(":/icons/rotate.svg"), tr("&Rotate"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRotate()));
    action->setObjectName("ModifyRotate");
    action->setData("rotate, ro");
    a_map["ModifyRotate"] = action;

    action = new QAction(QIcon(":/extui/modifyscale.png"), tr("&Scale"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyScale()));
    action->setObjectName("ModifyScale");
    action->setData("scale, sz");
    a_map["ModifyScale"] = action;

    action = new QAction(QIcon(":/extui/modifymirror.png"), tr("&Mirror"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMirror()));
    action->setObjectName("ModifyMirror");
    action->setData("mirror, mi");
    a_map["ModifyMirror"] = action;

    action = new QAction(QIcon(":/extui/modifymoverotate.png"), tr("M&ove and Rotate"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyMoveRotate()));
    action->setObjectName("ModifyMoveRotate");
    a_map["ModifyMoveRotate"] = action;

    action = new QAction(QIcon(":/icons/rotate2.svg"), tr("Rotate T&wo"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRotate2()));
    action->setObjectName("ModifyRotate2");
    a_map["ModifyRotate2"] = action;

    action = new QAction(tr("&Properties"), tool_group);
    action->setIcon(QIcon(":/extui/modifyentity.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyEntity()));
    action->setObjectName("ModifyEntity");
    action->setData("properties, prop");
    a_map["ModifyEntity"] = action;

    action = new QAction(tr("&Trim"), tool_group);
    action->setIcon(QIcon(":/extui/modifytrim.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrim()));
    action->setObjectName("ModifyTrim");
    action->setData("trim, tm");
    a_map["ModifyTrim"] = action;

    action = new QAction(tr("&Trim Two"), tool_group);
    action->setIcon(QIcon(":/extui/modifytrim2.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrim2()));
    action->setObjectName("ModifyTrim2");
    action->setData("trim2, tm2");
    a_map["ModifyTrim2"] = action;

    action = new QAction(tr("&Lengthen"), tool_group);
    action->setIcon(QIcon(":/extui/modifytrimamount.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyTrimAmount()));
    action->setObjectName("ModifyTrimAmount");
    action->setData("lengthen, le");
    a_map["ModifyTrimAmount"] = action;

    action = new QAction(QIcon(":/extui/arcspara.png"), tr("&Offset"),tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyOffset()));
    action->setObjectName("ModifyOffset");
    action->setData("offset, o");
    a_map["ModifyOffset"] = action;

    action = new QAction(tr("&Divide"), tool_group);
    action->setIcon(QIcon(":/extui/modifycut.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyCut()));
    action->setObjectName("ModifyCut");
    action->setData("divide, cut, div");
    a_map["ModifyCut"] = action;

    action = new QAction(QIcon(":/extui/modifystretch.png"), tr("&Stretch"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyStretch()));
    action->setObjectName("ModifyStretch");
    action->setData("stretch, ss");
    a_map["ModifyStretch"] = action;

    action = new QAction(tr("&Bevel"), tool_group);
    action->setIcon(QIcon(":/icons/bevel.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyBevel()));
    action->setObjectName("ModifyBevel");
    action->setData("bevel, bev, ch");
    a_map["ModifyBevel"] = action;

    action = new QAction(QIcon(":/icons/fillet.svg"), tr("&Fillet"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyRound()));
    action->setObjectName("ModifyRound");
    action->setData("fillet, fi");
    a_map["ModifyRound"] = action;

    action = new QAction(tr("&Explode Text into Letters"), tool_group);
    action->setIcon(QIcon(":/extui/modifyexplodetext.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyExplodeText()));
    action->setObjectName("ModifyExplodeText");
    a_map["ModifyExplodeText"] = action;

    action = new QAction(tr("Ex&plode"), tool_group);
    action->setIcon(QIcon(":/extui/modifyexplode.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksExplode()));
    action->setObjectName("BlocksExplode");
    a_map["BlocksExplode"] = action;

    // <[~ Info ~]>

    action = new QAction(tr("Point inside contour"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoInside()));
    action->setObjectName("InfoInside");
    a_map["InfoInside"] = action;

    action = new QAction(tr("&Distance Point to Point"), tool_group);
    action->setIcon(QIcon(":/tools/distance_point_to_point.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoDist()));
    action->setObjectName("InfoDist");
    a_map["InfoDist"] = action;

    action = new QAction(QIcon(":/extui/infodist2.png"), tr("&Distance Entity to Point"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoDist2()));
    action->setObjectName("InfoDist2");
    a_map["InfoDist2"] = action;

    action = new QAction(QIcon(":/extui/infoangle.png"), tr("An&gle between two lines"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoAngle()));
    action->setObjectName("InfoAngle");
    a_map["InfoAngle"] = action;

    action = new QAction(QIcon(":/extui/infototallength.png"), tr("&Total length of selected entities"), tool_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotInfoTotalLength()));
    action->setObjectName("InfoTotalLength");
    a_map["InfoTotalLength"] = action;

    action = new QAction(tr("Polygonal &Area"), tool_group);
    action->setIcon(QIcon(":/extui/infoarea.png"));
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

    // <[~ Edit ~]>

    action = new QAction(tr("&Selection pointer"), disable_group);
    action->setIcon(QIcon::fromTheme("go-previous-view", QIcon(":/icons/cursor.svg")));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditKillAllActions()));
    action->setObjectName("EditKillAllActions");
    a_map["EditKillAllActions"] = action;

    action = new QAction(tr("&Undo"), disable_group);
    action->setIcon(QIcon::fromTheme("edit-undo", QIcon(":/actions/undo2.png")));
    action->setShortcut(QKeySequence::Undo);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditUndo()));
    action->setObjectName("EditUndo");
    a_map["EditUndo"] = action;

    action = new QAction(tr("&Redo"), disable_group);
    action->setIcon(QIcon::fromTheme("edit-redo", QIcon(":/actions/redo2.png")));
    action->setShortcut(QKeySequence::Redo);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditRedo()));
    action->setObjectName("EditRedo");
    a_map["EditRedo"] = action;

    action = new QAction(tr("Cu&t"), disable_group);
    action->setIcon(QIcon::fromTheme("edit-cut", QIcon(":/actions/editcut2.png")));
    action->setShortcut(QKeySequence::Cut);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditCut()));
    action->setObjectName("EditCut");
    a_map["EditCut"] = action;

    action = new QAction(tr("&Copy"), disable_group);
    action->setIcon(QIcon::fromTheme("edit-copy", QIcon(":/actions/editcopy2.png")));
    action->setShortcut(QKeySequence::Copy);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditCopy()));
    action->setObjectName("EditCopy");
    a_map["EditCopy"] = action;

    action = new QAction(tr("&Paste"), disable_group);
    action->setIcon(QIcon::fromTheme("edit-paste", QIcon(":/actions/editpaste2.png")));
    action->setShortcut(QKeySequence::Paste);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotEditPaste()));
    action->setObjectName("EditPaste");
    a_map["EditPaste"] = action;

    action = new QAction(tr("move to bottom"), disable_group);
    action->setShortcut(QKeySequence(Qt::Key_End));
    action->setIcon(QIcon(":/icons/downmost_b.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderBottom()));
    action->setObjectName("OrderBottom");
    a_map["OrderBottom"] = action;

    action = new QAction(tr("lower after entity"), disable_group);
    action->setShortcut(QKeySequence(Qt::Key_PageDown));
    action->setIcon(QIcon(":/icons/down_b.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderLower()));
    action->setObjectName("OrderLower");
    a_map["OrderLower"] = action;

    action = new QAction(tr("raise over entity"), disable_group);
    action->setShortcut(QKeySequence(Qt::Key_PageUp));
    action->setIcon(QIcon(":/icons/up_b.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderRaise()));
    action->setObjectName("OrderRaise");
    a_map["OrderRaise"] = action;

    action = new QAction(tr("move to top"), disable_group);
    action->setShortcut(QKeySequence(Qt::Key_Home));
    action->setIcon(QIcon(":/icons/upmost_b.svg"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOrderTop()));
    action->setObjectName("OrderTop");
    a_map["OrderTop"] = action;

    // <[~ Layer ~]>

    action = new QAction(QIcon(":/ui/visibleblock.png"),
                         tr("&Show all"), disable_group);
    connect(action, SIGNAL(triggered()),
            action_handler, SLOT(slotLayersDefreezeAll()));
    action->setObjectName("LayersDefreezeAll");
    a_map["LayersDefreezeAll"] = action;

    action = new QAction(QIcon(":/ui/hiddenblock.png"),
                         tr("&Hide all"), disable_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersFreezeAll()));
    action->setObjectName("LayersFreezeAll");
    a_map["LayersFreezeAll"] = action;

    action = new QAction(QIcon(":/ui/layeradd.png"),
                         tr("&Add Layer"), disable_group);
    action->setShortcut(QKeySequence("Ctrl+L"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersAdd()));
    action->setObjectName("LayersAdd");
    a_map["LayersAdd"] = action;

    action = new QAction(QIcon(":/ui/layerremove.png"),
                         tr("&Remove Layer"), disable_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersRemove()));
    action->setObjectName("LayersRemove");
    a_map["LayersRemove"] = action;

    action = new QAction(QIcon(":/ui/layeredit.png"),
                         tr("&Edit Layer"), disable_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersEdit()));
    action->setObjectName("LayersEdit");
    a_map["LayersEdit"] = action;

    action = new QAction(QIcon(":/ui/lockedlayer.png"),
                         tr("Toggle Layer Loc&k"), disable_group);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersToggleLock()));
    action->setObjectName("LayersToggleLock");
    a_map["LayersToggleLock"] = action;

    action = new QAction(tr("&Toggle Layer Visibility"), disable_group);
    action->setIcon(QIcon(":/ui/layertoggle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotLayersToggleView()));
    action->setObjectName("LayersToggleView");
    a_map["LayersToggleView"] = action;

    action = new QAction(tr("Toggle Layer &Print"), disable_group);
    action->setIcon(QIcon(":/ui/fileprint.png"));
    connect(action, SIGNAL(triggered()), action_handler,
            SLOT(slotLayersTogglePrint()));
    action->setObjectName("LayersTogglePrint");
    a_map["LayersTogglePrint"] = action;

    action = new QAction(tr("Toggle &Construction Layer"), disable_group);
    action->setIcon(QIcon(":/ui/constructionlayer.png"));
    connect(action, SIGNAL(triggered()),
            action_handler, SLOT(slotLayersToggleConstruction()));
    action->setObjectName("LayersToggleConstruction");
    a_map["LayersToggleConstruction"] = action;

    // <[~ Block ~]>

    action = new QAction(tr("&Show all"), disable_group);
    action->setIcon(QIcon(":/ui/blockdefreeze.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksDefreezeAll()));
    action->setObjectName("BlocksDefreezeAll");
    a_map["BlocksDefreezeAll"] = action;

    action= new QAction(tr("&Hide all"), disable_group);
    action->setIcon(QIcon(":/ui/blockfreeze.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksFreezeAll()));
    action->setObjectName("BlocksFreezeAll");
    a_map["BlocksFreezeAll"] = action;

    action = new QAction(QIcon(":/ui/blockadd.png"),
                         tr("&Add Block"), disable_group);

    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksAdd()));
    action->setObjectName("BlocksAdd");
    a_map["BlocksAdd"] = action;

    action = new QAction(tr("&Remove Block"), disable_group);
    action->setIcon(QIcon(":/ui/blockremove.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksRemove()));
    action->setObjectName("BlocksRemove");
    a_map["BlocksRemove"] = action;

    action = new QAction(tr("&Rename Block"), disable_group);
    action->setIcon(QIcon(":/ui/blockattributes.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksAttributes()));
    action->setObjectName("BlocksAttributes");
    a_map["BlocksAttributes"] = action;

    action = new QAction( tr("&Edit Block"), disable_group);
    action->setIcon(QIcon(":/ui/blockedit.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksEdit()));
    action->setObjectName("BlocksEdit");
    a_map["BlocksEdit"] = action;

    action = new QAction( tr("&Save Block"), disable_group);
    action->setIcon(QIcon(":/main/filesave.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksSave()));
    action->setObjectName("BlocksSave");
    a_map["BlocksSave"] = action;

    action = new QAction(tr("&Insert Block"), disable_group);
    action->setIcon(QIcon(":/ui/blockinsert.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksInsert()));
    action->setObjectName("BlocksInsert");
    a_map["BlocksInsert"] = action;

    action = new QAction(tr("Toggle Block &Visibility"), disable_group);
    action->setIcon(QIcon(":/ui/layertoggle.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksToggleView()));
    action->setObjectName("BlocksToggleView");
    a_map["BlocksToggleView"] = action;

    action = new QAction(tr("&Create Block"), disable_group);
    action->setIcon(QIcon(":/extui/menublock.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotBlocksCreate()));
    action->setObjectName("BlocksCreate");
    a_map["BlocksCreate"] = action;

    // <[~ Options ~]>

    action = new QAction(tr("&Application Preferences"), main_window);
    action->setIcon(QIcon(":/actions/configure.png"));
    connect(action, SIGNAL(triggered()),
    main_window, SLOT(slotOptionsGeneral()));
    action->setMenuRole(QAction::NoRole);
    action->setObjectName("OptionsGeneral");
    a_map["OptionsGeneral"] = action;

    action = new QAction(tr("Current &Drawing Preferences"), disable_group);
    action->setIcon(QIcon(":/actions/drawingprefs.png"));
    action->setShortcut(QKeySequence::Preferences);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotOptionsDrawing()));
    action->setObjectName("OptionsDrawing");
    a_map["OptionsDrawing"] = action;

    action = new QAction(tr("Widget Options"), disable_group);
    action->setObjectName("WidgetOptions");
    a_map["WidgetOptions"] = action;
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(widgetOptionsDialog()));

    action = new QAction(tr("Device Options"), disable_group);
    action->setObjectName("DeviceOptions");
    a_map["DeviceOptions"] = action;
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(showDeviceOptions()));

    // <[~ Modify ~]>

    action = new QAction(tr("&Delete selected"), disable_group);
    action->setIcon(QIcon(":/extui/modifydelete.png"));
    action->setShortcuts(QList<QKeySequence>() << QKeySequence::Delete << QKeySequence(Qt::Key_Backspace));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotModifyDeleteQuick()));
    action->setObjectName("ModifyDeleteQuick");
    a_map["ModifyDeleteQuick"] = action;

    action = new QAction(tr("Select &All"), disable_group);
    action->setShortcut(QKeySequence::SelectAll);
    action->setIcon(QIcon(":/extui/selectall.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectAll()));
    action->setObjectName("SelectAll");
    a_map["SelectAll"] = action;

    // <[~ Select ~]>

    action = new QAction(tr("Deselect &all"), disable_group);
    // RVT April 29, 2011 - Added esc key to de-select all entities
    action->setShortcuts(QList<QKeySequence>() << QKeySequence(tr("Ctrl+K")));
    action->setIcon(QIcon(":/extui/selectnothing.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotDeselectAll()));
    action->setObjectName("DeselectAll");
    a_map["DeselectAll"] = action;

    action = new QAction(tr("Invert Selection"), disable_group);
    action->setIcon(QIcon(":/extui/selectinvert.png"));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotSelectInvert()));
    action->setObjectName("SelectInvert");
    a_map["SelectInvert"] = action;

    // <[~ Misc ~]>

    action = new QAction(tr("Export as &MakerCAM SVG..."), disable_group);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotFileExportMakerCam()));
    action->setObjectName("FileExportMakerCam");
    a_map["FileExportMakerCam"] = action;

    action = new QAction(tr("Regenerate Dimension Entities"), disable_group);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotToolRegenerateDimensions()));
    action->setObjectName("ToolRegenerateDimensions");
    a_map["ToolRegenerateDimensions"] = action;

    // <[~ Zoom ~]>

    action = new QAction(tr("Zoom &In"), disable_group);
    action->setIcon(QIcon::fromTheme("zoom-in", QIcon(":/icons/zoom_in.svg")));
    action->setShortcut(QKeySequence::ZoomIn);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomIn()));
    action->setObjectName("ZoomIn");
    a_map["ZoomIn"] = action;

    action = new QAction(tr("Zoom &Out"), disable_group);
    action->setIcon(QIcon::fromTheme("zoom-out", QIcon(":/icons/zoom_out.svg")));
    action->setShortcut(QKeySequence::ZoomOut);
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomOut()));
    action->setObjectName("ZoomOut");
    a_map["ZoomOut"] = action;

    action = new QAction(tr("&Auto Zoom"), disable_group);
    action->setIcon(QIcon::fromTheme("zoom-fit-best", QIcon(":/icons/zoom_auto.svg")));
    connect(action, SIGNAL(triggered()), action_handler, SLOT(slotZoomAuto()));
    action->setObjectName("ZoomAuto");
    a_map["ZoomAuto"] = action;

    action = new QAction(tr("Previous &View"), disable_group);
    action->setIcon(QIcon::fromTheme("zoom-previous", QIcon(":/icons/zoom_previous.svg")));
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomPrevious()));
    action->setEnabled(false);
    action->setObjectName("ZoomPrevious");
    a_map["ZoomPrevious"] = action;

    action = new QAction(tr("&Redraw"), disable_group);
    action->setIcon(QIcon::fromTheme("view-refresh", QIcon(":/icons/redraw.svg")));
    action->setShortcut(QKeySequence::Refresh);
    connect(action, SIGNAL(triggered()),
    action_handler, SLOT(slotZoomRedraw()));
    action->setObjectName("ZoomRedraw");
    a_map["ZoomRedraw"] = action;

    // ===========================
    // <[~ Main Window Actions ~]>
    // ===========================

    action = new QAction(tr("&New"), main_window);
    action->setIcon(QIcon::fromTheme("document-new", QIcon(":/actions/filenew.png")));
    action->setShortcut(QKeySequence::New);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileNewNew()));
    action->setObjectName("FileNew");
    a_map["FileNew"] = action;

    action = new QAction(tr("New From &Template"), main_window);
    action->setIcon(QIcon::fromTheme("document-new", QIcon(":/actions/filenew.png")));
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileNewTemplate()));
    action->setObjectName("FileNewTemplate");
    a_map["FileNewTemplate"] = action;

    action = new QAction(tr("&Open..."), main_window);
    action->setIcon(QIcon::fromTheme("document-open", QIcon(":/actions/fileopen2.png")));
    action->setShortcut(QKeySequence::Open);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileOpen()));
    action->setObjectName("FileOpen");
    a_map["FileOpen"] = action;

    action = new QAction(tr("&Save"), disable_group);
    action->setIcon(QIcon::fromTheme("document-save", QIcon(":/actions/filesave2.png")));
    action->setShortcut(QKeySequence::Save);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileSave()));
    action->setObjectName("FileSave");
    a_map["FileSave"] = action;

    action = new QAction(tr("Save &as..."), disable_group);
    action->setIcon(QIcon::fromTheme("document-save-as", QIcon(":/actions/filesaveas.png")));
    action->setShortcut(QKeySequence::SaveAs);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileSaveAs()));
    action->setObjectName("FileSaveAs");
    a_map["FileSaveAs"] = action;

    action = new QAction( QIcon(":/actions/fileexport.png"), tr("&Export as image"), disable_group);
    connect(action, SIGNAL( triggered()), main_window, SLOT(slotFileExport()));
    action->setObjectName("FileExport");
    a_map["FileExport"] = action;

    action = new QAction(tr("&Close"), main_window);
    action->setIcon(QIcon(":/actions/fileclose.png"));
    action->setShortcut(QKeySequence::Close);
    action->setShortcutContext(Qt::WidgetShortcut);
    action->setObjectName("FileClose");
    a_map["FileClose"] = action;

    action = new QAction(tr("&Print..."), disable_group);
    action->setIcon(QIcon::fromTheme("document-print", QIcon(":/actions/fileprint.png")));
    action->setShortcut(QKeySequence::Print);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFilePrint()));
    connect(main_window, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    action->setObjectName("FilePrint");
    a_map["FilePrint"] = action;

    action = new QAction(tr("Export as PDF"), disable_group);
    action->setIcon(QIcon(":/actions/fileexportpdf.png"));
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFilePrintPDF()));
    action->setObjectName("FilePrintPDF");
    a_map["FilePrintPDF"] = action;

    action = new QAction(tr("Print Pre&view"), disable_group);
    action->setIcon(QIcon::fromTheme("document-print-preview", QIcon(":/actions/fileprintpreview.png")));
    action->setCheckable(true);
    connect(action, SIGNAL(triggered(bool)), main_window, SLOT(slotFilePrintPreview(bool)));
    connect(main_window, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    action->setObjectName("FilePrintPreview");
    a_map["FilePrintPreview"] = action;

    action = new QAction(tr("&Quit"), main_window);
    action->setIcon(QIcon::fromTheme("application-exit", QIcon(":/actions/exit.png")));
    action->setShortcut(QKeySequence::Quit);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFileQuit()));
    action->setObjectName("FileQuit");
    a_map["FileQuit"] = action;

    action = new QAction(QIcon(":/ui/blockinsert.png"), tr("&Block"), disable_group);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotImportBlock()));
    action->setObjectName("BlocksImport");
    a_map["BlocksImport"] = action;

    // <[~ View ~]>

    action = new QAction(tr("&Fullscreen"), main_window);
    #if QT_VERSION >= 0x050000
    action->setShortcut(QKeySequence::FullScreen);
    #else
        #if defined(Q_OS_MAC)
        action->setShortcut(tr("Ctrl+Meta+F"));
        #elif defined(Q_OS_WIN)
        action->setShortcut(tr("F11"));
        #else
        action->setShortcut(tr("Ctrl+F11"));
        #endif
    #endif
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(toggleFullscreen(bool)));
    action->setObjectName("Fullscreen");
    a_map["Fullscreen"] = action;

    action = new QAction(tr("&Grid"), disable_group);
    action->setIcon(QIcon(":/icons/grid.svg"));
    action->setShortcut(QKeySequence(tr("Ctrl+G", "Toggle Grid")));
    action->setCheckable(true);
    action->setChecked(true);
    connect(main_window, SIGNAL(gridChanged(bool)), action, SLOT(setChecked(bool)));
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewGrid(bool)));
    action->setObjectName("ViewGrid");
    a_map["ViewGrid"] = action;

    action = new QAction(tr("&Draft"), disable_group);
    action->setIcon(QIcon(":/icons/draft.svg"));
    action->setCheckable(true);
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewDraft(bool)));
    connect(main_window, SIGNAL(draftChanged(bool)), action, SLOT(setChecked(bool)));
    action->setObjectName("ViewDraft");
    a_map["ViewDraft"] = action;

    action = new QAction(tr("&Statusbar"), main_window);
    action->setCheckable(true);
    action->setChecked(true);
    action->setShortcut(QKeySequence(tr("Ctrl+I", "Hide Statusbar")));
    connect(action, SIGNAL(toggled(bool)), main_window, SLOT(slotViewStatusBar(bool)));
    action->setObjectName("ViewStatusBar");
    a_map["ViewStatusBar"] = action;

    action = new QAction(tr("Focus on &Command Line"), main_window);
    action->setIcon(QIcon(":/main/editclear.png"));
    QList<QKeySequence> commandLineShortcuts;
    commandLineShortcuts<<QKeySequence(Qt::CTRL + Qt::Key_M)<<QKeySequence(Qt::Key_Colon)<<QKeySequence(Qt::Key_Space);
    action->setShortcuts(commandLineShortcuts);
    connect(action, SIGNAL(triggered()), main_window, SLOT(slotFocusCommandLine()));
    action->setObjectName("FocusCommand");
    a_map["FocusCommand"] = action;

    action = new QAction(tr("Left"), main_window);
    action->setIcon(QIcon(":/icons/dockwidgets_left"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleLeftDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("LeftDockAreaToggle");
    a_map["LeftDockAreaToggle"] = action;

    action = new QAction(tr("Right"), main_window);
    action->setIcon(QIcon(":/icons/dockwidgets_right"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleRightDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(true);
    action->setObjectName("RightDockAreaToggle");
    a_map["RightDockAreaToggle"] = action;

    action = new QAction(tr("Top"), main_window);
    action->setIcon(QIcon(":/icons/dockwidgets_top"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleTopDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("TopDockAreaToggle");
    a_map["TopDockAreaToggle"] = action;

    action = new QAction(tr("Bottom"), main_window);
    action->setIcon(QIcon(":/icons/dockwidgets_bottom"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleBottomDockArea(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("BottomDockAreaToggle");
    a_map["BottomDockAreaToggle"] = action;

    action = new QAction(tr("Floating"), main_window);
    action->setIcon(QIcon(":/icons/dockwidgets_floating"));
    connect(action, SIGNAL(toggled(bool)),
            main_window, SLOT(toggleFloatingDockwidgets(bool)));
    action->setCheckable(true);
    action->setChecked(false);
    action->setObjectName("FloatingDockwidgetsToggle");
    a_map["FloatingDockwidgetsToggle"] = action;

    action = new QAction(tr("Reload Style Sheet"), main_window);
    action->setShortcut(QKeySequence("Ctrl+T"));
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(reloadStyleSheet()));
    action->setObjectName("ReloadStyleSheet");
    a_map["ReloadStyleSheet"] = action;

    action = new QAction(tr("Create Double-click Menu"), main_window);
    connect(action, SIGNAL(triggered()),
            main_window, SLOT(invokeDoubleClickMenuCreator()));
    action->setObjectName("CreateDoubleClickMenu");
    a_map["CreateDoubleClickMenu"] = action;
}
