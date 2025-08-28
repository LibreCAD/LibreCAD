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

#include "rs_actiondrawcircletan2.h"

#include "qg_circletan2options.h"
#include "rs_atomicentity.h"
#include "rs_circle.h"
#include "rs_debug.h"

namespace {

    //list of entity types supported by current action
const EntityTypeList g_enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}
struct RS_ActionDrawCircleTan2::ActionData {
    RS_CircleData cData;
    RS_Vector coord;
    double radius{0.};
    bool valid{false};
    RS_VectorSolutions centers;
    std::vector<RS_AtomicEntity *> circles;
};

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan2::RS_ActionDrawCircleTan2(LC_ActionContext *actionContext)
    :LC_ActionDrawCircleBase("Tangential 2 Circles, Radius",actionContext,RS2::ActionDrawCircleTan2), m_actionData(std::make_unique<ActionData>()){
}

RS_ActionDrawCircleTan2::~RS_ActionDrawCircleTan2() = default;

void RS_ActionDrawCircleTan2::init(int status){
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_PreviewActionInterface::suspend();
    }
}

void RS_ActionDrawCircleTan2::doInitialInit() {
    m_actionData->circles.clear();
}

void RS_ActionDrawCircleTan2::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (g_enTypeList.contains(contextEntity->rtti())) {
        setCircleOne(contextEntity);
    }
}

