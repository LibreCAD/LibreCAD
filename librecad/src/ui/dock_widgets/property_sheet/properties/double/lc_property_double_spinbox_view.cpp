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

#include "lc_property_double_spinbox_view.h"

#include "lc_property_double.h"
#include "lc_property_double_spinbox.h"
#include "lc_property_editor_handler_value_typed.h"
#include "lc_property_editor_utils.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyDoubleSpinBoxView::VIEW_NAME = LC_PropertyViewUtils::getViewNameSpinBox();
const QByteArray LC_PropertyDoubleSpinBoxView::ATTR_PRECISION = QByteArrayLiteral("precision");
const QByteArray LC_PropertyDoubleSpinBoxView::ATTR_MULTIPLIER = QByteArrayLiteral("multiplier");
const QByteArray LC_PropertyDoubleSpinBoxView::ATTR_MIN = QByteArrayLiteral("min");
const QByteArray LC_PropertyDoubleSpinBoxView::ATTR_MAX = QByteArrayLiteral("max");
const QByteArray LC_PropertyDoubleSpinBoxView::ATTR_STEP = QByteArrayLiteral("step");
const QByteArray LC_PropertyDoubleSpinBoxView::ATTR_SUFFIX = QByteArrayLiteral("suffix");

class LC_PropertyDoubleSpinBoxViewHandler : public LC_PropertyEditorHandlerValueTyped<LC_PropertyDouble, QDoubleSpinBox> {
public:
    LC_PropertyDoubleSpinBoxViewHandler(LC_PropertyDoubleSpinBoxView* view, QDoubleSpinBox& editor)
    : LC_PropertyEditorHandlerValueTyped(view, editor), m_view(view) {
        LC_PropertyDoubleSpinBoxViewHandler::doUpdateEditor();

    editor.setKeyboardTracking(false);
    editor.installEventFilter(this);
    connect(&editor, &QDoubleSpinBox::valueChanged, this, &LC_PropertyDoubleSpinBoxViewHandler::onValueChanged);
}

protected:
    void doUpdateEditor() override{
        m_updating++;

        const auto sb = getEditor();
        sb->setReadOnly(!isEditableByUser());
        sb->setSingleStep(m_view->getStepValue());
        sb->setRange(m_view->getMinValue(), m_view->getMaxValue());

        if (isMultiValue()) {
            sb->setValue(sb->minimum());
            sb->setSpecialValueText(LC_Property::getMultiValuePlaceholder());
        }
        else {
            sb->setValue(m_view->getCurrentValue());
            sb->setSpecialValueText(QString());
        }

        sb->selectAll();
        m_updating--;
    }
    void doUpdatePropertyValue() override{
        if (getBaseProperty() != nullptr) {
            setPropertyValue(m_newValue / m_view->getMultiplier());
        }
    }
private:
    LC_PropertyDoubleSpinBoxView* m_view{nullptr};
};

LC_PropertyDoubleSpinBoxView::LC_PropertyDoubleSpinBoxView(LC_PropertyDouble& property)
    : LC_PropertyViewTyped(property), m_multiplier(1.0), m_precision(std::numeric_limits<double>::digits10 - 1) {
}

double LC_PropertyDoubleSpinBoxView::getStepValue() const {
    return m_step.isValid() ? m_step.toDouble() : typedProperty().stepValue();
}

QWidget* LC_PropertyDoubleSpinBoxView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    QWidget* result = nullptr;
    if (isEditableByUser()) {
        const auto sb = new LC_PropertyDoubleSpinBox(parent);
        sb->setDecimals(m_precision);
        sb->setSuffix(m_suffix);
        sb->setGeometry(rect);

        new LC_PropertyDoubleSpinBoxViewHandler(this, *sb);
        sb->selectAll();
        LC_PropertyEditorUtils::initializeNumberEditor(sb, ctx, LC_PropertyEditorUtils::NUM_TYPE_FLOAT);
        result = sb;
    }
    return result;
}

bool LC_PropertyDoubleSpinBoxView::doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const {
    if (LC_PropertyViewTyped::doAcceptKeyPressedForInplaceEdit(keyEvent)) {
        return true;
    }
    return LC_PropertyEditorUtils::isAcceptableKeyEvenForNumberEdit(keyEvent, LC_PropertyEditorUtils::NUM_TYPE_FLOAT);
}

bool LC_PropertyDoubleSpinBoxView::doPropertyValueToStr(QString& strValue) const {
    strValue = LC_PropertyDoubleSpinBox::valueToText(getCurrentValue(), QLocale(), m_precision, true);
    strValue.append(m_suffix);
    return true;
}

void LC_PropertyDoubleSpinBoxView::doApplyAttributes(const LC_PropertyViewDescriptor& attrs) {
    attrs.load(ATTR_MULTIPLIER, m_multiplier);
    attrs.load(ATTR_SUFFIX, m_suffix);
    attrs.load(ATTR_PRECISION, m_precision);
    m_step = attrs[ATTR_STEP];
    if (m_step.isValid()) {
        bool ok = false;
        const double step = m_step.toDouble(&ok);
        m_step = ok ? step : QVariant();
    }
    m_precision = qBound(0, m_precision, std::numeric_limits<double>::digits10);

    m_min = attrs[ATTR_MIN];
    m_max = attrs[ATTR_MAX];

    if (!qIsFinite(m_multiplier) || qFuzzyCompare(m_multiplier, 0.0)) {
        m_multiplier = 1.0;
    }
    LC_PropertyViewUtils::fixMinMaxVariant<double>(m_min, m_max);
}

double LC_PropertyDoubleSpinBoxView::getMinValue() const {
    return (m_min.isValid() ? m_min.toDouble() : typedProperty().minValue()) * m_multiplier;
}

double LC_PropertyDoubleSpinBoxView::getMaxValue() const {
    return (m_max.isValid() ? m_max.toDouble() : typedProperty().maxValue()) * m_multiplier;
}

double LC_PropertyDoubleSpinBoxView::getMultiplier() const {
    return m_multiplier;
}

double LC_PropertyDoubleSpinBoxView::getCurrentValue() const {
    return qBound(getMinValue(), propertyValue() * m_multiplier, getMaxValue());
}
