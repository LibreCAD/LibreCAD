/****************************************************************************
**
 * Draw circle by foci and a point on circle

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "lc_action_draw_circle_tangental_2entities_1point.h"

#include "lc_quadratic.h"
#include "rs_circle.h"
#include "rs_document.h"

namespace {
    //list of entity types supported by current action
    const EntityTypeList g_enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

struct LC_ActionDrawCircleTangental2Entities1Point::ActionData {
    RS_Vector point;
    RS_CircleData circleData;
    RS_Vector coord;
    //keep a list of centers found
    RS_VectorSolutions centers;
    std::vector<RS_AtomicEntity*> circles;
};

/**
 * Constructor.
 *
 */
LC_ActionDrawCircleTangental2Entities1Point::LC_ActionDrawCircleTangental2Entities1Point(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("ActionDrawCircleTangental1Entity2Points", actionContext, RS2::ActionDrawCircleTangental2Entities1Point),
      m_actionData(std::make_unique<ActionData>()) {
}

LC_ActionDrawCircleTangental2Entities1Point::~LC_ActionDrawCircleTangental2Entities1Point() = default;

void LC_ActionDrawCircleTangental2Entities1Point::init(int status) {
    if (status >= 0) {
        RS_PreviewActionInterface::suspend();
    }
    const int pointsSize = static_cast<int>(m_actionData->circles.size());
    if (status > pointsSize) {
        status = pointsSize;
    }
    for (int i = 0; i < status; ++i) {
        if (m_actionData->circles[i] == nullptr) {
            status = i;
            break;
        }
    }
    m_actionData->circles.resize(status >= 0 ? status : 0);
    LC_ActionDrawCircleBase::init(status);
}

void LC_ActionDrawCircleTangental2Entities1Point::doInitialInit() {
    LC_ActionDrawCircleBase::doInitialInit();
}

void LC_ActionDrawCircleTangental2Entities1Point::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    if (g_enTypeList.contains(contextEntity->rtti())) {
        setCircle(contextEntity, SetCircle1);
    }
}

void LC_ActionDrawCircleTangental2Entities1Point::finish() {
    RS_PreviewActionInterface::finish();
}

RS_Entity* LC_ActionDrawCircleTangental2Entities1Point::doTriggerCreateEntity() {
    auto* circle = new RS_Circle(m_document, m_actionData->circleData);
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(circle->getCenter());
    }
    return circle;
}

bool LC_ActionDrawCircleTangental2Entities1Point::isInVisualSnapStatus(int status) {
    return (status == SetPoint);
}

void LC_ActionDrawCircleTangental2Entities1Point::doTriggerCompletion([[maybe_unused]] bool success) {
    m_actionData->circles.clear();
    init(SetCircle1);
}

bool LC_ActionDrawCircleTangental2Entities1Point::getCenters() const {
    if (m_actionData->circles.size() < 2) {
        return false;
    }
    const LC_Quadratic lc0(m_actionData->circles[0], m_actionData->point);
    const LC_Quadratic lc1(m_actionData->circles[1], m_actionData->point);
    const auto& list = LC_Quadratic::getIntersection(lc0, lc1);
    m_actionData->centers.clear();
    for (const RS_Vector& vp : list) {
        const auto ds = vp.distanceTo(m_actionData->point) - RS_TOLERANCE;
        bool validBranch(true);
        for (const RS_AtomicEntity* circle : m_actionData->circles) {
            // issue #1288, pull request #1445 by melwyncarlo: ignore center on circles
            double distance = RS_MAXDOUBLE;
            circle->getNearestPointOnEntity(vp, false, &distance, nullptr);
            if (distance < ds) {
                validBranch = false;
                break;
            }
            if (isCircle(circle) || isArc(circle)) {
                if (vp.distanceTo(circle->getCenter()) <= ds) {
                    validBranch = false;
                    break;
                }
            }
        }
        if (validBranch) {
            m_actionData->centers.push_back(vp);
        }
    }
    return !m_actionData->centers.empty();
}

