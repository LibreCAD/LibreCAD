/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
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

#include "rs_actiondrawellipse4points.h"

#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_preview.h"

struct RS_ActionDrawEllipse4Points::ActionData {
    RS_VectorSolutions points;
    RS_CircleData cData;
    RS_EllipseData eData;
    bool valid = false, evalid = false;
    bool m_bUniqueEllipse{false}; //a message of non-unique ellipse is shown
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipse4Points::RS_ActionDrawEllipse4Points(LC_ActionContext *actionContext)
	:LC_ActionDrawCircleBase("Draw ellipse from 4 points", actionContext, RS2::ActionDrawEllipse4Points)
    ,m_actionData(std::make_unique<ActionData>()){
}

RS_ActionDrawEllipse4Points::~RS_ActionDrawEllipse4Points() = default;

void RS_ActionDrawEllipse4Points::init(int status) {
    LC_ActionDrawCircleBase::init(status);
    if(getStatus() == SetPoint1) {
        m_actionData->points.clear();
    }
}

void RS_ActionDrawEllipse4Points::doTrigger() {
    RS_Entity *en;
    if (getStatus() == SetPoint4 && m_actionData->evalid){
        en = new RS_Ellipse(m_container, m_actionData->eData);
    } else {
        en = new RS_Circle(m_container, m_actionData->cData);
    }

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(en->getCenter());
    }

    undoCycleAdd(en);

    setStatus(SetPoint1);
    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::trigger():" " entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipse4Points::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    if (status == SetPoint1){
        trySnapToRelZeroCoordinateEvent(e);
    }

    if (m_showRefEntitiesOnPreview) {
        for (int i = SetPoint2; i <= status; i++) {
            previewRefPoint(m_actionData->points.at(i - 1));
        }
    }

    if (status == SetPoint2){
        mouse = getSnapAngleAwarePoint(e, m_actionData->points.at(SetPoint1), mouse, true);
    }

    m_actionData->points.set(status, mouse);
    if (preparePreview()){
        switch (status) {
            case SetPoint2: {
                break;
            }
            case SetPoint3: {
                if (m_actionData->valid){
                    previewToCreateCircle(m_actionData->cData);

                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_actionData->cData.center);
                    }
                }
                break;
            }
            case SetPoint4: {
                if (m_actionData->evalid) {
                    auto ellipse = previewToCreateEllipse(m_actionData->eData);
                    if (m_showRefEntitiesOnPreview) {
                        previewEllipseReferencePoints(ellipse, true);
                        previewRefSelectablePoint(mouse);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

void RS_ActionDrawEllipse4Points::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    if (status == SetPoint2){
        snap = getSnapAngleAwarePoint(e, m_actionData->points.at(SetPoint1), snap);
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawEllipse4Points::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

bool RS_ActionDrawEllipse4Points::preparePreview(){
    m_actionData->valid = false;
    switch (getStatus()) {
        case SetPoint2:
        case SetPoint3: {
            RS_Circle c(m_preview.get(), m_actionData->cData);
            m_actionData->valid = c.createFrom3P(m_actionData->points);
            if (m_actionData->valid){
                m_actionData->cData = c.getData();
            }
            break;
        }
        case SetPoint4: {
            int j = SetPoint4;
            m_actionData->evalid = false;
            if ((m_actionData->points.get(j) - m_actionData->points.get(j - 1)).squared() < RS_TOLERANCE15){
                RS_Circle c(m_preview.get(), m_actionData->cData);
                m_actionData->valid = c.createFrom3P(m_actionData->points);
                if (m_actionData->valid){
                    m_actionData->cData = c.getData();
                }
            } else {
                RS_Ellipse e{m_preview.get(), m_actionData->eData};
                m_actionData->valid = e.createFrom4P(m_actionData->points);
                if (m_actionData->valid){
                    m_actionData->evalid = m_actionData->valid;
                    m_actionData->eData = e.getData();
                    m_actionData->m_bUniqueEllipse = false;
                } else {
                    m_actionData->evalid = false;
                    if (m_actionData->m_bUniqueEllipse == false){
                        commandMessage(tr("Can not determine uniquely an ellipse"));
                        m_actionData->m_bUniqueEllipse = true;
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return m_actionData->valid;
}

void RS_ActionDrawEllipse4Points::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    m_actionData->points.alloc(status + 1);
    m_actionData->points.set(status, mouse);

    switch (status) {
        case SetPoint1: {
            moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:
        case SetPoint3:
        case SetPoint4: {
            if (preparePreview()){
                moveRelativeZero(mouse);
                if (status == SetPoint4 ||
                    (m_actionData->points.get(status) - m_actionData->points.get(status - 1)).squared() < RS_TOLERANCE15){
                    //also draw the entity, if clicked on the same point twice
                    trigger();
                } else {
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
void RS_ActionDrawEllipse4Point::commandEvent(RS_CommandEvent* e) {
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
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
            } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
        break;

    default:
        break;
    }
}
*/

QStringList RS_ActionDrawEllipse4Points::getAvailableCommands() {
	return {};
}

void RS_ActionDrawEllipse4Points::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify the first point on ellipse"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify the second point on ellipse"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetPoint3:
            updateMouseWidgetTRBack(tr("Specify the third point on ellipse"));
            break;
        case SetPoint4:
            updateMouseWidgetTRBack(tr("Specify the fourth point on ellipse"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
