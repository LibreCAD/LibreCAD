/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
 ******************************************************************************/

#include "lc_defaultactioncontext.h"

#include "lc_actionoptionsmanager.h"
#include "lc_qtstatusbarmanager.h"
#include "qg_actionhandler.h"
#include "qg_commandwidget.h"
#include "qg_coordinatewidget.h"
#include "qg_mousewidget.h"
#include "qg_selectionwidget.h"

LC_DefaultActionContext::LC_DefaultActionContext(QG_ActionHandler* actionHandler):m_actionHandler{actionHandler} {
}

void LC_DefaultActionContext::addOptionsWidget(LC_ActionOptionsWidget *widget){
    m_actionOptionsManager->addOptionsWidget(widget);
}

void LC_DefaultActionContext::removeOptionsWidget(LC_ActionOptionsWidget *widget){
    m_actionOptionsManager->removeOptionsWidget(widget);
}

void LC_DefaultActionContext::requestSnapDistOptions(double *dist, bool on){
    m_actionOptionsManager->requestSnapDistOptions(dist, on);
}

void LC_DefaultActionContext::requestSnapMiddleOptions(int *middlePoints, bool on){
    m_actionOptionsManager->requestSnapMiddleOptions(middlePoints, on);
}

void LC_DefaultActionContext::hideSnapOptions(){
    m_actionOptionsManager->hideSnapOptions();
}

void LC_DefaultActionContext::updateSelectionWidget(int countSelected, double selectedLength){
    if (m_selectionWidget != nullptr) {
        m_selectionWidget->setNumber(countSelected);
        m_selectionWidget->setTotalLength(selectedLength);
    }
}

void LC_DefaultActionContext::updateMouseWidget(const QString &left, const QString &right, const LC_ModifiersInfo &modifiers){
    if (m_mouseWidget != nullptr) {
        m_mouseWidget->setHelp(left, right, modifiers);
    }

    if (m_commandWidget != nullptr) {
        m_commandWidget->setCommand(left);
    }

    if (m_statusBarManager != nullptr){
        m_statusBarManager->setActionHelp(left, right, modifiers);
    }
}

void LC_DefaultActionContext::commandMessage(const QString &message){
    if (m_commandWidget != nullptr) {
        m_commandWidget->appendHistory(message);
    }
}

void LC_DefaultActionContext::commandPrompt(const QString &message){
    if (m_commandWidget != nullptr) {
        m_commandWidget->setCommand(message);
    }
}

void LC_DefaultActionContext::updateCoordinateWidget(const RS_Vector &abs, const RS_Vector &rel, bool updateFormat){
    if (m_coordinateWidget != nullptr) {
        m_coordinateWidget->setCoordinates(abs, rel, updateFormat);
    }
}

void LC_DefaultActionContext::setDocumentAndView(RS_Document *document, RS_GraphicView *view){
    LC_ActionContext::setDocumentAndView(document, view);
    if (m_coordinateWidget != nullptr) {
        m_coordinateWidget->setGraphicView(view);
    }
}

void LC_DefaultActionContext::setSnapMode(const RS_SnapMode& mode) {
    m_actionHandler->setSnaps(mode);
}

void LC_DefaultActionContext::setCurrentAction(RS2::ActionType action, void* data) {
    m_actionHandler->setCurrentAction(action, data);
}

RS_ActionInterface* LC_DefaultActionContext::getCurrentAction() {
    return m_actionHandler->getCurrentAction();
}
