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

#include <QMouseEvent>

#include "rs_actiondrawellipsecenter3points.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "lc_actiondrawcirclebase.h"

struct RS_ActionDrawEllipseCenter3Points::Points {
	RS_VectorSolutions points;
	RS_CircleData cData;
	RS_EllipseData eData;
	bool valid{false};
};

/**
 * Constructor.
 *
 */
RS_ActionDrawEllipseCenter3Points::RS_ActionDrawEllipseCenter3Points(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :LC_ActionDrawCircleBase("Draw ellipse by center and 3 points",
                           container, graphicView)
    , pPoints(std::make_unique<Points>()){
    actionType=RS2::ActionDrawEllipseCenter3Points;
}

RS_ActionDrawEllipseCenter3Points::~RS_ActionDrawEllipseCenter3Points() = default;

void RS_ActionDrawEllipseCenter3Points::init(int status){
    LC_ActionDrawCircleBase::init(status);

    if (status == SetCenter){
        pPoints->points.clear();
    }
    drawSnapper();
}

void RS_ActionDrawEllipseCenter3Points::trigger(){
    RS_PreviewActionInterface::trigger();

    auto *ellipse = new RS_Ellipse(container, pPoints->eData);

    deletePreview();
    container->addEntity(ellipse);

    addToDocumentUndoable(ellipse);

    moveRelativeZero(ellipse->getCenter());
    graphicView->redraw(RS2::RedrawDrawing);
    drawSnapper();

    setStatus(SetCenter);

    RS_DEBUG->print("RS_ActionDrawEllipseCenter3Points::trigger():"
                    " entity added: %lu", ellipse->getId());
}

void RS_ActionDrawEllipseCenter3Points::mouseMoveEvent(QMouseEvent *e){
    //    RS_DEBUG->print("RS_ActionDrawEllipseCenter3Points::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    if (status == SetCenter){
        trySnapToRelZeroCoordinateEvent(e);
        return;
    }
    pPoints->points.resize(status);
    pPoints->points.push_back(mouse);

    deletePreview();

    if (showRefEntitiesOnPreview) {
        for (int i = SetPoint1; i <= status; i++) {
            previewRefPoint(pPoints->points.at(i - 1));
        }
    }

    previewRefSelectablePoint(mouse);

    if (preparePreview()){
        switch (status) {
            case SetPoint1: {
                previewCircle(pPoints->cData);
                break;
            }
            case SetPoint2:
            case SetPoint3: {
                auto ellipse = previewEllipse(pPoints->eData);
                    previewEllipseReferencePoints(ellipse, true, false);
                break;
            }
            default:
                break;
        }
    }
    drawPreview();
    RS_DEBUG->print("RS_ActionDrawEllipseCenter3Points::mouseMoveEvent end");
}

bool RS_ActionDrawEllipseCenter3Points::preparePreview(){
    pPoints->valid = false;
    switch (getStatus()) {
        case SetPoint1: {
            RS_Circle c(preview.get(), pPoints->cData);
            pPoints->valid = c.createFromCR(pPoints->points.at(0),
                                            pPoints->points.get(0).distanceTo(pPoints->points.get(1)));

            if (pPoints->valid){
                pPoints->cData = c.getData();
            }
            break;
        }
        case SetPoint2:
        case SetPoint3: {
            RS_Ellipse e(preview.get(), pPoints->eData);
            pPoints->valid = e.createFromCenter3Points(pPoints->points);
            if (pPoints->valid){
                pPoints->eData = e.getData();
            }
            break;
        }
        default:
            break;
    }
    return pPoints->valid;
}

void RS_ActionDrawEllipseCenter3Points::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionDrawEllipseCenter3Points::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawEllipseCenter3Points::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    pPoints->points.alloc(status + 1);
    pPoints->points.set(status, mouse);

    switch (getStatus()) {
        case SetCenter: {
            moveRelativeZero(mouse);
            setStatus(SetPoint1);
            break;
        }
        case SetPoint1:
        case SetPoint2:
            for (int i = 0; i < status - 1; i++) {
                if ((mouse - pPoints->points.get(i)).squared() < RS_TOLERANCE15){
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
