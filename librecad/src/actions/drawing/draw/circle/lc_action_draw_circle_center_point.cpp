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

#include "lc_action_draw_circle_center_point.h"

#include "rs_circle.h"
#include "rs_document.h"

LC_ActionDrawCircleCenterPoint::LC_ActionDrawCircleCenterPoint(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("ActionDrawCircleCenterPoint", actionContext, RS2::ActionDrawCircleCenterPoint), m_circleData(std::make_unique<RS_CircleData>()) {
}

LC_ActionDrawCircleCenterPoint::~LC_ActionDrawCircleCenterPoint() = default;

void LC_ActionDrawCircleCenterPoint::reset() {
    m_circleData = std::make_unique<RS_CircleData>();
}

RS_Entity* LC_ActionDrawCircleCenterPoint::doTriggerCreateEntity() {
    auto* circle = new RS_Circle(m_document, *m_circleData);
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(circle->getCenter());
    }
    return circle;
}

void LC_ActionDrawCircleCenterPoint::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetCenter);
    reset();
}

void LC_ActionDrawCircleCenterPoint::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetCenter: {
            m_circleData->center = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetRadius: {
            const auto& center = m_circleData->center;
            if (center.valid) {
                m_circleData->radius = center.distanceTo(mouse);
                previewToCreateCircle(*m_circleData);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(center);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(center, mouse);
                }
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawCircleCenterPoint::isInVisualSnapStatus(int status) {
    return (status == SetCenter) || (status == SetRadius);
}

void LC_ActionDrawCircleCenterPoint::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetCenter:
            m_circleData->center = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetRadius);
            break;
        case SetRadius:
            if (m_circleData->center.valid) {
                addSnappedPointToVisualSnap(coord);
                moveRelativeZero(coord);
                m_circleData->radius = m_circleData->center.distanceTo(coord);
                trigger();
            }
            break;
        default:
            break;
    }
}

bool LC_ActionDrawCircleCenterPoint::doProcessCommand(const int status, const QString& command) {
    bool accept = false;
    switch (status) {
        case SetRadius: {
            bool ok = false;
            const double r = RS_Math::eval(command, &ok);
            if (ok && r > RS_TOLERANCE) {
                m_circleData->radius = r;
                accept = true;
                trigger();
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            break;
        }
        default:
            break;
    }
    return accept;
}

void LC_ActionDrawCircleCenterPoint::updateActionPrompt() {
    switch (getStatus()) {
        case SetCenter:
            updatePromptTRCancel(tr("Specify center"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetRadius:
            updatePromptTRBack(tr("Specify point on circle"));
            break;
        default:
            updatePrompt();
            break;
    }
}
