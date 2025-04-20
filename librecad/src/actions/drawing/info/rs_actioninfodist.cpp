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

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "rs_debug.h"

struct RS_ActionInfoDist::ActionData {
    RS_Vector point1;
    RS_Vector point2;
};

RS_ActionInfoDist::RS_ActionInfoDist(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Info Dist", actionContext, RS2::ActionInfoDistPoint2Point), m_actionData(std::make_unique<ActionData>()){
}

RS_ActionInfoDist::~RS_ActionInfoDist() = default;

void RS_ActionInfoDist::init(int status) {
    RS_PreviewActionInterface::init(status);
}

// fixme - consider displaying information in EntityInfo widget
void RS_ActionInfoDist::doTrigger() {
    RS_DEBUG->print("RS_ActionInfoDist::trigger()");
    if (m_actionData->point1.valid && m_actionData->point2.valid){
        RS_Vector dV = m_actionData->point2 - m_actionData->point1;
        QStringList dists;
        for (double a: {dV.magnitude(), dV.x, dV.y, m_actionData->point1.x, m_actionData->point1.y, m_actionData->point2.x, m_actionData->point2.y}) {
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
            if (m_actionData->point1.valid){
                mouse = getSnapAngleAwarePoint(e, m_actionData->point1, mouse, true);
                m_actionData->point2 = mouse;
                previewLine(m_actionData->point1, m_actionData->point2);
                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(m_actionData->point1, m_actionData->point2);
                    previewRefPoint(m_actionData->point1);
                    previewRefSelectablePoint(m_actionData->point2);
                }
                RS_Vector &startPoint = m_actionData->point1;
                updateInfoCursor(mouse, startPoint);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionInfoDist::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &startPoint) {
    if (m_infoCursorOverlayPrefs->enabled) {
        double distance = startPoint.distanceTo(mouse);
        msg(tr("Info"))
            .linear(tr("Distance:"), distance)
            .wcsAngle(tr("Angle:"), startPoint.angleTo(mouse))
            .vector(tr("From:"), startPoint)
            .vector(tr("To:"), mouse)
            .toInfoCursorZone2(false);
    }
}

void RS_ActionInfoDist::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetPoint1:{
            fireCoordinateEvent(snap);
            moveRelativeZero(m_actionData->point1);
            break;
        }
        case (SetPoint2):{
            snap = getSnapAngleAwarePoint(e, m_actionData->point1,  snap);
            fireCoordinateEvent(snap);
            if (!e->isControl){
                moveRelativeZero(m_actionData->point2);
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
            m_actionData->point1 = mouse;
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            if (m_actionData->point1.valid){
                m_actionData->point2 = mouse;
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
