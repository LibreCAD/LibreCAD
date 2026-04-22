/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_ACTIONOPTIONSEDITOR_H
#define LC_ACTIONOPTIONSEDITOR_H

#include <QVariant>

class LC_ActionOptionsEditor {
public:
    virtual ~LC_ActionOptionsEditor() = default;
    virtual void showOptions() = 0;
    /**
     * Called externally when the widget should be hidded
     */
    virtual void hideOptions() = 0;
    virtual void updateOptions(const QString& tagToFocus = "") = 0;
    virtual void updateOptionsUI(int mode, const QVariant *value = nullptr) = 0;
    virtual void setup(bool showOptionsInToolOptionsWidget, bool showOptionsInPropertySheetWidget) = 0;
};
#endif
