/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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
 ******************************************************************************/

#include "lc_action_draw_circle_tangental_1entity_2points.h"

#include "lc_quadratic.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_line.h"

namespace {
    //list of entity types supported by current action
    const EntityTypeList g_enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

struct LC_ActionDrawCircleTangental1Entity2Points::ActionData {
    std::vector<RS_Vector> points;
    RS_CircleData circleData;
    RS_Vector coord;
    double radius = 0.;
    bool valid = false;
    //keep a list of centers found
    RS_VectorSolutions centers;
};

/**
 * Constructor.
 *
 */
LC_ActionDrawCircleTangental1Entity2Points::LC_ActionDrawCircleTangental1Entity2Points(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("ActionDrawCircleTangental1Entity2Points", actionContext, RS2::ActionDrawCircleTangental1Entity2Points),
      m_actionData(std::make_unique<ActionData>()) {
}

LC_ActionDrawCircleTangental1Entity2Points::~LC_ActionDrawCircleTangental1Entity2Points() = default;

void LC_ActionDrawCircleTangental1Entity2Points::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0) {
        RS_PreviewActionInterface::suspend();
    }

    if (status <= SetCircle1) {
        redrawDrawing();
        m_actionData->points.clear();
    }
}

void LC_ActionDrawCircleTangental1Entity2Points::finish() {
    if (m_baseEntity != nullptr) {
        redrawDrawing();
    }
    RS_PreviewActionInterface::finish();
}

void LC_ActionDrawCircleTangental1Entity2Points::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    if (g_enTypeList.contains(contextEntity->rtti())) {
        setCircleOne(contextEntity);
    }
}

void LC_ActionDrawCircleTangental1Entity2Points::doInitialInit() {
    LC_ActionDrawCircleBase::doInitialInit();
}

RS_Entity* LC_ActionDrawCircleTangental1Entity2Points::doTriggerCreateEntity() {
    auto* circle = new RS_Circle(m_document, m_actionData->circleData);
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(circle->getCenter());
    }
    return circle;
}

void LC_ActionDrawCircleTangental1Entity2Points::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetCircle1);
}

void LC_ActionDrawCircleTangental1Entity2Points::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector& mouse = e->snapPoint;
    auto& circleData = m_actionData->circleData;
    switch (status) {
        case SetCircle1: {
            deleteSnapper();
            const RS_Entity* en = catchTangentEntity(e, true);
            if (en != nullptr) {
                highlightHover(en);
            }
            break;
        }
        case SetPoint1: {
            highlightSelected(m_baseEntity);
            m_actionData->points.clear();
            m_actionData->points.push_back(mouse);
            switch (m_baseEntity->rtti()) {
                case RS2::EntityArc:
                case RS2::EntityCircle: {
                    const double baseEntityRadius = m_baseEntity->getRadius();
                    const RS_Vector& baseEntityCenter = m_baseEntity->getCenter();
                    const RS_Vector& dvp = mouse - baseEntityCenter;
                    const double rvp = dvp.magnitude();
                    if (rvp < RS_TOLERANCE2) {
                        break;
                    }
                    circleData.radius = (baseEntityRadius + rvp) * 0.5;
                    circleData.center = baseEntityCenter + dvp * (circleData.radius / rvp);
                    circleData.radius = fabs(baseEntityRadius - circleData.radius);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(circleData.center);
                        previewRefPoint(getTangentPoint(circleData.center, true));
                    }
                    break;
                }
                case RS2::EntityLine: {
                    const auto* line = static_cast<RS_Line*>(m_baseEntity);
                    RS_Vector&& vp = line->getNearestPointOnEntity(m_actionData->points[0], false);
                    if (vp.valid) {
                        circleData.center = (vp + m_actionData->points[0]) * 0.5;
                        circleData.radius = vp.distanceTo(circleData.center);
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(vp);
                            previewRefPoint(circleData.center);
                        }
                    }
                    break;
                }
                default:
                    return;
            }

            previewToCreateCircle(circleData);
            break;
        }
        case SetPoint2: {
            highlightSelected(m_baseEntity);
            m_actionData->points.resize(1);
            m_actionData->points.push_back(mouse);
            m_actionData->coord = mouse;
            if (getCenters()) {
                if (preparePreview()) {
                    previewToCreateCircle(circleData);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_actionData->points.at(0));
                        previewRefPoint(circleData.center);
                        if (isLine(m_baseEntity)) {
                            // fixme - sand - support polyline
                            previewRefPoint(m_baseEntity->getNearestPointOnEntity(circleData.center, false));
                        }
                        else {
                            const double baseEntityRadius = m_baseEntity->getRadius();
                            const RS_Vector& baseEntityCenter = m_baseEntity->getCenter();
                            const bool calcTangentFromOriginalCircle = (circleData.center.distanceTo(baseEntityCenter) < baseEntityRadius)
                                && (circleData.radius < baseEntityRadius);
                            previewRefPoint(getTangentPoint(circleData.center, calcTangentFromOriginalCircle));
                        }
                    }
                }
            }
            break;
        }
        case SetCenter: {
            deleteSnapper();
            highlightSelected(m_baseEntity);
            m_actionData->coord = e->graphPoint;
            if (preparePreview()) {
                previewToCreateCircle(circleData);
                for (const auto& center : m_actionData->centers) {
                    previewRefSelectablePoint(center);
                }

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->points.at(0));
                    previewRefPoint(m_actionData->points.at(1));
                    if (isLine(m_baseEntity)) {
                        // fixme - support of polyline
                        previewRefPoint(m_baseEntity->getNearestPointOnEntity(circleData.center, false));
                    }
                    else {
                        const double baseEntityRadius = m_baseEntity->getRadius();
                        const bool calcTangentFromOriginalCircle = (circleData.center.distanceTo(m_baseEntity->getCenter()) <
                            baseEntityRadius) && (circleData.radius < baseEntityRadius);
                        previewRefPoint(getTangentPoint(circleData.center, calcTangentFromOriginalCircle));
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawCircleTangental1Entity2Points::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2);
}

