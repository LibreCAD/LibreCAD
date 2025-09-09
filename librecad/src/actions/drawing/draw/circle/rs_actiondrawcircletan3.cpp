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

#include "rs_actiondrawcircletan3.h"

#include "lc_quadratic.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"

namespace {

    //list of entity types supported by current action
    const EntityTypeList g_enTypeList = {RS2::EntityArc, RS2::EntityCircle, RS2::EntityLine, RS2::EntityPoint};
}

// fixme - cleanup

struct RS_ActionDrawCircleTan3::ActionData {
		std::vector<RS_AtomicEntity*> circles;
		std::shared_ptr<RS_CircleData> cData{std::make_shared<RS_CircleData>()};
		RS_Vector coord;
		bool valid{false};
		//keep a list of centers found
		std::vector<std::shared_ptr<RS_CircleData> > candidates;
		RS_VectorSolutions centers;
};
/**
 * Constructor.
 *
 */
RS_ActionDrawCircleTan3::RS_ActionDrawCircleTan3(LC_ActionContext *actionContext)
	:LC_ActionDrawCircleBase("Draw circle tan 3 circles", actionContext, RS2::ActionDrawCircleTan3)
    , m_actionData(std::make_unique<ActionData>()){
}

RS_ActionDrawCircleTan3::~RS_ActionDrawCircleTan3() = default;

void RS_ActionDrawCircleTan3::drawSnapper() {
    // disable snapper
}

void RS_ActionDrawCircleTan3::init(int status){
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_Snapper::suspend();
    }
}

void RS_ActionDrawCircleTan3::doInitialInit() {
    m_actionData->circles.clear();
}

void RS_ActionDrawCircleTan3::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (g_enTypeList.contains(contextEntity->rtti())) {
        setCircle(contextEntity, SetCircle1);
    }
}

