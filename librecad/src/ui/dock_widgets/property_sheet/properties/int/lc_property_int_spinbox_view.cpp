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

#include "lc_property_int_spinbox_view.h"

#include <QSpinBox>

#include "lc_property_editor_handler_value_typed.h"
#include "lc_property_editor_utils.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyIntSpinBoxView::VIEW_NAME = LC_PropertyViewUtils::getViewNameSpinBox();
const QByteArray LC_PropertyIntSpinBoxView::ATTR_MIN = QByteArrayLiteral("min");
const QByteArray LC_PropertyIntSpinBoxView::ATTR_MAX = QByteArrayLiteral("max");
const QByteArray LC_PropertyIntSpinBoxView::ATTR_STEP = QByteArrayLiteral("step");
const QByteArray LC_PropertyIntSpinBoxView::ATTR_SUFFIX = QByteArrayLiteral("suffix");

class LC_PropertyIntSpinBoxViewHandler : public LC_PropertyEditorHandlerValueTyped<LC_PropertyInt, QSpinBox> {
public:
    LC_PropertyIntSpinBoxViewHandler(LC_PropertyIntSpinBoxView* view, QSpinBox& editor)
    : LC_PropertyEditorHandlerValueTyped(view, editor), m_view(view) {
        LC_PropertyIntSpinBoxViewHandler::doUpdateEditor();

        editor.setKeyboardTracking(false);
        editor.installEventFilter(this);
        connect(&editor, &QSpinBox::valueChanged, this, &LC_PropertyIntSpinBoxViewHandler::onValueChanged);
    }

protected:
    void doUpdateEditor() override {
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

private:
    LC_PropertyIntSpinBoxView* m_view;
};

LC_PropertyIntSpinBoxView::LC_PropertyIntSpinBoxView(LC_PropertyInt& property)
    : LC_PropertyViewTyped(property) {
}

int LC_PropertyIntSpinBoxView::getStepValue() const {
    return m_step.isValid() ? m_step.toInt() : typedProperty().stepValue();
}

int LC_PropertyIntSpinBoxView::getMinValue() const {
    return m_min.isValid() ? m_min.toInt() : typedProperty().minValue();
}

int LC_PropertyIntSpinBoxView::getMaxValue() const {
    return m_max.isValid() ? m_max.toInt() : typedProperty().maxValue();
}

int LC_PropertyIntSpinBoxView::getCurrentValue() const {
    return qBound(getMinValue(), propertyValue(), getMaxValue());
}

QWidget* LC_PropertyIntSpinBoxView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    const auto sb = new QSpinBox(parent);
    sb->setSuffix(m_suffix);
    sb->setGeometry(rect);
    new LC_PropertyIntSpinBoxViewHandler(this, *sb);
    sb->selectAll();
    if (isEditableByUser()) {
        LC_PropertyEditorUtils::initializeNumberEditor(sb, ctx, LC_PropertyEditorUtils::NUM_TYPE_SIGNED_INT);
    }
    return sb;
}

bool LC_PropertyIntSpinBoxView::doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const {
    if (LC_PropertyViewTyped::doAcceptKeyPressedForInplaceEdit(keyEvent)) {
        return true;
    }
    return LC_PropertyEditorUtils::isAcceptableKeyEvenForNumberEdit(keyEvent, LC_PropertyEditorUtils::NUM_TYPE_SIGNED_INT);
}

void LC_PropertyIntSpinBoxView::doApplyAttributes(const LC_PropertyViewDescriptor& attrs) {
    attrs.load(ATTR_SUFFIX, m_suffix);
    m_min = attrs[ATTR_MIN];
    m_max = attrs[ATTR_MAX];
    m_step = attrs[ATTR_STEP];
    if (m_step.isValid()) {
        bool ok = false;
        const int step = m_step.toInt(&ok);
        if (!ok) {
            m_step = QVariant();
        }
        else {
            m_step = step;
        }
    }
    LC_PropertyViewUtils::fixMinMaxVariant<int>(m_min, m_max);
}

bool LC_PropertyIntSpinBoxView::doPropertyValueToStr(QString& strValue) const {
    strValue = QLocale().toString(getCurrentValue());
    strValue.append(m_suffix);
    return true;
}
