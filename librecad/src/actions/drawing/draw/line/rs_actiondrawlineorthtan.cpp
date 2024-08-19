/****************************************************************************
**
 * This action class can handle user events to draw tangents normal to lines

Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)
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

#include "rs_actiondrawlineorthtan.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"

namespace{
    auto circleList={RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityParabola}; //this holds a list of entity types which supports tangent
}

/**
 * This action class can handle user events to draw tangents normal to lines
 *
 * @author Dongxu Li
 */
RS_ActionDrawLineOrthTan::RS_ActionDrawLineOrthTan(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw Tangent Orthogonal", container, graphicView)
	,normal(nullptr)
	,tangent(nullptr)
	,circle(nullptr){
	actionType=RS2::ActionDrawLineOrthTan;
}


void RS_ActionDrawLineOrthTan::finish(bool updateTB){
	clearLines();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineOrthTan::trigger(){
    if (!tangent) return;
    RS_PreviewActionInterface::trigger();


    circle = nullptr;
    RS_Entity *newEntity = new RS_Line(container,
                                       tangent->getData());

    deletePreview();
    newEntity->setLayerToActive();
    newEntity->setPenToActive();
    container->addEntity(newEntity);

    addToDocumentUndoable(newEntity);

    graphicView->redraw(RS2::RedrawDrawing);

    setStatus(SetCircle);
}

void RS_ActionDrawLineOrthTan::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawLineOrthTan::mouseMoveEvent begin");
    e->accept();

    snapPoint(e);

    deleteHighlights();
    switch (getStatus()) {
        case SetLine: {
            RS_Entity *en = catchModifiableEntity(e, RS2::EntityLine);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetCircle: {
            RS_Vector mouse{toGraph(e)};
            highlightSelected(normal);
            deletePreview();
            RS_Entity *en = catchEntity(e, circleList, RS2::ResolveAll);
            if (en != nullptr){
                circle = en;
                highlightHover(en);
                deletePreview();
                RS_Vector alternativeTangentPoint;
                RS_Creation creation(preview.get(), graphicView, false);
                tangent = creation.createLineOrthTan(mouse,
                                                     normal,
                                                     circle, alternativeTangentPoint);
                if (tangent != nullptr){
                    previewEntity(tangent);
                    previewRefSelectablePoint(alternativeTangentPoint);
                    previewRefSelectablePoint(tangent->getEndpoint());
                    if (showRefEntitiesOnPreview) {
                        previewRefPoint(tangent->getStartpoint());
                    }
                }
            }
            drawPreview();
        }
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionDrawLineOrthTan::mouseMoveEvent end");
}

void RS_ActionDrawLineOrthTan::clearLines(){
    if (circle) circle = nullptr;
    deletePreview();
}

void RS_ActionDrawLineOrthTan::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetLine: {
            RS_Entity *en = catchModifiableEntity(e, RS2::EntityLine);
            if (en != nullptr){
                if (en->getLength() < RS_TOLERANCE){
                    //ignore lines not long enough
                    break;
                }
                normal = dynamic_cast<RS_Line *>(en);
                setStatus(SetCircle);
            }
            break;
        }
        case SetCircle: {
            if (tangent != nullptr) {
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineOrthTan::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    clearLines();
    if (status == SetLine){
        finish(true);
    } else {
        initPrevious(status);
    }
}

void RS_ActionDrawLineOrthTan::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetLine:
            updateMouseWidgetTRCancel(tr("Select a line"));
            break;
        case SetCircle:
            updateMouseWidgetTRBack(tr("Select circle, arc or ellipse"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineOrthTan::doGetMouseCursor([[maybe_unused]] int status){
    return isFinished() ? RS2::ArrowCursor : RS2::SelectCursor;
}
