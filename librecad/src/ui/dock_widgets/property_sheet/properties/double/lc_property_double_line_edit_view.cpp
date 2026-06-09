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

#include "lc_property_double_line_edit_view.h"

#include "lc_convert.h"
#include "lc_entity_type_propertiesprovider.h"
#include "lc_linemath.h"
#include "lc_property_combobox.h"
#include "lc_property_editor_handler_value_typed.h"
#include "lc_property_editor_utils.h"

const QByteArray LC_PropertyDoubleLineEditView::VIEW_NAME = QByteArrayLiteral("double_lineedit");
const QByteArray LC_PropertyDoubleLineEditView::ATTR_POSITIVIE_VALUES_ONLY = QByteArrayLiteral("positiveOnly");
const QByteArray LC_PropertyDoubleLineEditView::ATTR_MAX_LENGTH = QByteArrayLiteral("maxLen");
 const QByteArray LC_PropertyDoubleLineEditView::ATTR_ZERO_PLACEHOLDER = QByteArrayLiteral("zeroPlaceholder");

class LC_PropertyDoubleLineEditViewHandler : public LC_PropertyEditorHandler<LC_PropertyDouble, QLineEdit> {
public:
    LC_PropertyDoubleLineEditViewHandler(LC_PropertyViewEditable* view, QLineEdit& editor)
        : LC_PropertyEditorHandler(view, editor) {
        LC_PropertyDoubleLineEditViewHandler::doUpdateEditor();

        editor.installEventFilter(this);
        connect(&editor, &QLineEdit::editingFinished, this, &LC_PropertyDoubleLineEditViewHandler::onEditingFinished);
    }

    void doUpdateEditor() override {
        const auto le = getEditor();
        le->setReadOnly(!isEditableByUser());
        if (isMultiValue()) {
            le->clear();
            le->setPlaceholderText(LC_Property::getMultiValuePlaceholder());
        }
        else {
            const auto* typedView = static_cast<LC_PropertyDoubleLineEditView*>(view());
            QString text;
            typedView->doPropertyValueToStrForEdit(text);
            le->setText(text);
            le->selectAll();
        }
    }
protected:
    bool fromString(const QString& text, double& val) const {
        auto* typedView = static_cast<LC_PropertyDoubleLineEditView*>(view());
        const bool result = typedView->getPropertyValueFromEditString(text, val);
        return result;
    }

    void onEditingFinished() {
        if (doCheckMayApply()) {
            const auto text = getEditor()->text();
            double newValue;
            const bool ok = fromString(text, newValue);
            if (ok) {
                getProperty().setValue(newValue, changeReasonDueToEdit());
            }
        }
        doApplyReset();
    }
};

void LC_PropertyDoubleLineEditView::doApplyAttributes(const LC_PropertyViewDescriptor& info) {
    info.load(ATTR_POSITIVIE_VALUES_ONLY, m_positiveOnly);
    info.load(ATTR_ZERO_PLACEHOLDER, m_zeroPlaceholder);
    info.load(ATTR_MAX_LENGTH, m_maxLength);
}

bool LC_PropertyDoubleLineEditView::doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const {
    if (LC_PropertyViewTyped::doAcceptKeyPressedForInplaceEdit(keyEvent)) {
        return true;
    }
    // accept any printable key
    return LC_PropertyEditorUtils::isAcceptableKeyEventForLineEdit(keyEvent);
}

QWidget* LC_PropertyDoubleLineEditView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    if (isEditableByUser()) {
        const auto le = new QLineEdit(parent);
        le->setGeometry(rect);
        le->setMaxLength(m_maxLength);
        new LC_PropertyDoubleLineEditViewHandler(this, *le);
        LC_PropertyEditorUtils::initializeLineEditor(le, ctx);
        return le;
    }
    return nullptr;
}

bool LC_PropertyDoubleLineEditView::getPropertyValueFromEditString(const QString& text, double& val) {
    QString txt = text;
    if (text == m_zeroPlaceholder) {
        txt = "0.0";
    }
    const bool result = LC_Convert::toDouble(txt, val, 0.0, m_positiveOnly);
    return result;
}

bool LC_PropertyDoubleLineEditView::doPropertyValueToStrForView(QString& strValue) {
    if (m_cachedStrValue.isEmpty()) {
        const double doubleValue = propertyValue();
        if (!LC_LineMath::isMeaningful(doubleValue)) {
            strValue = m_zeroPlaceholder;
            return true;
        }
        const auto formatter = typedProperty().getFormatter();
        const QString value = formatter->formatDouble(doubleValue);
        m_cachedStrValue = value;
        strValue = m_cachedStrValue;
    }
    else {
        strValue = m_cachedStrValue;
    }
    return true;
}

bool LC_PropertyDoubleLineEditView::doPropertyValueToStrForEdit(QString& strValue) const {
    const double doubleValue = propertyValue();
    if (!LC_LineMath::isMeaningful(doubleValue)) {
        strValue = m_zeroPlaceholder;
        return true;
    }
    const auto formatter = typedProperty().getFormatter();
    strValue = formatter->formatDouble(doubleValue);
    return true;
}
