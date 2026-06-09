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

#include "lc_action_draw_circle_2points.h"

#include "lc_creation_circle.h"
#include "rs_circle.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"

struct LC_ActionDrawCircle2Points::Points {
    /**
     * 1st point.
     */
    RS_Vector point1;
    /**
     * 2nd point.
     */
    RS_Vector point2;
};

LC_ActionDrawCircle2Points::LC_ActionDrawCircle2Points(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("ActionDrawCircle2Points", actionContext, RS2::ActionDrawCircle2Points), m_circleData{std::make_unique<RS_CircleData>()},
      m_actionData{std::make_unique<Points>()} {
    LC_ActionDrawCircle2Points::reset();
}

LC_ActionDrawCircle2Points::~LC_ActionDrawCircle2Points() = default;

void LC_ActionDrawCircle2Points::reset() {
    m_circleData.reset(new RS_CircleData{});
    m_actionData->point1 = {};
    m_actionData->point2 = {};
}

RS_Entity* LC_ActionDrawCircle2Points::doTriggerCreateEntity() {
    preparePreview();
    if (m_circleData->isValid()) {
        auto* circle = new RS_Circle(m_document, *m_circleData);
        if (m_moveRelPointAtCenterAfterTrigger) {
            moveRelativeZero(m_circleData->center);
        }
        return circle;
    }
    RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid Circle data.")); // fixme - sand - check whether it's possible at all??
    return nullptr;
}

void LC_ActionDrawCircle2Points::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetPoint1);
    reset();
}

void LC_ActionDrawCircle2Points::preparePreview() {
    // m_circleData.reset(new RS_CircleData{});
    if (m_actionData->point1.valid && m_actionData->point2.valid) {
        const bool success = LC_CreationCircle::createFrom2P(m_actionData->point1, m_actionData->point2,*m_circleData);
        if (!success) {
            m_circleData.reset(new RS_CircleData());
        }
    }
}

void LC_ActionDrawCircle2Points::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
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
            if (m_circleData->isValid()) {
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

bool LC_ActionDrawCircle2Points::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2);
}

void LC_ActionDrawCircle2Points::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector coord = e->snapPoint;
    if (status == SetPoint2) {
        coord = getSnapAngleAwarePoint(e, m_actionData->point1, coord);
    }
    fireCoordinateEvent(coord);
}

void LC_ActionDrawCircle2Points::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawCircle2Points::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            m_actionData->point2 = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawCircle2Points::updateActionPrompt() {
    switch (getStatus()) {
        case SetPoint1:
            updatePromptTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updatePromptTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}
