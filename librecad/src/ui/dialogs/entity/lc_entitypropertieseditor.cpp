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
#include "lc_dlgentityproperties.h"
#include "lc_latecompletionrequestor.h"
#include "qc_applicationwindow.h"
#include "rs_graphicview.h"

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
    auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
    bool updateInteractiveInputValues = interactiveInputInfo->m_state == LC_ActionContext::InteractiveInputInfo::REQUESTED;
    double interactiveInputValueOne {0.0};
    double  interactiveInputValueTwo {0.0};
    QString interactiveInputTag = "";

    auto inputType = interactiveInputInfo->m_inputType;
    if (updateInteractiveInputValues) {
        interactiveInputTag = interactiveInputInfo->m_requestorTag;
        switch (inputType) {
            case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
                interactiveInputValueOne = interactiveInputInfo->m_distance;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                interactiveInputValueOne = interactiveInputInfo->m_angleRad;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::POINT: {
                interactiveInputValueOne = interactiveInputInfo->m_wcsPoint.x;
                interactiveInputValueTwo = interactiveInputInfo->m_wcsPoint.y;
                break;
            }
            default:
                break;
        }
    }
    else {
        inputType = LC_ActionContext::InteractiveInputInfo::NOTNEEDED;
    }

    auto* dlg = new LC_DlgEntityProperties(m_parent, m_viewport, m_entity, inputType, interactiveInputTag,
                                           interactiveInputValueOne, interactiveInputValueTwo);

    int result = dlg->exec();
    if ( result == QDialog::Accepted) {
        auto interactiveInputRequestType = dlg->isInteractiveInputRequested();
        if (interactiveInputRequestType == LC_ActionContext::InteractiveInputInfo::NOTNEEDED) { // normal closing of the dialog
            m_actionContext->interactiveInputRequestCancel();
            m_lateCompletionRequestor->onLateRequestCompleted(false);
            delete dlg;
        }
        else { // interactive input requested
            m_actionContext->interactiveInputStart(interactiveInputRequestType, this, dlg->getInteractiveInputTag());
            delete dlg;
        }
    }
    else { // notify about cancel of the dialog
        delete dlg;
        m_actionContext->interactiveInputRequestCancel();
        m_lateCompletionRequestor->onLateRequestCompleted(true);
    }
}

void LC_EntityPropertiesEditor::onLateRequestCompleted(bool shouldBeSkipped) {
    if (shouldBeSkipped) {
        auto interactiveInput = m_actionContext->getInteractiveInputInfo();
        interactiveInput->m_requestor = nullptr;
        interactiveInput->m_state = LC_ActionContext::InteractiveInputInfo::NONE;
    }
    QTimer::singleShot(100, this, &LC_EntityPropertiesEditor::showEntityPropertiesDialog);
}
