/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2017 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2017 taoxumuye (tfy.hi@163.com)
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

#include "lc_action_draw_polygon_center_tan.h"

LC_ActionDrawLinePolygonCenterTangent::LC_ActionDrawLinePolygonCenterTangent(LC_ActionContext* actionContext)
    : LC_ActionDrawPolygonBase("ActionDrawLinePolygonCenTan", actionContext, m_actionType = RS2::ActionDrawLinePolygonCenTan) {
}

LC_ActionDrawLinePolygonCenterTangent::~LC_ActionDrawLinePolygonCenterTangent() = default;

void LC_ActionDrawLinePolygonCenterTangent::preparePolygonInfo(PolygonInfo& polygonInfo, const RS_Vector& snap) {
    const double angle = 2. * M_PI / m_edgesNumber / 2.0;
    const double tangensAngle = tan(angle);

    RS_Vector vertex(0, 0);
    const auto& data = m_actionData->point1;
    vertex.x = snap.x + (data.y - snap.y) * tangensAngle;
    vertex.y = snap.y + (snap.x - data.x) * tangensAngle;

    polygonInfo.vertexRadius = data.distanceTo(vertex);
    polygonInfo.startingAngle = data.angleTo(vertex);
    polygonInfo.centerPoint = data;
}

QString LC_ActionDrawLinePolygonCenterTangent::getPoint2Hint() const {
    return tr("Specify a tangent");
}
