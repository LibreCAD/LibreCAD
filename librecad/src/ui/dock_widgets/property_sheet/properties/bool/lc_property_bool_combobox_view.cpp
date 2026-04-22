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

#include "lc_property_bool_combobox_view.h"

#include <QCoreApplication>

#include "lc_property_combobox.h"
#include "lc_property_editor_handler_value_typed.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyBoolComboBoxView::VIEW_NAME = LC_PropertyViewUtils::getViewNameComboBox();
const QByteArray LC_PropertyBoolComboBoxView::ATTR_LABEL_TRUE = QByteArrayLiteral("labelTrue");
const QByteArray LC_PropertyBoolComboBoxView::ATTR_LABEL_FALSE = QByteArrayLiteral("labelFalse");

namespace {
    QString getBoolText(const bool value) {
        static constexpr char falseVal[] = QT_TRANSLATE_NOOP("LC_PropertyBool", "False");
        static constexpr char trueVal[] = QT_TRANSLATE_NOOP("LC_PropertyBool", "True");
        return QCoreApplication::translate("LC_PropertyBool", value ? trueVal : falseVal);
    }
}

class LC_PropertyBoolComboBoxViewHandler : public LC_PropertyEditorHandlerValueTyped<LC_PropertyBool, LC_PropertyComboBox> {
public:
    LC_PropertyBoolComboBoxViewHandler(LC_PropertyViewEditable* view, LC_PropertyComboBox& editor)
    : LC_PropertyEditorHandlerValueTyped(view, editor) {
        LC_PropertyBoolComboBoxViewHandler::doUpdateEditor();
        connect(&editor, &QComboBox::currentIndexChanged, this, &LC_PropertyBoolComboBoxViewHandler::onCurrentIndexChanged);
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
            cb->setCurrentIndex(getPropertyValue() ? 1 : 0);
        }
        m_updating--;
    }

private:
    void onCurrentIndexChanged(int index) {
        if (index >= 0) {
            const auto propertyComboBox = getEditor();
            const auto data = propertyComboBox->itemData(index);
            if (data.canConvert<bool>()) {
                propertyComboBox->disablePaint(true);
                onValueChanged(data.toBool());
            }
        }
    }
};

LC_PropertyBoolComboBoxView::LC_PropertyBoolComboBoxView(LC_PropertyBool& property)
    : LC_PropertyViewTyped(property) {
    m_labels[0] = getBoolText(false);
    m_labels[1] = getBoolText(true);
}

QWidget* LC_PropertyBoolComboBoxView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    const auto cb = new LC_PropertyComboBox(this, parent);
    cb->addItem(m_labels[0], false);
    cb->addItem(m_labels[1], true);
    cb->setGeometry(rect);

    // connect widget and property
    new LC_PropertyBoolComboBoxViewHandler(this, *cb);
    if (isInplaceEditAllowed(ctx)) {
        cb->showPopup();
    }
    return cb;
}

void LC_PropertyBoolComboBoxView::doApplyAttributes(const LC_PropertyViewDescriptor& attrs) {
    attrs.load(ATTR_LABEL_FALSE, m_labels[0]);
    attrs.load(ATTR_LABEL_TRUE, m_labels[1]);
}

bool LC_PropertyBoolComboBoxView::doPropertyValueToStr(QString& strValue) const {
    strValue = m_labels[propertyValue() ? 1 : 0];
    return true;
}
