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

#include "rs_actiondrawlinepolygon.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_point.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"

struct RS_ActionDrawLinePolygonCenCor::Points {
	/** Center of polygon */
	RS_Vector center;
	/** Edge */
	RS_Vector corner;
};

// fixme - support creation of polygone as polyline
// fixme - support of rounded corners

RS_ActionDrawLinePolygonCenCor::RS_ActionDrawLinePolygonCenCor(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_ActionDrawLinePolygonBase("Draw Polygons (Center,Corner)", container, graphicView, actionType = RS2::ActionDrawLinePolygonCenCor),
    pPoints(std::make_unique<Points>()), lastStatus(SetCenter){
}

RS_ActionDrawLinePolygonCenCor::~RS_ActionDrawLinePolygonCenCor() = default;


void RS_ActionDrawLinePolygonCenCor::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    bool ok = creation.createPolygon(pPoints->center, pPoints->corner, number);

    if (!ok){
        RS_DEBUG->print("RS_ActionDrawLinePolygon::trigger:"
                        " No polygon added\n");
    }
}

void RS_ActionDrawLinePolygonCenCor::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetCenter: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetCorner: {
            deletePreview();
            if (pPoints->center.valid){
                mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);
                previewRefPoint(pPoints->center);
                previewRefLine(pPoints->center, mouse);
                previewRefSelectablePoint(mouse);
                pPoints->corner = mouse;
                RS_Creation creation(preview.get(), nullptr, false);
                creation.createPolygon(pPoints->center, pPoints->corner, number);

            }
            drawPreview();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLinePolygonCenCor::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_Vector coord = snapPoint(e);
        if (getStatus() == SetCorner){
            coord = getSnapAngleAwarePoint(e, pPoints->center, coord);
        }
        fireCoordinateEvent(coord);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionDrawLinePolygonCenCor::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) return;

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
        case SetCenter: {
            pPoints->center = mouse;
            setStatus(SetCorner);
            graphicView->moveRelativeZero(mouse);
            break;
        }
        case SetCorner: {
            pPoints->corner = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLinePolygonCenCor::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetCenter:
            updateMouseWidgetTRCancel("Specify center", MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetCorner:
            updateMouseWidgetTRBack("Specify a corner", MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetNumber:
            updateMouseWidgetTRBack("Enter number:");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionDrawLinePolygonCenCor::commandEvent(RS_CommandEvent *e){
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)){
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
        case SetCenter:
        case SetCorner: {
            if (checkCommand("number", c)){
                deletePreview();
                lastStatus = (Status) getStatus();
                setStatus(SetNumber);
            }
            break;
        }
        case SetNumber: {
            parseNumber(e, c);
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        default:
            break;
    }
}


QStringList RS_ActionDrawLinePolygonCenCor::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetCenter:
        case SetCorner:
            cmd += command("number");
            break;
        default:
            break;
    }

    return cmd;
}
// EOF
