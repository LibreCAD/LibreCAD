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

#include "lc_property_double_interactivepick_view.h"

#include "lc_convert.h"
#include "lc_property_editor_button_handler.h"
#include "lc_property_editor_utils.h"
#include "lc_property_lineedit_with_button.h"

const QByteArray LC_PropertyDoubleInteractivePickView::VIEW_NAME = QByteArrayLiteral("double_pickable");
const QByteArray LC_PropertyDoubleInteractivePickView::ATTR_POSITIVIE_VALUES_ONLY = QByteArrayLiteral("positiveOnly");
const QByteArray LC_PropertyDoubleInteractivePickView::ATTR_NON_MEANINGFUL_DISTANCE = QByteArrayLiteral("nonMeaningfulDistance");
const QByteArray LC_PropertyDoubleInteractivePickView::ATTR_FORMAT_AS_INT = QByteArrayLiteral("formatAsInt");

class
    LC_PropertyDoubleInteractivePickViewHandler : public LC_PropertyEditorButtonHandler<LC_PropertyDouble, LC_PropertyLineEditWithButton> {
public:
    LC_PropertyDoubleInteractivePickViewHandler(LC_PropertyViewEditable* view,
                                                LC_PropertyLineEditWithButton& editor) : LC_PropertyEditorButtonHandler(view, editor, editor.getToolButton()) {
        const auto lineEdit = editor.getLineEdit();
        const auto toolButton = editor.getToolButton();
        lineEdit->installEventFilter(this);
        toolButton->installEventFilter(this);
        // editor.installEventFilter(this);
        // connect(toolButton, &QToolButton::clicked, this, &LC_PropertyDoubleInteractivePickViewHandler::onToolButtonClicked);
        connect(lineEdit, &QLineEdit::editingFinished, this, &LC_PropertyDoubleInteractivePickViewHandler::onEditingFinished);
        QSignalBlocker blocker(lineEdit);
        LC_PropertyDoubleInteractivePickViewHandler::doUpdateEditor();
    }

protected:
    void doUpdateEditor() override {
        const auto le = getEditor()->getLineEdit();
        le->setReadOnly(!isEditableByUser());
        const auto* typedView = static_cast<LC_PropertyDoubleInteractivePickView*>(view());
        QString text;
        typedView->getPropertyEditStringValue(text);
        le->setText(text);
        le->selectAll();
    }

    void doRollbackValue() override {
        m_reverted = true;
    }

    void doOnToolButtonClick() override {
        onToolButtonClicked(false);
    }

    bool fromString(const QString& text, double& val) const {
        auto* typedView = static_cast<LC_PropertyDoubleInteractivePickView*>(view());
        const bool result = typedView->getPropertyValueFromEditString(text, val);
        return result;
    }

    void onEditingFinished() {
        if (doCheckMayApply()) {
            const auto lineEdit = getEditor()->getLineEdit();
            const auto text = lineEdit->text();
            // LC_ERR << "ON EditingFinished " << text;
            double value;
            const bool ok = fromString(text, value);
            if (ok) {
                // LC_ERR << "ON EditingFinished " << "IN OK";
                getProperty().setValue(value, changeReasonDueToEdit());
                // due to some weird reasons, when dectructor of LC_PropertyLineWithButton is executed, onEditingFinishied is
                // called again... so we disconnect as we got the first value (during the normal edit) as the tree will be rebuild.
                disconnect(lineEdit, &QLineEdit::editingFinished, this, &LC_PropertyDoubleInteractivePickViewHandler::onEditingFinished);
                // doUpdateEditor();
            }
            doApplyReset();
        }
    }

    void onToolButtonClicked(bool) {
        LC_ERR << "Pick button clicked!";
        stopInplaceEdit();
        const auto propertyDouble = getBaseProperty();
        if (propertyDouble != nullptr) {
            const auto propDouble = static_cast<LC_PropertyDouble*>(propertyDouble);
            propDouble->requestInteractiveInput();
        }
    }
};

LC_PropertyDoubleInteractivePickView::LC_PropertyDoubleInteractivePickView(LC_PropertyDouble& property)
    : LC_PropertyViewTyped(property) {
}

