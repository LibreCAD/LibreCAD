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

#include "lc_property_rsvector_view.h"

#include <QLabel>

#include "lc_property_double_interactivepick_view.h"
#include "lc_property_editor_button_handler.h"
#include "lc_property_label_with_button.h"
#include "lc_property_multi.h"
#include "lc_property_paint_context.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyRSVectorView::VIEW_NAME = QByteArrayLiteral("RSVectorXY");
const QByteArray LC_PropertyRSVectorView::ATTR_SUFFIX = QByteArrayLiteral("suffix");
const QByteArray LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME = QByteArrayLiteral("xDisplayName");
const QByteArray LC_PropertyRSVectorView::ATTR_X_DESCRIPTION = QByteArrayLiteral("xDescription");
const QByteArray LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME = QByteArrayLiteral("yDisplayName");
const QByteArray LC_PropertyRSVectorView::ATTR_Y_DESCRIPTION = QByteArrayLiteral("yDescription");
const QByteArray LC_PropertyRSVectorView::ATTR_FORMAT_AS_INT = QByteArrayLiteral("formatAsInt");

class LC_PropertyRSVectorPickViewHandler : public LC_PropertyEditorButtonHandler<LC_PropertyRSVector, LC_PropertyLabelWithButton> {
public:
    LC_PropertyRSVectorPickViewHandler(LC_PropertyViewEditable* view, LC_PropertyLabelWithButton& editor) : LC_PropertyEditorButtonHandler(
        view, editor) {
        LC_PropertyRSVectorPickViewHandler::doUpdateEditor();
        editor.getLabel()->installEventFilter(this);

        connect(editor.getToolButton(), &QToolButton::clicked, this, &LC_PropertyRSVectorPickViewHandler::onToolButtonClicked);
    }

protected:
    QString toString(const RS_Vector& value) const {
        QString strValue;
        auto* typedView = static_cast<LC_PropertyRSVectorView*>(view());
        const auto formatter = typedView->typedProperty().getFormatter();
        LC_PropertyRSVectorView::formatVectorToString(value, "", formatter, strValue, typedView->isFormatAsInt());
        return strValue;
    }

    void doUpdateEditor() override {
        const auto le = getEditor()->getLabel();
        const auto value = getPropertyValue();
        const QString text = toString(value);
        le->setText(text);
        le->setSelection(0, text.length());
    }

    void doOnToolButtonClick() override {
        onToolButtonClicked(false);
    }

    bool fromString([[maybe_unused]] const QString& text, [[maybe_unused]] RS_Vector& ok) {
        return false;
    }

    void onToolButtonClicked(bool) {
        stopInplaceEdit();
        getProperty().requestInteractiveInput();
    }
};

LC_PropertyRSVectorView::LC_PropertyRSVectorView(LC_PropertyRSVector& property)
    : LC_PropertyViewTypedCompound(property) {
    const auto actionContext = property.getActionContext();
    const auto interactiveInputRequestor = property.getInteractiveInputRequestor();

    const bool readOnly = isReadOnly();

    const auto xProperty = static_cast<LC_PropertyDouble*>(property.createXProperty());

    const bool pickable = property.getInteractiveInputType() != LC_ActionContext::InteractiveInputInfo::NOTNEEDED;

    xProperty->setActionContextAndLaterRequestor(actionContext, interactiveInputRequestor);
    if (pickable && !readOnly) {
        xProperty->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::POINT_X);
    }
    addSubProperty(xProperty);
    const auto yProperty = static_cast<LC_PropertyDouble*>(property.createYProperty());
    yProperty->setActionContextAndLaterRequestor(actionContext, interactiveInputRequestor);
    if (pickable && !readOnly) {
        yProperty->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::POINT_Y);
    }
    if (readOnly) {
        property.setReadOnly(); // should restore original readonly
        yProperty->setReadOnly();
        xProperty->setReadOnly();
    }
    addSubProperty(yProperty);
}

void LC_PropertyRSVectorView::doApplyAttributes(const LC_PropertyViewDescriptor& atts) {
    atts.load(ATTR_SUFFIX, m_suffix);
    atts.load(ATTR_FORMAT_AS_INT, m_formatAsInt);

    enum {
        X,
        Y
    };

    static const std::vector<SubPropertyInfo> KEYS = {
        {X, LC_PropertyRSVector::getXKey(), ATTR_X_DISPLAY_NAME, ATTR_X_DESCRIPTION},
        {Y, LC_PropertyRSVector::getYKey(), ATTR_Y_DISPLAY_NAME, ATTR_Y_DESCRIPTION}
    };

    applySubPropertyInfos(atts, KEYS);
}

QWidget* LC_PropertyRSVectorView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    if (isEditableByUser()) {
        const auto interactiveInputType = typedProperty().getInteractiveInputType();
        const bool pickable = interactiveInputType != LC_ActionContext::InteractiveInputInfo::NOTNEEDED;
        if (!pickable) {
            return createValueEditorLineEdit(parent, rect, true, ctx);
        }
        const auto le = new LC_PropertyLabelWithButton(parent);
        le->setGeometry(rect);
        new LC_PropertyRSVectorPickViewHandler(this, *le);
        const QString buttonIconName = ":/icons/interactive_pick_point.lci";
        const auto toolButton = le->getToolButton();
        toolButton->setText("");
        toolButton->setToolTip(tr("Pick coordinate from drawing"));
        toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        toolButton->setIcon(QIcon(buttonIconName));
        return le;
    }
    return nullptr;
}

bool LC_PropertyRSVectorView::doPropertyValueToStrForView(QString& strValue) {
    if (m_cachedStrValue.isEmpty()) {
        QString value;
        vectorToString(propertyValue(), m_suffix, value);
        m_cachedStrValue = value;
        strValue = m_cachedStrValue;
    }
    else {
        strValue = m_cachedStrValue;
    }
    return true;
}

void LC_PropertyRSVectorView::formatVectorToString(const RS_Vector& value, const QString& suffix, const LC_Formatter* formatter,
                                                   QString& strValue, bool asInt) {
    const auto format = LC_PropertyRSVector::getToStrValueFormat();
    const QString valueX = asInt ? formatter->formatInt(value.getX()) : formatter->formatLinear(value.getX());
    const QString valueY =  asInt ? formatter->formatInt(value.getY()) :formatter->formatLinear(value.getY());
    strValue = format.arg(valueX + suffix).arg(valueY + suffix);
}

void LC_PropertyRSVectorView::invalidateCached() {
    m_cachedStrValue.clear();
}

void LC_PropertyRSVectorView::vectorToString(const RS_Vector& value, const QString& suffix, QString& strValue) {
    const LC_Formatter* formatter = typedProperty().getFormatter();
    formatVectorToString(value, suffix, formatter, strValue, m_formatAsInt);
}
