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

#ifndef LC_PROPERTYRSCOLORCOMBOBOXVIEW_H
#define LC_PROPERTYRSCOLORCOMBOBOXVIEW_H


#include "lc_property_rscolor.h"
#include "lc_property_view_typed.h"

class LC_PropertyRSColorComboBoxView : public LC_PropertyViewTyped<LC_PropertyRSColor> {
    Q_DISABLE_COPY(LC_PropertyRSColorComboBoxView)
    Q_OBJECT
public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_SHOW_BY_LAYER;

    explicit LC_PropertyRSColorComboBoxView(LC_PropertyRSColor* property) : LC_PropertyViewTyped(property) {
    }

protected:
    void doApplyAttributes(const LC_PropertyViewDescriptor& info) override;

    void doDrawValue(LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) override;
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    bool m_showByLayerItem = true;
};

#endif
