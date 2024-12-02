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

#include "lc_actiondrawlinepolygon3.h"

#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_creation.h"
#include "rs_point.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_actioninterface.h"
#include "lc_actiondrawlinepolygonbase.h"

LC_ActionDrawLinePolygonCenTan::LC_ActionDrawLinePolygonCenTan(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :LC_ActionDrawLinePolygonBase("Draw Polygons (Center,Corner)", container, graphicView, actionType=RS2::ActionDrawLinePolygonCenTan){
}

LC_ActionDrawLinePolygonCenTan::~LC_ActionDrawLinePolygonCenTan() = default;

void LC_ActionDrawLinePolygonCenTan::preparePolygonInfo(LC_ActionDrawLinePolygonBase::PolygonInfo &polygonInfo, const RS_Vector &snap) {
    //  creation.createPolygon3(pPoints->point1, mouse, number);
    double angle = 2.*M_PI/number/2.0;
    double tangensAngle = tan(angle);

    RS_Vector vertex(0, 0);
    vertex.x = snap.x + (pPoints->point1.y - snap.y) * tangensAngle;
    vertex.y = snap.y + (snap.x - pPoints->point1.x) * tangensAngle;

    polygonInfo.vertexRadius = pPoints->point1.distanceTo(vertex);
    polygonInfo.startingAngle = pPoints->point1.angleTo(vertex);
    polygonInfo.centerPoint = pPoints->point1;
}

QString LC_ActionDrawLinePolygonCenTan::getPoint2Hint() const { return tr("Specify a tangent"); }