RS_Vector LC_ActionDrawCircleTangental1Entity2Points::getTangentPoint(const RS_Vector& creatingCircleCenter, const bool fromOriginalCircle) const {
    const RS_Vector& circleCenter = m_baseEntity->getCenter();
    if (fromOriginalCircle) {
        return circleCenter + RS_Vector::polar(m_baseEntity->getRadius(), circleCenter.angleTo(creatingCircleCenter));
    }
    return creatingCircleCenter + RS_Vector::polar(m_actionData->circleData.radius, creatingCircleCenter.angleTo(circleCenter));
}

bool LC_ActionDrawCircleTangental1Entity2Points::getCenters() const {
    m_actionData->centers.clear();
    if (getStatus() < SetPoint2) {
        return false;
    }

    const LC_Quadratic lc0(m_baseEntity, m_actionData->points[0]);
    const LC_Quadratic lc1(m_actionData->points[1], m_actionData->points[0]);
    auto list = LC_Quadratic::getIntersection(lc0, lc1);
    //    DEBUG_HEADER
    //    std::cout<<"intersections : "<<list<<std::endl;

    for (const RS_Vector& vp : list) {
        //when taking the path of center of tangent circle passing a given point,
        // the center is never closer to the circle center than the point, for internal and external tangent circles
        const double ds0 = vp.distanceTo(m_actionData->points[0]);
        if (isCircle(m_baseEntity) || isArc(m_baseEntity)) {
            const double ds = vp.distanceTo(m_baseEntity->getCenter());
            //condition for tangential to the given circle
            const double baseEntityRadius = m_baseEntity->getRadius();
            if (fabs(ds - (ds0 + baseEntityRadius)) > RS_TOLERANCE && fabs(ds - fabs(ds0 - baseEntityRadius)) > RS_TOLERANCE) {
                continue;
            }
        }
        else {
            double ds = 0.;
            m_baseEntity->getNearestPointOnEntity(vp, false, &ds, nullptr);
            //condition for tangential to the given straight line
            if (fabs(ds - ds0) > RS_TOLERANCE) {
                continue;
            }
        }

        //avoid counting the same center
        bool existing = false;
        for (const auto& vq : m_actionData->centers) {
            if (vq.squaredTo(vp) < RS_TOLERANCE15) {
                existing = true;
                break;
            }
        }
        if (existing) {
            continue;
        }
        m_actionData->centers.push_back(vp);
    }
    //    DEBUG_HEADER
    //    std::cout<<"points: "<<points[0]<<" , "<<points[1]<<std::endl;
    //    std::cout<<"centers.size()="<<centers.size()<<std::endl;
    //    std::cout<<"centers: "<<centers<<std::endl;
    m_actionData->valid = !m_actionData->centers.empty();
    return m_actionData->valid;
}

