/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_action_options_editor_typed.h"

#include "lc_propertysheetwidget.h"
#include "qc_applicationwindow.h"
#include "rs_actioninterface.h"

void LC_ActionOptionsEditorTyped::showOptions() {
    if (m_showOptionsInToolOptionsWidget) {
        showOptionsInToolOptionsWidget();
    }
    if (m_showOptionsInPropertySheetWidget) {
        showOptionsInPropertySheetWidget();
    }
}

void LC_ActionOptionsEditorTyped::updateOptionsUI(int mode, const QVariant *value)  {
     if (m_showOptionsInToolOptionsWidget) {
         updateOptionsUIInToolOptionsWidget(mode, value);
     }
     if (m_showOptionsInPropertySheetWidget) {
         updateOptionsUIInPropertySheetWidget(mode, value);
     }
}

void LC_ActionOptionsEditorTyped::hideOptions() {
     if (m_showOptionsInToolOptionsWidget) {
         hideOptionsInToolOptionsWidget();
     }
     if (m_showOptionsInPropertySheetWidget) {
         hideOptionsInPropertySheetWidget();
     }
 }

void LC_ActionOptionsEditorTyped::updateOptions(const QString& tagToFocus) {
     if (m_showOptionsInToolOptionsWidget) {
         updateOptionsInToolOptionsWidget(tagToFocus);
     }
     if (m_showOptionsInPropertySheetWidget) {
         updateOptionsInPropertySheetWidget(tagToFocus);
     }
}

LC_ActionOptionsWidget* LC_ActionOptionsEditorTyped::createOptionsWidget() const {
    return m_funCreateOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionOptionsEditorTyped::createOptionsFiller() const {
    return m_funCreateOptionsFiller();
}

void LC_ActionOptionsEditorTyped::showOptionsInToolOptionsWidget() {
    if (m_optionWidget == nullptr) {
        m_optionWidget.reset(createOptionsWidget());
    }
    if (m_optionWidget != nullptr) {
        if (!m_optionWidget->isVisible()) {
            if (m_optionWidget->parent() == nullptr) {
                // first time created
                LC_ActionContext* actionContext = m_action->getActionContext();
                actionContext->addOptionsWidget(m_optionWidget.get());
                m_optionWidget->setAction(m_action);
                m_optionWidget->show();
            }
            else {
                m_optionWidget->show();
            }
        }
    }
}

void LC_ActionOptionsEditorTyped::updateOptionsUIInToolOptionsWidget(int mode,const QVariant *value) {
    if (m_optionWidget != nullptr) {
        m_optionWidget->updateUI(mode, value);
    }
}

void LC_ActionOptionsEditorTyped::hideOptionsInToolOptionsWidget() {
     if (m_optionWidget != nullptr) {
         m_optionWidget->hideOptions();
         auto actionContext = m_action->getActionContext();
         actionContext->removeOptionsWidget(m_optionWidget.get());
     }
}

void LC_ActionOptionsEditorTyped::updateOptionsInToolOptionsWidget(const QString& tagToFocus) {
     if (m_optionWidget == nullptr) {
         LC_ActionOptionsWidget* widget = createOptionsWidget();
         m_optionWidget.reset(widget);
     }
     if (m_optionWidget != nullptr) {
         if (!m_optionWidget->isVisible()) {
             if (m_optionWidget->parent() == nullptr) {
                 // first time created
                 LC_ActionContext* actionContext = m_action->getActionContext();
                 actionContext->addOptionsWidget(m_optionWidget.get());
                 m_optionWidget->setAction(m_action, true);
             }
             else {
                 m_optionWidget->setAction(m_action, true);
                 m_optionWidget->show();
             }
         }
         else {
             m_optionWidget->setAction(m_action, true);
         }
         if (!tagToFocus.isEmpty()) {
             m_optionWidget->requestFocusForTag(tagToFocus);
         }
     }
}

void LC_ActionOptionsEditorTyped::showOptionsInPropertySheetWidget() {
    if (m_optionFiller == nullptr) {
        LC_ActionOptionsPropertiesFiller* filler = createOptionsFiller();
        m_optionFiller.reset(filler);
    }
    if (m_optionFiller != nullptr) {
        m_optionFiller->setAction(m_action);
    }
    // fixme - more elegant depenency?
    const auto propertySheetWidget = QC_ApplicationWindow::getAppWindow()->getPropertySheetWidget();
    propertySheetWidget->showToolOptions(m_optionFiller.get());
}

void LC_ActionOptionsEditorTyped::updateOptionsUIInPropertySheetWidget([[maybe_unused]]int mode, [[maybe_unused]]const QVariant *value) {
    const auto propertySheetWidget = QC_ApplicationWindow::getAppWindow()->getPropertySheetWidget();
    propertySheetWidget->refill();
}

void LC_ActionOptionsEditorTyped::hideOptionsInPropertySheetWidget() {
    if (m_optionFiller != nullptr) {
        m_optionFiller->hideOptions();
        m_optionFiller.reset(nullptr);
        const auto propertySheetWidget = QC_ApplicationWindow::getAppWindow()->getPropertySheetWidget();
        propertySheetWidget->showToolOptions(nullptr);
    }
}

void LC_ActionOptionsEditorTyped::updateOptionsInPropertySheetWidget([[maybe_unused]]const QString& tagToFocus) {
    const auto propertySheetWidget = QC_ApplicationWindow::getAppWindow()->getPropertySheetWidget();
    propertySheetWidget->refill();
}
