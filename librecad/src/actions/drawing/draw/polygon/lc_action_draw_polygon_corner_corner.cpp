/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "lc_action_draw_polygon_corner_corner.h"

LC_ActionDrawLinePolygonCornerCorner::LC_ActionDrawLinePolygonCornerCorner(LC_ActionContext* actionContext)
    : LC_ActionDrawPolygonBase("ActionDrawLinePolygonCorCor", actionContext, m_actionType = RS2::ActionDrawLinePolygonCorCor) {
}

LC_ActionDrawLinePolygonCornerCorner::~LC_ActionDrawLinePolygonCornerCorner() = default;

void LC_ActionDrawLinePolygonCornerCorner::previewAdditionalReferences(const RS_Vector& mouse) {
    const RS_Vector center = determinePolygonCenter(mouse);
    previewRefPoint(center);
}

// fixme - move logic to RS_CreationPolygon?
void LC_ActionDrawLinePolygonCornerCorner::preparePolygonInfo(PolygonInfo& polygonInfo, const RS_Vector& snap) {
    const double len = m_actionData->point1.distanceTo(snap);
    const double da = 2. * M_PI / m_edgesNumber;
    polygonInfo.vertexRadius = 0.5 * len / sin(0.5 * da);

    const double angle1 = m_actionData->point1.angleTo(snap);
    RS_Vector center = (m_actionData->point1 + snap) * 0.5;

    //TODO, the center or the polygon could be at left or right side
    //left is chosen here
    center += RS_Vector::polar(0.5 * len / tan(0.5 * da), angle1 + M_PI_2);

    polygonInfo.centerPoint = center;

    polygonInfo.startingAngle = center.angleTo(m_actionData->point1);
}

RS_Vector LC_ActionDrawLinePolygonCornerCorner::determinePolygonCenter(const RS_Vector& mouse) const {
    // angle for edge
    const double edgeAngle = m_actionData->point1.angleTo(mouse);

    // rotate second corner so edge will be horizontal
    RS_Vector rotatedCorner2 = mouse;
    rotatedCorner2 = rotatedCorner2.rotate(m_actionData->point1, -edgeAngle);

    // half inner angle of polygon
    const double angleFromCornerToCenter = RS_Math::deg2rad(90.0 * (m_edgesNumber - 2) / m_edgesNumber);

    // middle point of edge
    const RS_Vector edgeCenter = (m_actionData->point1 + rotatedCorner2) * 0.5;

    // distance between corner and edge center
    const double distanceToEdgeCenter = m_actionData->point1.distanceTo(edgeCenter);

    // leg of triangle with vertexes in corner1, edgeCenter and polygon center
    const double distanceToPolygonCenter = distanceToEdgeCenter * tan(angleFromCornerToCenter);

    //normal angle to center of polygon from edge center - depends on whether center is on left or on right from the corner
    const double normalAngle = (edgeCenter.x > m_actionData->point1.x) ? M_PI_2 : -M_PI_2;

    // position of rotate polygon center
    RS_Vector center = edgeCenter + RS_Vector::polar(distanceToPolygonCenter, normalAngle);

    // actual position of center taking into consideration rotation of the edge
    center = center.rotate(m_actionData->point1, edgeAngle);
    return center;
}

QString LC_ActionDrawLinePolygonCornerCorner::getPoint1Hint() const {
    return tr("Specify first corner");
}

QString LC_ActionDrawLinePolygonCornerCorner::getPoint2Hint() const {
    return tr("Specify second corner");
}
