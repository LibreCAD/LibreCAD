/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_actioncontext.h"
#include "rs_document.h"
#include "rs_graphicview.h"

RS_EntityContainer * LC_ActionContext::getEntityContainer(){
    return m_entityContainer;
}

RS_GraphicView * LC_ActionContext::getGraphicView(){
    return m_graphicView;
}

void LC_ActionContext::setDocumentAndView(RS_Document *document, RS_GraphicView *view){
        m_graphicView = view;
        m_entityContainer = document;
}

void LC_ActionContext::saveContextMenuActionContext(RS_Entity* entity, const RS_Vector& position, bool clearEntitySelection) {
    m_contextMenuActionEntity = entity;
    m_contextMenuClickPosition = position;
    m_uselectContextMenuActionEntity = clearEntitySelection;
}

void LC_ActionContext::clearContextMenuActionContext() {
    if (m_uselectContextMenuActionEntity) {
        m_contextMenuActionEntity->setSelected(false);
    }
    m_contextMenuActionEntity = nullptr;
    m_contextMenuClickPosition = RS_Vector(false);
}

RS_Entity* LC_ActionContext::getContextMenuActionContextEntity() {
    return m_contextMenuActionEntity;
}

RS_Vector LC_ActionContext::getContextMenuActionClickPosition() {
    return m_contextMenuClickPosition;
}

void LC_ActionContext::interactiveInputStart(InteractiveInputInfo::InputType inputType,
    LC_LateCompletionRequestor* requestor, const QString& tag) {
    interactiveInputRequest(inputType, requestor, tag);
    interactiveInputInvoke(inputType);
}

void LC_ActionContext::interactiveInputRequestCancel() {
    m_interactiveInputInfo.m_requestor = nullptr;
    m_interactiveInputInfo.m_inputType = InteractiveInputInfo::NOTNEEDED;
    m_interactiveInputInfo.m_requestorTag = "";
    m_interactiveInputInfo.m_state = InteractiveInputInfo::NONE;
}

void LC_ActionContext::interactiveInputInvoke(InteractiveInputInfo::InputType inputType) {
    switch (inputType) {
        case InteractiveInputInfo::DISTANCE: {
            setCurrentAction(RS2::ActionInteractivePickLength, nullptr);
            break;
        }
        case InteractiveInputInfo::ANGLE: {
            setCurrentAction(RS2::ActionInteractivePickAngle, nullptr);
            break;
        }
        case InteractiveInputInfo::POINT: {
            setCurrentAction(RS2::ActionInteractivePickPoint, nullptr);
            break;
        }
        default:
            break;
    }
}

void LC_ActionContext::interactiveInputRequest(InteractiveInputInfo::InputType inputType, LC_LateCompletionRequestor* requestor, const QString &tag) {
    m_interactiveInputInfo.m_requestor = requestor;
    m_interactiveInputInfo.m_inputType = inputType;
    m_interactiveInputInfo.m_requestorTag = tag;
    m_interactiveInputInfo.m_state = InteractiveInputInfo::REQUESTED;
}