void LC_ActionDrawCircleTangental2Entities1Point::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector coord = e->snapPoint;
    switch (status) {
        case SetCircle1:
        case SetCircle2: {
            deleteSnapper();
            const auto* circle = catchTangentEntity(e, true);
            if (circle != nullptr) {
                highlightHover(circle);
            }
            break;
        }
        case SetPoint: {
            m_actionData->coord = coord;
            m_actionData->point = m_actionData->coord;
            break;
        }
        case SetCenter: {
            deleteSnapper();
            m_actionData->coord = e->graphPoint; // fixme -= or snap point?
            break;
        }
        default:
            return;
    }
    for (RS_AtomicEntity* const circle : m_actionData->circles) {
        highlightSelected(circle);
    }

    if (preparePreview()) {
        for (const auto& vp : m_actionData->centers) {
            previewRefSelectablePoint(vp);
        }

        if (m_showRefEntitiesOnPreview) {
            if (status == SetPoint || status == SetCenter) {
                const RS_Vector center = m_actionData->circleData.center;
                previewRefPoint(m_actionData->circles.at(0)->getNearestPointOnEntity(center, false));
                previewRefPoint(m_actionData->circles.at(1)->getNearestPointOnEntity(center, false));
            }
        }
        previewToCreateCircle(m_actionData->circleData);
    }
}

bool LC_ActionDrawCircleTangental2Entities1Point::preparePreview() const {
    if (!getCenters()) {
        return false;
    }
    m_actionData->circleData.center = m_actionData->centers.getClosest(m_actionData->coord);
    m_actionData->circleData.radius = m_actionData->point.distanceTo(m_actionData->circleData.center);
    return true;
}

RS_Entity* LC_ActionDrawCircleTangental2Entities1Point::catchTangentEntity(const LC_MouseEvent* e, const bool forPreview) const {
    RS_Entity* ret = nullptr;
    RS_Entity* en = nullptr;
    if (forPreview) {
        en = catchModifiableAndDescribe(e, g_enTypeList);
    }
    else {
        en = catchModifiableEntity(e, g_enTypeList);
    }
    if (en == nullptr) {
        return ret;
    }
    if (!en->isVisible()) {
        return ret;
    }
    for (const auto p : m_actionData->circles) {
        if ((p != nullptr) && en->getId() == p->getId()) {
            return ret; //do not pull in the same line again
        }
    }
    return en;
}

void LC_ActionDrawCircleTangental2Entities1Point::setCircle(RS_Entity* en, const int status) {
    const auto atomicEntity = dynamic_cast<RS_AtomicEntity*>(en);
    if (atomicEntity == nullptr) {
        return;
    }
    m_actionData->circles.resize(status);
    m_actionData->circles.push_back(atomicEntity);
    redrawDrawing();
    setStatus(status + 1);
}

void LC_ActionDrawCircleTangental2Entities1Point::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetCircle1:
        case SetCircle2: {
            const auto entity = catchTangentEntity(e, false);
            setCircle(entity, status);
            invalidateSnapSpot();
            break;
        }
        case SetPoint: {
            const RS_Vector snapped = e->snapPoint;
            fireCoordinateEvent(snapped);
            break;
        }
        case SetCenter: {
            m_actionData->coord = e->graphPoint;
            if (preparePreview()) {
                trigger();
            }
            invalidateSnapSpot();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawCircleTangental2Entities1Point::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    // Return to last status:
    if (status > 0) {
        deletePreview();
    }
    initPrevious(status);
}

void LC_ActionDrawCircleTangental2Entities1Point::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetPoint: {
            m_actionData->point = coord;
            m_actionData->coord = coord;
            if (getCenters()) {
                if (m_actionData->centers.size() == 1) {
                    trigger();
                }
                else {
                    setStatus(status + 1);
                }
            }
            break;
        }
        default:
            break;
    }
}

//fixme, support command line

/*
void RS_ActionDrawCircleTan2_1P::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
   if (ok) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
   if (ok) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
   if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
   } else
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
        }
        break;

    default:
        break;
    }
}
*/

void LC_ActionDrawCircleTangental2Entities1Point::updateActionPrompt() {
    switch (getStatus()) {
        case SetCircle1:
            updatePromptTRCancel(tr("Specify a line/arc/circle"));
            break;
        case SetCircle2:
            updatePromptTRBack(tr("Specify the another line/arc/circle"));
            break;
        case SetPoint:
            updatePromptTRBack(tr("Specify a point on the tangent circle"));
            break;
        case SetCenter:
            updatePromptTRBack(tr("Select the center of the tangent circle"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawCircleTangental2Entities1Point::doGetMouseCursor([[maybe_unused]] const int status) {
    switch (status) {
        case SetCircle1:
        case SetCircle2:
        case SetCenter:
            return RS2::SelectCursor;
        case SetPoint:
            return RS2::CadCursor;
        default:
            return RS2::ArrowCursor;
    }
}