bool LC_ActionDrawCircleTangental1Entity2Points::preparePreview() const {
    if (m_actionData->centers.empty()) {
        getCenters();
    }
    if (m_actionData->centers.empty()) {
        return false;
    }
    m_actionData->circleData.center = m_actionData->centers.getClosest(m_actionData->coord);
    m_actionData->circleData.radius = m_actionData->points[0].distanceTo(m_actionData->circleData.center);
    return true;
}

RS_Entity* LC_ActionDrawCircleTangental1Entity2Points::catchTangentEntity(const LC_MouseEvent* e, const bool forPreview) const {
    RS_Entity* ret = nullptr;
    RS_Entity* en;
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
    return en;
}

void LC_ActionDrawCircleTangental1Entity2Points::setCircleOne(RS_Entity* en) {
    m_baseEntity = dynamic_cast<RS_AtomicEntity*>(en);
    redrawDrawing();
    setStatus(SetPoint1);
}

void LC_ActionDrawCircleTangental1Entity2Points::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetCircle1: {
            RS_Entity* en = catchTangentEntity(e, false);
            if (en == nullptr) {
                return;
            }
            setCircleOne(en);
            invalidateSnapSpot();
            break;
        }
        case SetPoint1:
        case SetPoint2: {
            fireCoordinateEventForSnap(e);
            break;
        }
        case SetCenter:
            m_actionData->coord = e->graphPoint;
            if (preparePreview()) {
                trigger();
            }
            invalidateSnapSpot();
            break;

        default:
            break;
    }
}

void LC_ActionDrawCircleTangental1Entity2Points::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    // Return to last status:
    if (status > 0) {
        deletePreview();
    }
    initPrevious(status);
}

void LC_ActionDrawCircleTangental1Entity2Points::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    m_actionData->coord = coord;
    switch (status) {
        case SetPoint1: {
            m_actionData->points.clear();
            m_actionData->points.push_back(coord);
            setStatus(status + 1);
            break;
        }
        case SetPoint2: {
            m_actionData->points.push_back(coord);
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
void RS_ActionDrawCircleTan1_2P::commandEvent(RS_CommandEvent* e) {
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

//void RS_ActionDrawCircleTan1_2P::showOptions() {
//    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::showOptions");
//        RS_ActionInterface::showOptions();

//        RS_DIALOGFACTORY->requestOptions(this, true);
//    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::showOptions: OK");
//}

//void RS_ActionDrawCircleTan1_2P::hideOptions() {
//        RS_ActionInterface::hideOptions();

//        RS_DIALOGFACTORY->requestOptions(this, false);
//}

void LC_ActionDrawCircleTangental1Entity2Points::updateActionPrompt() {
    switch (getStatus()) {
        case SetCircle1:
            updatePromptTRCancel(tr("Specify a line/arc/circle"));
            break;
        case SetPoint1:
            updatePromptTRBack(tr("Specify the first point on the tangent circle"));
            break;
        case SetPoint2:
            updatePromptTRBack(tr("Specify the second point on the tangent circle"));
            break;
        case SetCenter:
            updatePromptTRBack(tr("Select the center of the tangent circle"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawCircleTangental1Entity2Points::doGetMouseCursor(const int status) {
    switch (status) {
        case SetCircle1:
        case SetCenter:
            return RS2::SelectCursor;
        case SetPoint1:
        case SetPoint2:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}

double LC_ActionDrawCircleTangental1Entity2Points::getRadius() const {
    return m_actionData->circleData.radius;
}
