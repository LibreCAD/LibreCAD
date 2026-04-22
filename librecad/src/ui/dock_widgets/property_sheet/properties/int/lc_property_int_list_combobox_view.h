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

#ifndef LC_PROPERTYINTLISTCOMBOBOXVIEW_H
#define LC_PROPERTYINTLISTCOMBOBOXVIEW_H

#include "lc_property_int_spinbox_view.h"

class LC_PropertyInt;

class LC_PropertyIntListComboBoxView : public LC_PropertyIntSpinBoxView {
    Q_DISABLE_COPY(LC_PropertyIntListComboBoxView)

public:
    static const QByteArray VIEW_NAME;
    explicit LC_PropertyIntListComboBoxView(LC_PropertyInt& property);

protected:
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
};

#endif
