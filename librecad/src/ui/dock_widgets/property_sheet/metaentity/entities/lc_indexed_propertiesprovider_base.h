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

#ifndef LC_INDEXEDPROPERTIESPROVIDERBASE_H
#define LC_INDEXEDPROPERTIESPROVIDERBASE_H

#include "lc_entity_type_propertiesprovider.h"
#include "lc_linemath.h"
#include "lc_property_rsvector_view.h"


class LC_IndexedPropertiesProviderBase : public LC_EntityTypePropertiesProvider {
    Q_OBJECT

public:
    LC_IndexedPropertiesProviderBase(const RS2::EntityType entityType, LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_EntityTypePropertiesProvider(entityType, actionContext, widget) {
    }

protected:
    using FunGetDoubleValueByIndex = typename std::function<double(int)>;
    using FunSetDoubleValueByIndex = typename std::function<void(int, double)>;

    using FunGetWCSVectorByIndex = typename std::function<RS_Vector(int)>;
    using FunPersistValue = typename std::function<void(int)>;

    class LC_IndexValueStorage : public LC_PropertyValueStorage<int> {
    public:
        LC_IndexValueStorage(LC_GraphicViewport* viewport, LC_PropertySheetWidget* widget, const int maxValue,
                             const FunGetWCSVectorByIndex& funGetWCSVector, const FunPersistValue& funPersistValue)
            : m_funGetWCSVector{funGetWCSVector}, m_funPersistValue{funPersistValue}, m_viewport{viewport}, m_widget{widget},
              m_maxValue(maxValue) {
        }

        int doGetValue() const override {
            return m_index;
        }

        void doSetValue(int newValue, [[maybe_unused]]LC_PropertyChangeReason reason) override {
            if (newValue > m_maxValue) {
                newValue = 1;
            }
            else if (newValue == 0) {
                newValue = m_maxValue;
            }
            m_index = newValue;

            const auto wcsVector = m_funGetWCSVector(newValue);
            m_funPersistValue(newValue);
            m_viewport->clearLocationsHighlight();
            m_viewport->highlightLocation(wcsVector);
            m_widget->invalidateCached();
        }

    protected:
        FunGetWCSVectorByIndex m_funGetWCSVector;
        FunPersistValue m_funPersistValue;
        LC_GraphicViewport* m_viewport{nullptr};
        LC_PropertySheetWidget* m_widget = nullptr;
        int m_index{0};
        int m_maxValue{0};
    };

    LC_PropertyInt* createIndexProperty(LC_PropertyContainer* container, const LC_Property::Names& indexPropertNames, int pointCount,
                                        const FunGetWCSVectorByIndex &funGetPointByIndex,
                                        const FunPersistValue &persistIndexValue) const;
    template <class EntityClass>
    void createIndexedPointProperty(LC_PropertyContainer* container, EntityClass* entity, LC_Property::Names indexedPointNames,
                                    const FunGetWCSVectorByIndex &funGetPointByIndex,
                                    LC_PropertyInt* propertyPointIndex);
    template <class EntityClass>
    void createIndexedDoubleProperty(LC_PropertyContainer* container, EntityClass* entity, LC_Property::Names indexedPointNames,
                                     const FunGetDoubleValueByIndex & funGetValueByIndex,
                                     const FunSetDoubleValueByIndex& funSetValueByIndex,
                                     LC_PropertyInt* propertyPointIndex);
    template <typename EntityClass>
    LC_PropertyInt* createIndexAndPointProperties(LC_PropertyContainer* container, EntityClass* entity,
                                                  LC_Property::Names indexPropertNames, LC_Property::Names indexedPointName, int pointCount,
                                                  int initialIndexToSet,
                                                  const FunGetWCSVectorByIndex &funGetPointByIndex,
                                                  const FunPersistValue &persistIndexValue);
};


inline LC_PropertyInt* LC_IndexedPropertiesProviderBase::createIndexProperty(LC_PropertyContainer* container, const LC_Property::Names& indexPropertNames,
    const int pointCount, const FunGetWCSVectorByIndex &funGetPointByIndex,
    const FunPersistValue &persistIndexValue) const {
    const auto propertyPointIndex = new LC_PropertyInt(container, false);
    propertyPointIndex->setNames(indexPropertNames);
    LC_PropertyViewDescriptor viewDescriptor(LC_PropertyIntSpinBoxView::VIEW_NAME);
    viewDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MIN] = 0;
    viewDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MAX] = pointCount + 1;
    viewDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_STEP] = 1;
    viewDescriptor.attributes[LC_PropertyView::ATTR_VIRTUAL] = true;
    propertyPointIndex->setViewDescriptor(viewDescriptor);

    const auto valueStorage = new LC_IndexValueStorage(m_actionContext->getGraphicView()->getViewPort(), m_widget, pointCount, funGetPointByIndex,
                                                 persistIndexValue);
    propertyPointIndex->setValueStorage(valueStorage, true);
    container->addChildProperty(propertyPointIndex);
    return propertyPointIndex;
}

