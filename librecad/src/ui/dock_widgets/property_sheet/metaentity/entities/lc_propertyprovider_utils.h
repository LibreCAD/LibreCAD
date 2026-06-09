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

#ifndef LC_PROPERTYPROVIDERUTILS_H
#define LC_PROPERTYPROVIDERUTILS_H

#include <QTimer>

#include "lc_enum_descriptor.h"
#include "lc_property_action.h"
#include "lc_property_action_link_view.h"
#include "lc_property_bool.h"
#include "lc_property_container.h"
#include "lc_property_double.h"
#include "lc_property_double_interactivepick_view.h"
#include "lc_property_enum.h"
#include "lc_property_qstring.h"
#include "lc_property_single.h"
#include "lc_property_valuestorage.h"
#include "rs.h"

namespace LC_PropertyProviderUtils {
    LC_EnumDescriptor* getLinearUnitsEnumDescriptor(RS2::LinearFormat format);
    LC_EnumDescriptor* getAngleUnitsEnumDescriptor(RS2::AngleFormat format);
    LC_EnumDescriptor* getAngleUnitFormatEnumDescriptor();
    LC_EnumDescriptor* getLinearUnitFormatEnumDescriptor();
    void notifyDrawingOptionsChanged();

     template <typename ValueType, typename EntityClass>
    class LC_EntityPropertyValueDirectEntityDelegate : public LC_PropertyValueStorage<ValueType> {
    public:
        using PropertyType = LC_PropertyAtomic;
        using ValueTypeStore = typename LC_PropertyValueStorage<ValueType>::ValueTypeStore;

        using FunValueGet = typename std::function<ValueTypeStore(EntityClass*)>;
        using FunValueSet = typename std::function<void(ValueType&, LC_PropertyChangeReason, EntityClass*)>;

        ValueType doGetValue() const override {
            Q_ASSERT(m_funGetValue);
            EntityClass* entity = m_entity;
            auto result = m_funGetValue(entity);
            return result;
        }

        void doSetValue(ValueType newValue, LC_PropertyChangeReason reason) override {
            Q_ASSERT(m_funSetValue);
            m_funSetValue(newValue, reason, m_entity);
            // fixme - do we need some modification there?
            // m_modificationContext->entityModified(entity, m_entity);
        }

        bool doCheckValueEqualToCurrent(ValueType valueToCompare) override {
            return LC_PropertyValueStorage<ValueType>::doCheckValueEqualToCurrent(valueToCompare);
        }

        void setup(EntityClass* entity, const FunValueGet& funGetValue, const FunValueSet& funSetValue) {
            m_entity = entity;
            m_funGetValue = funGetValue;
            m_funSetValue = funSetValue;
        }

    protected:
        EntityClass* m_entity{nullptr};

        FunValueGet m_funGetValue;
        FunValueSet m_funSetValue;
    };

    template <typename ValueType, typename EntityClass>
    void createDirectDelegatedStorage(const typename LC_EntityPropertyValueDirectEntityDelegate<ValueType, EntityClass>::FunValueGet &funGet,
                                      const std::function<void(const ValueType&, EntityClass*)> &funSet,
                                      EntityClass* entity, LC_PropertySingle<ValueType>* property) {
        auto valueStorage = new LC_EntityPropertyValueDirectEntityDelegate<ValueType, EntityClass>();

        auto funSetWrapped = [funSet](const ValueType& v, [[maybe_unused]] LC_PropertyChangeReason reason, EntityClass* e) -> void {
            funSet(v, e);
        };
        valueStorage->setup(entity, funGet, funSetWrapped);
        property->setValueStorage(valueStorage, true);
    }

    template <typename EntityClass>
    void createDirectDelegatedBool(LC_PropertyContainer* container, const LC_Property::Names& names,
                                   const typename LC_EntityPropertyValueDirectEntityDelegate<bool, EntityClass>::FunValueGet &funGet,
                                   const std::function<void(const bool&, EntityClass*)> &funSet, EntityClass* entity) {
        auto* property = new LC_PropertyBool(container, false);
        property->setNames(names);
        createDirectDelegatedStorage<bool, EntityClass>(funGet, funSet, entity, property);
        property->setReadOnly(funSet == nullptr);
        container->addChildProperty(property);
    }

    template <typename EntityClass>
    void createDirectDelegatedDouble(LC_PropertyContainer* container, const LC_Property::Names& names,
                                   const typename LC_EntityPropertyValueDirectEntityDelegate<double, EntityClass>::FunValueGet &funGet,
                                   const std::function<void(const double&, EntityClass*)> &funSet, EntityClass* entity,
                                   LC_ActionContext::InteractiveInputInfo::InputType inputType,
                                   LC_ActionContext* actionContext, LC_LateCompletionRequestor* requestor) {
        auto* property = new LC_PropertyDouble(container, false);
        property->setNames(names);
        property->setInteractiveInputType(inputType);
        LC_PropertyViewDescriptor attrs;
        attrs.viewName = LC_PropertyDoubleInteractivePickView::VIEW_NAME;
        property->setViewDescriptor(attrs);
        property->setActionContextAndLaterRequestor(actionContext, requestor);
        createDirectDelegatedStorage<double, RS_Graphic>(funGet, funSet, entity, property);
        property->setReadOnly(funSet == nullptr);
        container->addChildProperty(property);
    }

