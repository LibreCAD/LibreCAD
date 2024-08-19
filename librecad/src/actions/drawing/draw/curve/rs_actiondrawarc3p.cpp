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

#include "rs_actiondrawarc.h"
#include "rs_actiondrawarc3p.h"
#include "rs_arc.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"

struct RS_ActionDrawArc3P::Points {
    RS_ArcData data;
    /**
     * 1st point.
     */
    RS_Vector point1;
    /**
     * 2nd point.
     */
    RS_Vector point2;
    /**
     * 3rd point.
     */
    RS_Vector point3;
};

RS_ActionDrawArc3P::RS_ActionDrawArc3P(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_ActionDrawCircleBase("Draw arcs 3P", container, graphicView), pPoints(std::make_unique<Points>()){
    actionType = RS2::ActionDrawArc3P;
}

RS_ActionDrawArc3P::~RS_ActionDrawArc3P() = default;

void RS_ActionDrawArc3P::reset() {
    pPoints = std::make_unique<Points>();
}

void RS_ActionDrawArc3P::init(int status) {
    LC_ActionDrawCircleBase::init(status);
    //reset();
}

void RS_ActionDrawArc3P::trigger(){
    RS_PreviewActionInterface::trigger();

    preparePreview();
    if (pPoints->data.isValid()){
        auto *arc = new RS_Arc{container, pPoints->data};
        arc->setLayerToActive();
        arc->setPenToActive();
        container->addEntity(arc);

        addToDocumentUndoable(arc);

        graphicView->redraw(RS2::RedrawDrawing);
        RS_Vector rz = arc->getEndpoint();
        if (moveRelPointAtCenterAfterTrigger){
            rz = arc->getCenter();
        }
        moveRelativeZero(rz);

        setStatus(SetPoint1);
        reset();
    } else {
        //RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid arc data."));
        commandMessage(tr("Invalid arc data."));
    }
}

void RS_ActionDrawArc3P::preparePreview(){
    pPoints->data = {};
    if (pPoints->point1.valid && pPoints->point2.valid && pPoints->point3.valid){
        RS_Arc arc(nullptr, pPoints->data);
        bool suc = arc.createFrom3P(pPoints->point1, pPoints->point2, pPoints->point3);
        if (suc){
            pPoints->data = arc.getData();
        }
    }
}

void RS_ActionDrawArc3P::mouseMoveEvent(QMouseEvent *e){
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
        case SetPoint1: {
            pPoints->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2:
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, pPoints->point1, mouse, true);
            pPoints->point2 = mouse;
            if (pPoints->point1.valid){ // todo - redundant check
                previewLine(pPoints->point1, pPoints->point2);

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->point1);
                    previewRefSelectablePoint(pPoints->point2);
                }
            }
            drawPreview();
            break;

        case SetPoint3: {
            deletePreview();
            // todo - which point (1 or 2) is more suitable there for snap?
            mouse = getSnapAngleAwarePoint(e,pPoints->point1, mouse, true);
            pPoints->point3 = mouse;
            preparePreview();
            if (pPoints->data.isValid()){
                previewArc(pPoints->data);

                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->data.center);
                    previewRefPoint(pPoints->point1);
                    previewRefPoint(pPoints->point2);
                    previewRefSelectablePoint(pPoints->point3);
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawArc3P::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    switch (status) {
        case SetPoint2:
        case SetPoint3: {
            snap = getSnapAngleAwarePoint(e, pPoints->point1, snap);
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void RS_ActionDrawArc3P::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawArc3P::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
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
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3: {
            pPoints->point3 = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDrawArc3P::doProcessCommand([[maybe_unused]]int status, const QString &c) {
    bool accept = false;
    if (checkCommand("center", c, rtti())) {
        accept = true;
        finish(false);
        // fixme - review why this action is called ther
        graphicView->setCurrentAction(new RS_ActionDrawArc(*container, *graphicView));
    }
    return accept;
}

QStringList RS_ActionDrawArc3P::getAvailableCommands() {
    return {{"center"}};
}

void RS_ActionDrawArc3P::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetPoint1:
        updateMouseWidgetTRCancel(tr("Specify startpoint or [center]"), MOD_SHIFT_RELATIVE_ZERO);
        break;
    case SetPoint2:
        updateMouseWidgetTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
        break;
    case SetPoint3:
        updateMouseWidgetTRBack(tr("Specify endpoint"), MOD_SHIFT_ANGLE_SNAP);
        break;
    default:
        updateMouseWidget();
        break;
    }
}
