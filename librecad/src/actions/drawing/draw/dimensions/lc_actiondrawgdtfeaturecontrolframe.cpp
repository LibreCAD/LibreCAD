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

#include "lc_actiondrawgdtfeaturecontrolframe.h"

#include "lc_actioncontext.h"
#include "lc_dlg_tolerance.h"
#include "lc_graphicviewport.h"
#include "qc_applicationwindow.h"
#include "rs_document.h"


LC_ActionDrawGDTFeatureControlFrame::LC_ActionDrawGDTFeatureControlFrame(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("GDTFeatureControlFrame", actionContext, RS2::ActionGTDFCFrame)
    ,m_actionData{std::make_unique<ActionData>()}{
}

void LC_ActionDrawGDTFeatureControlFrame::init(const int status) {
    RS_PreviewActionInterface::init(status);
    if (status == ShowDialog) {
        // fixme - sand - potentially, we may reuse values were entered previously via settings or so .... not sure whether it's necessary, yet still
        const auto entity  = new LC_Tolerance(m_document, LC_ToleranceData{{0,0}, {0,0}, "", ""});
        LC_DlgTolerance dlg(QC_ApplicationWindow::getAppWindow().get(), m_viewport, entity, true);
        if (dlg.exec() == QDialog::Accepted) {
            m_actionData->entity = entity;
            setStatus(SetInsertionPoint);
        }
        else {
            delete entity;
            init(-1);
        }
    }
}

RS_Entity* LC_ActionDrawGDTFeatureControlFrame::doTriggerCreateEntity() {
    const auto e = m_actionData->entity;

    auto entityData = e->getData();

    entityData.insertionPoint = m_actionData->insertionPoint;
    RS_Vector ucsOrigin;
    double xAxisDirection;
    m_viewport->fillCurrentUCSInfo(ucsOrigin, xAxisDirection);
    entityData.directionVector = RS_Vector(xAxisDirection);

    // create a copy that will be added into the document (and not deleted by the action)
    const auto controlFrame  = new LC_Tolerance(m_document, entityData);
    controlFrame->update();
    moveRelativeZero(m_actionData->insertionPoint);

    return controlFrame;
}

void LC_ActionDrawGDTFeatureControlFrame::doTriggerCompletion([[maybe_unused]]bool success) {
    m_actionData->clear();
    setStatus(ShowDialog);
}

QStringList LC_ActionDrawGDTFeatureControlFrame::getAvailableCommands() {
    return RS_PreviewActionInterface::getAvailableCommands();
}

void LC_ActionDrawGDTFeatureControlFrame::updateActionPrompt() {
    switch (getStatus()) {
        case ShowDialog: {
            updatePromptTRCancel("Specify features data");
            break;
        }
        case SetInsertionPoint: {
            updatePromptTRCancel("Specify feature control frame insertion point");
            break;
        }
        default:
            break;
    }
}


void LC_ActionDrawGDTFeatureControlFrame::onCoordinateEvent(const int status, [[maybe_unused]]bool isZero, const RS_Vector& pos) {
    if (status == SetInsertionPoint) {
        m_actionData->insertionPoint = pos;
        trigger();
    }
}

void LC_ActionDrawGDTFeatureControlFrame::onMouseMoveEvent(const int status, const LC_MouseEvent* event) {
    auto snap = event->snapPoint;
    if (status == SetInsertionPoint){
        // less restrictive snap
        snap = getRelZeroAwarePoint(event, snap);
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDrawGDTFeatureControlFrame::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_PreviewActionInterface::onMouseLeftButtonRelease(status, e);
}

void LC_ActionDrawGDTFeatureControlFrame::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]] const LC_MouseEvent* e) {
    init(-1);
}

QStringList LC_ActionDrawGDTFeatureControlFrame::doGetAvailableCommands(const int status) {
    return RS_PreviewActionInterface::doGetAvailableCommands(status);
}


bool LC_ActionDrawGDTFeatureControlFrame::doProcessCommand(const int status, const QString& command) {
    return RS_PreviewActionInterface::doProcessCommand(status, command);
}
