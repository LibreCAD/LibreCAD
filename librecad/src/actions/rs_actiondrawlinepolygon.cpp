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
                pPoints->corner = mouse;
                RS_Creation creation(preview.get(), nullptr, false);
                creation.createPolygon(pPoints->center, pPoints->corner, number);

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->center);
                    previewRefLine(pPoints->center, mouse);
                    previewRefSelectablePoint(mouse);
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLinePolygonCenCor::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector coord = snapPoint(e);
    if (status == SetCorner){
        coord = getSnapAngleAwarePoint(e, pPoints->center, coord);
    }
    fireCoordinateEvent(coord);
}

void RS_ActionDrawLinePolygonCenCor::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawLinePolygonCenCor::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
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
            updateMouseWidgetTRCancel(tr("Specify center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetCorner:
            updateMouseWidgetTRBack(tr("Specify a corner"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetNumber:
            updateMouseWidgetTRBack(tr("Enter number:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

// fixme - move to base polygon action?
bool RS_ActionDrawLinePolygonCenCor::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetCenter:
        case SetCorner: {
            if (checkCommand("number", c)){
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetNumber);
                accept = true;
            }
            break;
        }
        case SetNumber: {
            accept = parseNumber(c);
            if (accept){
                updateOptions();
                setStatus(lastStatus);
            }
            break;
        }
        default:
            break;
    }
    return accept;
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
