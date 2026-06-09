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

#ifndef LC_PROPERTYEDITORHANDLERVALUETYPED_H
#define LC_PROPERTYEDITORHANDLERVALUETYPED_H

#include "lc_property_editor_handler.h"

template <typename PropertyClass, typename PropertyEditorClass>
class LC_PropertyEditorHandlerValueTyped : public LC_PropertyEditorHandler<PropertyClass, PropertyEditorClass> {
protected:
    using ValueType = typename PropertyClass::ValueType;
    using ValueTypeStore = typename PropertyClass::ValueTypeStore;

    LC_PropertyEditorHandlerValueTyped(LC_PropertyViewEditable* view, PropertyEditorClass& editor)
        : LC_PropertyEditorHandler<PropertyClass, PropertyEditorClass>(view, editor) {
        m_newValue = this->getPropertyValue();
    }

    void onValueChanged(ValueType value) {
        if (m_updating > 0) {
            return;
        }
        m_newValue = value;
        doUpdatePropertyValue();
    }

    virtual void doUpdatePropertyValue() {
        if (this->getBaseProperty() != nullptr) {
            this->getProperty().setValue(m_newValue, this->changeReasonDueToEdit());
        }
    }

    ValueTypeStore m_newValue;
    unsigned m_updating{0};
};

#endif
