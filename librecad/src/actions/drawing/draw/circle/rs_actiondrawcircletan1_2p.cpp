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

#include <QMouseEvent>

#include "lc_quadratic.h"
#include "rs_actiondrawcircletan1_2p.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_point.h"
#include "rs_preview.h"

namespace {
    //list of entity types supported by current action
    const EntityTypeList enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

struct RS_ActionDrawCircleTan1_2P::Points {
	std::vector<RS_Vector> points;
	RS_CircleData cData;
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
RS_ActionDrawCircleTan1_2P::RS_ActionDrawCircleTan1_2P(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :LC_ActionDrawCircleBase("Draw tangent circle 2P",
                               container, graphicView)
    ,pPoints(std::make_unique<Points>()), baseEntity(nullptr){
	actionType = RS2::ActionDrawCircleTan1_2P;
}

RS_ActionDrawCircleTan1_2P::~RS_ActionDrawCircleTan1_2P() = default;


void RS_ActionDrawCircleTan1_2P::init(int status) {
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_PreviewActionInterface::suspend();
    }

    if (status <= SetCircle1){
       graphicView->redraw(RS2::RedrawDrawing);
       pPoints->points.clear();
    }
}


void RS_ActionDrawCircleTan1_2P::finish(bool updateTB){
    if (baseEntity){
        graphicView->redraw(RS2::RedrawDrawing);
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan1_2P::trigger() {
    //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
    //    std::cout<<"begin"<<std::endl;

    RS_PreviewActionInterface::trigger();

    auto *c = new RS_Circle(container, pPoints->cData);

    container->addEntity(c);

    addToDocumentUndoable(c);

//    circle->setHighlighted(false);

    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    if (moveRelPointAtCenterAfterTrigger){
        rz = c->getCenter();
    }
    moveRelativeZero(rz);
    //    drawSnapper();

    setStatus(SetCircle1);

    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::trigger():"
                    " entity added: %lu", c->getId());
}

void RS_ActionDrawCircleTan1_2P::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::mouseMoveEvent begin");
    RS_Vector const &mouse = snapPoint(e);
    deletePreview();
    deleteHighlights();
    switch (getStatus()) {
        case SetCircle1:{
            RS_Entity *en = catchCircle(e);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }            
        case SetPoint1: {
            highlightSelected(baseEntity);
            pPoints->points.clear();
            pPoints->points.push_back(mouse);
            switch (baseEntity->rtti()) {
                case RS2::EntityArc:
                case RS2::EntityCircle: {
                    double baseEntityRadius = baseEntity->getRadius();
                    const RS_Vector &baseEntityCenter = baseEntity->getCenter();
                    RS_Vector const &dvp = mouse - baseEntityCenter;
                    double rvp = dvp.magnitude();
                    if (rvp < RS_TOLERANCE2) break;
                    pPoints->cData.radius = (baseEntityRadius + rvp) * 0.5;
                    pPoints->cData.center = baseEntityCenter + dvp * (pPoints->cData.radius / rvp);
                    pPoints->cData.radius = fabs(baseEntityRadius - pPoints->cData.radius);
                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(pPoints->cData.center);
                        previewRefPoint(getTangentPoint(pPoints->cData.center, true));
                    }
                    break;
                }
                case RS2::EntityLine: {
                    auto *line = dynamic_cast<RS_Line *>(baseEntity);
                    RS_Vector &&vp = line->getNearestPointOnEntity(pPoints->points[0], false);
                    if (vp.valid){
                        pPoints->cData.center = (vp + pPoints->points[0]) * 0.5;
                        pPoints->cData.radius = vp.distanceTo(pPoints->cData.center);
                        if (showRefEntitiesOnPreview) {
                            previewRefPoint(vp);
                            previewRefPoint(pPoints->cData.center);
                        }
                    }
                    break;
                }
                default:
                    return;
            }

            previewCircle(pPoints->cData);
            break;
        }
        case SetPoint2: {
            highlightSelected(baseEntity);
            pPoints->points.resize(1);
            pPoints->points.push_back(mouse);
            pPoints->coord = mouse;
            if (getCenters()){
                if (preparePreview()){
                    previewCircle((pPoints->cData));
                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(pPoints->points.at(0));
                        previewRefPoint(pPoints->cData.center);
                        if (isLine(baseEntity)) {
                            previewRefPoint(baseEntity->getNearestPointOnEntity(pPoints->cData.center, false));
                        } else {
                            double baseEntityRadius = baseEntity->getRadius();
                            bool calcTangentFromOriginalCircle =
                                (pPoints->cData.center.distanceTo(baseEntity->getCenter()) < baseEntityRadius) &&
                                (pPoints->cData.radius < baseEntityRadius);
                            previewRefPoint(getTangentPoint(pPoints->cData.center, calcTangentFromOriginalCircle));
                        }
                    }
                }
            }
            break;
        }
        case SetCenter: {
            highlightSelected(baseEntity);
            pPoints->coord = toGraph(e);
            if (preparePreview()){
                previewCircle(pPoints->cData);
                for (const auto &center: pPoints->centers) {
                    previewRefSelectablePoint(center);
                }

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->points.at(0));
                    previewRefPoint(pPoints->points.at(1));
                    if (isLine(baseEntity)) {
                        previewRefPoint(baseEntity->getNearestPointOnEntity(pPoints->cData.center, false));
                    } else {
                        double baseEntityRadius = baseEntity->getRadius();
                        bool calcTangentFromOriginalCircle =
                            (pPoints->cData.center.distanceTo(baseEntity->getCenter()) < baseEntityRadius) &&
                            (pPoints->cData.radius < baseEntityRadius);
                        previewRefPoint(getTangentPoint(pPoints->cData.center, calcTangentFromOriginalCircle));
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    drawPreview();
    drawHighlights();
    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::mouseMoveEvent end");
}

RS_Vector RS_ActionDrawCircleTan1_2P::getTangentPoint(RS_Vector& creatingCircleCenter, bool fromOriginalCircle) const{
    const RS_Vector &circleCenter = baseEntity->getCenter();
    if (fromOriginalCircle){
        return circleCenter + RS_Vector::polar(baseEntity->getRadius(), circleCenter.angleTo(creatingCircleCenter));
    }
    else{
        return creatingCircleCenter + RS_Vector::polar(pPoints->cData.radius, creatingCircleCenter.angleTo(circleCenter));
    }
}

//void RS_ActionDrawCircleTan1_2P::setRadius(const double& r)
//{
//    cData.radius=r;
//    if(getStatus() == SetPoint2){
//        RS_Circle c(nullptr,cData);
//        centers=c.createTan1_2P(circle,cData.radius);
//    }
//}

bool RS_ActionDrawCircleTan1_2P::getCenters(){
    pPoints->centers.clear();
    if (getStatus() < SetPoint2) return false;

    LC_Quadratic lc0(baseEntity, pPoints->points[0]);
//    LC_Quadratic lc1(circle, points[1]);
    LC_Quadratic lc1(pPoints->points[1], pPoints->points[0]);
    auto list = LC_Quadratic::getIntersection(lc0, lc1);
//    DEBUG_HEADER
//    std::cout<<"intersections : "<<list<<std::endl;

    for (const RS_Vector &vp: list) {
        //when taking the path of center of tangent circle passing a given point,
        // the center is never closer to the circle center than the point, for internal and external tangent circles
        double ds0 = vp.distanceTo(pPoints->points[0]);
        if (isCircle(baseEntity) || isArc(baseEntity)){
            double ds = vp.distanceTo(baseEntity->getCenter());
            //condition for tangential to the given circle
            double baseEntityRadius = baseEntity->getRadius();
            if (fabs(ds - (ds0 + baseEntityRadius)) > RS_TOLERANCE && fabs(ds - fabs(ds0 - baseEntityRadius)) > RS_TOLERANCE) continue;
        } else {
            double ds = 0.;
            baseEntity->getNearestPointOnEntity(vp, false, &ds);
            //condition for tangential to the given straight line
            if (fabs(ds - ds0) > RS_TOLERANCE) continue;
        }

        //avoid counting the same center
        bool existing = false;
        for (auto const &vq: pPoints->centers) {
            if (vq.squaredTo(vp) < RS_TOLERANCE15){
                existing = true;
                break;
            }
        }
        if (existing) continue;
        pPoints->centers.push_back(vp);
    }
//    DEBUG_HEADER
//    std::cout<<"points: "<<points[0]<<" , "<<points[1]<<std::endl;
//    std::cout<<"centers.size()="<<centers.size()<<std::endl;
//    std::cout<<"centers: "<<centers<<std::endl;
    pPoints->valid = !pPoints->centers.empty();
    return pPoints->valid;
}

bool RS_ActionDrawCircleTan1_2P::preparePreview(){
    if (pPoints->centers.empty()) getCenters();
    if (pPoints->centers.empty()) return false;
    pPoints->cData.center = pPoints->centers.getClosest(pPoints->coord);
    pPoints->cData.radius = pPoints->points[0].distanceTo(pPoints->cData.center);
    return true;
}

RS_Entity *RS_ActionDrawCircleTan1_2P::catchCircle(QMouseEvent *e){
    RS_Entity *ret = nullptr;
    RS_Entity *en = catchModifiableEntity(e, enTypeList);
    if (!en) return ret;
    if (!en->isVisible()) return ret;
    return en;
}

void RS_ActionDrawCircleTan1_2P::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetCircle1: {
            RS_Entity *en = catchCircle(e);
            if (!en) return;
            baseEntity = dynamic_cast<RS_AtomicEntity *>(en);
            graphicView->redraw(RS2::RedrawDrawing);
            setStatus(status + 1);
            break;
        }
        case SetPoint1:
        case SetPoint2: {
            fireCoordinateEventForSnap(e);
            break;
        }
        case SetCenter:
            pPoints->coord = toGraph(e);
            if (preparePreview()) trigger();
            break;

        default:
            break;
    }
}

void RS_ActionDrawCircleTan1_2P::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    // Return to last status:
    if (status > 0){
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionDrawCircleTan1_2P::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    pPoints->coord = mouse;
    switch (status) {
        case SetPoint1: {
            pPoints->points.clear();
            pPoints->points.push_back(mouse);
            setStatus(status + 1);
            break;
        }
        case SetPoint2: {
            pPoints->points.push_back(mouse);
            if (getCenters()) {
                if (pPoints->centers.size() == 1) {
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


void RS_ActionDrawCircleTan1_2P::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCircle1:
            updateMouseWidgetTRCancel(tr("Specify a line/arc/circle"));
            break;
        case SetPoint1:
            updateMouseWidgetTRBack(tr("Specify the first point on the tangent circle"));
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify the second point on the tangent circle"));
            break;
        case SetCenter:
            updateMouseWidgetTRBack(tr("Select the center of the tangent circle"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawCircleTan1_2P::doGetMouseCursor(int status){
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

double RS_ActionDrawCircleTan1_2P::getRadius() const{
    return pPoints->cData.radius;
}
