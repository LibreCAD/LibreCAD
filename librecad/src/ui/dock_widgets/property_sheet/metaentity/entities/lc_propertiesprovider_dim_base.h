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

#ifndef LC_PROPERTIES_PROVIDER_DIM_BASE_H
#define LC_PROPERTIES_PROVIDER_DIM_BASE_H

#include "lc_dimarrowregistry.h"
#include "lc_entity_type_propertiesprovider.h"
#include "rs_dimension.h"

class LC_PropertiesProviderDimBase : public LC_EntityTypePropertiesProvider {
    Q_OBJECT

public:
    static const QString SECTION_DIM_GENERAL;

    LC_PropertiesProviderDimBase(const RS2::EntityType entityType, LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_EntityTypePropertiesProvider(entityType, actionContext, widget) {
    }

protected:
    static const QString SECTION_DIM_LINES_AND_ARROWS;
    static const QString SECTION_DIM_TEXT;
    static const QString SECTION_DIM_FIT;
    static const QString SECTION_DIM_PRIMARY_UNITS;
    static const QString SECTION_DIM_ALTERNATE_UNITS;
    static const QString SECTION_DIM_TOLERANCES;
    static const QString SECTION_DIM_GEOMETRY;

    void doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) override;
    void addArrowsFlipLinks(const QList<RS_Entity*>& list, LC_PropertyContainer* cont) const;
    void createMiscSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);
    void prepareArrowheadItemsDescriptor(QString arrowName, const std::vector<LC_DimArrowRegistry::ArrowInfo>& arrowTypes,
                                         bool hasCustomBlocks, const QStringList& blocksNames, LC_PropertyViewDescriptor& descriptor);
    void createLinesAndArrowsSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);
    void createTextSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);
    void createFitSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);
    void createPrimaryUnitsSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);
    void createAlternateUnitsSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);
    void createTolerancesSection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);

    void addDecimalPlacesField_DS(const LC_Property::Names& precisionNames,
                                  const std::function<LC_PropertyEnumValueType(LC_DimStyle * ds)>& funGetValue,
                                  const std::function<void(LC_PropertyEnumValueType & v, LC_DimStyle * ds)>&
                                  funSetValue, const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                  const std::function<bool(RS_Dimension *, LC_PropertyViewDescriptor &)>& funPrepareDescriptor = nullptr);
    void addAngleDecimalPlacesField_DS(const LC_Property::Names& precisionNames,
                                       const std::function<LC_PropertyEnumValueType(LC_DimStyle * ds)>& funGetValue,
                                       const std::function<void(LC_PropertyEnumValueType & v,
                                                                LC_DimStyle * ds)>& funSetValue, const QList<RS_Entity*>& list,
                                       LC_PropertyContainer* cont);
    void addAngleUnitFormat(const LC_Property::Names& names, const std::function<LC_PropertyEnumValueType(LC_DimStyle * ds)>& funGetValue,
                            const std::function<void(LC_PropertyEnumValueType & v, LC_DimStyle * ds)>&
                            funSetValue, const QList<RS_Entity*>& list, LC_PropertyContainer* cont);
    void addLinearUnitFormat_DS(const LC_Property::Names& names,
                                const std::function<LC_PropertyEnumValueType(LC_DimStyle * ds)>& funGetValue,
                                const std::function<void(LC_PropertyEnumValueType & v, LC_DimStyle * ds)>&
                                funSetValue, const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                const std::function<bool(RS_Dimension *, LC_PropertyViewDescriptor &)>& funPrepareDescriptor = nullptr);
    void addColor_DS(const LC_Property::Names& names, const std::function<RS_Color(LC_DimStyle * ds)>& funGetValue,
                     const std::function<void(RS_Color & v,  LC_DimStyle * ds)>& funSetValue,
                     const QList<RS_Entity*>& list, LC_PropertyContainer* cont);
    void addLineType_DS(const LC_Property::Names& names, const std::function<RS2::LineType(LC_DimStyle * ds)>& funGetValue,
                        const std::function<void(RS2::LineType & v, LC_DimStyle * ds)>& funSetValue,
                        const QList<RS_Entity*>& list, LC_PropertyContainer* cont);
    void addLineWidth_DS(const LC_Property::Names& names, const std::function<RS2::LineWidth(LC_DimStyle * e)>& funGetValue,
                         const std::function<void(RS2::LineWidth & v, LC_DimStyle * e)>& funSetValue,
                         const QList<RS_Entity*>& list, LC_PropertyContainer* cont);
    void addBoolean_DS(const LC_Property::Names& names, const std::function<bool(LC_DimStyle * e)>& funGetValue,
                       const std::function<void(bool&v,LC_DimStyle * e)>& funSetValue,
                       const QList<RS_Entity*>& list, LC_PropertyContainer* container,
                       const QString& viewName = LC_PropertyBoolCheckBoxView::VIEW_NAME,
                       const std::function<bool(RS_Dimension *, LC_PropertyViewDescriptor & descriptor)>& funPrepareDescriptor = nullptr);
    void addString_DS(const LC_Property::Names& names, const std::function<QString(LC_DimStyle * e)>& funGetValue,
                      const std::function<void(QString & v, LC_DimStyle * e)>& funSetValue,
                      const QList<RS_Entity*>& list, LC_PropertyContainer* container, bool multiLine,
                      const std::function<bool(RS_Dimension *, LC_PropertyViewDescriptor & descriptor)>& funPrepareDescriptor = nullptr);
    void addDouble_DS(const LC_Property::Names& names, const std::function<double(LC_DimStyle * e)>& funGetValue,
                      const std::function<void(double&v, LC_DimStyle * e)>& funSetValue,
                      const QList<RS_Entity*>& list, LC_PropertyContainer* container,
                      const std::function<bool(RS_Dimension *, LC_PropertyViewDescriptor & descriptor)>& funPrepareDescriptor = nullptr);
    void addEnum_DS(const LC_Property::Names& names, const LC_EnumDescriptor* enumDescriptor,
                    const std::function<LC_PropertyEnumValueType(LC_DimStyle * e)>& funGetValue,
                    const std::function<void(LC_PropertyEnumValueType & v, LC_DimStyle * e)>& funSetValue,
                    const QList<RS_Entity*>& list, LC_PropertyContainer* container,
                    const std::function<bool(RS_Dimension *, LC_PropertyViewDescriptor & descriptor)>& funPrepareDescriptor = nullptr);
    void addVarEnum_DS(const LC_Property::Names& names,
                       const std::function<LC_EnumDescriptor * (RS_Dimension * dim)>& enumDescriptorProvider,
                       const std::function<LC_PropertyEnumValueType(LC_DimStyle * e)>& funGetValue,
                       const std::function<void(LC_PropertyEnumValueType & v,  LC_DimStyle * e)>&
                       funSetValue, const QList<RS_Entity*>& list, LC_PropertyContainer* container,
                       const std::function<bool(RS_Dimension *, LC_PropertyViewDescriptor & descriptor)>& funPrepareDescriptor = nullptr);

    void createDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list);
    virtual void doCreateDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) = 0;
    void fillComputedProperites([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& entitiesList) override {}
    void doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) override;
};

#endif
