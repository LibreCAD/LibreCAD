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

#include "lc_action_draw_circle_tangental_2entities_radius.h"

#include "lc_circle_tangental_2_entities_radius_options_filler.h"
#include "lc_circle_tangental_2_entities_radius_options_widget.h"
#include "lc_creation_circle.h"
#include "rs_atomicentity.h"
#include "rs_circle.h"
#include "rs_document.h"

namespace {
    //list of entity types supported by current action
    const EntityTypeList g_enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

struct LC_ActionDrawCircleTangental2EntitiesRadius::ActionData {
    RS_CircleData circleData;
    RS_Vector coord;
    bool valid{false};
    RS_VectorSolutions centers;
    std::vector<RS_AtomicEntity*> circles;
};

/**
 * Constructor.
 *
 */
LC_ActionDrawCircleTangental2EntitiesRadius::LC_ActionDrawCircleTangental2EntitiesRadius(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleBase("ActionDrawCircleTan2EntitiesRadius", actionContext, RS2::ActionDrawCircleTan2EntitiesRadius),
      m_actionData(std::make_unique<ActionData>()) {
}

LC_ActionDrawCircleTangental2EntitiesRadius::~LC_ActionDrawCircleTangental2EntitiesRadius() = default;

void LC_ActionDrawCircleTangental2EntitiesRadius::doSaveOptions() {
    save("Radius", getRadius());
}

void LC_ActionDrawCircleTangental2EntitiesRadius::doLoadOptions() {
    const double radius  = loadDouble("Radius", 1.0);
    setRadius(radius);
}

void LC_ActionDrawCircleTangental2EntitiesRadius::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0) {
        RS_PreviewActionInterface::suspend();
    }
}

void LC_ActionDrawCircleTangental2EntitiesRadius::doInitialInit() {
    m_actionData->circles.clear();
}

void LC_ActionDrawCircleTangental2EntitiesRadius::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    if (g_enTypeList.contains(contextEntity->rtti())) {
        setCircleOne(contextEntity);
    }
}

void LC_ActionDrawCircleTangental2EntitiesRadius::finish() {
    if (!m_actionData->circles.empty()) {
        for (const auto p : m_actionData->circles) {
            // todo - check whether we really need this?
            if (p != nullptr) {
                p->setHighlighted(false);
            }
        }
        redrawDrawing();
        m_actionData->circles.clear();
    }
    RS_PreviewActionInterface::finish();
}

RS_Entity* LC_ActionDrawCircleTangental2EntitiesRadius::doTriggerCreateEntity() {
    auto* circle = new RS_Circle(m_document, m_actionData->circleData);
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(circle->getCenter());
    }
    return circle;
}

void LC_ActionDrawCircleTangental2EntitiesRadius::doTriggerCompletion([[maybe_unused]] bool success) {
    for (const auto p : m_actionData->circles) {
        p->setHighlighted(false);
    }
    m_actionData->circles.clear();
    setStatus(SetCircle1);
}

bool LC_ActionDrawCircleTangental2EntitiesRadius::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

void LC_ActionDrawCircleTangental2EntitiesRadius::drawSnapper() {
    // disable snapper for action
}

