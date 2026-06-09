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

#include "lc_property_rect_view.h"

#include "lc_formatter.h"
#include "lc_property_double.h"

const QByteArray LC_PropertyRectView::VIEW_NAME = QByteArrayLiteral("RSVectorXY");

LC_PropertyRectView::LC_PropertyRectView(LC_PropertyRect* property)
    : LC_PropertyViewTypedCompound(property) {
    const auto actionContext = property->getActionContext();
    const bool readOnly = isReadOnly();

    const auto leftProperty = static_cast<LC_PropertyDouble*>(property->createLeftProperty());
    leftProperty->setActionContextAndLaterRequestor(actionContext, nullptr);
    addSubProperty(leftProperty);

    const auto rightProperty = static_cast<LC_PropertyDouble*>(property->createRightProperty());
    rightProperty->setActionContextAndLaterRequestor(actionContext, nullptr);
    addSubProperty(rightProperty);

    const auto topProperty = static_cast<LC_PropertyDouble*>(property->createTopProperty());
    topProperty->setActionContextAndLaterRequestor(actionContext, nullptr);
    addSubProperty(topProperty);

    const auto bottomProperty = static_cast<LC_PropertyDouble*>(property->createBottomProperty());
    bottomProperty->setActionContextAndLaterRequestor(actionContext, nullptr);
    addSubProperty(bottomProperty);

    if (readOnly) {
        property->setReadOnly(); // should restore original readonly
        leftProperty->setReadOnly();
        rightProperty->setReadOnly();
        topProperty->setReadOnly();
        bottomProperty->setReadOnly();
    }

}

// void LC_PropertyRSVectorView::doApplyAttributes(const LC_PropertyViewDescriptor& atts) {
//     atts.load(ATTR_SUFFIX, m_suffix);
//
//     enum {
//         X,
//         Y
//     };
//
//     static const std::vector<SubPropertyInfo> KEYS = {
//         {X, LC_PropertyRSVector::getXKey(), ATTR_X_DISPLAY_NAME, ATTR_X_DESCRIPTION},
//         {Y, LC_PropertyRSVector::getYKey(), ATTR_Y_DISPLAY_NAME, ATTR_Y_DESCRIPTION}
//     };
//
//     applySubPropertyInfos(atts, KEYS);
// }

QWidget* LC_PropertyRectView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    if (isEditableByUser()) {
       return createValueEditorLineEdit(parent, rect, true, ctx);
    }
    return nullptr;
}

bool LC_PropertyRectView::doPropertyValueToStrForView(QString& strValue) {
    if (m_cachedStrValue.isEmpty()) {
        QString value;
        marginsToString(propertyValue(), value);
        m_cachedStrValue = value;
        strValue = m_cachedStrValue;
    }
    else {
        strValue = m_cachedStrValue;
    }
    return true;
}

void LC_PropertyRectView::formatMarginsToString(const LC_MarginsRect& value, const LC_Formatter* formatter,
                                                   QString& strValue) {
    const auto format = LC_PropertyRect::getToStrValueFormat();
    const QString valueLeft = formatter->formatLinear(value.getLeft());
    const QString valueRight = formatter->formatLinear(value.getRight());
    const QString valueTop = formatter->formatLinear(value.getRight());
    const QString valueBottom = formatter->formatLinear(value.getRight());
    strValue = format.arg(valueLeft).arg(valueRight).arg(valueTop).arg(valueBottom);
}

void LC_PropertyRectView::invalidateCached() {
    m_cachedStrValue.clear();
}

void LC_PropertyRectView::marginsToString(const LC_MarginsRect& value, QString& strValue) {
    const LC_Formatter* formatter = typedProperty().getFormatter();
    formatMarginsToString(value, formatter, strValue);
}
