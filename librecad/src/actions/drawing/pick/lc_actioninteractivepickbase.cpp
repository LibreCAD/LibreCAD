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

LC_ActionInteractivePickBase::LC_ActionInteractivePickBase(const char* name, LC_ActionContext* actionContext, const RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, actionContext, actionType) {
}

void LC_ActionInteractivePickBase::createOptionsEditor() {
    RS_PreviewActionInterface::createOptionsEditor();
    m_optionsEditor->setup(false, false);
}

void LC_ActionInteractivePickBase::setPredecessor(std::shared_ptr<RS_ActionInterface> pre) {
    if (pre != nullptr) {
        const RS2::ActionType rtti = pre->rtti();
        if (!RS2::isInteractiveInputAction(rtti)) {
            // we support predecessor only if previos action is not interactive picK.
            // otherwise - we'll skip it, as with property sheet it is possible to
            // invoke precessor twice...
            RS_PreviewActionInterface::setPredecessor(pre);
        }
        else {
            pre.reset();
        }
    }
}

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
            finish();
            break;
        }
        default: {
            RS_PreviewActionInterface::keyPressEvent(e);
            break;
        }
    }
}

void LC_ActionInteractivePickBase::skipInteractiveInput() const {
    const auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
    if (interactiveInputInfo->state == LC_ActionContext::InteractiveInputInfo::REQUESTED) {
        if (interactiveInputInfo->requestor != nullptr) {
            interactiveInputInfo->requestor->onLateRequestCompleted(true);
        }
    }
}

void LC_ActionInteractivePickBase::storeInteractiveInput() {
    const auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
    if (interactiveInputInfo->state == LC_ActionContext::InteractiveInputInfo::REQUESTED) {
        doSetInteractiveInputValue(interactiveInputInfo);
        if (interactiveInputInfo->requestor != nullptr) {
            interactiveInputInfo->requestor->onLateRequestCompleted(false);
        }
    }
}
