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

#include "lc_entitypropertieseditor.h"

#include <QTimer>

#include "lc_actioncontext.h"
#include "lc_dlg_entityproperties.h"
#include "lc_latecompletionrequestor.h"

LC_EntityPropertiesEditor::LC_EntityPropertiesEditor(LC_ActionContext* actionContext,  LC_LateCompletionRequestor* requestor)
    :m_actionContext{actionContext}, m_lateCompletionRequestor(requestor) {
}

void LC_EntityPropertiesEditor::editEntity(QWidget* parent, RS_Entity* entity, LC_GraphicViewport* viewport) {
    m_entity = entity;
    m_viewport = viewport;
    m_parent = parent;
    QTimer::singleShot(20, this, &LC_EntityPropertiesEditor::showEntityPropertiesDialog);
}

void LC_EntityPropertiesEditor::showEntityPropertiesDialog() {
    const auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
    const bool updateInteractiveInputValues = interactiveInputInfo->state == LC_ActionContext::InteractiveInputInfo::REQUESTED;
    double interactiveInputValueOne {0.0};
    double  interactiveInputValueTwo {0.0};
    QString interactiveInputTag = "";

    auto inputType = interactiveInputInfo->inputType;
    if (updateInteractiveInputValues) {
        interactiveInputTag = interactiveInputInfo->requestorTag;
        switch (inputType) {
            case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
                interactiveInputValueOne = interactiveInputInfo->distance;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                interactiveInputValueOne = interactiveInputInfo->angleRad;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::POINT: {
                interactiveInputValueOne = interactiveInputInfo->wcsPoint.x;
                interactiveInputValueTwo = interactiveInputInfo->wcsPoint.y;
                break;
            }
            default:
                break;
        }
    }
    else {
        inputType = LC_ActionContext::InteractiveInputInfo::NOTNEEDED;
    }

    auto dlg = LC_DlgEntityProperties(m_parent, m_viewport, m_entity, inputType, interactiveInputTag,
                                           interactiveInputValueOne, interactiveInputValueTwo);

    const int result = dlg.exec();
    if ( result == QDialog::Accepted) {
        const auto interactiveInputRequestType = dlg.isInteractiveInputRequested();
        if (interactiveInputRequestType == LC_ActionContext::InteractiveInputInfo::NOTNEEDED) { // normal closing of the dialog
            m_actionContext->interactiveInputRequestCancel();
            m_lateCompletionRequestor->onLateRequestCompleted(false);
        }
        else { // interactive input requested
            m_actionContext->interactiveInputStart(interactiveInputRequestType, this, dlg.getInteractiveInputTag());
        }
    }
    else { // notify about cancel of the dialog
        m_actionContext->interactiveInputRequestCancel();
        m_lateCompletionRequestor->onLateRequestCompleted(true);
    }
}

void LC_EntityPropertiesEditor::onLateRequestCompleted(const bool shouldBeSkipped) {
    if (shouldBeSkipped) {
        const auto interactiveInput = m_actionContext->getInteractiveInputInfo();
        interactiveInput->requestor = nullptr;
        interactiveInput->state = LC_ActionContext::InteractiveInputInfo::NONE;
    }
    QTimer::singleShot(100, this, &LC_EntityPropertiesEditor::showEntityPropertiesDialog);
}
