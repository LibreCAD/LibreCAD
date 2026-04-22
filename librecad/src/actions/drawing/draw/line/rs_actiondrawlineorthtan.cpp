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
#include "rs_document.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {
    //this holds a list of entity types which supports tangent
    const auto g_supportedCircleEntityTypes = EntityTypeList{
        {RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityHyperbola, RS2::EntityParabola}
    };
}

struct RS_ActionDrawLineOrthTan::ActionData {
    RS_Vector mousePosition;
    /** normal to tangent. */
    RS_Line* normal = nullptr; // the select normal line
    /** m_tangent. */
    /** arc/circle/ellipse to generate tangent */
    RS_Entity* circle = nullptr;
    bool altMode = false;
    bool setCircleFirst = false;
};

/**
 * This action class can handle user events to draw tangents normal to lines
 *
 * @author Dongxu Li
 */
RS_ActionDrawLineOrthTan::RS_ActionDrawLineOrthTan(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("Draw Tangent Orthogonal", actionContext, RS2::ActionDrawLineOrthTan),
      m_actionData{std::make_unique<ActionData>()} {
}

RS_ActionDrawLineOrthTan::~RS_ActionDrawLineOrthTan() = default;

void RS_ActionDrawLineOrthTan::finish() {
    clearLines();
    RS_PreviewActionInterface::finish();
}

void RS_ActionDrawLineOrthTan::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(entity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    if (isLine(entity)) {
        // fixme - support of polyline
        setLine(entity);
    }
    else {
        const RS2::EntityType rtti = entity->rtti();
        if (g_supportedCircleEntityTypes.contains(rtti)) {
            m_actionData->setCircleFirst = true;
            m_actionData->circle = entity;
            m_actionData->mousePosition = clickPos;
            setStatus(SetLine);
            highlightHover(entity);
            drawPreview();
        }
    }
}

RS_Entity* RS_ActionDrawLineOrthTan::doTriggerCreateEntity() {
    RS_Vector altTangentPosition;
    auto tangent = RS_Creation::createLineOrthTan(m_actionData->mousePosition, m_actionData->normal, m_actionData->circle,
                                                  altTangentPosition);
    if (tangent == nullptr) {
        return nullptr; // fixme - merge - review
    }
    if (m_actionData->altMode) {
        tangent.reset();
        tangent = RS_Creation::createLineOrthTan(altTangentPosition, m_actionData->normal, m_actionData->circle, altTangentPosition);
    }

    const auto tangentData = tangent->getData();
    auto* newEntity = new RS_Line(m_document, tangentData);
    return newEntity;
}

void RS_ActionDrawLineOrthTan::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetCircle);
    m_actionData->circle = nullptr;
}

void RS_ActionDrawLineOrthTan::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetLine: {
            const auto en = catchModifiableAndDescribe(e, RS2::EntityLine);
            if (en != nullptr) {
                highlightHover(en);
            }
            if (m_actionData->setCircleFirst) {
                highlightSelected(m_actionData->circle);
                RS_Vector alternativeTangentPoint;
                const RS_Vector mouse = e->graphPoint;
                const auto line = static_cast<RS_Line*>(en);
                auto tangent = RS_Creation::createLineOrthTan(mouse, line, m_actionData->circle, alternativeTangentPoint);
                if (tangent != nullptr) {
                    if (e->isControl) {
                        tangent.reset();
                        tangent = RS_Creation::createLineOrthTan(alternativeTangentPoint, line, m_actionData->circle,
                                                                 alternativeTangentPoint);
                    }
                    const auto tangentClone = tangent->clone();
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
            if (m_actionData->setCircleFirst) {
                const auto en = catchAndDescribe(e, g_supportedCircleEntityTypes, RS2::ResolveAll);
                if (en != nullptr) {
                    highlightHover(en);
                }
            }
            else {
                const auto normal = m_actionData->normal;
                highlightSelected(normal);
                deleteSnapper();
                const auto en = catchAndDescribe(e, g_supportedCircleEntityTypes, RS2::ResolveAll);
                if (en != nullptr) {
                    highlightHover(en);
                    RS_Vector alternativeTangentPoint;
                    const RS_Vector mouse = e->graphPoint;
                    const auto tangent = RS_Creation::createLineOrthTan(mouse, normal, en, alternativeTangentPoint);
                    if (tangent != nullptr) {
                        previewEntityToCreate(tangent->clone(), true);
                        previewRefSelectablePoint(alternativeTangentPoint);
                        previewRefSelectablePoint(tangent->getEndpoint());
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(tangent->getStartpoint());
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineOrthTan::clearLines() {
    m_actionData->circle = nullptr;
    deletePreview();
}

bool RS_ActionDrawLineOrthTan::setLine(RS_Entity* en) {
    if (en->getLength() < RS_TOLERANCE) {
        //ignore lines not long enough
        return true;
    }
    m_actionData->normal = dynamic_cast<RS_Line*>(en);
    if (m_actionData->setCircleFirst) {
        trigger();
    }
    else {
        setStatus(SetCircle);
    }
    return false;
}

void RS_ActionDrawLineOrthTan::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetLine: {
            const auto en = catchModifiableEntity(e, RS2::EntityLine);
            if (en != nullptr) {
                if (m_actionData->setCircleFirst) {
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
            const auto en = catchAndDescribe(e, g_supportedCircleEntityTypes, RS2::ResolveAll);
            if (en != nullptr) {
                m_actionData->circle = en;
                m_actionData->mousePosition = e->graphPoint;
                if (m_actionData->setCircleFirst) {
                    setStatus(SetLine);
                }
                else {
                    trigger();
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineOrthTan::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    clearLines();
    if (m_actionData->setCircleFirst) {
        if (status == SetCircle) {
            finish();
        }
        else {
            setStatus(SetCircle);
        }
    }
    else {
        if (status == SetLine) {
            finish();
        }
        else {
            initPrevious(status);
        }
    }
}

void RS_ActionDrawLineOrthTan::updateActionPrompt() {
    switch (getStatus()) {
        case SetLine:
            if (m_actionData->setCircleFirst) {
                updatePromptTRCancel(tr("Select a line"), MOD_CTRL(tr("Alternate Point")));
            }
            else {
                updatePromptTRCancel(tr("Select a line"));
            }
            break;
        case SetCircle:
            updatePromptTRBack(tr("Select circle, arc or ellipse"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType RS_ActionDrawLineOrthTan::doGetMouseCursor([[maybe_unused]] int status) {
    return isFinished() ? RS2::ArrowCursor : RS2::SelectCursor;
}