void LC_ActionDrawCircleTangental2EntitiesRadius::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    for (RS_AtomicEntity* const pc : m_actionData->circles) {
        // highlight already selected
        highlightSelected(pc);
    }
    switch (getStatus()) {
        case SetCircle1: {
            const auto* c = catchTangentEntity(e, true);
            if (c != nullptr) {
                highlightHover(c);
            }
            break;
        }
        case SetCircle2: {
            auto* c = catchTangentEntity(e, true);
            if (c != nullptr) {
                if (getCenters(c)) {
                    highlightHover(c);
                }
            }
            break;
        }
        case SetCenter: {
            m_actionData->coord = e->graphPoint;
            if (preparePreview()) {
                previewToCreateCircle(m_actionData->circleData);
                for (const auto& center : m_actionData->centers) {
                    previewRefSelectablePoint(center);
                }
                if (m_showRefEntitiesOnPreview) {
                    for (const RS_AtomicEntity* const pc : m_actionData->circles) {
                        // highlight already selected // fixme - test and review, which cicle center is used
                        RS_Vector candidateCircleCenter = m_actionData->circleData.center;
                        if (isLine(pc)) {
                            // fixme - support of polyline
                            previewRefPoint(pc->getNearestPointOnEntity(candidateCircleCenter, false));
                        }
                        else {
                            previewRefPoint(getTangentPoint(candidateCircleCenter, m_actionData->circleData.radius, pc));
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawCircleTangental2EntitiesRadius::setRadius(const double r) {
    m_actionData->circleData.radius = r;
    if (getStatus() == SetCenter) {
        // force re-selection of circles to check whether this new radius is suitable
        m_actionData->circles.clear();
        setStatus(SetCircle1);
    }
}

bool LC_ActionDrawCircleTangental2EntitiesRadius::getCenters(RS_Entity* secondEntityCandidate) const {
    std::vector<RS_AtomicEntity*> circlesList;
    if (secondEntityCandidate != nullptr) {
        std::vector<RS_AtomicEntity*> testCirclesList = m_actionData->circles;
        auto* atomicSecond = dynamic_cast<RS_AtomicEntity*>(secondEntityCandidate);
        testCirclesList.push_back(atomicSecond);
        circlesList = testCirclesList;
    }
    else {
        circlesList = m_actionData->circles;
    }

    m_actionData->centers = LC_CreationCircle::createTan2(circlesList, m_actionData->circleData.radius);
    m_actionData->valid = !m_actionData->centers.empty();
    return m_actionData->valid;
}

bool LC_ActionDrawCircleTangental2EntitiesRadius::preparePreview() const {
    if (m_actionData->valid) {
        m_actionData->circleData.center = m_actionData->centers.getClosest(m_actionData->coord);
    }
    return m_actionData->valid;
}

RS_Entity* LC_ActionDrawCircleTangental2EntitiesRadius::catchTangentEntity(const LC_MouseEvent* e, const bool forPreview) const {
    RS_Entity* en = nullptr;
    // fixme - sand - check whether snap is used for entity selection?  Ensure free snap?
    if (forPreview) {
        en = catchModifiableAndDescribe(e, g_enTypeList);
    }
    else {
        en = catchModifiableEntity(e, g_enTypeList);
    }
    if (en == nullptr) {
        return nullptr;
    }
    if (!en->isVisible()) {
        return nullptr;
    }
    for (int i = 0; i < getStatus(); i++) {
        if (en->getId() == m_actionData->circles[i]->getId()) {
            return nullptr; //do not pull in the same line again
        }
    }

    return en;
}

void LC_ActionDrawCircleTangental2EntitiesRadius::setCircleOne(RS_Entity* en) {
    m_actionData->circles.resize(SetCircle1); // todo - what for? Why not have fixes size
    m_actionData->circles.push_back(dynamic_cast<RS_AtomicEntity*>(en));
    setStatus(SetCircle2);
}

void LC_ActionDrawCircleTangental2EntitiesRadius::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetCircle1: {
            RS_Entity* en = catchTangentEntity(e, false);
            if (en != nullptr) {
                setCircleOne(en);
            }
            break;
        }
        case SetCircle2: {
            RS_Entity* en = catchTangentEntity(e, false);
            if (en != nullptr) {
                m_actionData->circles.resize(getStatus());
                const bool hasCenters = getCenters(en);
                if (hasCenters) {
                    m_actionData->circles.push_back(dynamic_cast<RS_AtomicEntity*>(en));
                    if (m_actionData->centers.size() == 1) {
                        m_actionData->coord = m_actionData->centers.at(0);
                        trigger();
                    }
                    else {
                        setStatus(SetCenter);
                    }
                }
                else {
                    commandMessage(tr("No common tangential circle for radius '%1'").arg(m_actionData->circleData.radius));
                }
            }
            break;
        }
        case SetCenter:
            m_actionData->coord = e->graphPoint;
            if (preparePreview()) {
                trigger();
            }
            break;

        default:
            break;
    }
}

void LC_ActionDrawCircleTangental2EntitiesRadius::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    // Return to last status:
    if (getStatus() > 0) {
        m_actionData->circles.pop_back();
        redrawDrawing();
        deletePreview();
    }
    initPrevious(status);
}

void LC_ActionDrawCircleTangental2EntitiesRadius::updateActionPrompt() {
    switch (getStatus()) {
        case SetCircle1:
            updatePromptTRCancel(tr("Specify the first line/arc/circle"));
            break;
        case SetCircle2:
            updatePromptTRBack(tr("Specify the second line/arc/circle"));
            break;
        case SetCenter:
            updatePromptTRBack(tr("Select the center of the tangent circle"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionDrawCircleTangental2EntitiesRadius::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

double LC_ActionDrawCircleTangental2EntitiesRadius::getRadius() const {
    return m_actionData->circleData.radius;
}

// fixme - sand -  move to base class or util - and reuse among other actions
RS_Vector LC_ActionDrawCircleTangental2EntitiesRadius::getTangentPoint(const RS_Vector& creatingCircleCenter, const double creatingCircleRadius,
                                                   const RS_AtomicEntity* circle) {
    const double circleRadius = circle->getRadius();
    const RS_Vector& circleCenter = circle->getCenter();
    const bool calcTangentFromOriginalCircle = (creatingCircleCenter.distanceTo(circleCenter) < circleRadius) && (
        creatingCircleRadius < circleRadius);

    if (calcTangentFromOriginalCircle) {
        return circleCenter + RS_Vector::polar(circleRadius, circleCenter.angleTo(creatingCircleCenter));
    }
    return creatingCircleCenter + RS_Vector::polar(creatingCircleRadius, creatingCircleCenter.angleTo(circleCenter));
}

LC_ActionOptionsWidget* LC_ActionDrawCircleTangental2EntitiesRadius::createOptionsWidget() {
    return new LC_CircleTangental2EntitiesRadiusOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawCircleTangental2EntitiesRadius::createOptionsFiller() {
    return new LC_CircleTangental2EntitiesRadiusOptionsFiller();
}
