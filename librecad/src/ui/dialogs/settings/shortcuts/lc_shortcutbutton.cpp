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

#include <QKeyEvent>
#include <QApplication>
#include "lc_shortcutbutton.h"
#include "lc_shortcutinfo.h"

LC_ShortcutButton::LC_ShortcutButton(QWidget *parent)
    : QPushButton(parent)
    , m_key({{0, 0, 0, 0}}){
    // Using ShortcutButton::tr() as workaround for QTBUG-34128
    setToolTip(LC_ShortcutButton::tr("Click and type the new key sequence."));
    setCheckable(true);
    m_checkedText = LC_ShortcutButton::tr("Stop Recording");
    m_uncheckedText = LC_ShortcutButton::tr("Record Shortcut");
    updateText();
    connect(this, &LC_ShortcutButton::toggled, this, &LC_ShortcutButton::handleToggleChange);
}

QSize LC_ShortcutButton::sizeHint() const {
    if (m_preferredWidth < 0) { // initialize size hint
        const QString originalText = text();
        auto *that = const_cast<LC_ShortcutButton *>(this);
        that->setText(m_checkedText);
        m_preferredWidth = QPushButton::sizeHint().width();
        that->setText(m_uncheckedText);
        int otherWidth = QPushButton::sizeHint().width();
        if (otherWidth > m_preferredWidth)
            m_preferredWidth = otherWidth;
        that->setText(originalText);
    }
    return QSize(m_preferredWidth, QPushButton::sizeHint().height());
}

bool LC_ShortcutButton::eventFilter(QObject *obj, QEvent *evt) {
    if (evt->type() == QEvent::ShortcutOverride) {
        evt->accept();
        return true;
    }
    if (evt->type() == QEvent::KeyRelease
        || evt->type() == QEvent::Shortcut
        || evt->type() == QEvent::Close/*Escape tries to close dialog*/) {
        return true;
    }
    if (evt->type() == QEvent::MouseButtonPress && isChecked()) {
        setChecked(false);
        return true;
    }
    if (evt->type() == QEvent::KeyPress) {
        auto *k = static_cast<QKeyEvent *>(evt);
        int nextKey = k->key();
        if (m_keyNum > 3
            || nextKey == Qt::Key_Control
            || nextKey == Qt::Key_Shift
            || nextKey == Qt::Key_Meta
            || nextKey == Qt::Key_Alt) {
            return false;
        }

        nextKey |= LC_ShortcutInfo::translateModifiers(k->modifiers(), k->text());
        switch (m_keyNum) {
            case 0:
                m_key[0] = nextKey;
                break;
            case 1:
                m_key[1] = nextKey;
                break;
            case 2:
                m_key[2] = nextKey;
                break;
            case 3:
                m_key[3] = nextKey;
                break;
            default:
                break;
        }
        m_keyNum++;
        k->accept();
        emit keySequenceChanged(QKeySequence(m_key[0], m_key[1], m_key[2], m_key[3]));
        if (m_keyNum > 3)
            setChecked(false);
        return true;
    }
    return QPushButton::eventFilter(obj, evt);
}

void LC_ShortcutButton::updateText() {
    setText(isChecked() ? m_checkedText : m_uncheckedText);
}

void LC_ShortcutButton::handleToggleChange(bool toogleState) {
    updateText();
    m_keyNum = m_key[0] = m_key[1] = m_key[2] = m_key[3] = 0;
    if (toogleState) {
        if (QApplication::focusWidget()) {
            QApplication::focusWidget()->clearFocus();
        } // funny things happen otherwise
        qApp->installEventFilter(this);
    } else {
        qApp->removeEventFilter(this);
    }
}
