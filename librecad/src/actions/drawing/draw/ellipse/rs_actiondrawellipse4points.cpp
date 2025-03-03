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

#include <QMouseEvent>

#include "rs_actiondrawellipse4points.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
#include "rs_preview.h"

struct RS_ActionDrawEllipse4Points::Points {
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
RS_ActionDrawEllipse4Points::RS_ActionDrawEllipse4Points(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:LC_ActionDrawCircleBase("Draw ellipse from 4 points", container,graphicView)
    ,pPoints(std::make_unique<Points>()){
	actionType=RS2::ActionDrawEllipse4Points;
}

RS_ActionDrawEllipse4Points::~RS_ActionDrawEllipse4Points() = default;

void RS_ActionDrawEllipse4Points::init(int status) {
    LC_ActionDrawCircleBase::init(status);
    if(getStatus() == SetPoint1) {
        pPoints->points.clear();
    }
}

void RS_ActionDrawEllipse4Points::trigger(){
    LC_ActionDrawCircleBase::trigger();
    RS_Entity *en;
    if (getStatus() == SetPoint4 && pPoints->evalid){
        en = new RS_Ellipse(container, pPoints->eData);
    } else {
        en = new RS_Circle(container, pPoints->cData);
    }

    deletePreview();
    container->addEntity(en);

    addToDocumentUndoable(en);

    RS_Vector rz = graphicView->getRelativeZero();
    graphicView->redraw(RS2::RedrawDrawing);
    if (moveRelPointAtCenterAfterTrigger){
        rz = en->getCenter();
    }
    moveRelativeZero(rz);
    drawSnapper();
    setStatus(SetPoint1);
    //    RS_DEBUG->print("RS_ActionDrawEllipse4Point::trigger():" " entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipse4Points::mouseMoveEvent(QMouseEvent *e){
//    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    if (status == SetPoint1){
        trySnapToRelZeroCoordinateEvent(e);
    }
    deletePreview();

    if (showRefEntitiesOnPreview) {
        for (int i = SetPoint2; i <= status; i++) {
            previewRefPoint(pPoints->points.at(i - 1));
        }
    }

    if (status == SetPoint2){
        mouse = getSnapAngleAwarePoint(e, pPoints->points.at(SetPoint1), mouse, true);
    }

    pPoints->points.set(status, mouse);
    if (preparePreview()){
        switch (status) {
            case SetPoint2: {
                break;
            }
            case SetPoint3: {
                if (pPoints->valid){
                    previewCircle(pPoints->cData);

                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(pPoints->cData.center);
                    }
                }
                break;
            }
            case SetPoint4: {
                if (pPoints->evalid) {
                    auto ellipse = previewEllipse(pPoints->eData);
                    if (showRefEntitiesOnPreview) {
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
    drawPreview();
//    RS_DEBUG->print("RS_ActionDrawEllipse4Point::mouseMoveEvent end");
}

void RS_ActionDrawEllipse4Points::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    if (status == SetPoint2){
        snap = getSnapAngleAwarePoint(e, pPoints->points.at(SetPoint1), snap);
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawEllipse4Points::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

bool RS_ActionDrawEllipse4Points::preparePreview(){
    pPoints->valid = false;
    switch (getStatus()) {
        case SetPoint2:
        case SetPoint3: {
            RS_Circle c(preview.get(), pPoints->cData);
            pPoints->valid = c.createFrom3P(pPoints->points);
            if (pPoints->valid){
                pPoints->cData = c.getData();
            }
            break;
        }
        case SetPoint4: {
            int j = SetPoint4;
            pPoints->evalid = false;
            if ((pPoints->points.get(j) - pPoints->points.get(j - 1)).squared() < RS_TOLERANCE15){
                RS_Circle c(preview.get(), pPoints->cData);
                pPoints->valid = c.createFrom3P(pPoints->points);
                if (pPoints->valid){
                    pPoints->cData = c.getData();
                }
            } else {
                RS_Ellipse e{preview.get(), pPoints->eData};
                pPoints->valid = e.createFrom4P(pPoints->points);
                if (pPoints->valid){
                    pPoints->evalid = pPoints->valid;
                    pPoints->eData = e.getData();
                    pPoints->m_bUniqueEllipse = false;
                } else {
                    pPoints->evalid = false;
                    if (pPoints->m_bUniqueEllipse == false){
                        commandMessage(tr("Can not determine uniquely an ellipse"));
                        pPoints->m_bUniqueEllipse = true;
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return pPoints->valid;
}

void RS_ActionDrawEllipse4Points::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    pPoints->points.alloc(status + 1);
    pPoints->points.set(status, mouse);

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
                    (pPoints->points.get(status) - pPoints->points.get(status - 1)).squared() < RS_TOLERANCE15){
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
