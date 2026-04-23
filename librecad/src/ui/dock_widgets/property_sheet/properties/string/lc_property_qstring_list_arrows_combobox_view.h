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

#ifndef LC_PROPERTYQSTRINGARROWVIEW_H
#define LC_PROPERTYQSTRINGARROWVIEW_H

#include "lc_property_qstring_list_combobox_view.h"

class LC_PropertyQStringListArrowsComboboxView : public LC_PropertyQStringListComboBoxView {
public:
    static const QByteArray VIEW_NAME;
    static const QByteArray ATTR_BLOCK_NAMES;
    static const QByteArray ATTR_CURRENT_INDEX;

    explicit LC_PropertyQStringListArrowsComboboxView(LC_PropertyQString* property)
        : LC_PropertyQStringListComboBoxView(property) {
    }

    int getCurrentIndex() const {
        return m_currentIndex;
    }

protected:
    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) override;
    void doDrawValue(LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) override;
    void doApplyAttributes(const LC_PropertyViewDescriptor& attrs) override;

    QString m_currentName;
    QIcon m_icon;
    int m_currentIndex = -1;
};

#endif
