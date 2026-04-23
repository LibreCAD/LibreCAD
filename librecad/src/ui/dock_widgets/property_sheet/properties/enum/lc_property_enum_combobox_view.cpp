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

#include "lc_property_enum_combobox_view.h"

#include "lc_property_combobox.h"
#include "lc_property_editor_handler_value_typed.h"
#include "lc_property_enum.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyEnumComboBoxView::VIEW_NAME = LC_PropertyViewUtils::getViewNameComboBox();

class LC_PropertyEnumComboBoxViewHandler : public LC_PropertyEditorHandlerValueTyped<LC_PropertyEnum, LC_PropertyComboBox> {
public:
    LC_PropertyEnumComboBoxViewHandler(LC_PropertyViewEditable* view, LC_PropertyComboBox& editor)
        : LC_PropertyEditorHandlerValueTyped(view, editor) {
        LC_PropertyEnumComboBoxViewHandler::doUpdateEditor();
        connect(&editor, &QComboBox::currentIndexChanged, this, &LC_PropertyEnumComboBoxViewHandler::onCurrentIndexChanged);
    }

protected:
    void doUpdateEditor() override {
        m_updating++;
        const auto cb = getEditor();
        cb->setEnabled(isEditableByUser());

        if (isMultiValue()) {
            cb->setCurrentIndex(-1);
        }
        else {
            const int value = getPropertyValue();
            const int index = cb->findData(value);
            cb->setCurrentIndex(index);
        }
        m_updating--;
    }

private:
    void onCurrentIndexChanged(int index) {
        if (index >= 0) {
            const auto propertyComboBox = getEditor();
            const QVariant data = propertyComboBox->itemData(index);
            if (data.canConvert<int>()) {
                propertyComboBox->disablePaint(true);
                onValueChanged(data.toInt());
            }
        }
    }
};

LC_PropertyEnumComboBoxView::LC_PropertyEnumComboBoxView(LC_PropertyEnum* property)
    : LC_PropertyViewTyped(property) {
}

bool LC_PropertyEnumComboBoxView::doPropertyValueToStr(QString& strValue) const {
    const LC_EnumDescriptor* descriptor = typedProperty().getEnumDescriptor();
    const LC_EnumValueDescriptor* valueDescriptor = descriptor != nullptr ? descriptor->findByValue(propertyValue()) : nullptr;
    if (valueDescriptor == nullptr) {
        return false;
    }
    strValue = valueDescriptor->getDisplayName();
    return true;
}

QWidget* LC_PropertyEnumComboBoxView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    const LC_EnumDescriptor* descriptor = typedProperty().getEnumDescriptor();
    if (descriptor == nullptr) {
        return nullptr;
    }
    auto cb = new LC_PropertyComboBox(this, parent);
    descriptor->doProcessEachEnumValue([cb](const LC_EnumValueDescriptor& value) -> bool {
        cb->addItem(value.getDisplayName(), QVariant(value.getValue()));
        return true;
    });
    cb->setGeometry(rect);
    new LC_PropertyEnumComboBoxViewHandler(this, *cb);
    if (isInplaceEditAllowed(ctx)) {
        cb->showPopup();
    }
    return cb;
}
