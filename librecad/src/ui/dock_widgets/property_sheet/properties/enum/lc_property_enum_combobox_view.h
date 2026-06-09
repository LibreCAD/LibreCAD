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

#ifndef LC_PROPERTYENUMCOMBOBOXVIEW_H
#define LC_PROPERTYENUMCOMBOBOXVIEW_H

#include "lc_property_enum.h"
#include "lc_property_view_typed.h"

class LC_PropertyComboBox;

class LC_PropertyEnumComboBoxView : public LC_PropertyViewTyped<LC_PropertyEnum> {
    Q_DISABLE_COPY(LC_PropertyEnumComboBoxView)

public:
    static const QByteArray VIEW_NAME;
    explicit LC_PropertyEnumComboBoxView(LC_PropertyEnum* property);
    ~LC_PropertyEnumComboBoxView() override;
protected:
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    bool doPropertyValueToStr(QString& strValue) const override;
    LC_PropertyComboBox* m_editorCombobox = nullptr;
};

#endif
