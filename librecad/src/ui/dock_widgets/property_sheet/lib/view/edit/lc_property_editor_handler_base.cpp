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

#include "lc_property_editor_handler_base.h"

#include <QDialog>
#include <QKeyEvent>

LC_PropertyEditorHandlerBase::LC_PropertyEditorHandlerBase(LC_PropertyViewEditable* view, QWidget& editor)
    : QObject(&editor), m_reverted(false), m_returned(false), m_view(view), m_editor(&editor) {
    Q_ASSERT(view != nullptr);
    view->m_editorHandler = this;
    const auto property = view->getProperty();
    Q_ASSERT(property);
    connect(property, &LC_Property::afterPropertyChange, this, &LC_PropertyEditorHandlerBase::onPropertyDidChange, Qt::QueuedConnection);
    connect(property, &QObject::destroyed, this, &LC_PropertyEditorHandlerBase::onPropertyDestroyed);

    const auto stateProperty = view->getStateProperty();
    if (stateProperty != property) {
        connect(stateProperty, &LC_Property::afterPropertyChange, this, &LC_PropertyEditorHandlerBase::onPropertyDidChange,
                Qt::QueuedConnection);
    }
}

LC_PropertyEditorHandlerBase::~LC_PropertyEditorHandlerBase() {
    cleanup();
}

void LC_PropertyEditorHandlerBase::cleanup() {
    if (m_editor != nullptr) {
        m_editor->removeEventFilter(this);
    }
    // stopInplaceEdit();
    if (m_view != nullptr) {
        m_view->m_editorHandler = nullptr;
        m_view = nullptr;
    }
}

void LC_PropertyEditorHandlerBase::doRollbackValue() {
    m_reverted = true;
    doUpdateEditor();
}

void LC_PropertyEditorHandlerBase::stopInplaceEdit() const {
    if (m_view != nullptr) {
        m_view->stopInplaceEditing();
    }
}

bool LC_PropertyEditorHandlerBase::eventFilter(QObject* obj, QEvent* event) {
    switch (event->type()) {
        case QEvent::KeyPress: {
            const auto keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
                case Qt::Key_Enter:
                case Qt::Key_Return: {
                    m_returned = true;
                    break;
                }
                case Qt::Key_Escape: {
                    doRollbackValue();
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default: {
            break;
        }
    }
    return QObject::eventFilter(obj, event);
}

bool LC_PropertyEditorHandlerBase::doCheckMayApply() const {
    if (isEditableByUser()) {
        return !m_reverted && (m_returned || !isMultiValue());
    }
    return false;
}

void LC_PropertyEditorHandlerBase::doApplyReset() {
    m_reverted = false;
    m_returned = false;
}

void LC_PropertyEditorHandlerBase::onPropertyDestroyed() {
    cleanup();
}

void LC_PropertyEditorHandlerBase::onPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (sender() == nullptr) {
        return;
    }
    if (reason & (PropertyChangeReasonValue | PropertyChangeReasonState)) {
        if (getBaseProperty() == sender() || getStateProperty() == sender()) {
            doUpdateEditor();
        }
    }
}

LC_Property* LC_PropertyEditorHandlerBase::getStateProperty() const {
    if (m_view != nullptr) {
        return m_view->getStateProperty();
    }
    return nullptr;
}

LC_Property* LC_PropertyEditorHandlerBase::getBaseProperty() const {
    if (m_view != nullptr) {
        return m_view->getProperty();
    }
    return nullptr;
}

bool LC_PropertyEditorHandlerBase::isEditableByUser() const {
    const auto stateProp = getStateProperty();
    return (stateProp != nullptr) ? stateProp->isEditableByUser() : false;
}

bool LC_PropertyEditorHandlerBase::isMultiValue() const {
    const auto stateProp = getStateProperty();
    return (stateProp != nullptr) ? stateProp->isMultiValue() : false;
}

LC_PropertyEditorHandlerBase::DialogRAIIGuard::DialogRAIIGuard(QDialog* dialog)
    : dialog(dialog) {
}

LC_PropertyEditorHandlerBase::DialogRAIIGuard::~DialogRAIIGuard() {
    if (nullptr != dialog && nullptr == dialog->parent()) {
        delete dialog;
    }
}

LC_PropertyEditorHandlerBase::PtrDialogContainer LC_PropertyEditorHandlerBase::connectDialog(QDialog* dialog) {
    PtrDialogContainer result(new DialogRAIIGuard(dialog));
    connectDialog(result);
    return result;
}

void LC_PropertyEditorHandlerBase::connectDialog(const PtrDialogContainer& containerPtr) {
    Q_ASSERT(nullptr != containerPtr);
    const auto dialog = containerPtr->dialog;
    Q_ASSERT(nullptr != dialog);

    const auto parent = dialog->parent();
    Q_ASSERT(nullptr != parent);

    connect(parent, &QObject::destroyed, [containerPtr] {
        containerPtr->dialog->setParent(nullptr);
    });
}
