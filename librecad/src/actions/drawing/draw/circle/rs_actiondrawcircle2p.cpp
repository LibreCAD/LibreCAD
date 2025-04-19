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


#include "rs_actiondrawcircle2p.h"

#include "rs_circle.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"

struct RS_ActionDrawCircle2P::Points {
    /**
     * 1st point.
     */
    RS_Vector point1;
    /**
     * 2nd point.
     */
    RS_Vector point2;
};

RS_ActionDrawCircle2P::RS_ActionDrawCircle2P(LC_ActionContext *actionContext)
    :LC_ActionDrawCircleBase("Draw circles",actionContext, RS2::ActionDrawCircle2P)
    , m_circleData(new RS_CircleData())
    , m_actionData(std::make_unique<Points>()){
    reset();
}

RS_ActionDrawCircle2P::~RS_ActionDrawCircle2P() = default;

void RS_ActionDrawCircle2P::reset() {
    m_circleData.reset(new RS_CircleData{});
    m_actionData->point1 = {};
    m_actionData->point2 = {};
}

void RS_ActionDrawCircle2P::doTrigger() {
    preparePreview();
    if (m_circleData->isValid()){
        auto *circle = new RS_Circle(m_container,*m_circleData);

        setPenAndLayerToActive(circle);

        if (m_moveRelPointAtCenterAfterTrigger){
            moveRelativeZero(m_circleData->center);
        }

        undoCycleAdd(circle);
        setStatus(SetPoint1);
        reset();
    } else
        RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid Circle data."));
}

void RS_ActionDrawCircle2P::preparePreview() {
    m_circleData.reset(new RS_CircleData{});
    if (m_actionData->point1.valid && m_actionData->point2.valid) {
        RS_Circle circle(nullptr, *m_circleData);
        bool suc = circle.createFrom2P(m_actionData->point1, m_actionData->point2);
        if (suc) {
            m_circleData.reset(new RS_CircleData(circle.getData()));
        }
    }
}

void RS_ActionDrawCircle2P::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->point1, mouse, true);
            m_actionData->point2 = mouse;
            preparePreview();
            if (m_circleData->isValid()){
                previewToCreateCircle(*m_circleData);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_circleData->center);
                    previewRefPoint(m_actionData->point1);
                    previewRefSelectablePoint(m_actionData->point2);
                    previewRefLine(m_circleData->center, m_actionData->point1);
                }
            }

            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawCircle2P::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector coord = e->snapPoint;
    if (status == SetPoint2){
        coord = getSnapAngleAwarePoint(e, m_actionData->point1, coord);
    }
    fireCoordinateEvent(coord);
}

void RS_ActionDrawCircle2P::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawCircle2P::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            m_actionData->point2 = mouse;
            moveRelativeZero(mouse);
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawCircle2P::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
