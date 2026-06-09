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

#ifndef LC_PROPERTYLINETYPECOMBOBOXVIEW_H
#define LC_PROPERTYLINETYPECOMBOBOXVIEW_H

#include "lc_property_linetype.h"
#include "lc_property_simple_combobox_view.h"
#include "lc_property_view_typed.h"

class LC_PropertyLineTypeComboboxView : public LC_PropertySimpleComboboxView<LC_PropertyLineType> {
    Q_DISABLE_COPY(LC_PropertyLineTypeComboboxView)
    Q_OBJECT

public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_SHOW_BY_LAYER;

    explicit LC_PropertyLineTypeComboboxView(LC_PropertyLineType* property) : LC_PropertySimpleComboboxView(property) {
    }

protected:
    void doApplyAttributes(const LC_PropertyViewDescriptor& info) override;
    QComboBox* doCreateEditCombobox(QWidget* parent) override;
    void doDrawValueDetails(const QStyle* style, const RS2::LineType& linetype, QPainter& painter, const QRect& rect) const override;
    bool m_showByLayer = true;
};

#endif
