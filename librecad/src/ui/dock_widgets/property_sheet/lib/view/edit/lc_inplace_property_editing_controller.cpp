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

#include "lc_inplace_property_editing_controller.h"

#include <QApplication>
#include <QCoreApplication>

class LC_InplacePropertyEditingController::InplaceEditorHandler : public QObject {
public:
    explicit InplaceEditorHandler(LC_InplacePropertyEditingController* inplaceController)
        : m_inplaceController{inplaceController} {
    }

    bool eventFilter(QObject* watched, QEvent* event) override;
    void onEditorDestroyed(const QObject* obj) const;

private:
    bool hasParent(const QObject* child, QObject* parent);
    LC_InplacePropertyEditingController* m_inplaceController;
};

bool LC_InplacePropertyEditingController::startInplaceEdit(QWidget* editor) {
    if (editor == nullptr) {
        return false;
    }
    Q_ASSERT(m_inplaceEditorRetainCount == 0);
    if (m_inplaceEditor != nullptr) {
        stopInplaceEdit(false, true);
    }

    Q_ASSERT(QCoreApplication::instance());

    if (editor->objectName().isEmpty()) {
        editor->setObjectName("LCPropertyValueEditor");
    }
    if (!editor->isVisible()) {
        editor->show();
    }
    m_inplaceEditor = editor;
    m_inplaceEditorHandler = new InplaceEditorHandler(this);

    // move focus to editor
    if (QApplication::focusWidget() != m_inplaceEditor->focusWidget()) {
        m_inplaceEditor->setFocus();
    }

    // connect to editor destroyed signal
    QObject::connect(m_inplaceEditor, &QObject::destroyed, m_inplaceEditorHandler, &InplaceEditorHandler::onEditorDestroyed);

    // install application event filter
    QCoreApplication::instance()->installEventFilter(m_inplaceEditorHandler);
    return true;
}

void LC_InplacePropertyEditingController::retainInplaceEditor() {
    ++m_inplaceEditorRetainCount;
}

void LC_InplacePropertyEditingController::releaseInplaceEditor() {
    Q_ASSERT(m_inplaceEditorRetainCount > 0);
    --m_inplaceEditorRetainCount;
}

QWidget* LC_InplacePropertyEditingController::getInplaceEdit() const {
    return m_inplaceEditor;
}

void LC_InplacePropertyEditingController::onInplaceWidgetDestroyed(const QObject* object) {
    // set focus to parent of inplace widget
    const auto parent = qobject_cast<QWidget*>(object->parent());
    if (parent != nullptr) {
        parent->setFocus();
    }
}

bool LC_InplacePropertyEditingController::stopInplaceEdit(const bool doDeleteLater, const bool restoreParentFocus) {
    if (m_inplaceEditor == nullptr) {
        return false;
    }
    if (m_inplaceEditorRetainCount > 0) {
        return false;
    }
    delete m_inplaceEditorHandler;
    m_inplaceEditorHandler = nullptr;

    if (restoreParentFocus) {
        // &LC_InplacePropertyEditingController::onInplaceWidgetDestroyed
        QObject::connect(m_inplaceEditor, &QObject::destroyed, [](const QObject* object)-> void {
            const auto parent = qobject_cast<QWidget*>(object->parent());
            if (parent != nullptr) {
                parent->setFocus();
            }
        });
    }

    if (doDeleteLater) {
        m_inplaceEditor->deleteLater();
    }
    else {
        delete m_inplaceEditor;
    }

    m_inplaceEditor = nullptr;
    return true;
}

bool LC_InplacePropertyEditingController::InplaceEditorHandler::hasParent(const QObject* child, QObject* parent) {
    if (child == nullptr) {
        return false;
    }
    if (child == parent) {
        return true;
    }
    return hasParent(child->parent(), parent);
}

bool LC_InplacePropertyEditingController::InplaceEditorHandler::eventFilter(QObject* watched, QEvent* event) {
    const auto inplaceEdit = m_inplaceController->getInplaceEdit();

    Q_ASSERT(inplaceEdit);
    if (event == nullptr) {
        return false;
    }
    // try handle by base class
    if (QObject::eventFilter(watched, event)) {
        return true;
    }
    if (event->type() == QEvent::FocusIn) {
        if (!hasParent(QApplication::focusObject(), inplaceEdit)) {
            m_inplaceController->stopInplaceEdit(true, false);
        }
        return false;
    }
    return false;
}

void LC_InplacePropertyEditingController::cleanupOnEditorDestroyed(const QObject* object) {
    Q_ASSERT(object == m_inplaceEditor);
    delete m_inplaceEditorHandler;
    m_inplaceEditorHandler = nullptr;
    m_inplaceEditor = nullptr;
}

void LC_InplacePropertyEditingController::InplaceEditorHandler::onEditorDestroyed(const QObject* obj) const {
    m_inplaceController->cleanupOnEditorDestroyed(obj);
}
