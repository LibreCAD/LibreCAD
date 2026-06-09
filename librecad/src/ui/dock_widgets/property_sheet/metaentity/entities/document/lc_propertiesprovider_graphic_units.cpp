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

#include "lc_propertiesprovider_graphic_units.h"

#include "lc_propertyprovider_utils.h"
#include "rs_graphic.h"
#include "rs_units.h"

namespace {
    LC_EnumDescriptor createUnitsDescriptor() {
        QVector<LC_EnumValueDescriptor> unitValues;
        for (int i = RS2::None; i < RS2::LastUnit; i++) {
            auto desc = LC_EnumValueDescriptor(i, RS_Units::unitToString(static_cast<RS2::Unit>(i)));
            unitValues.append(desc);
        }
        auto result = LC_EnumDescriptor("drawingUnits", unitValues);
        return result;
    }
}

const QString LC_PropertiesProviderGraphicUnits::SECTION_UNITS = "_secUnits";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderGraphicUnits::fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) {
    if (m_widget->getOptions()->noSelectionDrawingUnits) {
        const LC_Property::Names names = {SECTION_UNITS, tr("Drawing Units"), tr("Units and precisions for current drawing")};
        const auto cont = createSection(container, names);
        createDrawingUnit(cont, graphic);
        createLinearFormat(graphic, cont);
        createLinearPrecision(cont, graphic);
        createAngleFormat(cont, graphic);
        createAngularPrecision(cont, graphic);
        createAnglesBasisZeroDirection(cont, graphic);
        createAnglesBasisIncreaseDirection(graphic, cont);
    }
}

void LC_PropertiesProviderGraphicUnits::createAngleFormat(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    const auto enumDescriptor = getAngleUnitFormatEnumDescriptor();
    auto funGet = [](const RS_Graphic* e) -> int {
        return e->getAngleFormat();
    };
    auto funSet = [this](const LC_PropertyEnumValueType& v, RS_Graphic* e) -> void {
        e->setAngleFormat(static_cast<RS2::AngleFormat>(v));
        notifyDrawingOptionsChanged();
    };

    const LC_Property::Names names = {"unitsAngleFormat", tr("Angle"), tr("Format of angle units used in drawing")};
    addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, enumDescriptor, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicUnits::createAngularPrecision(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    const auto enumDescriptor = getAngleUnitsEnumDescriptor(graphic->getAngleFormat());
    auto funGet = [](const RS_Graphic* e) -> int {
        return e->getAnglePrecision() + 1;
    };
    auto funSet = [this](const LC_PropertyEnumValueType& v, RS_Graphic* e) -> void {
        e->addAnglePrecision(v - 1);
        notifyDrawingOptionsChanged();
    };

    const LC_Property::Names names = {"unitsAnglePrecision", tr("Angle precision"), tr("Precision of angular units used in drawing")};
    addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, enumDescriptor, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicUnits::createLinearFormat(RS_Graphic* graphic, LC_PropertyContainer* const cont) const {
    const auto* linearFormatDescriptor = getLinearUnitFormatEnumDescriptor();
    auto funGet = [](const RS_Graphic* e) -> RS2::LinearFormat {
        return e->getLinearFormat();
    };
    auto funSet = [this](const LC_PropertyEnumValueType& v, RS_Graphic* e) -> void {
        e->setLinearFormat(static_cast<RS2::LinearFormat>(v));
        notifyDrawingOptionsChanged();
    };

    const LC_Property::Names names = {"unitsLinearFormat", tr("Linear"), tr("Format of linear units used in drawing")};
    addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, linearFormatDescriptor, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicUnits::createLinearPrecision(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    const auto* enumDescriptor = getLinearUnitsEnumDescriptor(graphic->getLinearFormat());
    auto funGet = [](const RS_Graphic* e) -> int {
        return e->getLinearPrecision() + 1;
    };
    auto funSet = [this](const LC_PropertyEnumValueType& v, RS_Graphic* e) -> void {
        e->setLinearPrecision(v - 1);
        notifyDrawingOptionsChanged();
    };

    const LC_Property::Names names = {
        "unitsLinearPrecision",
        tr("Linear precision"),
        tr("Precision of linear units units used in drawing")
    };
    addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, enumDescriptor, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicUnits::createDrawingUnit(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    static LC_EnumDescriptor enumDescriptor = createUnitsDescriptor();
    auto funGet = [](const RS_Graphic* e) -> RS2::Unit {
        return e->getUnit();
    };
    auto funSet = [this](const LC_PropertyEnumValueType& v, RS_Graphic* e) -> void {
        e->setUnit(static_cast<RS2::Unit>(v));
        notifyDrawingOptionsChanged();
    };

    const LC_Property::Names names = {"unitsUnits", tr("Primary Units"), tr("Primary linear units used by drawing")};
    addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, &enumDescriptor, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicUnits::createAnglesBasisIncreaseDirection(RS_Graphic* graphic, LC_PropertyContainer* const cont) const {
    const LC_Property::Names names = {
        "unitsBaseAngleDir",
        tr("Angles clockwize"),
        tr("Direction of angles. If counterclockwize, 90 degrees is north, for clockwise - 90 degrees is south")
    };

    auto funGet = [](const RS_Graphic* e) -> bool {
        return !e->areAnglesCounterClockWise();
    };
    auto funSet = [this](const bool& v, RS_Graphic* e) -> void {
        e->setAnglesCounterClockwise(!v);
        notifyDrawingOptionsChanged();
    };

    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicUnits::createAnglesBasisZeroDirection(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    const LC_Property::Names names = {
        "unitsBaseAngle",
        tr("Base angle"),
        tr("Direction of zero angle (in degrees). 0 degrees there is 3 pm.")
    };
    auto funGet = [](const RS_Graphic* e) -> double {
        const double base = e->getAnglesBase();
        return base;
    };
    auto funSet = [this](const double& v, RS_Graphic* e) -> void {
        e->setAnglesBase(v);
        notifyDrawingOptionsChanged();
    };
    createDirectDelegatedDouble<RS_Graphic>(cont, names, funGet, funSet, graphic,
        LC_ActionContext::InteractiveInputInfo::InputType::ANGLE,m_actionContext, m_widget);
}
