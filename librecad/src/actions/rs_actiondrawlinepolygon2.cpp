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

struct RS_ActionDrawLinePolygonCorCor::Points {
	/** 1st corner */
	RS_Vector corner1;
	/** 2nd corner */
	RS_Vector corner2;
};

// fixme - support creation of polygone as polyline
// fixme - support of rounded corners

RS_ActionDrawLinePolygonCorCor::RS_ActionDrawLinePolygonCorCor(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
		:RS_PreviewActionInterface("Draw Polygons (Corner,Corner)", container, graphicView)
		, pPoints(std::make_unique<Points>())
		,number(3)
		,lastStatus(SetCorner1)
{
	actionType=RS2::ActionDrawLinePolygonCorCor;
}

RS_ActionDrawLinePolygonCorCor::~RS_ActionDrawLinePolygonCorCor() = default;


void RS_ActionDrawLinePolygonCorCor::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    bool ok = creation.createPolygon2(pPoints->corner1, pPoints->corner2, number);

    if (!ok){
        RS_DEBUG->print("RS_ActionDrawLinePolygon2::trigger:"
                        " No polygon added\n");
    }
}

void RS_ActionDrawLinePolygonCorCor::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon2::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
        case SetCorner1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetCorner2: {
            if (pPoints->corner1.valid){
                mouse = getSnapAngleAwarePoint(e, pPoints->corner1, mouse);
                pPoints->corner2 = mouse;
                deletePreview();
                addReferencePointToPreview(pPoints->corner1);

                RS_Vector center = determinePolygonCenter();

                addReferencePointToPreview(center);

                addReferenceLineToPreview(center, pPoints->corner1);

                RS_Creation creation(preview.get(), nullptr, false);
                creation.createPolygon2(pPoints->corner1, pPoints->corner2, number);

                drawPreview();
            }
            break;
        }
        default:
            break;
    }
}

RS_Vector RS_ActionDrawLinePolygonCorCor::determinePolygonCenter() const{
    // angle for edge
    double edgeAngle = this->pPoints->corner1.angleTo(this->pPoints->corner2);

    // rotate second corner so edge will be horizontal
    RS_Vector rotatedCorner2 = this->pPoints->corner2;
    rotatedCorner2 = rotatedCorner2.rotate(this->pPoints->corner1, -edgeAngle);

    // half inner angle of polygon
    double angleFromCornerToCenter = RS_Math::deg2rad(90 * (this->number - 2) / this->number);

    // middle point of edge
    RS_Vector edgeCenter = (this->pPoints->corner1 + rotatedCorner2) * 0.5;

    // distance between corner and edge center
    double distanceToEdgeCenter = this->pPoints->corner1.distanceTo(edgeCenter);

    // leg of triangle with vertexes in corner1, edgeCenter and polygon center
    double distanceToPolygonCenter =  distanceToEdgeCenter * tan(angleFromCornerToCenter);

    //normal angle to center of polygon from edge center - depends on whether center is on left or on right from the corner
    double normalAngle = (edgeCenter.x > this->pPoints->corner1.x) ? M_PI_2 : - M_PI_2;

    // position of rotate polygon center
    RS_Vector center = edgeCenter + RS_Vector::polar(distanceToPolygonCenter, normalAngle);

    // actual position of center taking into consideration rotation of the edge
    center = center.rotate(this->pPoints->corner1, edgeAngle);
    return center;
}

void RS_ActionDrawLinePolygonCorCor::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_Vector coord = snapPoint(e);
        if (getStatus() == SetCorner2){
            coord = getSnapAngleAwarePoint(e, pPoints->corner1, coord);
        }
        RS_CoordinateEvent ce(coord);
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionDrawLinePolygonCorCor::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==nullptr) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
        case SetCorner1:
            pPoints->corner1 = mouse;
            setStatus(SetCorner2);
            graphicView->moveRelativeZero(mouse);
            break;

        case SetCorner2:
            pPoints->corner2 = mouse;
            trigger();
            break;

        default:
            break;
    }
}

void RS_ActionDrawLinePolygonCorCor::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetCorner1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first corner"),
                                                tr("Cancel"));
            break;

        case SetCorner2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second corner"),
                                                tr("Back"));
            break;

        case SetNumber:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Number:"), tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void RS_ActionDrawLinePolygonCorCor::showOptions(){
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}

void RS_ActionDrawLinePolygonCorCor::hideOptions(){
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}

void RS_ActionDrawLinePolygonCorCor::commandEvent(RS_CommandEvent *e){
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)){
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
        case SetCorner1:
        case SetCorner2:
            if (checkCommand("number", c)){
                deletePreview();
                lastStatus = (Status) getStatus();
                setStatus(SetNumber);
            }
            break;

        case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok){
                e->accept();
                if (n > 0 && n < 10000){
                    number = n;
                } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid number. "
                                                        "Try 1..9999"));
            } else
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression."));
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
            break;

        default:
            break;
    }
}

QStringList RS_ActionDrawLinePolygonCorCor::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
    case SetCorner1:
    case SetCorner2:
        cmd += command("number");
        break;
    default:
        break;
    }

    return cmd;
}

void RS_ActionDrawLinePolygonCorCor::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
