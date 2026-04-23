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

#include "lc_property_int_list_combobox_view.h"

#include "lc_property_combobox.h"
#include "lc_property_editor_handler_value_typed.h"

const QByteArray LC_PropertyIntListComboBoxView::VIEW_NAME = QByteArrayLiteral("IntList");

class LC_PropertyIntListComboBoxViewHandler : public LC_PropertyEditorHandlerValueTyped<LC_PropertyInt, LC_PropertyComboBox> {
public:
    LC_PropertyIntListComboBoxViewHandler(LC_PropertyIntListComboBoxView* view, LC_PropertyComboBox& editor)
    : LC_PropertyEditorHandlerValueTyped(view, editor) {
        LC_PropertyIntListComboBoxViewHandler::doUpdateEditor();
        connect(&editor, &QComboBox::currentIndexChanged, this, &LC_PropertyIntListComboBoxViewHandler::onCurrentIndexChanged);
    }

private:
    void doUpdateEditor() override{
        m_updating++;
        const auto cb = getEditor();
        cb->setEnabled(isEditableByUser());
        if (isMultiValue()) {
            cb->setCurrentIndex(-1);
        }
        else {
            const int index = cb->findData(getProperty().value()); // fixme - review!
            cb->setCurrentIndex(index);
        }
        m_updating--;
    }
    void onCurrentIndexChanged(int index){
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

LC_PropertyIntListComboBoxView::LC_PropertyIntListComboBoxView(LC_PropertyInt* property)
    : LC_PropertyIntSpinBoxView(property) {
}

QWidget* LC_PropertyIntListComboBoxView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    const auto cb = new LC_PropertyComboBox(this, parent);
    const auto desc = typedProperty().getViewDescriptor();
    if (desc != nullptr) {
        QList<int> values;
        desc->load("values", values);
        for (const auto value : std::as_const(values)) {
            cb->addItem(QString::number(value), value);
        }
    }
    cb->setGeometry(rect);
    new LC_PropertyIntListComboBoxViewHandler(this, *cb);

    if (isInplaceEditAllowed(ctx)) {
        cb->showPopup();
    }

    return cb;
}
