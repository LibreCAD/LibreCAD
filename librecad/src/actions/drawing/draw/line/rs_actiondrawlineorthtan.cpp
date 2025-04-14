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

#include "rs_actiondrawlineorthtan.h"
#include "rs_creation.h"

#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"

namespace{
//this holds a list of entity types which supports tangent
const auto g_circleList = EntityTypeList{{RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityParabola}};
}

/**
 * This action class can handle user events to draw tangents normal to lines
 *
 * @author Dongxu Li
 */
RS_ActionDrawLineOrthTan::RS_ActionDrawLineOrthTan(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Draw Tangent Orthogonal", actionContext,RS2::ActionDrawLineOrthTan){
}

RS_ActionDrawLineOrthTan::~RS_ActionDrawLineOrthTan() = default;
void RS_ActionDrawLineOrthTan::finish(bool updateTB){
	clearLines();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineOrthTan::doTrigger() {
    if (!m_tangent)
        return;
    // fixme - sand - files - MERGE - what for smart pointer is used there??
    auto newEntity = std::make_unique<RS_Line>(m_container, m_tangent->getData());

    setPenAndLayerToActive(newEntity.get());
    undoCycleAdd(newEntity.get());
    newEntity.release();

    setStatus(SetCircle);
    circle = nullptr;
}

void RS_ActionDrawLineOrthTan::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetLine: {
            RS_Entity *en = catchModifiableAndDescribe(e, RS2::EntityLine);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetCircle: {
            RS_Vector mouse = e->graphPoint;
            highlightSelected(normal);
            deleteSnapper();
            RS_Entity *en = catchAndDescribe(e, g_circleList, RS2::ResolveAll);
            if (en != nullptr){
                circle = en;
                highlightHover(en);
                RS_Vector alternativeTangentPoint;
                RS_Creation creation(m_preview.get(), m_viewport, false);
                m_tangent = creation.createLineOrthTan(mouse,
                                                     normal,
                                                     circle, alternativeTangentPoint);
                // fixme - sand - files - MERGE REGRESSION - DIRTY

                if (m_tangent != nullptr){
                    // merged
                    previewLine(m_tangent->getStartpoint(), m_tangent->getEndpoint());
                    // original
                   // previewEntityToCreate(m_tangent.get(), false);
                    previewRefSelectablePoint(alternativeTangentPoint);
                    previewRefSelectablePoint(m_tangent->getEndpoint());
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_tangent->getStartpoint());
                    }
                }
            }
        }
        default:
            break;
    }
}

void RS_ActionDrawLineOrthTan::clearLines()
{
    circle = nullptr;
    deletePreview();
}

void RS_ActionDrawLineOrthTan::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
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
                invalidateSnapSpot();
            }
            break;
        }
        case SetCircle: {
            if (m_tangent != nullptr) {
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineOrthTan::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
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
