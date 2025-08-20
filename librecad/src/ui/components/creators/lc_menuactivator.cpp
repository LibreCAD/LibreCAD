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

#include "lc_menuactivator.h"

#include <QMouseEvent>

LC_MenuActivator::LC_MenuActivator()
    :m_keyModifiers{NONE}, m_eventType{CLICK_RELEASE}, m_button{RIGHT}{
    update();
}

LC_MenuActivator::LC_MenuActivator(const QString& shortcutString, bool ctrl, bool alt, bool shift, Button button, Type type, bool entityRequired)
    :m_keyModifiers{NONE}, m_eventType{type}, m_requiresEntity{entityRequired}, m_button{button}, m_shortcutString{shortcutString}{
    setKeys(ctrl, alt, shift);
}

LC_MenuActivator::LC_MenuActivator(LC_MenuActivator& other) {
    m_shortcutString = other.m_shortcutString;
    m_button = other.m_button;
    m_eventType = other.m_eventType;
    m_keyModifiers = other.m_keyModifiers;
    m_requiresEntity = other.m_requiresEntity;
    m_menuName = other.m_menuName;
}

LC_MenuActivator* LC_MenuActivator::getCopy() {
    return new LC_MenuActivator(*this);
}

void LC_MenuActivator::copyTo(LC_MenuActivator& other) {
    other.m_shortcutString = m_shortcutString;
    other.m_button = m_button;
    other.m_eventType = m_eventType;
    other.m_keyModifiers = m_keyModifiers;
    other.m_requiresEntity = m_requiresEntity;
}

bool LC_MenuActivator::isEventApplicable(QMouseEvent* event) {
    auto type = event->type();

    switch (type) {
        case QEvent::MouseButtonRelease: {
            if (m_eventType != CLICK_RELEASE) {
                return false;
            }
            break;
        }
        case QEvent::MouseButtonDblClick: {
            if (m_eventType != DOUBLE_CLICK) {
                return false;
            }
            break;
        }
        default:
            return false;
    }

    auto modifiers = event->modifiers();

    bool eventShift = modifiers & Qt::ShiftModifier;
    bool ownShift = m_keyModifiers & SHIFT;
    if (eventShift != ownShift) {
        return false;
    }

    bool eventCtrl = modifiers & Qt::ControlModifier;
    bool ownCtrl = m_keyModifiers & CTRL;
    if (eventCtrl != ownCtrl) {
        return false;
    }

    bool eventAlt = modifiers & Qt::AltModifier;
    bool ownAlt = m_keyModifiers & ALT;
    if (eventAlt != ownAlt) {
        return false;
    }

    auto button = event->button();
    switch (button) {
        case Qt::LeftButton: {
            if (m_button != LEFT) {
                return false;
            }
            break;
        }
        case Qt::RightButton: {
            if (m_button != RIGHT) {
                return false;
            }
            break;
        }
        case Qt::MiddleButton: {
            if (m_button != MIDDLE) {
                return false;
            }
            break;
        }
        case Qt::BackButton: {
            if (m_button != BACK) {
                return false;
            }
            break;
        }
        case Qt::ForwardButton: {
            if (m_button != FORWARD) {
                return false;
            }
            break;
        }
        case Qt::TaskButton: {
            if (m_button != TASK) {
                return false;
            }
            break;
        }
        default:
            return false;
    }

    return true;
}

bool LC_MenuActivator::isSameAs(LC_MenuActivator* other) {
    if (other != nullptr) {
        return other->getShortcut() == m_shortcutString;
    }
    return false;
}

LC_MenuActivator* LC_MenuActivator::fromShortcut(QString& shortcut){
    auto str = shortcut.trimmed().toUpper();
    int size = str.length();
    if (size != 6) {
        return nullptr;
    }
    bool shift = str[0] == 'S';
    bool ctrl = str[1] == 'C';
    bool alt = str[2] == 'A';

    auto buttonName = str[3];
    Button button;
    if (buttonName == 'L') {
        button = LEFT;
    }
    else if (buttonName == 'R') {
        button = RIGHT;
    }
    else if (buttonName == 'M') {
        button = MIDDLE;
    }
    else if (buttonName == 'B') {
        button = BACK;
    }
    else if (buttonName == 'F') {
        button = FORWARD;
    }
    else if (buttonName == 'B') {
        button = BACK;
    }
    else {
        return nullptr;
    }

    Type type;
    if (str[4] == 'D') {
        type = DOUBLE_CLICK;
    }
    else if (str[4] == 'C') {
        type = CLICK_RELEASE;
    }
    else {
        return nullptr;
    }

    bool requiresEntity = false;
    if (str[5] == 'Y') {
        requiresEntity = true;
    }

    auto* result = new LC_MenuActivator(str, ctrl, alt, shift, button, type, requiresEntity);
    return result;
}

