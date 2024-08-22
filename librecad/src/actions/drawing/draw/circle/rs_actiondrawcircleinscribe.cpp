/****************************************************************************
**
 * Draw circle by foci and a point on circle

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
#include<vector>

#include <QMouseEvent>

#include "rs_actiondrawcircleinscribe.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"

struct RS_ActionDrawCircleInscribe::Points {
    RS_CircleData cData;
    RS_Vector coord;
    std::vector<RS_Line *> lines;
};

// fixme - cleanup, optoins

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleInscribe::RS_ActionDrawCircleInscribe(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_ActionDrawCircleBase("Draw circle inscribed",
                             container, graphicView), pPoints(std::make_unique<Points>()), valid(false){
    actionType = RS2::ActionDrawCircleInscribe;
}

RS_ActionDrawCircleInscribe::~RS_ActionDrawCircleInscribe() = default;

void RS_ActionDrawCircleInscribe::clearLines(bool checkStatus){
    while (!pPoints->lines.empty()) {
        if (checkStatus && (int) pPoints->lines.size() <= getStatus())
            break;
        pPoints->lines.pop_back();
    }
}

void RS_ActionDrawCircleInscribe::init(int status){
    LC_ActionDrawCircleBase::init(status);
    if (status >= 0){
        RS_PreviewActionInterface::suspend();
    }
    clearLines(true);
}

void RS_ActionDrawCircleInscribe::finish(bool updateTB){
    clearLines();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawCircleInscribe::trigger(){
    RS_PreviewActionInterface::trigger();

    auto *circle = new RS_Circle(container, pPoints->cData);

    deletePreview();
    deleteHighlights();
    container->addEntity(circle);

    addToDocumentUndoable(circle);

    clearLines(false);

    graphicView->redraw(RS2::RedrawDrawing);
    if (moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }
    setStatus(SetLine1);

    RS_DEBUG->print("RS_ActionDrawCircle4Line::trigger():"
                    " entity added: %lu", circle->getId());
}

void RS_ActionDrawCircleInscribe::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawCircle4Line::mouseMoveEvent begin");
    snapPoint(e);
    int status = getStatus();
    deleteHighlights();
    deletePreview();
    for(RS_AtomicEntity* const pc: pPoints->lines) { // highlight already selected
        highlightSelected(pc);
    }
    auto en = catchModifiableEntity(e, RS2::EntityLine);  // fixme - check whether snap is used for entity selection?  Ensure free snap?

    if (en != nullptr){
        auto *line = dynamic_cast<RS_Line *>(en);
        switch (status) {
            case SetLine1: {
                highlightHover(en);
                break;
            }
            case SetLine2: {
                if (en != pPoints->lines[SetLine1]){
                    highlightHover(en);
                }
                break;
            }
            case SetLine3: {
                if (pPoints->lines[SetLine1] != line && pPoints->lines[SetLine2] != line){
                    pPoints->coord = toGraph(e);
                    if (preparePreview(line)){
                        highlightHover(en);
                        previewCircle(pPoints->cData);
                        if (showRefEntitiesOnPreview) {
                            RS_Vector &center = pPoints->cData.center;
                            previewRefPoint(pPoints->lines[SetLine1]->getNearestPointOnEntity(center, false));
                            previewRefPoint(pPoints->lines[SetLine2]->getNearestPointOnEntity(center, false));
                            previewRefPoint(pPoints->lines[SetLine3]->getNearestPointOnEntity(center, false));
                        }
                        drawPreview();
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionDrawCircle4Line::mouseMoveEvent end");
}

void RS_ActionDrawCircleInscribe::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Entity *en = catchModifiableEntity(e, RS2::EntityLine);
    if (!en) return;
    if (!(en->isVisible() && isLine(en))) return;
    for (int i = 0; i < status; i++) {
        if (en->getId() == pPoints->lines[i]->getId()) return; //do not pull in the same line again
    }

    pPoints->coord = toGraph(e);
    auto *line = dynamic_cast<RS_Line *>(en);

    switch (status) {
        case SetLine1:{
            pPoints->lines.push_back(line);
            setStatus(SetLine2);
            break;
        }
        case SetLine2:
            pPoints->lines.push_back(line);
            setStatus(SetLine3);
            break;
        case SetLine3:
            if (preparePreview(line)){
                trigger();
            }
            break;
        default:
            break;
    }
}

void RS_ActionDrawCircleInscribe::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    // Return to last status:
    if (status > 0){
        pPoints->lines.pop_back();
        deletePreview();
    }
    initPrevious(status);
}

bool RS_ActionDrawCircleInscribe::preparePreview(RS_Line* en){
    valid = false;
    if (getStatus() == SetLine3){
        if (en != nullptr){
          pPoints->lines.push_back(en);
        }
        RS_Circle c(preview.get(), pPoints->cData);
        valid = c.createInscribe(pPoints->coord, pPoints->lines);
        if (valid){
            pPoints->cData = c.getData();
        }
    }
    if (en != nullptr){
        pPoints->lines.pop_back();
    }
    return valid;
}

void RS_ActionDrawCircleInscribe::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetLine1:
            updateMouseWidgetTRCancel(tr("Specify the first line"));
            break;
        case SetLine2:
            updateMouseWidgetTRBack(tr("Specify the second line"));
            break;
        case SetLine3:
            updateMouseWidgetTRBack(tr("Specify the third line"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawCircleInscribe::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
