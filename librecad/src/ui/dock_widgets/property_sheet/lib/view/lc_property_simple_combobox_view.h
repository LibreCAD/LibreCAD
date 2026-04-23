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

#ifndef LC_PROPERTYSIMPLECOMBOBOXVIEW_H
#define LC_PROPERTYSIMPLECOMBOBOXVIEW_H

#include <QComboBox>

#include "lc_property_view_typed.h"
#include "lc_property_view_utils.h"

template <typename PropertyClass>
class LC_PropertySimpleComboboxView : public LC_PropertyViewTyped<PropertyClass> {
    Q_DISABLE_COPY(LC_PropertySimpleComboboxView)

public:
    using ValueType = typename PropertyClass::ValueType;

    explicit LC_PropertySimpleComboboxView(PropertyClass* property) : LC_PropertyViewTyped<PropertyClass>(property) {
    }

protected:
    virtual void doDrawValueDetails(const QStyle* style, const ValueType& value, QPainter& painter, const QRect& rect) const = 0;

    QWidget* doCreateValueEditor(QWidget* parent, const QRect& rect, const LC_PropertyViewEditable::EditActivationContext* ctx) override {
        if (this->isEditableByUser()) {
            const auto combo = doCreateEditCombobox(parent);
            combo->setGeometry(rect);
            if (this->isInplaceEditAllowed(ctx)) {
                combo->showPopup();
            }
            return combo;
        }
        return nullptr;
    }

    virtual QComboBox* doCreateEditCombobox(QWidget* parent) = 0;

    void doDrawValue([[maybe_unused]] LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) override {
        if (this->isMultiValue()) {
            LC_PropertyViewUtils::drawElidedText(painter, LC_Property::getMultiValuePlaceholder(), rect, painter.style());
            return;
        }

        auto value = this->typedProperty().value();
        doDrawValueDetails(painter.style(), value, painter, rect);
    }
};

#endif
