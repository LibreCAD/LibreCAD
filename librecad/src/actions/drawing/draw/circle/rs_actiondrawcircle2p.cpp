/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/



#include <QMouseEvent>

#include "rs_actiondrawcircle2p.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"

struct RS_ActionDrawCircle2P::Points {
    /**
     * 1st point.
     */
    RS_Vector point1;
    /**
     * 2nd point.
     */
    RS_Vector point2;
};

RS_ActionDrawCircle2P::RS_ActionDrawCircle2P(RS_EntityContainer& container,
                                             RS_GraphicView& graphicView)
    :LC_ActionDrawCircleBase("Draw circles",
                               container, graphicView)
    , data(new RS_CircleData())
    , pPoints(std::make_unique<Points>()){
    actionType=RS2::ActionDrawCircle2P;
    reset();
}

RS_ActionDrawCircle2P::~RS_ActionDrawCircle2P() = default;

void RS_ActionDrawCircle2P::reset() {
    data.reset(new RS_CircleData{});
    pPoints->point1 = {};
    pPoints->point2 = {};
}

void RS_ActionDrawCircle2P::trigger() {
    RS_PreviewActionInterface::trigger();

    preparePreview();
    if (data->isValid()){
        auto *circle = new RS_Circle(container,*data);
        circle->setLayerToActive();
        circle->setPenToActive();
        container->addEntity(circle);

        addToDocumentUndoable(circle);

        if (moveRelPointAtCenterAfterTrigger){
            moveRelativeZero(data->center);

        } else {
            RS_Vector rz = graphicView->getRelativeZero();
            moveRelativeZero(rz);
        }
        graphicView->redraw(RS2::RedrawDrawing);

        setStatus(SetPoint1);
        reset();
    } else
        RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid Circle data."));
}

void RS_ActionDrawCircle2P::preparePreview() {
    data.reset(new RS_CircleData{});
    if (pPoints->point1.valid && pPoints->point2.valid) {
        RS_Circle circle(nullptr, *data);
        bool suc = circle.createFrom2P(pPoints->point1, pPoints->point2);
        if (suc) {
            data.reset(new RS_CircleData(circle.getData()));
        }
    }
}

void RS_ActionDrawCircle2P::mouseMoveEvent(QMouseEvent* e) {
    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetPoint1: {
            pPoints->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2: {
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, pPoints->point1, mouse, true);
            pPoints->point2 = mouse;
            preparePreview();
            if (data->isValid()){
                previewCircle(*data);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(data->center);
                    previewRefPoint(pPoints->point1);
                    previewRefSelectablePoint(pPoints->point2);
                    previewRefLine(data->center, pPoints->point1);
                    //                    previewRefLine(pPoints->point1, pPoints->point2);
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawCircle2P::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector coord = snapPoint(e);
    if (status == SetPoint2){
        coord = getSnapAngleAwarePoint(e, pPoints->point1, coord);
    }
    fireCoordinateEvent(coord);
}

void RS_ActionDrawCircle2P::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawCircle2P::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            pPoints->point1 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            pPoints->point2 = mouse;
            moveRelativeZero(mouse);
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawCircle2P::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}
