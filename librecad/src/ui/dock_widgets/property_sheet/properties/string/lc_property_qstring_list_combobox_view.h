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

#ifndef LC_PROPERTYQSTRINGLISTCOMBOBOXVIEW_H
#define LC_PROPERTYQSTRINGLISTCOMBOBOXVIEW_H

#include "lc_property_qstring_lineedit_view.h"

class LC_PropertyQStringListComboBoxView : public LC_PropertyQStringLineEditView {
    Q_DISABLE_COPY(LC_PropertyQStringListComboBoxView)

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_ITEMS;
    static const QByteArray ATTR_ITEMS_ICON_NAMES;
    static const QByteArray ATTR_ITEMS_DATA;
    static const QByteArray ATTR_EDITABLE;

    explicit LC_PropertyQStringListComboBoxView(LC_PropertyQString* property);

protected:
    void doApplyAttributes(const LC_PropertyViewDescriptor& attrs) override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    LC_PropertyViewDescriptor m_viewDescriptor;
};

#endif