void RS_ActionDrawCircleTan2::finish(bool updateTB){
    if (!m_actionData->circles.empty()){
        for (auto p: m_actionData->circles) { // todo - check whether we really need this?
            if (p != nullptr){
                p->setHighlighted(false);
            }
        }
        redrawDrawing();
        m_actionData->circles.clear();
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan2::doTrigger() {
    auto *circle = new RS_Circle(m_container, m_actionData->cData);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }

    undoCycleAdd(circle);

    for (auto p: m_actionData->circles) {
        p->setHighlighted(false);
    }
    m_actionData->circles.clear();
    setStatus(SetCircle1);

    RS_DEBUG->print("RS_ActionDrawCircleTan2::trigger(): entity added: %lu", circle->getId());
}

bool RS_ActionDrawCircleTan2::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

void RS_ActionDrawCircleTan2::drawSnapper() {
    // disable snapper for action
}

void RS_ActionDrawCircleTan2::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    for (RS_AtomicEntity *const pc: m_actionData->circles) { // highlight already selected
        highlightSelected(pc);
    }
    switch (getStatus()) {
        case SetCircle1: {
            auto *c = catchTangentEntity(e, true);
            if (c != nullptr){
                highlightHover(c);
            }
            break;
        }
        case SetCircle2: {
            auto *c = catchTangentEntity(e, true);
            if (c != nullptr){
                if (getCenters(c)){
                    highlightHover(c);
                }
            }
            break;
        }
        case SetCenter: {
            m_actionData->coord = e->graphPoint;
            if (preparePreview()){
                previewToCreateCircle(m_actionData->cData);
                for (const auto &center: m_actionData->centers) {
                    previewRefSelectablePoint(center);
                }
                if (m_showRefEntitiesOnPreview) {
                    for (RS_AtomicEntity *const pc: m_actionData->circles) { // highlight already selected // fixme - test and review, which cicle center is used
                        RS_Vector candidateCircleCenter = m_actionData->cData.center;
                        if (isLine(pc)) { // fixme - support of polyline
                            previewRefPoint(pc->getNearestPointOnEntity(candidateCircleCenter, false));
                        } else {
                            previewRefPoint(getTangentPoint(candidateCircleCenter, m_actionData->cData.radius, pc));
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

void RS_ActionDrawCircleTan2::setRadius(double r){
    m_actionData->cData.radius = r;
    if (getStatus() == SetCenter){ // force re-selection of circles to check whether this new radius is suitable
        m_actionData->circles.clear();
        setStatus(SetCircle1);
    }
}

bool RS_ActionDrawCircleTan2::getCenters(RS_Entity *secondEntityCandidate){
    std::vector<RS_AtomicEntity *> circlesList;
    if (secondEntityCandidate != nullptr){
        std::vector<RS_AtomicEntity *> testCirclesList = m_actionData->circles;
        auto *atomicSecond = dynamic_cast<RS_AtomicEntity *>(secondEntityCandidate);
        testCirclesList.push_back(atomicSecond);
        circlesList = testCirclesList;
    } else {
        circlesList = m_actionData->circles;
    }

    m_actionData->centers = RS_Circle::createTan2(circlesList, m_actionData->cData.radius);
    m_actionData->valid = !m_actionData->centers.empty();
    return m_actionData->valid;
}

bool RS_ActionDrawCircleTan2::preparePreview(){
    if (m_actionData->valid){
        m_actionData->cData.center = m_actionData->centers.getClosest(m_actionData->coord);
    }
    return m_actionData->valid;
}

RS_Entity *RS_ActionDrawCircleTan2::catchTangentEntity(LC_MouseEvent *e, bool forPreview){
    RS_Entity* en;
    // fixme - sand - check whether snap is used for entity selection?  Ensure free snap?
    if (forPreview) {
        en = catchModifiableAndDescribe(e, g_enTypeList);
    }
    else{
        en = catchModifiableEntity(e, g_enTypeList);
    }
    if (en == nullptr){
        return nullptr;
    }
    if (!en->isVisible()) {
        return nullptr;
    }
    for (int i = 0; i < getStatus(); i++) {
        if (en->getId() == m_actionData->circles[i]->getId()){
            return nullptr; //do not pull in the same line again
        }
    }

    return en;
}

void RS_ActionDrawCircleTan2::setCircleOne(RS_Entity* en) {
    m_actionData->circles.resize(SetCircle1); // todo - what for? Why not have fixes size
    m_actionData->circles.push_back(dynamic_cast<RS_AtomicEntity *>(en));
    setStatus(SetCircle2);
}

void RS_ActionDrawCircleTan2::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetCircle1: {
            RS_Entity *en = catchTangentEntity(e,false);
            if (en != nullptr){
                setCircleOne(en);
            }
            break;
        }
        case SetCircle2: {
            RS_Entity *en = catchTangentEntity(e, false);
            if (en != nullptr){
                m_actionData->circles.resize(getStatus());
                bool hasCenters = getCenters(en);
                if (hasCenters){
                    m_actionData->circles.push_back(dynamic_cast<RS_AtomicEntity *>(en));
                    if (m_actionData->centers.size() == 1){
                        m_actionData->coord = m_actionData->centers.at(0);
                        trigger();
                    }
                    else {
                        setStatus(SetCenter);
                    }
                } else {
                    commandMessage(tr("No common tangential circle for radius '%1'").arg(m_actionData->cData.radius));
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

void RS_ActionDrawCircleTan2::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    // Return to last status:
    if (getStatus() > 0){
//            pPoints->circles[getStatus() - 1]->setHighlighted(false);
        m_actionData->circles.pop_back();
        redrawDrawing();
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionDrawCircleTan2::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCircle1:
            updateMouseWidgetTRCancel(tr("Specify the first line/arc/circle"));
            break;
        case SetCircle2:
            updateMouseWidgetTRBack(tr("Specify the second line/arc/circle"));
            break;
        case SetCenter:
            updateMouseWidgetTRBack(tr("Select the center of the tangent circle"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawCircleTan2::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

double RS_ActionDrawCircleTan2::getRadius() const{
    return m_actionData->cData.radius;
}

// fixme - sand -  move to base class or util - and reuse among other actions
RS_Vector RS_ActionDrawCircleTan2::getTangentPoint(RS_Vector creatingCircleCenter, double creatingCircleRadius, const RS_AtomicEntity *circle){
    bool calcTangentFromOriginalCircle = (creatingCircleCenter.distanceTo(circle->getCenter()) < circle->getRadius()) &&
                                         (creatingCircleRadius < circle->getRadius());
    const RS_Vector &circleCenter = circle->getCenter();
    if (calcTangentFromOriginalCircle){
        return circleCenter + RS_Vector::polar(circle->getRadius(), circleCenter.angleTo(creatingCircleCenter));
    } else {
        return creatingCircleCenter + RS_Vector::polar(creatingCircleRadius, creatingCircleCenter.angleTo(circleCenter));
    }
}

LC_ActionOptionsWidget* RS_ActionDrawCircleTan2::createOptionsWidget(){
    return new QG_CircleTan2Options();
}
