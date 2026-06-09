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

#include "lc_action_select_quick.h"

#include <QTimer>

#include "lc_dlgquickselection.h"
#include "qc_applicationwindow.h"

LC_ActionSelectQuick::~LC_ActionSelectQuick() = default;

void LC_ActionSelectQuick::onLateRequestCompleted(const bool shouldBeSkipped) {
    if (shouldBeSkipped) {
        const auto interactiveInput = m_actionContext->getInteractiveInputInfo();
        interactiveInput->requestor = nullptr;
        interactiveInput->state = LC_ActionContext::InteractiveInputInfo::NONE;
    }
    else {
        QTimer::singleShot(100, this, &LC_ActionSelectQuick::showDialog);
    }
}

void LC_ActionSelectQuick::trigger() {
    switch (getStatus()){
        case SHOW_DIALOG: {
            showSelectionDialog();
            break;
        }
        case EDITING:{
            break;
        }
        default:
            break;
    }
}

void LC_ActionSelectQuick::init(const int status) {
    if (status == SHOW_DIALOG) {
        m_selectionComplete = true;
        trigger();
    }
    else {
        LC_ActionPreSelectionAwareBase::init(status);
    }
}

void LC_ActionSelectQuick::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Choose entities") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Return to dialog immediately after selection")));
}

void LC_ActionSelectQuick::onSelectionCompleted([[maybe_unused]]bool singleEntity, [[maybe_unused]]bool fromInit) {
    showSelectionDialog();
}

void LC_ActionSelectQuick::showSelectionDialog() {
    QTimer::singleShot(20, this, &LC_ActionSelectQuick::showDialog);
}

void LC_ActionSelectQuick::performSelection(LC_DlgQuickSelection* dlg) const {
    RS_Selection::ConditionalSelectionOptions conditionalSelectionOptions;
    dlg->getSelectionOptions(conditionalSelectionOptions);
    const RS_Selection sel(m_document, m_viewport);
    sel.conditionalSelection(conditionalSelectionOptions);
    conditionalSelectionOptions.funCleanup();
}

void LC_ActionSelectQuick::showDialog() {
    const auto interactiveInputInfo = m_actionContext->getInteractiveInputInfo();
    const bool updateInteractiveInputValues = interactiveInputInfo->state == LC_ActionContext::InteractiveInputInfo::REQUESTED;
    double interactiveInputValueOne {0.0};
    double  interactiveInputValueTwo {0.0};

    auto inputType = interactiveInputInfo->inputType;
    if (updateInteractiveInputValues) {
        switch (inputType) {
            case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
                interactiveInputValueOne = interactiveInputInfo->distance;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                interactiveInputValueOne = interactiveInputInfo->angleRad;
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::POINT:
            case LC_ActionContext::InteractiveInputInfo::POINT_X:
            case LC_ActionContext::InteractiveInputInfo::POINT_Y: {
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

    QWidget* parent = QC_ApplicationWindow::getAppWindow().get();
    auto* dlg = new LC_DlgQuickSelection(parent,m_actionContext, inputType, m_savedState,
                                           interactiveInputValueOne, interactiveInputValueTwo);
    m_allowExternalTermination = false;
    const int result = dlg->exec();
    if (result == QDialog::Accepted) {
        const auto interactiveInputRequestType = dlg->isInteractiveInputRequested();
        if (interactiveInputRequestType == LC_ActionContext::InteractiveInputInfo::NOTNEEDED) { // normal closing of the dialog or ask for additional selection
            if (dlg->isAdditionalSelectionRequested()) {
                m_selectionComplete = false;
                m_actionContext->interactiveInputRequestCancel();
                onLateRequestCompleted(true);
            }
            else { // normal closing of the dialog
                m_savedState = nullptr;
                m_actionContext->interactiveInputRequestCancel();
                onLateRequestCompleted(true);
                performSelection(dlg);
                finish();
            }
        }
        else { // interactive input requested
            m_savedState = dlg->getSavedState();
            m_actionContext->interactiveInputStart(interactiveInputRequestType, this, "");
        }
    }
    else { // notify about cancel of the dialog
        m_savedState = nullptr;
        m_actionContext->interactiveInputRequestCancel();
        onLateRequestCompleted(true);
        finish();
    }
    m_allowExternalTermination = true;
    delete dlg;
}