void RS_ActionDrawCircleTan3::finish(bool updateTB){
    if (!m_actionData->circles.empty()){
        redrawDrawing();
        m_actionData->circles.clear();
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleTan3::doTrigger() {
    auto circle = new RS_Circle(m_container, *m_actionData->cData);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }

    undoCycleAdd(circle);

    m_actionData->circles.clear();
    setStatus(SetCircle1);

    RS_DEBUG->print("RS_ActionDrawCircleTan3::trigger(): entity added: %lu", circle->getId());
}

void RS_ActionDrawCircleTan3::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    for(RS_AtomicEntity* const pc: m_actionData->circles) { // highlight already selected
        highlightSelected(pc);
    }
    switch (status) {
        case SetCircle1:
        case SetCircle2: {
            auto *c = catchCircle(e, true); // fixme - sand - CHECK! Modifiable catch for preview?
            if (c != nullptr){
                highlightHover(c);
            }
            break;
        }
        case SetCircle3: {
            auto *c = catchCircle(e, true);
            if (c != nullptr){
                bool canBuildCircle = getData(c);
                if (canBuildCircle){
                    highlightHover(c);
                }
            }
            break;
        }
        case SetCenter: {
            m_actionData->coord = e->graphPoint;

            if (preparePreview()){
                previewToCreateCircle(*m_actionData->cData);
                for (auto &c: m_actionData->candidates) {
                    previewRefSelectablePoint(c->center);
                }

                if (m_showRefEntitiesOnPreview) {
                    for (RS_AtomicEntity *const pc: m_actionData->circles) { // highlight already selected
                        RS_Vector candidateCircleCenter = m_actionData->cData->center;
                        if (isLine(pc)) { // fixme - support of polyline
                            previewRefPoint(pc->getNearestPointOnEntity(candidateCircleCenter, false));
                        } else {
                            previewRefPoint(getTangentPoint(candidateCircleCenter, m_actionData->cData->radius, pc));
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

bool RS_ActionDrawCircleTan3::getData(RS_Entity* testThirdEntity) {
    std::vector<RS_AtomicEntity*> circlesList;
    if (testThirdEntity != nullptr) {
        std::vector<RS_AtomicEntity*> testCirclesList = m_actionData->circles;
        auto* atomicThird = dynamic_cast<RS_AtomicEntity*>(testThirdEntity);
        testCirclesList.push_back(atomicThird);
        circlesList = testCirclesList;
    }
    else {
        circlesList = m_actionData->circles;
    }
    //find the nearest circle
    size_t i = 0;
    size_t const countLines = std::count_if(circlesList.begin(), circlesList.end(), [](RS_AtomicEntity* e) -> bool {
        return e->rtti() == RS2::EntityLine;
    });

    for (; i < circlesList.size(); ++i) {
        if (isLine(circlesList[i])) { // fixme- support of polyline
            break;
        }
    }
    m_actionData->candidates.clear();
    size_t i1 = (i + 1) % 3;
    size_t i2 = (i + 2) % 3;
    if (i < circlesList.size() && isLine(circlesList[i])) { // fixme- support of polyline
        //one or more lines

        LC_Quadratic lc0(circlesList[i], circlesList[i1], false);
        LC_Quadratic lc1;
        RS_VectorSolutions sol;
        //detect degenerate case two circles with the same radius
        switch (countLines) {
            default:
            case 0:
                //this should not happen
                assert(false);
            case 1: { //1 line, two circles
                auto cc = LC_Quadratic(circlesList[i1], circlesList[i2]);
                for (unsigned k = 0; k < 4; ++k) {
                    //loop through all mirroring cases
                    lc1 = LC_Quadratic(circlesList[i], circlesList[i1], k & 1u);
                    LC_Quadratic lc2 = LC_Quadratic(circlesList[i], circlesList[i2], k & 2u);
                    sol.push_back(LC_Quadratic::getIntersection(lc1, lc2));
                    sol.push_back(LC_Quadratic::getIntersection(cc, lc1));
                    sol.push_back(LC_Quadratic::getIntersection(cc, lc2));
                }
                break;
            }
            case 2: { //2 lines, one circle
                if (isLine(circlesList[i2])) { // fixme- support of polyline
                    std::swap(i1, i2);
                }
                //i2 is circle

                for (unsigned k = 0; k < 4; ++k) {
                    //loop through all mirroring cases
                    lc1 = LC_Quadratic(circlesList[i2], circlesList[i], k & 1u);
                    LC_Quadratic lc2 = LC_Quadratic(circlesList[i2], circlesList[i1], k & 2u);
                    sol.push_back(LC_Quadratic::getIntersection(lc1, lc2));
                }
                break;
            }
            case 3: { //3 lines
                lc0 = circlesList[i]->getQuadratic();
                lc1 = circlesList[i1]->getQuadratic();
                auto lc2 = circlesList[i2]->getQuadratic();
                //attempt to have intersections (lc0, lc1), (lc0, lc2)
                auto sol1 = LC_Quadratic::getIntersection(lc0, lc1);
                if (sol1.empty()) {
                    std::swap(lc0, lc2);
                    std::swap(i, i2);
                }
                sol1 = LC_Quadratic::getIntersection(lc0, lc2);
                if (sol1.empty()) {
                    std::swap(lc0, lc1);
                    std::swap(i, i1);
                }

                auto line0 = dynamic_cast<RS_Line*>(circlesList[i]);
                auto line1 = dynamic_cast<RS_Line*>(circlesList[i1]);
                auto line2 = dynamic_cast<RS_Line*>(circlesList[i2]);
                lc0 = line0->getQuadratic();
                lc1 = line1->getQuadratic();
                lc2 = line2->getQuadratic();
                //intersection 0, 1
                sol1 = LC_Quadratic::getIntersection(lc0, lc1);
                if (sol1.empty()) {
                    return false;
                }
                RS_Vector const v1 = sol1.at(0);
                double angle1 = 0.5 * (line0->getAngle1() + line1->getAngle1());

                //intersection 0, 2
                sol1 = LC_Quadratic::getIntersection(lc0, lc2);
                double angle2;
                if (sol1.empty()) {
                    return false;
                }
                angle2 = 0.5 * (line0->getAngle1() + line2->getAngle1());
                RS_Vector const& v2 = sol1.at(0);
                //two bisector lines per intersection
                for (unsigned j = 0; j < 2; ++j) {
                    RS_Line l1{v1, v1 + RS_Vector{angle1}};
                    for (unsigned j1 = 0; j1 < 2; ++j1) {
                        RS_Line l2{v2, v2 + RS_Vector{angle2}};
                        sol.push_back(RS_Information::getIntersectionLineLine(&l1, &l2));
                        angle2 += M_PI_2;
                    }
                    angle1 += M_PI_2;
                }
            }
        }

        //line passes circle center, need a second parabola as the image of the line
        for (int j = 1; j <= 2; j++) {
            if (circlesList[(i + j) % 3]->rtti() == RS2::EntityCircle) {
                double d = RS_MAXDOUBLE;
                circlesList[i]->getNearestPointOnEntity(circlesList[(i + j) % 3]->getCenter(),
                                                        false, &d);
                if (d < 200. * RS_TOLERANCE) {
                    LC_Quadratic lc2(circlesList[i], circlesList[(i + j) % 3], true);
                    sol.push_back(LC_Quadratic::getIntersection(lc2, lc1));
                }
            }
        }

        //clean up duplicate and invalid
        RS_VectorSolutions sol1;
        for (const RS_Vector& vp : sol) {
            if (vp.magnitude() > RS_MAXDOUBLE) {
                continue;
            }
            if (!sol1.empty() && sol1.getClosestDistance(vp) < RS_TOLERANCE) {
                continue;
            }
            sol1.push_back(vp);
        }

        for (auto const& v : sol1) {
            double d = RS_MAXDOUBLE;
            circlesList[i]->getNearestPointOnEntity(v, false, &d);
            std::vector<std::shared_ptr<RS_CircleData>> data;
            data.push_back(std::make_shared<RS_CircleData>(v, d));
            if (circlesList[i]->rtti() == RS2::EntityCircle) {
                auto* circle = static_cast<RS_Circle*>(circlesList[i]);
                if (circle->getCenter().distanceTo(v) > circle->getRadius() * (1. + RS_TOLERANCE)) {
                    data.push_back(std::make_shared<RS_CircleData>(v, d + 2. * circle->getRadius()));
                }
            }
            for (const auto& circleData : data) {
                if (circlesList[(i + 1) % 3]->isTangent(*circleData)
                    && circlesList[(i + 2) % 3]->isTangent(*circleData)) {
                    m_actionData->candidates.push_back(circleData);
                }
            }
        }
    }
    else {
        RS_Circle c{nullptr, *m_actionData->cData};
        auto solutions = c.createTan3(circlesList);
        m_actionData->candidates.clear();
        for (const RS_Circle& s : solutions) {
            m_actionData->candidates.push_back(std::make_shared<RS_CircleData>(s.getData()));
        }
    }
    m_actionData->valid = !m_actionData->candidates.empty();
    return m_actionData->valid;
}

bool RS_ActionDrawCircleTan3::preparePreview(){
    if (getStatus() != SetCenter || m_actionData->valid == false){
        m_actionData->valid = false;
        return false;
    }
//find the nearest circle
    size_t index = m_actionData->candidates.size();
    double dist = RS_MAXDOUBLE * RS_MAXDOUBLE;
    for (size_t i = 0; i < m_actionData->candidates.size(); ++i) {
//        previewRefSelectablePoint(pPoints->candidates.at(i)->center);
        double d = RS_MAXDOUBLE;
        RS_Circle(nullptr, *m_actionData->candidates.at(i)).getNearestPointOnEntity(m_actionData->coord, false, &d);
        double dCenter = m_actionData->coord.distanceTo(m_actionData->candidates.at(i)->center);
        d = std::min(d, dCenter);
        if (d < dist){
            dist = d;
            index = i;
        }
    }
    if (index < m_actionData->candidates.size()){
        m_actionData->cData = m_actionData->candidates.at(index);
        m_actionData->valid = true;
    } else {
        m_actionData->valid = false;
    }
    return m_actionData->valid;
}

RS_Entity *RS_ActionDrawCircleTan3::catchCircle(LC_MouseEvent *e, bool forPreview) {
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
    for (int i = 0; i < getStatus(); ++i) {
        if (en->getId() == m_actionData->circles[i]->getId()){
            return ret; //do not pull in the same line again
        }
    }
    return en;
}

void RS_ActionDrawCircleTan3::setCircle(RS_Entity* en, int status) {
    m_actionData->circles.resize(status);// todo - what for? Why not have fixes size
    for (const RS_AtomicEntity *const pc: m_actionData->circles) {
        if (pc == en) {
            continue;
        }
    }
    m_actionData->circles.push_back(dynamic_cast<RS_AtomicEntity *>(en));
    setStatus(status + 1);
}

void RS_ActionDrawCircleTan3::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetCircle1:
        case SetCircle2:{
            RS_Entity *en = catchCircle(e, false);
            if (en != nullptr){
                setCircle(en, status);
            }
            break;
        }
        case SetCircle3: {
            RS_Entity *en = catchCircle(e,false);
            if (en != nullptr){
                m_actionData->circles.resize(getStatus());// todo - what for? Why not have fixes size
                for (const RS_AtomicEntity *const pc: m_actionData->circles) {
                    if (pc == en){
                        continue;
                    }
                }
                if (getData(en)){
                    m_actionData->circles.push_back(dynamic_cast<RS_AtomicEntity *>(en));
                    setStatus(SetCenter);
                    if (m_actionData->centers.size() == 1){
                        m_actionData->coord = m_actionData->centers.at(0);
                        if (preparePreview()) {
                            trigger();
                        }
                    }
                }
                else {
                    commandMessage(tr("No common tangential circle for selected entities"));
                }
            }
            break;
        }
        case SetCenter:
            m_actionData->coord = e->graphPoint;
            if (preparePreview()) trigger();
            break;

        default:
            break;
    }

}

void RS_ActionDrawCircleTan3::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    // Return to last status:
    if (status > 0){
        m_actionData->circles[status - 1]->setHighlighted(false);
        m_actionData->circles.pop_back();
        redrawDrawing();
        deletePreview();
    }
    initPrevious(status);
}

/*
void RS_ActionDrawCircleTan3::commandEvent(RS_CommandEvent* e) {
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

void RS_ActionDrawCircleTan3::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetCircle1:
            updateMouseWidgetTRCancel(tr("Specify the first line/arc/circle"));
            break;
        case SetCircle2:
            updateMouseWidgetTRBack(tr("Specify the second line/arc/circle"));
            break;
        case SetCircle3:
            updateMouseWidgetTRBack(tr("Specify the third line/arc/circle"));
            break;
        case SetCenter:
            updateMouseWidgetTRBack(tr("Select the center of the tangent circle"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionDrawCircleTan3::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

RS_Vector RS_ActionDrawCircleTan3::getTangentPoint(RS_Vector creatingCircleCenter, double creatingCircleRadius, RS_AtomicEntity *const circle){
    bool calcTangentFromOriginalCircle = (creatingCircleCenter.distanceTo(circle->getCenter()) < circle->getRadius()) &&
                                         (creatingCircleRadius < circle->getRadius());

    const RS_Vector &circleCenter = circle->getCenter();
    if (calcTangentFromOriginalCircle){
        return circleCenter + RS_Vector::polar(circle->getRadius(), circleCenter.angleTo(creatingCircleCenter));
    }
    else{
        return creatingCircleCenter + RS_Vector::polar(creatingCircleRadius, creatingCircleCenter.angleTo(circleCenter));
    }
}
