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

#ifndef LC_PROPERTYLABELWITHBUTTON_H
#define LC_PROPERTYLABELWITHBUTTON_H

#include <QWidget>

class LC_Property;

class QToolButton;
class QLabel;

class LC_PropertyLabelWithButton : public QWidget {
public:
    explicit LC_PropertyLabelWithButton(QWidget* parent, const QString& bttnText = "...", QLabel* label = nullptr);
    void setTextForProperty(const LC_Property* property, const QString& text) const;

    QLabel* getLabel() const {
        return m_label;
    }

    QToolButton* getToolButton() const {
        return m_toolButton;
    }
protected:
    QLabel* m_label;
    QToolButton* m_toolButton;
};
#endif
