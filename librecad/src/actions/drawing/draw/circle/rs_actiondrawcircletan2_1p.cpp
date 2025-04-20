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

#include "rs_actiondrawcircletan2_1p.h"

#include "lc_quadratic.h"
#include "rs_circle.h"
#include "rs_debug.h"

namespace {

    //list of entity types supported by current action
    const EntityTypeList g_enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}
struct RS_ActionDrawCircleTan2_1P::ActionData {
    RS_Vector point;
    RS_CircleData cData;
    RS_Vector coord;
    double radius = 0.;
    bool valid = false;
//keep a list of centers found
    RS_VectorSolutions centers;
    std::vector<RS_AtomicEntity *> circles;
};

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan2_1P::RS_ActionDrawCircleTan2_1P(LC_ActionContext *actionContext)
    :LC_ActionDrawCircleBase("Draw tangent circle 2P", actionContext,RS2::ActionDrawCircleTan2_1P), m_actionData(std::make_unique<ActionData>()){
}

RS_ActionDrawCircleTan2_1P::~RS_ActionDrawCircleTan2_1P() = default;

void RS_ActionDrawCircleTan2_1P::init(int status){
    if (status >= 0){
        RS_PreviewActionInterface::suspend();
    }
    int pointsSize = (int) m_actionData->circles.size();
    if (status > pointsSize) {
        status = pointsSize;
    }
    LC_ActionDrawCircleBase::init(status);
    for (int i = 0; i < status; ++i) {
        if (!m_actionData->circles[i]){
            status = i;
            break;
        }
    }
    m_actionData->circles.resize(status >= 0 ? status : 0);
}

void RS_ActionDrawCircleTan2_1P::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan2_1P::doTrigger() {
    auto *circle = new RS_Circle(m_container, m_actionData->cData);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }

    undoCycleAdd(circle);

    m_actionData->circles.clear();

    RS_DEBUG->print("RS_ActionDrawCircleTan2_1P::trigger():"
                    " entity added: %lu", circle->getId());
    init(SetCircle1);
}

bool RS_ActionDrawCircleTan2_1P::getCenters(){
    if (m_actionData->circles.size() < 2) return false;
    LC_Quadratic lc0(m_actionData->circles[0], m_actionData->point);
    LC_Quadratic lc1(m_actionData->circles[1], m_actionData->point);
    auto const &list = LC_Quadratic::getIntersection(lc0, lc1);
    m_actionData->centers.clear();
    for (const RS_Vector &vp: list) {
        auto ds = vp.distanceTo(m_actionData->point) - RS_TOLERANCE;
        bool validBranch(true);
        for (RS_AtomicEntity *circle: m_actionData->circles) {
            // issue #1288, pull request #1445 by melwyncarlo: ignore center on circles
            double distance = RS_MAXDOUBLE;
            circle->getNearestPointOnEntity(vp, false, &distance);
            if (distance < ds){
                validBranch = false;
                break;
            }
            if (isCircle(circle) || isArc(circle)){
                if (vp.distanceTo(circle->getCenter()) <= ds){
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

void RS_ActionDrawCircleTan2_1P::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector coord = e->snapPoint;
    switch (status) {
        case SetCircle1:
        case SetCircle2: {
            deleteSnapper();
            auto *circle = catchCircle(e, true);
            if (circle != nullptr){
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
    for (RS_AtomicEntity *const circle: m_actionData->circles) {
        highlightSelected(circle);
    }

    if (preparePreview()){
        for (auto const &vp: m_actionData->centers) {
            previewRefSelectablePoint(vp);
        }

        if (m_showRefEntitiesOnPreview) {
            if (status == SetPoint || status == SetCenter) {
                RS_Vector center = m_actionData->cData.center;
                previewRefPoint(m_actionData->circles.at(0)->getNearestPointOnEntity(center, false));
                previewRefPoint(m_actionData->circles.at(1)->getNearestPointOnEntity(center, false));
            }
        }
        previewToCreateCircle(m_actionData->cData);
    }
}

bool RS_ActionDrawCircleTan2_1P::preparePreview(){
    if (!getCenters()) return false;
    m_actionData->cData.center = m_actionData->centers.getClosest(m_actionData->coord);
    m_actionData->cData.radius = m_actionData->point.distanceTo(m_actionData->cData.center);
    return true;
}

RS_Entity *RS_ActionDrawCircleTan2_1P::catchCircle(LC_MouseEvent *e, bool forPreview){
    RS_Entity *ret = nullptr;
    RS_Entity *en;
    if (forPreview) {
        en = catchModifiableAndDescribe(e, g_enTypeList);
    }
    else{
        en = catchModifiableEntity(e, g_enTypeList);
    }
    if (!en) return ret;
    if (!en->isVisible()) return ret;
    for (auto p: m_actionData->circles) {
        if (p && en->getId() == p->getId()) return ret; //do not pull in the same line again
    }
    return en;
}

void RS_ActionDrawCircleTan2_1P::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetCircle1:
        case SetCircle2: {
            m_actionData->circles.resize(status);
            auto en = dynamic_cast<RS_AtomicEntity *>(catchCircle(e, false));
            if (en == nullptr) {
                return;
            }
            m_actionData->circles.push_back(en);
            redrawDrawing();
            setStatus(getStatus() + 1);
            invalidateSnapSpot();
            break;
        }
        case SetPoint: {
            RS_Vector snapped = e->snapPoint;
            fireCoordinateEvent(snapped);                ;
            break;
        }
        case SetCenter: {
            m_actionData->coord = e->graphPoint;
            if (preparePreview()) trigger();
            invalidateSnapSpot();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawCircleTan2_1P::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    // Return to last status:
    if (status > 0){
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionDrawCircleTan2_1P::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint: {
            m_actionData->point = mouse;
            m_actionData->coord = mouse;
            if (getCenters()){
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

void RS_ActionDrawCircleTan2_1P::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetCircle1:
            updateMouseWidgetTRCancel(tr("Specify a line/arc/circle"));
            break;
        case SetCircle2:
            updateMouseWidgetTRBack(tr("Specify the another arc/circle"));
            break;
        case SetPoint:
            updateMouseWidgetTRBack(tr("Specify a point on the tangent circle"));
            break;
        case SetCenter:
            updateMouseWidgetTRBack(tr("Select the center of the tangent circle"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawCircleTan2_1P::doGetMouseCursor([[maybe_unused]] int status){
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
