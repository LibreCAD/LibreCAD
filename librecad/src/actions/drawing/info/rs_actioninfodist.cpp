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

#include "rs_actioninfodist.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_units.h"

struct RS_ActionInfoDist::Points {
    RS_Vector point1;
    RS_Vector point2;
};

RS_ActionInfoDist::RS_ActionInfoDist(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Info Dist", container, graphicView), pPoints(std::make_unique<Points>()){
    actionType = RS2::ActionInfoDistPoint2Point;
}

RS_ActionInfoDist::~RS_ActionInfoDist() = default;

void RS_ActionInfoDist::init(int status) {
    RS_PreviewActionInterface::init(status);
}

// fixme - consider displaying information in EntityInfo widget
void RS_ActionInfoDist::doTrigger() {
    RS_DEBUG->print("RS_ActionInfoDist::trigger()");
    if (pPoints->point1.valid && pPoints->point2.valid){
        RS_Vector dV = pPoints->point2 - pPoints->point1;
        QStringList dists;
        for (double a: {dV.magnitude(), dV.x, dV.y, pPoints->point1.x, pPoints->point1.y, pPoints->point2.x, pPoints->point2.y}) {
            dists << formatLinear(a);
        }

        double wcsAngle = dV.angle();
        QString angle = formatWCSAngle(wcsAngle);
        commandMessage("--- ");
        const QString &templateStr = tr("Distance: %1\nCartesian: (%2 , %3)\nPolar: (%4 < %5)\nStart: (%6 , %7)\nEnd: (%8 , %9)");
        QString message = templateStr.arg(dists[0],dists[1],dists[2],dists[0],angle, dists[3], dists[4], dists[5], dists[6]);
        commandMessage(message);
    }
}

void RS_ActionInfoDist::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2: {
            if (pPoints->point1.valid){
                mouse = getSnapAngleAwarePoint(e, pPoints->point1, mouse, true);
                pPoints->point2 = mouse;
                previewLine(pPoints->point1, pPoints->point2);
                if (showRefEntitiesOnPreview) {
                    previewRefLine(pPoints->point1, pPoints->point2);
                    previewRefPoint(pPoints->point1);
                    previewRefSelectablePoint(pPoints->point2);
                }
                RS_Vector &startPoint = pPoints->point1;
                updateInfoCursor(mouse, startPoint);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoDist::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &startPoint) {
    if (infoCursorOverlayPrefs->enabled) {
        double distance = startPoint.distanceTo(mouse);
        LC_InfoMessageBuilder msg(tr("Info"));
        msg.add(tr("Distance:"), formatLinear(distance));
        msg.add(tr("Angle:"), formatWCSAngle(startPoint.angleTo(mouse)));
        msg.add(tr("From:"), formatVector(startPoint));
        msg.add(tr("To:"), formatVector(mouse));
        appendInfoCursorZoneMessage(msg.toString(), 2, false);
    }
}

void RS_ActionInfoDist::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetPoint1:{
            fireCoordinateEvent(snap);
            moveRelativeZero(pPoints->point1);
            break;
        }
        case (SetPoint2):{
            snap = getSnapAngleAwarePoint(e, pPoints->point1,  snap);
            fireCoordinateEvent(snap);
            if (!e->isControl){
                moveRelativeZero(pPoints->point2);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoDist::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionInfoDist::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            pPoints->point1 = mouse;
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            if (pPoints->point1.valid){
                pPoints->point2 = mouse;
                deletePreview();
                trigger();
                setStatus(SetPoint1);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoDist::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify first point of distance"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify second point of distance"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Don't move relative zero")));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionInfoDist::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
