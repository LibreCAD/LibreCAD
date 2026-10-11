/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "lc_dialog.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QFocusEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSpinBox>

#include "rs_settings.h"

namespace {
class SpinBoxSelectionFilter : public QObject {
public:
    explicit SpinBoxSelectionFilter(QAbstractSpinBox* spinBox) : QObject(spinBox) {
        spinBox->installEventFilter(this);
        if (auto* editor = spinBox->findChild<QLineEdit*>()) {
            editor->installEventFilter(this);
            connect(editor, &QLineEdit::selectionChanged, this, [this, editor] {
                // A deliberate selection cancels replacement even if the drag returns to its start.
                if (editor->hasSelectedText()) {
                    m_state = State::Idle;
                }
            });
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        auto* editor = qobject_cast<QLineEdit*>(watched);
        if (event->type() == QEvent::FocusIn) {
            m_state = static_cast<QFocusEvent*>(event)->reason() == Qt::MouseFocusReason
                ? State::AwaitingPress : State::Idle;
        } else if (event->type() == QEvent::FocusOut || event->type() == QEvent::Hide
                   || event->type() == QEvent::UngrabMouse || event->type() == QEvent::KeyPress
                   || event->type() == QEvent::KeyRelease || event->type() == QEvent::MouseButtonDblClick) {
            m_state = State::Idle;
        } else if (event->type() == QEvent::MouseButtonPress) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            const bool select = m_state == State::AwaitingPress && editor
                && mouse->button() == Qt::LeftButton && mouse->buttons() == Qt::LeftButton
                && mouse->modifiers() == Qt::NoModifier;
            m_state = select ? State::AwaitingRelease : State::Idle;
            m_pressPosition = mouse->position().toPoint();
        } else if (event->type() == QEvent::MouseMove && m_state == State::AwaitingRelease) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->buttons() != Qt::LeftButton || mouse->modifiers() != Qt::NoModifier
                || (mouse->position().toPoint() - m_pressPosition).manhattanLength()
                    >= QApplication::startDragDistance()) {
                m_state = State::Idle;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            const bool select = m_state == State::AwaitingRelease;
            m_state = State::Idle;
            auto* mouse = static_cast<QMouseEvent*>(event);
            auto* spinBox = static_cast<QAbstractSpinBox*>(parent());
            if (select && editor && !editor->hasSelectedText() && !spinBox->isReadOnly()
                && spinBox->hasFocus() && mouse->button() == Qt::LeftButton
                && mouse->buttons() == Qt::NoButton && mouse->modifiers() == Qt::NoModifier
                && editor->rect().contains(mouse->position().toPoint())
                && (mouse->position().toPoint() - m_pressPosition).manhattanLength()
                    < QApplication::startDragDistance()) {
                spinBox->selectAll();
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    enum class State { Idle, AwaitingPress, AwaitingRelease };
    State m_state = State::Idle;
    QPoint m_pressPosition;
};
}

// fixme - sand - review all dialogs and actions and make all conversions (double/string, angle/string and vise versa consistent)!
LC_Dialog::LC_Dialog(QWidget* parent, const QString& dlgName)
    :QDialog(parent)
    ,m_dialogName(dlgName){
}

void LC_Dialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (!m_positionLoaded) {
        for (auto* spinBox : findChildren<QAbstractSpinBox*>()) {
            if (qobject_cast<QSpinBox*>(spinBox) || qobject_cast<QDoubleSpinBox*>(spinBox)) {
                new SpinBoxSelectionFilter(spinBox);
            }
        }
        loadDialogPosition();
        m_positionLoaded = true;
    }
}

QString LC_Dialog::getPositionSettingsGroupName() const{
    return "Dlg"+m_dialogName;
}

void LC_Dialog::loadDialogPosition() {
    LC_GROUP("Appearance");
    const bool persistentDialogPositions = LC_GET_BOOL("PersistDialogPositions", false);
    const bool restoreSizeOnly = LC_GET_BOOL("PersistDialogRestoreSizeOnly", false);
    if (persistentDialogPositions) {
        LC_GROUP_GUARD(getPositionSettingsGroupName());
        {
            const bool hasSettings = LC_GET_BOOL("hasPosition");
            if (hasSettings) {
                const int x = LC_GET_INT("X", 0);
                const int y = LC_GET_INT("Y", 0);
                const int h = LC_GET_INT("Height", 0);
                const int w = LC_GET_INT("Width", 0);
                if (x > 0 && y > 0 && h > 0 && w > 0) {
                    if (!restoreSizeOnly) {
                        move(x, y);
                    }
                    resize(w, h);
                }
            }
        }
    }
}

void LC_Dialog::saveDialogPosition() const {
    const bool persistentDialogPositions = LC_GET_ONE_BOOL("Appearance","PersistDialogPositions", false);
    if (persistentDialogPositions) {
        LC_GROUP_GUARD(getPositionSettingsGroupName());
        {
            LC_SET("hasPosition", true);

            const QPoint &point = pos();
            const QSize &size = QWidget::size();

            const int x = point.x();
            const int y = point.y();
            const int h = size.height();
            const int w = size.width();
            LC_SET("X", x);
            LC_SET("Y", y);
            LC_SET("Height", h);
            LC_SET("Width", w);
        }
    }
}

void LC_Dialog::accept() {
    saveDialogPosition();
    QDialog::accept();
}

void LC_Dialog::reject() {
    saveDialogPosition();
    QDialog::reject();
}
