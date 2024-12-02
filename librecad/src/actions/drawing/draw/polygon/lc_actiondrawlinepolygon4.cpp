/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_actiondrawlinepolygon4.h"

LC_ActionDrawLinePolygon4::LC_ActionDrawLinePolygon4(RS_EntityContainer& container,RS_GraphicView& graphicView)
    :LC_ActionDrawLinePolygonBase("Draw Polygons (V,V)", container, graphicView, actionType=RS2::ActionDrawLinePolygonSideSide){
}

LC_ActionDrawLinePolygon4::~LC_ActionDrawLinePolygon4() = default;

QString LC_ActionDrawLinePolygon4::getPoint2Hint() const {
    if (useVertexVertexMode){
        return tr("Specify second corner");
    }
    else{
        return tr("Specify second tangent");
    }
}

QString LC_ActionDrawLinePolygon4::getPoint1Hint() const {
    if (useVertexVertexMode){
        return tr("Specify first corner");
    }
    else{
        return tr("Specify first tangent");
    }
}

void LC_ActionDrawLinePolygon4::preparePolygonInfo(LC_ActionDrawLinePolygonBase::PolygonInfo &polygonInfo, const RS_Vector &snap) {
    double angle = M_PI/number;
    double pointsAngle = pPoints->point1.angleTo(snap);

    double dist = pPoints->point1.distanceTo(snap) / 2.0;

    double oppositeLap = tan(angle) * dist;
    double hipotenuse = sqrt((dist * dist) + (oppositeLap * oppositeLap));

    RS_Vector center = pPoints->point1.relative(dist, pointsAngle);
    RS_Vector vertex;
    if (useVertexVertexMode) {
        vertex = snap;
    } else {
        vertex = center.relative(hipotenuse, center.angleTo(pPoints->point1) + angle);
    }

    if ((number % 2) == 1) {
        // odd no. of corners
        double newdist = (dist / (dist + hipotenuse)) * (dist * 2);
        double newopp = tan(angle) * newdist;
        double newhyp = sqrt((newdist * newdist) + (newopp * newopp));
        RS_Vector newcen = RS_Vector::polar(newdist, pointsAngle);
        RS_Vector newcenter = pPoints->point1 + newcen;
        if (useVertexVertexMode) {
            newcenter = snap - newcen;
            vertex = newcenter.relative(newhyp, newcenter.angleTo(pPoints->point1));
        } else {
            vertex = newcenter.relative(newhyp, newcenter.angleTo(pPoints->point1) + angle);
        }
        center = newcenter;
    }

    polygonInfo.centerPoint = center;
    polygonInfo.startingAngle = center.angleTo(vertex);
    polygonInfo.vertexRadius = center.distanceTo(vertex);
}
