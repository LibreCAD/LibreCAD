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


#include <QMouseEvent>

#include "rs_actiondrawlinepolygon2.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_point.h"
#include "rs_math.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_actioninterface.h"
#include "lc_actiondrawlinepolygonbase.h"


// fixme - support creation of polygone as polyline
// fixme - support of rounded corners

RS_ActionDrawLinePolygonCorCor::RS_ActionDrawLinePolygonCorCor(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
		  :LC_ActionDrawLinePolygonBase("Draw Polygons (Corner,Corner)", container, graphicView, actionType=RS2::ActionDrawLinePolygonCorCor){
}

RS_ActionDrawLinePolygonCorCor::~RS_ActionDrawLinePolygonCorCor() = default;


void RS_ActionDrawLinePolygonCorCor::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    bool ok = creation.createPolygon2(pPoints->point1, pPoints->point2, number);

    if (!ok){
        RS_DEBUG->print("RS_ActionDrawLinePolygon2::trigger:"
                        " No polygon added\n");
    }
}

void RS_ActionDrawLinePolygonCorCor::previewAdditionalReferences(const RS_Vector &mouse) {
    RS_Vector center = determinePolygonCenter(mouse);
    previewRefPoint(center);
}

void RS_ActionDrawLinePolygonCorCor::previewPolygon(const RS_Vector &mouse) const {
    RS_Creation creation(preview.get(), nullptr, false);
    creation.createPolygon2(pPoints->point1, mouse, number);
}

RS_Vector RS_ActionDrawLinePolygonCorCor::determinePolygonCenter(const RS_Vector &mouse) const{
    // angle for edge
    double edgeAngle = pPoints->point1.angleTo(mouse);

    // rotate second corner so edge will be horizontal
    RS_Vector rotatedCorner2 = mouse;
    rotatedCorner2 = rotatedCorner2.rotate(pPoints->point1, -edgeAngle);

    // half inner angle of polygon
    double angleFromCornerToCenter = RS_Math::deg2rad((90.0 * (number - 2)) / number);

    // middle point of edge
    RS_Vector edgeCenter = (pPoints->point1 + rotatedCorner2) * 0.5;

    // distance between corner and edge center
    double distanceToEdgeCenter = pPoints->point1.distanceTo(edgeCenter);

    // leg of triangle with vertexes in corner1, edgeCenter and polygon center
    double distanceToPolygonCenter =  distanceToEdgeCenter * tan(angleFromCornerToCenter);

    //normal angle to center of polygon from edge center - depends on whether center is on left or on right from the corner
    double normalAngle = (edgeCenter.x > pPoints->point1.x) ? M_PI_2 : - M_PI_2;

    // position of rotate polygon center
    RS_Vector center = edgeCenter + RS_Vector::polar(distanceToPolygonCenter, normalAngle);

    // actual position of center taking into consideration rotation of the edge
    center = center.rotate(pPoints->point1, edgeAngle);
    return center;
}

QString RS_ActionDrawLinePolygonCorCor::getPoint1Hint() const { return tr("Specify first corner"); }
QString RS_ActionDrawLinePolygonCorCor::getPoint2Hint() const { return tr("Specify second corner"); }
