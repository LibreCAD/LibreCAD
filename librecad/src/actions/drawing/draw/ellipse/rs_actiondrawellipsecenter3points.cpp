/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2011-2015 Dongxu Li (dongxuli2011@gmail.com)
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

#include "rs_actiondrawellipsecenter3points.h"

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_preview.h"

struct RS_ActionDrawEllipseCenter3Points::ActionData {
	RS_VectorSolutions points;
	RS_CircleData cData;
	RS_EllipseData eData;
	bool valid{false};
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseCenter3Points::RS_ActionDrawEllipseCenter3Points(LC_ActionContext *actionContext)
        :LC_ActionDrawCircleBase("Draw ellipse by center and 3 points", actionContext, RS2::ActionDrawEllipseCenter3Points)
    , m_actionData(std::make_unique<ActionData>()){
}

RS_ActionDrawEllipseCenter3Points::~RS_ActionDrawEllipseCenter3Points() = default;

void RS_ActionDrawEllipseCenter3Points::init(int status){
    LC_ActionDrawCircleBase::init(status);

    if (status == SetCenter){
        m_actionData->points.clear();
    }
    drawSnapper();
}

void RS_ActionDrawEllipseCenter3Points::doTrigger() {
    auto *ellipse = new RS_Ellipse(m_container, m_actionData->eData);

    undoCycleAdd(ellipse);
    moveRelativeZero(ellipse->getCenter());
    setStatus(SetCenter);

    RS_DEBUG->print("RS_ActionDrawEllipseCenter3Points::trigger():entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipseCenter3Points::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    if (status == SetCenter){
        trySnapToRelZeroCoordinateEvent(e);
        return;
    }
    m_actionData->points.resize(status);
    m_actionData->points.push_back(mouse);

    if (m_showRefEntitiesOnPreview) {
        for (int i = SetPoint1; i <= status; i++) {
            previewRefPoint(m_actionData->points.at(i - 1));
        }
    }

    previewRefSelectablePoint(mouse);

    if (preparePreview()){
        switch (status) {
            case SetPoint1: {
                previewToCreateCircle(m_actionData->cData);
                break;
            }
            case SetPoint2:{
                auto ellipse = previewToCreateEllipse(m_actionData->eData);
                previewEllipseReferencePoints(ellipse, true, false);
                break;
            }
            case SetPoint3: {
                auto ellipse = previewToCreateEllipse(m_actionData->eData);
                    previewEllipseReferencePoints(ellipse, true, false);
                break;
            }
            default:
                break;
        }
    }
}

bool RS_ActionDrawEllipseCenter3Points::preparePreview(){
    m_actionData->valid = false;
    switch (getStatus()) {
        case SetPoint1: {
            RS_Circle c(m_preview.get(), m_actionData->cData);
            m_actionData->valid = c.createFromCR(m_actionData->points.at(0),
                                            m_actionData->points.get(0).distanceTo(m_actionData->points.get(1)));

            if (m_actionData->valid){
                m_actionData->cData = c.getData();
            }
            break;
        }
        case SetPoint2:
        case SetPoint3: {
            RS_Ellipse e(m_preview.get(), m_actionData->eData);
            m_actionData->valid = e.createFromCenter3Points(m_actionData->points);
            if (m_actionData->valid){
                m_actionData->eData = e.getData();
            }
            break;
        }
        default:
            break;
    }
    return m_actionData->valid;
}

void RS_ActionDrawEllipseCenter3Points::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawEllipseCenter3Points::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseCenter3Points::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    m_actionData->points.alloc(status + 1);
    m_actionData->points.set(status, mouse);

    switch (getStatus()) {
        case SetCenter: {
            moveRelativeZero(mouse);
            setStatus(SetPoint1);
            break;
        }
        case SetPoint1:
        case SetPoint2:
            for (int i = 0; i < status - 1; i++) {
                if ((mouse - m_actionData->points.get(i)).squared() < RS_TOLERANCE15){
                    return;//refuse to accept points already chosen
                }
            }
//                setStatus(getStatus()+1);
//                break;
            // fall-through
        case SetPoint3: {
            if (preparePreview()){
                if (status == SetPoint3){
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
void RS_ActionDrawEllipseCenter3Points::commandEvent(RS_CommandEvent* e) {
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
        }
        break;

    default:
        break;
    }
}
*/

QStringList RS_ActionDrawEllipseCenter3Points::getAvailableCommands() {
    return {};
}

void RS_ActionDrawEllipseCenter3Points::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCenter:
            updateMouseWidgetTRCancel(tr("Specify the center of ellipse"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify the first point on ellipse"));
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify the second point on ellipse"));
            break;
        case SetPoint3:
            updateMouseWidgetTRBack(tr("Specify the third point on ellipse"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
