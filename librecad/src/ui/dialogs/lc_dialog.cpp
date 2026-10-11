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
        }
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::FocusIn) {
            m_selectOnRelease = static_cast<QFocusEvent*>(event)->reason() == Qt::MouseFocusReason;
        } else if (event->type() == QEvent::FocusOut) {
            m_selectOnRelease = false;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            const bool select = m_selectOnRelease;
            m_selectOnRelease = false;
            auto* editor = qobject_cast<QLineEdit*>(watched);
            auto* mouse = static_cast<QMouseEvent*>(event);
            auto* spinBox = static_cast<QAbstractSpinBox*>(parent());
            // Keep deliberate drag selections and subsequent caret-positioning clicks.
            if (select && editor && !editor->hasSelectedText() && !spinBox->isReadOnly()
                && mouse->button() == Qt::LeftButton && mouse->modifiers() == Qt::NoModifier) {
                spinBox->selectAll();
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    bool m_selectOnRelease = false;
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
