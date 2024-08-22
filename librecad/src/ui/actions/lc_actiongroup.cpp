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

#include "lc_actiongroup.h"

LC_ActionGroup::LC_ActionGroup(QObject *parent, const QString &name, const QString &description, const char* iconName)
    :QActionGroup(parent) {
    setObjectName(name);
    this->name = name;
    this->description = description;
    if (iconName != nullptr){
        icon = QIcon(iconName);
    }
}

LC_ActionGroup::~LC_ActionGroup() {
}

const QString &LC_ActionGroup::getName() const {
    return name;
}

void LC_ActionGroup::setName(const QString &name) {
    LC_ActionGroup::name = name;
}

const QString &LC_ActionGroup::getDescription() const {
    return description;
}

void LC_ActionGroup::setDescription(const QString &description) {
    LC_ActionGroup::description = description;
}

const QIcon &LC_ActionGroup::getIcon() const {
    return icon;
}

void LC_ActionGroup::setIcon(const QIcon &icon) {
    LC_ActionGroup::icon = icon;
}

bool LC_ActionGroup::isActionMappingsMayBeConfigured() const {
    return actionMappingsMayBeConfigured;
}

void LC_ActionGroup::setActionMappingsMayBeConfigured(bool actionMappingsMayBeConfigured) {
    LC_ActionGroup::actionMappingsMayBeConfigured = actionMappingsMayBeConfigured;
}
