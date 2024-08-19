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

#include<vector>

#include <QMouseEvent>

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "qg_circletan2options.h"

namespace {

    //list of entity types supported by current action
const EntityTypeList enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}
struct RS_ActionDrawCircleTan2::Points {
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
RS_ActionDrawCircleTan2::RS_ActionDrawCircleTan2(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_ActionDrawCircleBase("Tangential 2 Circles, Radius",
                             container, graphicView), pPoints(std::make_unique<Points>()){
    actionType = RS2::ActionDrawCircleTan2;
}

RS_ActionDrawCircleTan2::~RS_ActionDrawCircleTan2() = default;

void RS_ActionDrawCircleTan2::init(int status){
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_PreviewActionInterface::suspend();
    }

    if (status == SetCircle1){
        pPoints->circles.clear();
    }
}

void RS_ActionDrawCircleTan2::finish(bool updateTB){
    if (!pPoints->circles.empty()){
        for (auto p: pPoints->circles) { // todo - check whether we really need this?
            if (p != nullptr){
                p->setHighlighted(false);
            }
        }
        graphicView->redraw(RS2::RedrawDrawing);
        pPoints->circles.clear();
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan2::trigger(){

    RS_PreviewActionInterface::trigger();

    auto *circle = new RS_Circle(container, pPoints->cData);

    container->addEntity(circle);

    addToDocumentUndoable(circle);

    for (auto p: pPoints->circles) {
        p->setHighlighted(false);
    }

    graphicView->redraw(RS2::RedrawDrawing);
    if (moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }
    //    drawSnapper();

    pPoints->circles.clear();
    setStatus(SetCircle1);

    RS_DEBUG->print("RS_ActionDrawCircleTan2::trigger(): entity added: %lu", circle->getId());
}

void RS_ActionDrawCircleTan2::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawCircleTan2::mouseMoveEvent begin");
    deleteHighlights();
    snapPoint(e);
    for (RS_AtomicEntity *const pc: pPoints->circles) { // highlight already selected
        highlightSelected(pc);
    }
    switch (getStatus()) {
        case SetCircle1: {
            auto *c = catchCircle(e);
            if (c != nullptr){
                highlightHover(c);
            }
            break;
        }
        case SetCircle2: {
            auto *c = catchCircle(e);
            if (c != nullptr){
                if (getCenters(c)){
                    highlightHover(c);
                }
            }
            break;
        }
        case SetCenter: {
            pPoints->coord = toGraph(e);

            if (preparePreview()){
                deletePreview();
                previewCircle(pPoints->cData);

                for (const auto &center: pPoints->centers) {
                    previewRefSelectablePoint(center);
                }

                if (showRefEntitiesOnPreview) {
                    for (RS_AtomicEntity *const pc: pPoints->circles) { // highlight already selected // fixme - test and review, which cicle center is used
                        RS_Vector candidateCircleCenter = pPoints->cData.center;
                        if (isLine(pc)) {
                            previewRefPoint(pc->getNearestPointOnEntity(candidateCircleCenter, false));
                        } else {
                            previewRefPoint(getTangentPoint(candidateCircleCenter, pPoints->cData.radius, pc));
                        }
                    }
                }
                drawPreview();
            }
        }
            break;
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionDrawCircleTan2::mouseMoveEvent end");
}

void RS_ActionDrawCircleTan2::setRadius(double r){
    pPoints->cData.radius = r;
    if (getStatus() == SetCenter){ // force re-selection of circles to check whether this new radius is suitable
        pPoints->circles.clear();
        setStatus(SetCircle1);
    }
}

bool RS_ActionDrawCircleTan2::getCenters(RS_Entity *secondEntityCandidate){
    std::vector<RS_AtomicEntity *> circlesList;
    if (secondEntityCandidate != nullptr){
        std::vector<RS_AtomicEntity *> testCirclesList = pPoints->circles;
        auto *atomicSecond = dynamic_cast<RS_AtomicEntity *>(secondEntityCandidate);
        testCirclesList.push_back(atomicSecond);
        circlesList = testCirclesList;
    } else {
        circlesList = pPoints->circles;
    }

    pPoints->centers = RS_Circle::createTan2(circlesList, pPoints->cData.radius);
    pPoints->valid = !pPoints->centers.empty();
    return pPoints->valid;
}

bool RS_ActionDrawCircleTan2::preparePreview(){
    if (pPoints->valid){
        pPoints->cData.center = pPoints->centers.getClosest(pPoints->coord);
    }
    return pPoints->valid;
}

RS_Entity *RS_ActionDrawCircleTan2::catchCircle(QMouseEvent *e){
    RS_Entity *en = catchModifiableEntity(e, enTypeList);  // fixme - sand - check whether snap is used for entity selection?  Ensure free snap?
    if (!en) return nullptr;
    if (!en->isVisible()) return nullptr;
    for (int i = 0; i < getStatus(); i++) {
        if (en->getId() == pPoints->circles[i]->getId()) return nullptr; //do not pull in the same line again
    }

    return en;
}

void RS_ActionDrawCircleTan2::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetCircle1: {
            RS_Entity *en = catchCircle(e);
            if (en != nullptr){
                pPoints->circles.resize(SetCircle1); // todo - what for? Why not have fixes size
                pPoints->circles.push_back(dynamic_cast<RS_AtomicEntity *>(en));
                setStatus(SetCircle2);
            }
            break;
        }
        case SetCircle2: {
            RS_Entity *en = catchCircle(e);
            if (en != nullptr){
                pPoints->circles.resize(getStatus());
                bool hasCenters = getCenters(en);
                if (hasCenters){
                    pPoints->circles.push_back(dynamic_cast<RS_AtomicEntity *>(en));
                    if (pPoints->centers.size() == 1){
                        pPoints->coord = pPoints->centers.at(0);
                        trigger();
                    }
                    else {
                        setStatus(SetCenter);
                    }
                } else {
                    commandMessage(tr("No common tangential circle for radius '%1'").arg(pPoints->cData.radius));
                }
            }
            break;
        }
        case SetCenter:
            pPoints->coord = toGraph(e);
            if (preparePreview()) {
                trigger();
            }
            break;

        default:
            break;
    }
}

void RS_ActionDrawCircleTan2::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    // Return to last status:
    if (getStatus() > 0){
//            pPoints->circles[getStatus() - 1]->setHighlighted(false);
        pPoints->circles.pop_back();
        graphicView->redraw(RS2::RedrawDrawing);
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
    return pPoints->cData.radius;
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
