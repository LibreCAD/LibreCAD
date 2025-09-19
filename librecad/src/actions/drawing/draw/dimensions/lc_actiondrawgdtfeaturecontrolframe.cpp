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
#include "lc_dlgtolerance.h"
#include "lc_graphicviewport.h"
#include "qc_applicationwindow.h"
#include "rs_document.h"





LC_ActionDrawGDTFeatureControlFrame::LC_ActionDrawGDTFeatureControlFrame(LC_ActionContext* actionContext)
    : RS_PreviewActionInterface("GDTFeatureControlFrame", actionContext, RS2::ActionGTDFCFrame)
    ,m_actionData{std::make_unique<ActionData>()}{
}

void LC_ActionDrawGDTFeatureControlFrame::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (status == ShowDialog) {
        // fixme - sand - potentially, we may reuse values were entered previously via settings or so .... not sure whether it's necessary, yet still
        auto entity  = new LC_Tolerance(m_document, LC_ToleranceData{{0,0}, {0,0}, "", ""});
        LC_DlgTolerance dlg(QC_ApplicationWindow::getAppWindow().get(), m_viewport, entity, true);
        if (dlg.exec() == QDialog::Accepted) {
            m_actionData->m_entity = entity;
            setStatus(SetInsertionPoint);
        }
        else {
            delete entity;
            init(-1);
        }
    }
}

void LC_ActionDrawGDTFeatureControlFrame::doTrigger() {
    auto e = m_actionData->m_entity;

    auto entityData = e->getData();

    entityData.m_insertionPoint = m_actionData->m_insertionPoint;
    RS_Vector ucsOrigin;
    double xAxisDirection;
    m_viewport->fillCurrentUCSInfo(ucsOrigin, xAxisDirection);
    entityData.m_directionVector = RS_Vector(xAxisDirection);

    // create a copy that will be added into the document (and not deleted by the action)
    auto controlFrame  = new LC_Tolerance(m_document, entityData);
    setPenAndLayerToActive(controlFrame);
    controlFrame->update();
    undoCycleAdd(controlFrame);

    moveRelativeZero(m_actionData->m_insertionPoint);

    m_actionData->clear();
    setStatus(ShowDialog);
}

QStringList LC_ActionDrawGDTFeatureControlFrame::getAvailableCommands() {
    return RS_PreviewActionInterface::getAvailableCommands();
}

void LC_ActionDrawGDTFeatureControlFrame::updateMouseButtonHints() {
    switch (getStatus()) {
        case ShowDialog: {
            updateMouseWidgetTRCancel("Specify features data");
            break;
        }
        case SetInsertionPoint: {
            updateMouseWidgetTRCancel("Specify feature control frame insertion point");
            break;
        }
        default:
            break;
    }
}


void LC_ActionDrawGDTFeatureControlFrame::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector& pos) {
    if (status == SetInsertionPoint) {
        m_actionData->m_insertionPoint = pos;
        trigger();
    }
}

void LC_ActionDrawGDTFeatureControlFrame::onMouseMoveEvent(int status, LC_MouseEvent* event) {
    auto snap = event->snapPoint;
    if (status == SetInsertionPoint){
        // less restrictive snap
        snap = getRelZeroAwarePoint(event, snap);
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDrawGDTFeatureControlFrame::onMouseLeftButtonRelease(int status, LC_MouseEvent* e) {
    RS_PreviewActionInterface::onMouseLeftButtonRelease(status, e);
}

void LC_ActionDrawGDTFeatureControlFrame::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent* e) {
    init(-1);
}

QStringList LC_ActionDrawGDTFeatureControlFrame::doGetAvailableCommands(int status) {
    return RS_PreviewActionInterface::doGetAvailableCommands(status);
}


bool LC_ActionDrawGDTFeatureControlFrame::doProcessCommand(int status, const QString& command) {
    return RS_PreviewActionInterface::doProcessCommand(status, command);
}
