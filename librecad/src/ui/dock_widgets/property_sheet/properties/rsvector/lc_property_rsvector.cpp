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

#include "lc_property_rsvector.h"

#include "lc_graphicviewport.h"
#include "lc_property_double_interactivepick_view.h"

LC_PropertyRSVector::LC_PropertyRSVector(QObject* parent, const bool holdValue)
    : ParentClass(parent, holdValue) {
}

QString LC_PropertyRSVector::getXKey() {
    return QStringLiteral("x");
}

LC_PropertyAtomic* LC_PropertyRSVector::createXProperty() {
    const auto viewName = m_interactiveInputType == LC_ActionContext::InteractiveInputInfo::POINT
                              ? LC_PropertyDoubleInteractivePickView::VIEW_NAME
                              : LC_PropertyDoubleInteractivePickView::VIEW_NAME;
    return createFieldProperty(&RS_Vector::getX, &RS_Vector::setX, getXKey(), getXLabel(), getXDescriptionFormat(), viewName);
}

QString LC_PropertyRSVector::getYKey() {
    return QStringLiteral("y");
}

LC_PropertyAtomic* LC_PropertyRSVector::createYProperty() {
    const auto viewName = m_interactiveInputType == LC_ActionContext::InteractiveInputInfo::POINT
                              ? LC_PropertyDoubleInteractivePickView::VIEW_NAME
                              : LC_PropertyDoubleInteractivePickView::VIEW_NAME;
    return createFieldProperty(&RS_Vector::getY, &RS_Vector::setY, getYKey(), getYLabel(), getYDescriptionFormat(), viewName);
}

QString LC_PropertyRSVector::getXLabel() const {
    return tr("X");
}

QString LC_PropertyRSVector::getXDescriptionFormat() const {
    return tr("X of the %1");
}

QString LC_PropertyRSVector::getYLabel() const {
    return tr("Y");
}

QString LC_PropertyRSVector::getYDescriptionFormat() const {
    return tr("Y of the %1");
}

QString LC_PropertyRSVector::getToStrValueFormat() {
    return tr("[%1, %2]");
}

void LC_PropertyRSVector::setActionContextAndLaterRequestor(LC_ActionContext* actionContext, LC_LateCompletionRequestor* requestor) {
    m_actionContext = actionContext;
    m_lateCompletionRequestor = requestor;
    m_formatter = actionContext->getFormatter();
}

void LC_PropertyRSVector::requestInteractiveInput() const {
    const QString propertyName = getName();
    m_actionContext->interactiveInputStart(getInteractiveInputType(), m_lateCompletionRequestor, propertyName);
}
