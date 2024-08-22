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

struct LC_ActionDrawLinePolygonCenTan::Points {
    /** Center of polygon */
    RS_Vector center;
    /** Edge */
    RS_Vector corner;
};

// TODO - sand - support creation of polygone as polyline
// TODO - sand - support of rounded corners?

LC_ActionDrawLinePolygonCenTan::LC_ActionDrawLinePolygonCenTan(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :LC_ActionDrawLinePolygonBase("Draw Polygons (Center,Corner)", container, graphicView, actionType=RS2::ActionDrawLinePolygonCenTan)
        , pPoints(std::make_unique<Points>())
        ,lastStatus(SetCenter){
}

LC_ActionDrawLinePolygonCenTan::~LC_ActionDrawLinePolygonCenTan() = default;

void LC_ActionDrawLinePolygonCenTan::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    RS_Creation creation(container, graphicView);
    bool ok = creation.createPolygon3(pPoints->center, pPoints->corner, number);

    if (!ok) {
        RS_DEBUG->print("RS_ActionDrawLinePolygon::trigger:  No polygon added\n");
    }
}

void LC_ActionDrawLinePolygonCenTan::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLinePolygon::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
        case SetCenter: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetTangent: {
            if (pPoints->center.valid){
                deletePreview();

                mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);
                pPoints->corner = mouse;

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->center);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(pPoints->center, mouse);
                }

                RS_Creation creation(preview.get(), nullptr, false);
                creation.createPolygon3(pPoints->center, pPoints->corner, number);

                drawPreview();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLinePolygonCenTan::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector coord = snapPoint(e);
    if (status ==SetTangent){
        coord = getSnapAngleAwarePoint(e, pPoints->center, coord);
    }
    fireCoordinateEvent(coord);
}

void LC_ActionDrawLinePolygonCenTan::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawLinePolygonCenTan::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetCenter:
            pPoints->center = mouse;
            setStatus(SetTangent);
            moveRelativeZero(mouse);
            break;

        case SetTangent:
            pPoints->corner = mouse;
            trigger();
            break;

        default:
            break;
    }
}

void LC_ActionDrawLinePolygonCenTan::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetCenter:
        updateMouseWidgetTRCancel(tr("Specify center"), MOD_SHIFT_RELATIVE_ZERO);
        break;
    case SetTangent:
        updateMouseWidgetTRBack(tr("Specify a tangent"), MOD_SHIFT_ANGLE_SNAP);
        break;
    case SetNumber:
        updateMouseWidget(tr("Enter number:"),"");
        break;
    default:
        updateMouseWidget();
        break;
    }
}

bool LC_ActionDrawLinePolygonCenTan::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    switch (status) {
        case SetCenter:
        case SetTangent: {
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

QStringList LC_ActionDrawLinePolygonCenTan::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetCenter:
        case SetTangent:
            cmd += command("number");
            break;
        default:
            break;
    }
    return cmd;
}
