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

#include "lc_shortcutinfo.h"

const char* LC_ShortcutInfo::PROPERTY_ACTION_SHORTCUT_CONFIGURABLE = "actionShortcut.configurable";

QString LC_ShortcutInfo::retrieveKey(bool useDefault) const {
    return useDefault ? m_defaultKey.toString() : m_key.toString();
};

QString LC_ShortcutInfo::getKeyAsString() {
    QString text = m_key.toString(QKeySequence::PortableText);
    return text;
}

QList<QKeySequence> LC_ShortcutInfo::getKeysList() const { return QList<QKeySequence>(); };

void LC_ShortcutInfo::resetToDefault() {
    m_key = m_defaultKey;
    m_modified = false;
}

void LC_ShortcutInfo::setKey(QKeySequence newKey) {
    m_key = newKey;
    m_modified = m_key != m_defaultKey;
}

void LC_ShortcutInfo::clear() {
    setKey(QKeySequence());
}

int LC_ShortcutInfo::translateModifiers(Qt::KeyboardModifiers state,
                                        const QString& text) {
    int result = 0;
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key anyway
    if ((state & Qt::ShiftModifier) && (text.size() == 0
        || !text.at(0).isPrint()
        || text.at(0).isLetterOrNumber()
        || text.at(0).isSpace()))
        result |= Qt::SHIFT;
    if (state & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (state & Qt::MetaModifier)
        result |= Qt::META;
    if (state & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}

bool LC_ShortcutInfo::hasTheSameKey(QKeySequence sequenceToTest) {
    bool result = false;
    if (m_key.isEmpty()) {
        if (!m_defaultKey.isEmpty()) {
            result = m_defaultKey == sequenceToTest;
        }
    }
    else {
        result = m_key == sequenceToTest;
    }
    return result;
}
