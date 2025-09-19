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

#include "lc_actioninteractivepickbase.h"

#include <QKeyEvent>

#include "lc_actioncontext.h"

LC_ActionInteractivePickBase::LC_ActionInteractivePickBase(const char* name, LC_ActionContext* actionContext, RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, actionContext, actionType) {}

void LC_ActionInteractivePickBase::doTrigger() {
    if (isInteractiveDataValid()){
        storeInteractiveInput();
    }
    finish();
}

void LC_ActionInteractivePickBase::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {
        case Qt::Key_Escape: {
            skipInteractiveInput();
            finish(false);
            break;
        }
        default: {
            RS_PreviewActionInterface::keyPressEvent(e);
        }
    }
}

void LC_ActionInteractivePickBase::skipInteractiveInput() {
    auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
    if (interactiveInputInfo->m_state == LC_ActionContext::InteractiveInputInfo::REQUESTED) {
        if (interactiveInputInfo->m_requestor != nullptr) {
            interactiveInputInfo->m_requestor->onLateRequestCompleted(false);
        }
    }
}

void LC_ActionInteractivePickBase::storeInteractiveInput() {
    auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
    if (interactiveInputInfo->m_state == LC_ActionContext::InteractiveInputInfo::REQUESTED) {
        doSetInteractiveInputValue(interactiveInputInfo);
        if (interactiveInputInfo->m_requestor != nullptr) {
            interactiveInputInfo->m_requestor->onLateRequestCompleted(false);
        }
    }
}