bool LC_PropertyDoubleInteractivePickView::getPropertyValueFromEditString(const QString& text, double& val) {
    const auto interactiveInputType = typedProperty().getInteractiveInputType();
    bool result = false;
    switch (interactiveInputType) {
        case LC_ActionContext::InteractiveInputInfo::POINT_X: {
            result = LC_Convert::toDouble(text, val, 0.0, false);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::POINT_Y: {
            result = LC_Convert::toDouble(text, val, 0.0, false);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::POINT: {
            result = false;
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::ANGLE: {
            result = LC_Convert::toDoubleAngleRad(text, val, 0.0, m_positiveOnly);
            break;
        }
        case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
            result = LC_Convert::toDouble(text, val, m_notMeaningfulDistance, m_positiveOnly);
            break;
        }
        default:
            result = LC_Convert::toDouble(text, val, 0.0, false);
            break;
    }
    return result;
}

bool LC_PropertyDoubleInteractivePickView::doAcceptKeyPressedForInplaceEdit(QKeyEvent* keyEvent) const {
    if (LC_PropertyViewTyped::doAcceptKeyPressedForInplaceEdit(keyEvent)) {
        return true;
    }
    // accept any printable key
    return LC_PropertyEditorUtils::isAcceptableKeyEventForLineEdit(keyEvent);
}

QString LC_PropertyDoubleInteractivePickView::getButtonIconName() {
    const auto interactiveInputType = typedProperty().getInteractiveInputType();
    switch (interactiveInputType) {
        case LC_ActionContext::InteractiveInputInfo::POINT_X:
            return ":/icons/interactive_pick_point_x.lci";
        case LC_ActionContext::InteractiveInputInfo::POINT_Y:
            return ":/icons/interactive_pick_point_y.lci";
        case LC_ActionContext::InteractiveInputInfo::POINT:
            return ":/icons/interactive_pick_point.lci";
        case LC_ActionContext::InteractiveInputInfo::ANGLE:
            return ":/icons/interactive_pick_angle.lci";
        case LC_ActionContext::InteractiveInputInfo::DISTANCE:
            return ":/icons/interactive_pick_distance.lci";
        default:
            break;
    }
    return "";
}

QString LC_PropertyDoubleInteractivePickView::getButtonTooltip(
    const LC_ActionContext::InteractiveInputInfo::InputType interactiveInputType) {
    switch (interactiveInputType) {
        case LC_ActionContext::InteractiveInputInfo::POINT_X:
            return tr("Pick X component of position from drawing");
        case LC_ActionContext::InteractiveInputInfo::POINT_Y:
            return tr("Pick Y component of position from drawing");
        case LC_ActionContext::InteractiveInputInfo::POINT:
            return tr("Pick coordinates of point from drawing");
        case LC_ActionContext::InteractiveInputInfo::ANGLE:
            return tr("Pick angle value from drawing");
        case LC_ActionContext::InteractiveInputInfo::DISTANCE:
            return tr("Pick value as distance from drawing");
        default:
            break;
    }
    return "";
}

QWidget* LC_PropertyDoubleInteractivePickView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    if (isEditableByUser()) {
        const auto le = new LC_PropertyLineEditWithButton(parent);
        le->setGeometry(rect);
        new LC_PropertyDoubleInteractivePickViewHandler(this, *le);
        LC_PropertyEditorUtils::initializeLineEditor(le->getLineEdit(), ctx);
        const QString buttonIconName = getButtonIconName();
        const auto toolButton = le->getToolButton();
        if (buttonIconName.isEmpty()) {
            toolButton->setVisible(false);
        }
        else {
            toolButton->setText("");
            const auto interactiveInputType = typedProperty().getInteractiveInputType();
            const QString tooltip = getButtonTooltip(interactiveInputType);
            toolButton->setToolTip(tooltip);
            toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
            toolButton->setIcon(QIcon(buttonIconName));
        }
        return le;
    }
    return nullptr;
}

void LC_PropertyDoubleInteractivePickView::doApplyAttributes(const LC_PropertyViewDescriptor& info) {
    info.load(ATTR_POSITIVIE_VALUES_ONLY, m_positiveOnly);
    info.load(ATTR_NON_MEANINGFUL_DISTANCE, m_notMeaningfulDistance);
    info.load(ATTR_FORMAT_AS_INT, m_formatAsInt);
}

bool LC_PropertyDoubleInteractivePickView::doPropertyValueToStrForView(QString& strValue) {
    if (m_cachedStrValue.isEmpty()) {
        QString value;
        const double doubleValue = propertyValue();
        const auto formatter = typedProperty().getFormatter();
        const auto interactiveInputType = typedProperty().getInteractiveInputType();
        switch (interactiveInputType) {
            case LC_ActionContext::InteractiveInputInfo::POINT_X:
            case LC_ActionContext::InteractiveInputInfo::POINT_Y:
            case LC_ActionContext::InteractiveInputInfo::POINT:
                if (m_formatAsInt) { // fixme - might it be that for other modes int formatting is needed?
                    strValue = formatter->formatInt(doubleValue);
                }
                else {
                    value = formatter->formatDouble(doubleValue);
                }
                break;
            case LC_ActionContext::InteractiveInputInfo::ANGLE:
                value = formatter->formatRawAngle(doubleValue);
                break;
            case LC_ActionContext::InteractiveInputInfo::DISTANCE:
                value = formatter->formatDouble(doubleValue);
                break;
            default: {
                value = formatter->formatDouble(doubleValue);
                break;
            }
        }
        m_cachedStrValue = value;
        strValue = m_cachedStrValue;
    }
    else {
        strValue = m_cachedStrValue;
    }

    return true;
}

bool LC_PropertyDoubleInteractivePickView::doPropertyValueToStrForEdit(QString& strValue) const {
    const double doubleValue = propertyValue();
    const auto formatter = typedProperty().getFormatter();
    const auto interactiveInputType = typedProperty().getInteractiveInputType();
    switch (interactiveInputType) {
        case LC_ActionContext::InteractiveInputInfo::POINT_X:
        case LC_ActionContext::InteractiveInputInfo::POINT_Y:
        case LC_ActionContext::InteractiveInputInfo::POINT:
            strValue = formatter->formatDouble(doubleValue);
            break;
        case LC_ActionContext::InteractiveInputInfo::ANGLE:
            strValue = formatter->formatRawAngle(doubleValue);
            // todo - this is workaround for editing angles in decimal degrees. What about other formats?
            strValue = strValue.remove( QChar(0xB0));
            break;
        case LC_ActionContext::InteractiveInputInfo::DISTANCE:
            strValue = formatter->formatDouble(doubleValue);
            break;
        default: {
            if (m_formatAsInt) {
                strValue = formatter->formatInt(doubleValue);
            }
            else {
                strValue = formatter->formatDouble(doubleValue);
            }
            break;
        }
    }
    return true;
}
