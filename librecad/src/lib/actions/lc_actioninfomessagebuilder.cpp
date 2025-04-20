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

#include "lc_actioninfomessagebuilder.h"

#include "rs_previewactioninterface.h"

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::string(const QString& name, const QString& value) {
    add(name, value);
    return *this;
}
LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::vector(const QString& name, const RS_Vector& value) {
    add(name, m_action->formatVector(value));
    return *this;
}

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::relative(const QString& name, const RS_Vector& value) {
    add(name, m_action->formatRelative(value));
    return *this;
}

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::relativePolar(const QString& name, const RS_Vector& value) {
    add(name, m_action->formatRelativePolar(value));
    return *this;
}

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::relative(const RS_Vector& value) {
    add(m_action->formatRelative(value));
    return *this;
}

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::relativePolar(const RS_Vector& value) {
    add(m_action->formatRelativePolar(value));
    return *this;
}

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::wcsAngle(const QString& name, double value) {
    add(name, m_action->formatWCSAngle(value));
    return *this;
}

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::rawAngle(const QString& name, double value) {
    add(name, m_action->formatAngleRaw(value));
    return *this;
}

LC_ActionInfoMessageBuilder& LC_ActionInfoMessageBuilder::linear(const QString& name, double value) {
    add(name, m_action->formatLinear(value));
    return *this;
}

void LC_ActionInfoMessageBuilder::toInfoCursorZone2(bool replace) {
    QString message = toString();
    m_action->appendInfoCursorZoneMessage(message, 2, replace);
    clear();
}