void LC_MenuActivator::update() {
    m_shortcutString = "";
    if (m_keyModifiers & SHIFT) {
        m_shortcutString.append("S");
    }
    else {
        m_shortcutString.append("N");
    }
    if (m_keyModifiers & CTRL) {
        m_shortcutString.append("C");
    }
    else {
        m_shortcutString.append("N");
    }
    if (m_keyModifiers & ALT) {
        m_shortcutString.append("A");
    }
    else {
        m_shortcutString.append("N");
    }

    if (m_button == LEFT) {
        m_shortcutString.append("L");
    }
    if (m_button == MIDDLE) {
        m_shortcutString.append("M");
    }
    if (m_button == RIGHT) {
        m_shortcutString.append("R");
    }
    if (m_button == FORWARD) {
        m_shortcutString.append("F");
    }
    if (m_button == BACK) {
        m_shortcutString.append("B");
    }
    if (m_button == TASK) {
        m_shortcutString.append("T");
    }

    if (m_eventType == CLICK_RELEASE) {
        m_shortcutString.append("C");
    }
    else {
        m_shortcutString.append("D");
    }

    if (m_requiresEntity) {
        m_shortcutString.append("Y");
    }
    else {
        m_shortcutString.append("N");
    }
}

void LC_MenuActivator::setKeys(bool ctrl, bool alt, bool shift) {
    m_keyModifiers = NONE;
    if (ctrl) {
        m_keyModifiers |= CTRL;
    }
    if (alt) {
        m_keyModifiers |= ALT;
    }
    if (shift) {
        m_keyModifiers |= SHIFT;
    }
}

QString LC_MenuActivator::getShortcut() const {
    return m_shortcutString;
}

QString LC_MenuActivator::getShortcutView() {
    QString result = "";
    if (m_keyModifiers & CTRL) {
        result.append("CTRL+");
    }
    if (m_keyModifiers & ALT) {
        result.append("ALT+");
    }
    if (m_keyModifiers & SHIFT) {
        result.append("SHIFT+");
    }

    if (m_button == LEFT) {
        result.append(QObject::tr("Left-"));
    }
    if (m_button == MIDDLE) {
        result.append(QObject::tr("Middle-"));
    }
    if (m_button == RIGHT) {
        result.append(QObject::tr("Right-"));
    }
    if (m_button == FORWARD) {
        result.append(QObject::tr("Forward-"));
    }
    if (m_button == BACK) {
        result.append(QObject::tr("Back-"));
    }
    if (m_button == TASK) {
        result.append(QObject::tr("Task-"));
    }

    if (m_eventType == CLICK_RELEASE) {
        result.append(QObject::tr("Click"));
    }
    else {
        result.append(QObject::tr("Double-Click"));
    }
    if (m_requiresEntity) {
        result.append("+" + QObject::tr("Entity"));
    }
    return result;
}

void LC_MenuActivator::setButtonType(Button type) {
    m_button = type;
}

void LC_MenuActivator::setEventType(Type event) {
    m_eventType = event;
}

LC_MenuActivator::Type LC_MenuActivator::getEventType() const {
    return m_eventType;
}

void LC_MenuActivator::getKeysState(bool& ctrl, bool& alt, bool& shift) {
    ctrl = m_keyModifiers & CTRL;
    alt = m_keyModifiers & ALT;
    shift = m_keyModifiers & SHIFT;
}

bool LC_MenuActivator::hasKeys() {
    return m_keyModifiers != NONE;
}

LC_MenuActivator::Button LC_MenuActivator::getButtonType() const {
    return m_button;
}

void LC_MenuActivator::setMenuName(const QString& menuName) {
    m_menuName = menuName;
}

QString LC_MenuActivator::getMenuName() const {
    return m_menuName;
}

bool LC_MenuActivator::isEntityRequired() const {
    return m_requiresEntity;
}

void LC_MenuActivator::setEntityRequired(bool value) {
    m_requiresEntity = value;
}
