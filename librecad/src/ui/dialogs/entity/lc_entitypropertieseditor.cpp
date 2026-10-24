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
#include <utility>

#include "lc_actioncontext.h"
#include "lc_dlg_entityproperties.h"
#include "lc_latecompletionrequestor.h"

namespace {
void silenceGuardedDialog(QObject& object) {
    object.blockSignals(true);
    for (auto* child : object.children()) silenceGuardedDialog(*child);
}
}

LC_EntityPropertiesEditSession::~LC_EntityPropertiesEditSession() {
    if (entity) entity->setParent(nullptr);
}

LC_EntityPropertiesEditor::~LC_EntityPropertiesEditor() {
    cancelGuardedEdit();
}

LC_EntityPropertiesEditor::LC_EntityPropertiesEditor(LC_ActionContext* actionContext,  LC_LateCompletionRequestor* requestor)
    :m_actionContext{actionContext}, m_lateCompletionRequestor(requestor) {
}

void LC_EntityPropertiesEditor::editEntity(QWidget* parent, RS_Entity* entity, LC_GraphicViewport* viewport,
    std::function<bool()> guard, std::shared_ptr<LC_EntityPropertiesEditSession> session) {
    m_entity = entity;
    m_viewport = viewport;
    m_parent = parent;
    m_editGuard = std::move(guard);
    m_guardedSession = std::move(session);
    ++m_editGeneration;
    scheduleDialog(20);
}

void LC_EntityPropertiesEditor::scheduleDialog(int delay) {
    const auto generation = m_editGeneration;
    QTimer::singleShot(delay, this, [this, generation] {
        if (generation == m_editGeneration && m_entity) showEntityPropertiesDialog();
    });
}

void LC_EntityPropertiesEditor::cancelGuardedEdit() {
    if (!m_editGuard) return;
    if (m_actionContext && m_actionContext->getInteractiveInputInfo()->requestor == this)
        m_actionContext->interactiveInputRequestCancel();
    ++m_editGeneration;
    m_editGuard = {};
    m_entity = nullptr;
    m_viewport = nullptr;
    m_parent = nullptr;
    const auto dialog = m_activeGuardedDialog;
    m_activeGuardedDialog.clear();
    if (dialog) {
        // Close before another paint/focus callback can visit stale layer/document pointers.
        silenceGuardedDialog(*dialog);
        dialog->QDialog::reject();
    }
    m_guardedSession.reset();
}

void LC_EntityPropertiesEditor::showEntityPropertiesDialog() {
    if (!m_entity) return;
    if (m_editGuard && !m_editGuard()) {
        m_lateCompletionRequestor->onLateRequestCompleted(false); // native guard reports and owns cleanup
        return;
    }
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

    const bool guarded = bool(m_editGuard);
    const auto generation = m_editGeneration;
    const QPointer<LC_EntityPropertiesEditor> self(this);
    // Declare before dlg: the native clone is released only AFTER all its widgets are destroyed.
    const auto session = m_guardedSession;
    auto dlg = LC_DlgEntityProperties(m_parent, m_viewport, m_entity, inputType, interactiveInputTag,
                                           interactiveInputValueOne, interactiveInputValueTwo);

    if (guarded) m_activeGuardedDialog = &dlg;
    const int result = dlg.exec();
    // exec() may have destroyed this editor AND its requesting action. Do not read this first.
    if (guarded && (!self || generation != self->m_editGeneration)) return;
    if (guarded) m_activeGuardedDialog.clear();
    if (m_editGuard && !m_editGuard()) {
        m_lateCompletionRequestor->onLateRequestCompleted(false);
        return;
    }
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
    if (!m_entity) return;
    if (m_editGuard && !m_editGuard()) {
        m_lateCompletionRequestor->onLateRequestCompleted(false);
        return;
    }
    if (shouldBeSkipped) {
        const auto interactiveInput = m_actionContext->getInteractiveInputInfo();
        interactiveInput->requestor = nullptr;
        interactiveInput->state = LC_ActionContext::InteractiveInputInfo::NONE;
    }
    scheduleDialog(100);
}
