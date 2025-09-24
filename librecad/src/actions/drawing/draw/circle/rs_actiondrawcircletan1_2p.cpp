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

#include "rs_actiondrawcircletan1_2p.h"

#include "lc_quadratic.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_line.h"

namespace {
    //list of entity types supported by current action
    const EntityTypeList g_enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

struct RS_ActionDrawCircleTan1_2P::ActionData {
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
RS_ActionDrawCircleTan1_2P::RS_ActionDrawCircleTan1_2P(LC_ActionContext *actionContext)
    :LC_ActionDrawCircleBase("Draw tangent circle 2P",actionContext, RS2::ActionDrawCircleTan1_2P)
    ,m_actionData(std::make_unique<ActionData>()), m_baseEntity(nullptr){
}

RS_ActionDrawCircleTan1_2P::~RS_ActionDrawCircleTan1_2P() = default;


void RS_ActionDrawCircleTan1_2P::init(int status) {
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_PreviewActionInterface::suspend();
    }

    if (status <= SetCircle1){
       redrawDrawing();
       m_actionData->points.clear();
    }
}

void RS_ActionDrawCircleTan1_2P::finish(bool updateTB){
    if (m_baseEntity){
        redrawDrawing();
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan1_2P::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
     if (g_enTypeList.contains(contextEntity->rtti())) {
         setCircleOne(contextEntity);
     }
}

void RS_ActionDrawCircleTan1_2P::doInitialInit() {
    LC_ActionDrawCircleBase::doInitialInit();
}

void RS_ActionDrawCircleTan1_2P::doTrigger() {
    //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
    //    std::cout<<"begin"<<std::endl;
    auto *c = new RS_Circle(m_container, m_actionData->cData);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(c->getCenter());
    }

    undoCycleAdd(c);

    setStatus(SetCircle1);
    RS_DEBUG->print("RS_ActionDrawCircleTan1_2P::trigger(): entity added: %lu", c->getId());
}

void RS_ActionDrawCircleTan1_2P::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector const &mouse = e->snapPoint;
    switch (status) {
        case SetCircle1:{
            deleteSnapper();
            RS_Entity *en = catchTangentEntity(e, true);
            if (en != nullptr){
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
                    double baseEntityRadius = m_baseEntity->getRadius();
                    const RS_Vector &baseEntityCenter = m_baseEntity->getCenter();
                    RS_Vector const &dvp = mouse - baseEntityCenter;
                    double rvp = dvp.magnitude();
                    if (rvp < RS_TOLERANCE2) {
                        break;
                    }
                    m_actionData->cData.radius = (baseEntityRadius + rvp) * 0.5;
                    m_actionData->cData.center = baseEntityCenter + dvp * (m_actionData->cData.radius / rvp);
                    m_actionData->cData.radius = fabs(baseEntityRadius - m_actionData->cData.radius);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_actionData->cData.center);
                        previewRefPoint(getTangentPoint(m_actionData->cData.center, true));
                    }
                    break;
                }
                case RS2::EntityLine: {
                    auto *line = dynamic_cast<RS_Line *>(m_baseEntity);
                    RS_Vector &&vp = line->getNearestPointOnEntity(m_actionData->points[0], false);
                    if (vp.valid){
                        m_actionData->cData.center = (vp + m_actionData->points[0]) * 0.5;
                        m_actionData->cData.radius = vp.distanceTo(m_actionData->cData.center);
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(vp);
                            previewRefPoint(m_actionData->cData.center);
                        }
                    }
                    break;
                }
                default:
                    return;
            }

            previewToCreateCircle(m_actionData->cData);
            break;
        }
        case SetPoint2: {
            highlightSelected(m_baseEntity);
            m_actionData->points.resize(1);
            m_actionData->points.push_back(mouse);
            m_actionData->coord = mouse;
            if (getCenters()){
                if (preparePreview()){
                    previewToCreateCircle((m_actionData->cData));
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_actionData->points.at(0));
                        previewRefPoint(m_actionData->cData.center);
                        if (isLine(m_baseEntity)) { // fixme - sand - support polyline
                            previewRefPoint(m_baseEntity->getNearestPointOnEntity(m_actionData->cData.center, false));
                        } else {
                            double baseEntityRadius = m_baseEntity->getRadius();
                            bool calcTangentFromOriginalCircle =
                                (m_actionData->cData.center.distanceTo(m_baseEntity->getCenter()) < baseEntityRadius) &&
                                (m_actionData->cData.radius < baseEntityRadius);
                            previewRefPoint(getTangentPoint(m_actionData->cData.center, calcTangentFromOriginalCircle));
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
            if (preparePreview()){
                previewToCreateCircle(m_actionData->cData);
                for (const auto &center: m_actionData->centers) {
                    previewRefSelectablePoint(center);
                }

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->points.at(0));
                    previewRefPoint(m_actionData->points.at(1));
                    if (isLine(m_baseEntity)) { // fixme - support of polyline
                        previewRefPoint(m_baseEntity->getNearestPointOnEntity(m_actionData->cData.center, false));
                    } else {
                        double baseEntityRadius = m_baseEntity->getRadius();
                        bool calcTangentFromOriginalCircle =
                            (m_actionData->cData.center.distanceTo(m_baseEntity->getCenter()) < baseEntityRadius) &&
                            (m_actionData->cData.radius < baseEntityRadius);
                        previewRefPoint(getTangentPoint(m_actionData->cData.center, calcTangentFromOriginalCircle));
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

RS_Vector RS_ActionDrawCircleTan1_2P::getTangentPoint(RS_Vector& creatingCircleCenter, bool fromOriginalCircle) const{
    const RS_Vector &circleCenter = m_baseEntity->getCenter();
    if (fromOriginalCircle){
        return circleCenter + RS_Vector::polar(m_baseEntity->getRadius(), circleCenter.angleTo(creatingCircleCenter));
    }
    else{
        return creatingCircleCenter + RS_Vector::polar(m_actionData->cData.radius, creatingCircleCenter.angleTo(circleCenter));
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
    m_actionData->centers.clear();
    if (getStatus() < SetPoint2) {
        return false;
    }

    LC_Quadratic lc0(m_baseEntity, m_actionData->points[0]);
    LC_Quadratic lc1(m_actionData->points[1], m_actionData->points[0]);
    auto list = LC_Quadratic::getIntersection(lc0, lc1);
//    DEBUG_HEADER
//    std::cout<<"intersections : "<<list<<std::endl;

    for (const RS_Vector &vp: list) {
        //when taking the path of center of tangent circle passing a given point,
        // the center is never closer to the circle center than the point, for internal and external tangent circles
        double ds0 = vp.distanceTo(m_actionData->points[0]);
        if (isCircle(m_baseEntity) || isArc(m_baseEntity)){
            double ds = vp.distanceTo(m_baseEntity->getCenter());
            //condition for tangential to the given circle
            double baseEntityRadius = m_baseEntity->getRadius();
            if (fabs(ds - (ds0 + baseEntityRadius)) > RS_TOLERANCE && fabs(ds - fabs(ds0 - baseEntityRadius)) > RS_TOLERANCE) {
                continue;
            }
        } else {
            double ds = 0.;
            m_baseEntity->getNearestPointOnEntity(vp, false, &ds);
            //condition for tangential to the given straight line
            if (fabs(ds - ds0) > RS_TOLERANCE) {
                continue;
            }
        }

        //avoid counting the same center
        bool existing = false;
        for (auto const &vq: m_actionData->centers) {
            if (vq.squaredTo(vp) < RS_TOLERANCE15){
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

bool RS_ActionDrawCircleTan1_2P::preparePreview(){
    if (m_actionData->centers.empty()) {
        getCenters();
    }
    if (m_actionData->centers.empty()) {
        return false;
    }
    m_actionData->cData.center = m_actionData->centers.getClosest(m_actionData->coord);
    m_actionData->cData.radius = m_actionData->points[0].distanceTo(m_actionData->cData.center);
    return true;
}

RS_Entity *RS_ActionDrawCircleTan1_2P::catchTangentEntity(LC_MouseEvent *e, bool forPreview){
    RS_Entity *ret = nullptr;
    RS_Entity *en;
    if (forPreview){
        en = catchModifiableAndDescribe(e, g_enTypeList);
    }
    else{
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

void RS_ActionDrawCircleTan1_2P::setCircleOne(RS_Entity* en) {
    m_baseEntity = dynamic_cast<RS_AtomicEntity *>(en);
    redrawDrawing();
    setStatus(SetPoint1);
}

void RS_ActionDrawCircleTan1_2P::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetCircle1: {
            RS_Entity *en = catchTangentEntity(e, false);
            if (en == nullptr){
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

void RS_ActionDrawCircleTan1_2P::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    // Return to last status:
    if (status > 0){
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionDrawCircleTan1_2P::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    m_actionData->coord = mouse;
    switch (status) {
        case SetPoint1: {
            m_actionData->points.clear();
            m_actionData->points.push_back(mouse);
            setStatus(status + 1);
            break;
        }
        case SetPoint2: {
            m_actionData->points.push_back(mouse);
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
    return m_actionData->cData.radius;
}
