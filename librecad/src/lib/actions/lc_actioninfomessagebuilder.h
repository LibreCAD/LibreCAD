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

#ifndef LC_ACTIONINFOMESSAGEBUILDER_H
#define LC_ACTIONINFOMESSAGEBUILDER_H
#include <QString>

#include "lc_cursoroverlayinfo.h"

class RS_PreviewActionInterface;

class LC_ActionInfoMessageBuilder: public LC_InfoMessageBuilder {
public:
    explicit LC_ActionInfoMessageBuilder(RS_PreviewActionInterface * snapper)
        : m_action{snapper} {
    }

    LC_ActionInfoMessageBuilder& string(const QString& name, const QString& value = nullptr);
    LC_ActionInfoMessageBuilder& vector(const QString& name, const RS_Vector& value);
    LC_ActionInfoMessageBuilder& relative(const QString& name, const RS_Vector& value);
    LC_ActionInfoMessageBuilder& polar(const QString& name, const RS_Vector& value);
    LC_ActionInfoMessageBuilder& relativePolar(const QString& name, const RS_Vector& value);
    LC_ActionInfoMessageBuilder& relative(const RS_Vector& value);
    LC_ActionInfoMessageBuilder& relativePolar(const RS_Vector& value);
    LC_ActionInfoMessageBuilder& wcsAngle(const QString& name, double value);
    LC_ActionInfoMessageBuilder& rawAngle(const QString& name, double value);
    LC_ActionInfoMessageBuilder& linear(const QString& name, double value);
    void toInfoCursorZone2(bool replace);
private:
    RS_PreviewActionInterface *m_action;
};

#endif // LC_ACTIONINFOMESSAGEBUILDER_H
