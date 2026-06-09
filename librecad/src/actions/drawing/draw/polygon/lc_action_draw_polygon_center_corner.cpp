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
#include "lc_action_draw_polygon_center_corner.h"

LC_ActionDrawLinePolygonCenterCorner::LC_ActionDrawLinePolygonCenterCorner(LC_ActionContext *actionContext)
    :LC_ActionDrawPolygonBase("ActionDrawLinePolygonCenCor", actionContext, m_actionType = RS2::ActionDrawLinePolygonCenCor){
}

LC_ActionDrawLinePolygonCenterCorner::~LC_ActionDrawLinePolygonCenterCorner() = default;

void LC_ActionDrawLinePolygonCenterCorner::preparePolygonInfo(PolygonInfo &polygonInfo, const RS_Vector &snap) {
    // creation.createPolygon(pPoints->point1, mouse, number);
    polygonInfo.centerPoint = m_actionData->point1;
    polygonInfo.vertexRadius = m_actionData->point1.distanceTo(snap);
    polygonInfo.startingAngle = m_actionData->point1.angleTo(snap);
}

QString LC_ActionDrawLinePolygonCenterCorner::getPoint2Hint() const { return tr("Specify a corner"); }