    inline void createDirectDelegatedReadonlyString(LC_PropertyContainer* container, const LC_Property::Names& names, const QString& value) {
        const auto gridTypeProperty = new LC_PropertyQString(container, true);
        gridTypeProperty->setNames(names);
        gridTypeProperty->setValue(value);
        gridTypeProperty->setReadOnly(true);
        container->addChildProperty(gridTypeProperty);
    }

    template <typename ValueType, typename EntityClass>
    auto addDirectEnumWithDescriptor(LC_PropertyContainer* container, const LC_Property::Names& names, const LC_EnumDescriptor* enumDescriptor,
                       const typename LC_EntityPropertyValueDirectEntityDelegate<ValueType, EntityClass>::FunValueGet &funGetValue,
                       const typename LC_EntityPropertyValueDirectEntityDelegate<ValueType, EntityClass>::FunValueSet &funSetValue,
                       EntityClass* entity, std::function<bool(EntityClass*, LC_PropertyViewDescriptor& descriptor)> &funPrepareDescriptor,
                       bool ownDescriptor = false) -> void {
        auto property = new LC_PropertyEnum(container, false);
        property->setNames(names);
        property->setEnumInfo(enumDescriptor, ownDescriptor);
        auto valueStorage = new LC_EntityPropertyValueDirectEntityDelegate<ValueType, EntityClass>();
        valueStorage->setup(entity, funGetValue, funSetValue);
        property->setValueStorage(valueStorage, true);

        bool readonly = false;
        if (funPrepareDescriptor != nullptr) {
            LC_PropertyViewDescriptor descriptor;
            readonly = funPrepareDescriptor(entity, descriptor);
            property->setViewDescriptor(descriptor);
        }
        if (readonly || funSetValue == nullptr) {
            property->setReadOnly();
        }
        container->addChildProperty(property);
    }

    template <typename ValueType, typename EntityClass>
    auto addDirectEnum(LC_PropertyContainer* container, const LC_Property::Names& names, const LC_EnumDescriptor* enumDescriptor,
                       const typename LC_EntityPropertyValueDirectEntityDelegate<ValueType, EntityClass>::FunValueGet &funGet,
                       const std::function<void(const ValueType&, EntityClass*)> &funSet,
                       EntityClass* entity, const bool ownDescriptor = false) -> void {
        auto property = new LC_PropertyEnum(container, false);
        property->setNames(names);
        property->setEnumInfo(enumDescriptor, ownDescriptor);
        auto valueStorage = new LC_EntityPropertyValueDirectEntityDelegate<ValueType, EntityClass>();
        auto funSetWrapped = [funSet](const ValueType& v, [[maybe_unused]] LC_PropertyChangeReason reason, EntityClass* e) -> void {
            funSet(v, e);
        };
        valueStorage->setup(entity, funGet, funSetWrapped);
        property->setValueStorage(valueStorage, true);
        if (funSet == nullptr) {
            property->setReadOnly();
        }
        container->addChildProperty(property);
    }

    template <typename EntityClass>
    void createSingleEntityCommand(LC_PropertyContainer* container, const QString& propertyName, const QString& linkTitle,
                                                      const QString& linkTooltip, const QString& linkTitleRight,
                                                      const QString& linkTooltipRight, EntityClass* entity,
                                                      const std::function<void(EntityClass*, int linkIndex)> &clickHandler, const QString &commonDescription) {
        auto* property = new LC_PropertyAction(container, true);
        property->setName(propertyName);
        property->setDisplayName("");
        LC_PropertyViewDescriptor viewDescriptor("Link");
        viewDescriptor[LC_PropertyActionLinkView::ATTR_TITLE] = linkTitle;
        viewDescriptor[LC_PropertyActionLinkView::ATTR_TOOLTIP_LEFT] = linkTooltip;
        if (!linkTitleRight.isEmpty()) {
            viewDescriptor[LC_PropertyActionLinkView::ATTR_TITLE_RIGHT] = linkTitleRight;
            viewDescriptor[LC_PropertyActionLinkView::ATTR_TOOLTIP_RIGHT] = linkTooltipRight;
        }
        property->setEntity(entity);
        auto wrappingClickHandler = [entity, clickHandler](const LC_PropertyAction*, int linkIndex) {
            QTimer::singleShot(10, [entity, linkIndex, clickHandler] { clickHandler(entity, linkIndex); });
        };
        property->setClickHandler(wrappingClickHandler);
        property->setDescription(commonDescription);
        property->setViewDescriptor(viewDescriptor);
        container->addChildProperty(property);
    }
}

#endif
