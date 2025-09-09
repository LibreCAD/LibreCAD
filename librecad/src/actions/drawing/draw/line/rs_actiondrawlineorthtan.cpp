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
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
    //this holds a list of entity types which supports tangent
    const auto g_supportedCircleEntityTypes = EntityTypeList{
        {RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityParabola}
    };
}

struct RS_ActionDrawLineOrthTan::ActionData {
    /** normal to tangent. */
    RS_Line* normal = nullptr; // the select normal line
    /** m_tangent. */
    /** arc/circle/ellipse to generate tangent */
    RS_Entity* circle = nullptr;
    bool altMode = false;
    RS_Vector mousePosition;
    bool m_setCircleFirst = false;
};

/**
 * This action class can handle user events to draw tangents normal to lines
 *
 * @author Dongxu Li
 */
RS_ActionDrawLineOrthTan::RS_ActionDrawLineOrthTan(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Draw Tangent Orthogonal", actionContext,RS2::ActionDrawLineOrthTan),
    m_actionData{std::make_unique<ActionData>()}{
}

RS_ActionDrawLineOrthTan::~RS_ActionDrawLineOrthTan() = default;
void RS_ActionDrawLineOrthTan::finish(bool updateTB){
	clearLines();
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineOrthTan::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) { // fixme - support of polyline
        setLine(entity);
    }
    else {
        RS2::EntityType rtti = entity->rtti();
        if (g_supportedCircleEntityTypes.contains(rtti)) {
            m_actionData->m_setCircleFirst = true;
            m_actionData->circle = entity;
            m_actionData->mousePosition = clickPos;
            setStatus(SetLine);
            highlightHover(entity);
            drawPreview();
        }
    }
}

void RS_ActionDrawLineOrthTan::doTrigger() {
    RS_Creation creation(m_container, m_viewport, false);
    RS_Vector altTangentPosition;
    auto tangent = creation.createLineOrthTan(m_actionData->mousePosition,
        m_actionData->normal, m_actionData->circle, altTangentPosition);

    if (m_actionData->altMode) {
        tangent.reset();
        tangent = creation.createLineOrthTan(altTangentPosition,m_actionData->normal, m_actionData->circle, altTangentPosition);
    }

    auto tangentData = tangent->getData();
    auto *newEntity = new RS_Line(m_container, tangentData);

    setPenAndLayerToActive(newEntity);
    undoCycleAdd(newEntity);

    setStatus(SetCircle);
    m_actionData->circle = nullptr;
}

void RS_ActionDrawLineOrthTan::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetLine: {
            auto en = catchModifiableAndDescribe(e, RS2::EntityLine);
            if (en != nullptr){
                highlightHover(en);
            }
            if (m_actionData->m_setCircleFirst) {
                highlightSelected(m_actionData->circle);
                RS_Vector alternativeTangentPoint;
                RS_Creation creation(m_preview.get(), m_viewport, false);
                RS_Vector mouse = e->graphPoint;
                auto line = static_cast<RS_Line*>(en);
                auto tangent = creation.createLineOrthTan(mouse,line, m_actionData->circle, alternativeTangentPoint);
                if (tangent != nullptr){
                    if (e->isControl) {
                        tangent.reset();
                        tangent = creation.createLineOrthTan(alternativeTangentPoint,line, m_actionData->circle, alternativeTangentPoint);
                    }
                    auto tangentClone = tangent.get()->clone();
                    previewEntityToCreate(tangentClone, true);
                    previewRefSelectablePoint(alternativeTangentPoint);
                    previewRefSelectablePoint(tangent->getEndpoint());
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(tangent->getStartpoint());
                    }
                }
            }
            break;
        }
        case SetCircle: {
            RS_Vector mouse = e->graphPoint;
            if (m_actionData->m_setCircleFirst) {
                auto en = catchAndDescribe(e, g_supportedCircleEntityTypes, RS2::ResolveAll);
                if (en != nullptr) {
                    highlightHover(en);
                }
            }
            else {
                auto normal = m_actionData->normal;
                highlightSelected(normal);
                deleteSnapper();
                auto en = catchAndDescribe(e, g_supportedCircleEntityTypes, RS2::ResolveAll);
                if (en != nullptr){
                    highlightHover(en);
                    RS_Vector alternativeTangentPoint;
                    RS_Creation creation(m_preview.get(), m_viewport, false);
                    auto tangent = creation.createLineOrthTan(mouse,normal, en, alternativeTangentPoint);
                    if (tangent != nullptr){
                        previewEntityToCreate(tangent.get()->clone(), true);
                        previewRefSelectablePoint(alternativeTangentPoint);
                        previewRefSelectablePoint(tangent->getEndpoint());
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(tangent->getStartpoint());
                        }
                    }
                }
            }
        }
        default:
            break;
    }
}

void RS_ActionDrawLineOrthTan::clearLines(){
    m_actionData->circle = nullptr;
    deletePreview();
}

bool RS_ActionDrawLineOrthTan::setLine(RS_Entity* en) {
    if (en->getLength() < RS_TOLERANCE){
        //ignore lines not long enough
        return true;
    }
    m_actionData->normal = dynamic_cast<RS_Line *>(en);
    if (m_actionData->m_setCircleFirst) {
        trigger();
    }
    else {
        setStatus(SetCircle);
    }
    return false;
}

void RS_ActionDrawLineOrthTan::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetLine: {
            auto en = catchModifiableEntity(e, RS2::EntityLine);
            if (en != nullptr){
                if (m_actionData->m_setCircleFirst) {
                    m_actionData->mousePosition = e->graphPoint;
                    m_actionData->altMode = e->isControl;
                }
                if (setLine(en)) {
                    break;
                }
                invalidateSnapSpot();
            }
            break;
        }
        case SetCircle: {
            auto en = catchAndDescribe(e, g_supportedCircleEntityTypes, RS2::ResolveAll);
            if (en != nullptr) {
                m_actionData->circle = en;
                m_actionData->mousePosition = e->graphPoint;
                if (m_actionData->m_setCircleFirst) {
                    setStatus(SetLine);
                }
                else {
                    trigger();
                }
            };
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineOrthTan::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    clearLines();
    if (m_actionData->m_setCircleFirst) {
        if (status == SetCircle) {
            finish(true);
        }
        else {
            setStatus(SetCircle);
        }
    }
    else {
        if (status == SetLine){
            finish(true);
        } else {
            initPrevious(status);
        }
    }
}

void RS_ActionDrawLineOrthTan::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetLine:
            if (m_actionData->m_setCircleFirst) {
                updateMouseWidgetTRCancel(tr("Select a line"), MOD_CTRL(tr("Alternate Point")));
            }
            else {
                updateMouseWidgetTRCancel(tr("Select a line"));
            }
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
