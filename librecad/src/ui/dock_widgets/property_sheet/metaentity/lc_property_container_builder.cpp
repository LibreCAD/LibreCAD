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

#include "lc_property_container_builder.h"

#include "lc_graphicviewport.h"
#include "lc_property_bool.h"
#include "lc_property_container.h"
#include "lc_property_double.h"
#include "lc_property_double_interactivepick_view.h"
#include "lc_property_qstring.h"
#include "lc_property_rsvector.h"
#include "lc_property_rsvector_view.h"
#include "lc_propertysheetwidget.h"

LC_PropertyQString* LC_PropertyContainerBuilder::createReadonlyStringProperty(const LC_Property::Names& names,
                                                                              QList<LC_PropertyAtomic*>* props, LC_PropertyContainer* cont,
                                                                              const QString& value) {
    auto* property = new LC_PropertyQString(cont, true);
    property->setNames(names);
    property->setValue(value);
    property->setReadOnly();
    props->push_back(property);
    return property;
}

LC_PropertyDouble* LC_PropertyContainerBuilder::createDoubleProperty(const LC_Property::Names& names, LC_PropertyContainer* cont,
                                                                     const LC_ActionContext::InteractiveInputInfo::InputType inputType,
                                                                     LC_ActionContext* actionContext,
                                                                     LC_LateCompletionRequestor* requestor) {
    auto* property = new LC_PropertyDouble(cont, false);
    property->setNames(names);
    property->setInteractiveInputType(inputType);
    if (inputType != LC_ActionContext::InteractiveInputInfo::NOTNEEDED) {
        LC_PropertyViewDescriptor attrs;
        attrs.viewName = LC_PropertyDoubleInteractivePickView::VIEW_NAME;
        property->setViewDescriptor(attrs);
    }
    property->setActionContextAndLaterRequestor(actionContext, requestor);
    return property;
}

LC_PropertyDouble* LC_PropertyContainerBuilder::createDoubleProperty(const LC_Property::Names& names, QList<LC_PropertyAtomic*>* props,
                                                                     LC_PropertyContainer* cont,
                                                                     const LC_ActionContext::InteractiveInputInfo::InputType inputType,
                                                                     LC_ActionContext* actionContext,
                                                                     LC_LateCompletionRequestor* requestor) {
    LC_PropertyDouble* property = createDoubleProperty(names, cont, inputType, actionContext, requestor);
    props->push_back(property);
    return property;
}

LC_PropertyRSVector* LC_PropertyContainerBuilder::createVectorProperty(const LC_Property::Names& names, LC_PropertyContainer* cont,
                                                                       LC_ActionContext* actionContext,
                                                                       LC_LateCompletionRequestor* requestor) {
    auto* property = new LC_PropertyRSVector(cont, false);
    property->setNames(names);
    if (requestor != nullptr) {
        property->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::POINT);
    }
    property->setViewDescriptorProvider([]() -> LC_PropertyViewDescriptor {
        return {{{LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME, tr("X")}, {LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME, tr("Y")}}};
    });
    property->setActionContextAndLaterRequestor(actionContext, requestor);
    return property;
}

LC_PropertyBool* LC_PropertyContainerBuilder::createBoolProperty(const LC_Property::Names& names, LC_PropertyContainer* cont) {
    const auto property = new LC_PropertyBool(cont, false);
    property->setNames(names);
    return property;
}

LC_PropertyRSVector* LC_PropertyContainerBuilder::createVectorProperty(const LC_Property::Names& names, QList<LC_PropertyAtomic*>* props,
                                                                       LC_PropertyContainer* cont, LC_ActionContext* actionContext,
                                                                       LC_LateCompletionRequestor* requestor) {
    LC_PropertyRSVector* property = createVectorProperty(names, cont, actionContext, requestor);
    props->push_back(property);
    return property;
}

LC_PropertyContainer* LC_PropertyContainerBuilder::createSection(LC_PropertyContainer* container, const LC_Property::Names& names) const {
    const auto result = new LC_PropertyContainer(container);
    result->setNames(names);
    container->addChildProperty(result);
    m_widget->checkSectionCollapsed(result);
    return result;
}

LC_Formatter* LC_PropertyContainerBuilder::getFormatter() const {
    const auto graphicView = m_actionContext->getGraphicView();
    const auto viewport = graphicView->getViewPort();
    LC_Formatter* result = viewport->getFormatter();
    return result;
}

RS_Vector LC_PropertyContainerBuilder::toUCS(const RS_Vector& wcs) const {
    const auto graphicView = m_actionContext->getGraphicView();
    const auto viewport = graphicView->getViewPort();
    const auto ucs = viewport->toUCS(wcs);
    return ucs;
}

double LC_PropertyContainerBuilder::toUCSBasisAngle(const double wcsAngle) const {
    const auto graphicView = m_actionContext->getGraphicView();
    const auto viewport = graphicView->getViewPort();
    const double ucsAngle = viewport->toUCSAngle(wcsAngle);
    const double ucsBasisAngle = viewport->toBasisUCSAngle(ucsAngle);
    return ucsBasisAngle;
}

double LC_PropertyContainerBuilder::toWCSAngle(const double ucsBasicAngle) const {
    const auto graphicView = m_actionContext->getGraphicView();
    const auto viewport = graphicView->getViewPort();
    const double ucsAngle = viewport->toAbsUCSAngle(ucsBasicAngle);
    const double wcsAngle = viewport->toWorldAngle(ucsAngle);
    return wcsAngle;
}

RS_Vector LC_PropertyContainerBuilder::toWCS(const RS_Vector& ucs) const {
    const auto viewport = m_actionContext->getGraphicView()->getViewPort();
    const auto wcs = viewport->toWorld(ucs);
    return wcs;
}

QString LC_PropertyContainerBuilder::formatLinear(const double length) const {
    const LC_Formatter* formatter = getFormatter();
    return formatter->formatLinear(length);
}

QString LC_PropertyContainerBuilder::formatDouble(const double length) const {
    const LC_Formatter* formatter = getFormatter();
    return formatter->formatDouble(length);
}

QString LC_PropertyContainerBuilder::formatInt(const int length) const {
    const LC_Formatter* formatter = getFormatter();
    return formatter->formatInt(length);
}

QString LC_PropertyContainerBuilder::formatInt(const double length) const {
    const LC_Formatter* formatter = getFormatter();
    const int len = static_cast<int>(length);
    return formatter->formatInt(len);
}

QString LC_PropertyContainerBuilder::formatInt(const qsizetype length) const {
    const LC_Formatter* formatter = getFormatter();
    const int len = static_cast<int>(length);
    return formatter->formatInt(len);
}

QString LC_PropertyContainerBuilder::formatWCSAngleDegrees(const double wcsAngle) const {
    const LC_Formatter* formatter = getFormatter();
    auto result = formatter->formatWCSAngleDegrees(wcsAngle);
    return result;
}

QString LC_PropertyContainerBuilder::formatRawAngle(const double length) const {
    const LC_Formatter* formatter = getFormatter();
    auto result = formatter->formatRawAngle(length);
    return result;
}

QString LC_PropertyContainerBuilder::formatRawAngle(const double length, const RS2::AngleFormat format) const {
    const LC_Formatter* formatter = getFormatter();
    auto result = formatter->formatRawAngle(length, format);
    return result;
}
