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

#ifndef LC_MENUACTIVATOR_H
#define LC_MENUACTIVATOR_H
#include <QString>

#include "lc_dlgdimension.h"

class QMouseEvent;

class LC_MenuActivator {
public:
    enum Type: unsigned {
        CLICK_RELEASE = 1 << 0,
        DOUBLE_CLICK  = 1 << 1
    };

    enum Button: unsigned {
        NONE    = 0,
        LEFT    = 1 << 0,
        MIDDLE  = 1 << 1,
        RIGHT   = 1 << 2,
        BACK    = 1 << 3,
        FORWARD = 1 << 4,
        TASK    = 1 << 5
    };

    enum Keys: unsigned {
        CTRL  = 1 << 0,
        ALT   = 1 << 1,
        SHIFT = 1 << 2
    };

    LC_MenuActivator(const QString& shortcutString, bool ctrl, bool alt, bool shift, Button button, Type type,
                     bool entityRequired, RS2::EntityType entityType);
    LC_MenuActivator(LC_MenuActivator& other);
    LC_MenuActivator();
    LC_MenuActivator* getCopy();
    void copyTo(LC_MenuActivator& other);
    void setKeys(bool ctrl, bool alt, bool shift);
    bool isEventApplicable(QMouseEvent* event);
    bool isSameAs(LC_MenuActivator* other);
    static void parseEntityType(QString entityTypeStr, bool& requiresEntity, RS2::EntityType& entityType);
    QString getShortcut() const;
    QString getEventView();
    QString getShortcutView();
    void setButtonType(Button type);
    void setEventType(Type event);
    Type getEventType() const;
    void getKeysState(bool& ctrl, bool& alt, bool& shift);
    Button getButtonType() const;
    bool hasKeys();
    static LC_MenuActivator* fromShortcut(QString& shortcut);
    QString getEntityTypeStr() const;
    void update();
    void setMenuName(const QString& menuName);
    QString getMenuName() const;
    bool isEntityRequired() const;
    void setEntityRequired(bool value);
    RS2::EntityType getEntityType()const;
    void setEntityType(RS2::EntityType entityType);
private:
    unsigned m_keyModifiers {NONE};
    Type m_eventType {CLICK_RELEASE};
    bool m_requiresEntity{false};
    RS2::EntityType m_entityType{RS2::EntityUnknown};
    Button m_button;
    QString m_shortcutString;
    QString m_menuName;
};

class QMouseEvent;

#endif // LC_MENUACTIVATOR_H