template <class EntityClass>
void LC_IndexedPropertiesProviderBase::createIndexedPointProperty(LC_PropertyContainer* container, EntityClass* entity,
    LC_Property::Names indexedPointNames, const FunGetWCSVectorByIndex& funGetPointByIndex, LC_PropertyInt* propertyPointIndex) {

    auto* propertyPoint = new LC_PropertyRSVector(container, false);

    propertyPoint->setNames(indexedPointNames);
    propertyPoint->setViewDescriptorProvider([]() -> LC_PropertyViewDescriptor {
        return {{{LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME, tr("X")}, {LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME, tr("Y")}}};
    });
    propertyPoint->setActionContextAndLaterRequestor(m_actionContext, m_widget);
    propertyPoint->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::POINT);

    auto vertexValueStorage = new LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>();
    vertexValueStorage->setup(entity, this->m_widget,
                              [this,propertyPointIndex, funGetPointByIndex]([[maybe_unused]] EntityClass* e) -> RS_Vector {
                                  const int index = propertyPointIndex->value();
                                  const RS_Vector wcsVector = funGetPointByIndex(index);
                                  const RS_Vector ucsVector = toUCS(wcsVector); // here we return in UCS for editing
                                  return ucsVector;
                              }, [this,propertyPointIndex, funGetPointByIndex](const RS_Vector& userUCS,
                                                                               [[maybe_unused]] LC_PropertyChangeReason reason,
                                                                               EntityClass* e) -> void {
                                  const RS_Vector wcsVector = toWCS(userUCS);
                                  const int index = propertyPointIndex->value();
                                  RS_Vector wcsOriginalVertex = funGetPointByIndex(index);
                                  RS_Vector wcsOffset = wcsVector - wcsOriginalVertex;
                                  e->moveRef(wcsOriginalVertex, wcsOffset);
                              }, [this, propertyPointIndex,funGetPointByIndex](const RS_Vector& userUCS,
                                                                               [[maybe_unused]] EntityClass* e) -> bool {
                                  const RS_Vector wcsVector = toWCS(userUCS);
                                  const int index = propertyPointIndex->value();
                                  const RS_Vector wcsOriginalVertex = funGetPointByIndex(index);
                                  return !LC_LineMath::isMeaningfulDistance(wcsVector, wcsOriginalVertex);
                              });

    propertyPoint->setValueStorage(vertexValueStorage, true);
    container->addChildProperty(propertyPoint);
}

template <typename EntityClass>
void LC_IndexedPropertiesProviderBase::createIndexedDoubleProperty(LC_PropertyContainer* container, EntityClass* entity,
                                                                  const LC_Property::Names indexedPointNames,
                                                                  const FunGetDoubleValueByIndex &funGetValueByIndex,
                                                                  const FunSetDoubleValueByIndex &funSetValueByIndex,
                                                                  LC_PropertyInt* propertyPointIndex) {
    auto* propertyPoint = new LC_PropertyDouble(container, false);

    propertyPoint->setNames(indexedPointNames);
    propertyPoint->setViewDescriptorProvider([]() -> LC_PropertyViewDescriptor {
        return LC_PropertyViewDescriptor(LC_PropertyDoubleInteractivePickView::VIEW_NAME, {});
    });

    propertyPoint->setActionContextAndLaterRequestor(m_actionContext, m_widget);
    propertyPoint->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::NOTNEEDED);

    auto vertexValueStorage = new LC_EntityPropertyValueDelegate<double, EntityClass>();
    vertexValueStorage->setup(entity, this->m_widget,
                              [this,propertyPointIndex, funGetValueByIndex]([[maybe_unused]] EntityClass* e) -> double {
                                  const int index = propertyPointIndex->value();
                                  const double result = funGetValueByIndex(index);
                                  return result;
                              }, [this,propertyPointIndex, funSetValueByIndex](const double &v,
                                                                               [[maybe_unused]] LC_PropertyChangeReason reason,
                                                                               [[maybe_unused]] EntityClass* e) -> void {
                                  const int index = propertyPointIndex->value();
                                  funSetValueByIndex(index, v);
                              }, [this, propertyPointIndex,funGetValueByIndex](const double &v, [[maybe_unused]] EntityClass* e) -> bool {
                                  const int index = propertyPointIndex->value();
                                  const double originalValue = funGetValueByIndex(index);
                                  const bool valuesAreEqual = LC_LineMath::isNotMeaningful(originalValue - v);
                                  return valuesAreEqual;
                              });

    propertyPoint->setValueStorage(vertexValueStorage, true);
    container->addChildProperty(propertyPoint);
}

template <typename EntityClass>
LC_PropertyInt* LC_IndexedPropertiesProviderBase::createIndexAndPointProperties(LC_PropertyContainer* container, EntityClass* entity,
                                                                                const LC_Property::Names indexPropertNames,
                                                                                const LC_Property::Names indexedPointName, const int pointCount,
                                                                                const int initialIndexToSet,
                                                                                const FunGetWCSVectorByIndex &funGetPointByIndex,
                                                                                const FunPersistValue &persistIndexValue) {

    LC_PropertyInt* propertyPointIndex = createIndexProperty(container, indexPropertNames, pointCount, funGetPointByIndex, persistIndexValue);

    createIndexedPointProperty<EntityClass>(container, entity, indexedPointName, funGetPointByIndex, propertyPointIndex);

    constexpr LC_PropertyChangeReason reason(PropertyChangeReasonValueLoaded);
    propertyPointIndex->setValue(initialIndexToSet, reason);

    return propertyPointIndex;
}

#endif
