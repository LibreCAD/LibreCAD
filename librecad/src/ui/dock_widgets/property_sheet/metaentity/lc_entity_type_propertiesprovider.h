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

#ifndef LC_ENTITYTYPEPROPERTIESPROVIDER_H
#define LC_ENTITYTYPEPROPERTIESPROVIDER_H

#include "lc_entitypropertyvaluedelegate.h"
#include "lc_formatter.h"
#include "lc_linemath.h"
#include "lc_property_bool_checkbox_view.h"
#include "lc_property_container.h"
#include "lc_property_container_builder.h"
#include "lc_property_enum.h"
#include "lc_property_int.h"
#include "lc_property_int_spinbox_view.h"
#include "lc_property_qstring.h"
#include "lc_property_qstring_font_combobox_view.h"
#include "lc_property_qstring_list_combobox_view.h"
#include "lc_property_rsvector.h"
#include "lc_propertyprovider_utils.h"
#include "lc_propertysheetwidget.h"
#include "rs.h"
#include "rs_entity.h"

class LC_EnumDescriptor;
class LC_PropertySheetWidget;
class LC_ActionContext;
class RS_Document;

class LC_EntityTypePropertiesProvider : public LC_PropertyContainerBuilder {
    Q_OBJECT public:
    using FunCreateGenericProperty = typename std::function<void(const LC_Property::Names& names, RS_Entity* entity,
                                                                 LC_PropertyContainer* container, QList<LC_PropertyAtomic*>*)>;

    LC_EntityTypePropertiesProvider(const RS2::EntityType entityType, LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_PropertyContainerBuilder(actionContext,widget), m_entityType{entityType} {
    }

    static const QString SECTION_GENERAL;
    static const QString SECTION_GEOMETRY;
    static const QString SECTION_CALCULATED_INFO;
    static const QString SECTION_SINGLE_ENTITY_ACTIONS;
    static const QString SECTION_MULTI_ENTITY_ACTIONS;
    static const QString SECTION_TEXT;
    static const QString SECTION_TOOL_OPTIONS;

    void fillEntityProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& entitiesList);
protected:

    RS2::EntityType m_entityType;

    virtual void fillSelectedSetCommands(LC_PropertyContainer* container, const QList<RS_Entity*>& entitiesList);
    virtual void fillComputedProperites(LC_PropertyContainer* container, const QList<RS_Entity*>& entitiesList);
    virtual void fillSingleEntityCommands(LC_PropertyContainer* container, const QList<RS_Entity*>& entitiesList);

    virtual void doCreateCalculatedProperties([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& list) {}
    virtual void doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity);
    virtual void doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) = 0;
    virtual void doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list);


    bool isShowLinks() const {
        return m_widget->getOptions()->showLinks;
    }

    bool isShowSingleEntitySection() const {
        return m_widget->getOptions()->showSingleEntityCommands;
    }

    bool isShowComputedSection() const {
        return m_widget->getOptions()->showComputed;
    }

    template <typename EntityType>
    void add(const LC_Property::Names& names,
             std::function<void(const LC_Property::Names& names, EntityType* entity, LC_PropertyContainer* container,
                                QList<LC_PropertyAtomic*>*)> propertyInit, const QList<RS_Entity*>& list, LC_PropertyContainer* cont);

    template <class ValueType, class EntityClass>
    void createDelegatedStorage(typename LC_EntityPropertyValueDelegate<ValueType, EntityClass>::FunValueGet funGet,
                                typename LC_EntityPropertyValueDelegate<ValueType, EntityClass>::FunValueSetShort funSet,
                                typename LC_EntityPropertyValueDelegate<ValueType, EntityClass>::FunValueEqual funEqual,
                                EntityClass* entity, LC_PropertySingle<ValueType>* property);

    template <class EntityClass>
    void createEntityContextCommand(LC_PropertyContainer* container, const QString& propertyName, RS2::ActionType actionType,
                                    const QString& linkTitle, const QString& linkTooltip, RS2::ActionType actionTypeRight,
                                    const QString& linkTitleRight, const QString& linkTooltipRight, EntityClass* entity, const QString& commonDescription,
                                    bool setContextEntity = true);



    template <class EntityClass>
    void createEntityContextCommands(const std::list<CommandLinkInfo>& links, LC_PropertyContainer* container, EntityClass* entity, const QString& namePrefix, bool setContextEntity = true);


    template <class EntityClass>
    void doCreateDelegatedVector(const LC_Property::Names& names,
                                 typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueGet funGet,
                                 typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueSetShort funSet,
                                 RS2::EntityType entityType, const QList<RS_Entity*>& list, LC_PropertyContainer* cont);

    void addCommon(const LC_Property::Names& names, const FunCreateGenericProperty& propertyInit, const QList<RS_Entity*>& list,
                   LC_PropertyContainer* cont);

    void addMultipleProperties(LC_PropertyContainer* cont, QList<LC_PropertyAtomic*> props);

    void fillGenericAttributes(LC_PropertyContainer* container, const QList<RS_Entity*>& list);

    template <typename EntityClass>
    void addVector(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueGet funGet,
                   typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueSetShort funSet, const QList<RS_Entity*>& list,
                   LC_PropertyContainer* cont) {
        doCreateDelegatedVector<EntityClass>(names, funGet, funSet, m_entityType, list, cont);
    }

    template <typename EntityClass>
    void addReadOnlyVector(const LC_Property::Names& names,
                           typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueGet funGet,
                           const QList<RS_Entity*>& list, LC_PropertyContainer* cont);

    template <typename EntityClass>
    void addLinearDistance(const LC_Property::Names& names,
                           typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                           typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort funSet,
                           const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                           std::function<void(LC_PropertyViewDescriptor*)> funFillViewAttrs = nullptr);

    template <typename EntityClass>
    void addDouble(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                   typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort funSet, const QList<RS_Entity*>& list,
                   LC_PropertyContainer* cont, std::function<bool(EntityClass*, LC_PropertyViewDescriptor&)> funFillViewAttrs = nullptr);

    template <typename EntityClass>
    void addWCSAngle(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                     typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort funSet, const QList<RS_Entity*>& list,
                     LC_PropertyContainer* cont);

    template <typename EntityClass>
    void addRawAngle(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                     typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort funSet, const QList<RS_Entity*>& list,
                     LC_PropertyContainer* cont);

    template <typename EntityClass>
    void addBoolean(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<bool, EntityClass>::FunValueGet funGet,
                    typename LC_EntityPropertyValueDelegate<bool, EntityClass>::FunValueSetShort funSet, const QList<RS_Entity*>& list,
                    LC_PropertyContainer* cont, const QString& viewName = LC_PropertyBoolCheckBoxView::VIEW_NAME,
                    std::function<bool(EntityClass*, LC_PropertyViewDescriptor& descriptor)> funPrepareDescriptor = nullptr);

    template <typename EntityClass>
    void addStringList(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueGet funGet,
                       typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueSetShort funSet,
                       std::function<bool(EntityClass*, LC_PropertyViewDescriptor& descriptor)> funFillList,
                       // fixme - this may be expanded to support item icon, display name and data + change in View
                       const QList<RS_Entity*>& list, LC_PropertyContainer* cont);

    template <class EntityClass>
    void addStringFont(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueGet funGet,
                       typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueSetShort funSet,
                       const QList<RS_Entity*>& list, LC_PropertyContainer* cont);

    template <class EntityClass>
    void addString(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueGet funGet,
                   typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueSetShort funSet, const QList<RS_Entity*>& list,
                   LC_PropertyContainer* cont, bool multiLine,
                   std::function<bool(EntityClass*, LC_PropertyViewDescriptor& descriptor)> funPrepareDescriptor = nullptr);

    template <typename EntityClass>
    void addIntSpinbox(const LC_Property::Names& names, typename LC_EntityPropertyValueDelegate<int, EntityClass>::FunValueGet funGet,
                       typename LC_EntityPropertyValueDelegate<int, EntityClass>::FunValueSetShort funSet, const QList<RS_Entity*>& list,
                       LC_PropertyContainer* cont, int minVal = 1, int maxVal = -1);

    template <typename EntityClass>
    void addEnum(const LC_Property::Names& names, const LC_EnumDescriptor* enumDescriptor,
                 typename LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>::FunValueGet funGetValue,
                 typename LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>::FunValueSetShort funSetValue,
                 const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                 std::function<bool(EntityClass*, LC_PropertyViewDescriptor& descriptor)> funPrepareDescriptor = nullptr);

    template <typename EntityClass>
    void addVarEnum(const LC_Property::Names& names, std::function<LC_EnumDescriptor*(EntityClass*)> funEnumDescriptorProvider,
                    typename LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>::FunValueGet funGetValue,
                    typename LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>::FunValueSetShort funSetValue,
                    const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                    std::function<bool(EntityClass*, LC_PropertyViewDescriptor&)> funPrepareDescriptor = nullptr);

    template <typename EntityClass>
    void addReadOnlyString(const LC_Property::Names& names, std::function<QString(EntityClass*)> funValue, const QList<RS_Entity*>& list,
                           LC_PropertyContainer* cont);

    LC_PropertyContainer* createGeometrySection(LC_PropertyContainer* container) const;
    LC_PropertyContainer* createTextContainer(LC_PropertyContainer* container) const;
    LC_PropertyContainer* createCalculatedInfoSection(LC_PropertyContainer* container) const;
    LC_PropertyContainer* createSingleEntityActionsSection(LC_PropertyContainer* container) const;
    LC_PropertyContainer* createMultipleEntityActionsSection(LC_PropertyContainer* container) const;
};

template <typename EntityType>
void LC_EntityTypePropertiesProvider::add(const LC_Property::Names& names,
                                          std::function<void(const LC_Property::Names& names, EntityType* entity,
                                                             LC_PropertyContainer* container, QList<LC_PropertyAtomic*>*)> propertyInit,
                                          const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    QList<LC_PropertyAtomic*> props;
    props.reserve(list.size());
    for (const auto entity : list) {
        if (entity->rtti() != m_entityType) {
            continue;
        }
        auto typedEntity = static_cast<EntityType*>(entity);
        propertyInit(names, typedEntity, cont, &props);
    }
    addMultipleProperties(cont, props);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addLinearDistance(const LC_Property::Names& names,
                                                        typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                                                        typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort
                                                        funSet, const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                                        std::function<void(LC_PropertyViewDescriptor*)> funFillViewAttrs) {
    add<EntityClass>(names, [this, funGet, funSet,funFillViewAttrs](const LC_Property::Names& n, EntityClass* entity,
                                                                    LC_PropertyContainer* container,
                                                                    QList<LC_PropertyAtomic*>* props) -> void {
        auto property = createDoubleProperty(n, props, container, LC_ActionContext::InteractiveInputInfo::InputType::DISTANCE,
                                             m_actionContext, m_widget);
        if (funFillViewAttrs != nullptr) {
            LC_PropertyViewDescriptor descriptor;
            funFillViewAttrs(&descriptor);
            property->setViewDescriptor(descriptor);
        }

        auto valueStorage = new LC_EntityPropertyValueDelegate<double, EntityClass>();
        property->setValueStorage(valueStorage, true);
        valueStorage->setup(entity, m_widget, funGet, funSet, [funGet](const double& v, EntityClass* e) -> bool {
            return LC_LineMath::isSameLength(v, funGet(e));
        });
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addDouble(const LC_Property::Names& names,
                                                typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                                                typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort funSet,
                                                const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                                std::function<bool(EntityClass*, LC_PropertyViewDescriptor&)> funFillViewAttrs) {
    add<EntityClass>(names, [this, funGet, funSet,funFillViewAttrs](const LC_Property::Names& n, EntityClass* entity,
                                                                    LC_PropertyContainer* container,
                                                                    QList<LC_PropertyAtomic*>* props) -> void {
        auto property = createDoubleProperty(n, props, container, LC_ActionContext::InteractiveInputInfo::InputType::NOTNEEDED,
                                             m_actionContext, m_widget);
        bool readonly = false;
        if (funFillViewAttrs != nullptr) {
            LC_PropertyViewDescriptor descriptor;
            readonly = funFillViewAttrs(entity, descriptor);
            property->setViewDescriptor(descriptor);
        }

        auto valueStorage = new LC_EntityPropertyValueDelegate<double, EntityClass>();
        property->setValueStorage(valueStorage, true);
        valueStorage->setup(entity, m_widget, funGet, funSet, [funGet](const double& v, EntityClass* e) -> bool {
            return LC_LineMath::isSameLength(v, funGet(e));
        });
        if (readonly || funSet == nullptr) {
            property->setReadOnly();
        }
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addBoolean(const LC_Property::Names& names,
                                                 typename LC_EntityPropertyValueDelegate<bool, EntityClass>::FunValueGet funGet,
                                                 typename LC_EntityPropertyValueDelegate<bool, EntityClass>::FunValueSetShort funSet,
                                                 const QList<RS_Entity*>& list, LC_PropertyContainer* cont, const QString& viewName,
                                                 std::function<bool(EntityClass*, LC_PropertyViewDescriptor& descriptor)>
                                                 funPrepareDescriptor) {
    add<EntityClass>(names, [this, funGet, funSet, viewName,funPrepareDescriptor](const LC_Property::Names& n, EntityClass* entity,
                                                                                  LC_PropertyContainer* container,
                                                                                  QList<LC_PropertyAtomic*>* props) -> void {
        auto property = new LC_PropertyBool(container, false);
        property->setNames(n);

        LC_PropertyViewDescriptor descriptor(viewName.toLatin1());
        bool readOnly = false;
        if (funPrepareDescriptor != nullptr) {
            readOnly = funPrepareDescriptor(entity, descriptor);
        }
        property->setViewDescriptor(descriptor);
        props->push_back(property);
        createDelegatedStorage<bool, EntityClass>(funGet, funSet, [funGet](bool& v, EntityClass* e) -> bool {
            return v == funGet(e);
        }, entity, property);
        if (funSet == nullptr || readOnly) {
            property->setReadOnly();
        }
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addReadOnlyString(const LC_Property::Names& names, std::function<QString(EntityClass*)> funValue,
                                                        const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<EntityClass>(names, [this, funValue](const LC_Property::Names& n, EntityClass* e, LC_PropertyContainer* container,
                                             QList<LC_PropertyAtomic*>* props) -> void {
        const QString value = funValue(e);
        createReadonlyStringProperty(n, props, container, value);
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addStringList(const LC_Property::Names& names,
                                                    typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueGet funGet,
                                                    typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueSetShort funSet,
                                                    std::function<bool(EntityClass*, LC_PropertyViewDescriptor&)> funFillList,
                                                    const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<EntityClass>(names, [this, funFillList, funGet, funSet](const LC_Property::Names& n, EntityClass* entity,
                                                                LC_PropertyContainer* container, QList<LC_PropertyAtomic*>* props) -> void {
        auto property = new LC_PropertyQString(container, false);
        property->setNames(n);
        LC_PropertyViewDescriptor descriptor(LC_PropertyQStringListComboBoxView::VIEW_NAME);
        bool readonly = funFillList(entity, descriptor);
        property->setViewDescriptor(descriptor);
        props->push_back(property);
        createDelegatedStorage<QString, EntityClass>(funGet, funSet, [funGet](QString& v, EntityClass* e) -> bool {
            return v == funGet(e);
        }, entity, property);
        if (readonly || funSet == nullptr) {
            property->setReadOnly();
        }
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addStringFont(const LC_Property::Names& names,
                                                    typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueGet funGet,
                                                    typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueSetShort funSet,
                                                    const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<EntityClass>(names, [this, funGet, funSet](const LC_Property::Names& n, EntityClass* entity, LC_PropertyContainer* container,
                                                   QList<LC_PropertyAtomic*>* props) -> void {
        auto property = new LC_PropertyQString(container, false);
        property->setNames(n);
        LC_PropertyViewDescriptor viewDescriptor;
        viewDescriptor.viewName = LC_PropertyQStringFontComboboxView::VIEW_NAME;
        property->setViewDescriptor(viewDescriptor);
        props->push_back(property);

        auto valueStorage = new LC_EntityPropertyValueDelegate<QString, EntityClass>();
        property->setValueStorage(valueStorage, true);
        valueStorage->setup(entity, this->m_widget, funGet, funSet, [funGet](QString& v, EntityClass* e) -> bool {
            return v == funGet(e);
        });
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addString(const LC_Property::Names& names,
                                                typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueGet funGet,
                                                typename LC_EntityPropertyValueDelegate<QString, EntityClass>::FunValueSetShort funSet,
                                                const QList<RS_Entity*>& list, LC_PropertyContainer* cont, bool multiLine,
                                                std::function<bool(EntityClass*, LC_PropertyViewDescriptor&)> funPrepareDescriptor) {
    add<EntityClass>(names, [this, funGet, funSet, multiLine, funPrepareDescriptor](const LC_Property::Names& n, EntityClass* entity,
                                                                                    LC_PropertyContainer* container,
                                                                                    QList<LC_PropertyAtomic*>* props) -> void {
        auto property = new LC_PropertyQString(container, false);
        property->setNames(n);
        LC_PropertyViewDescriptor viewDescriptor;
        viewDescriptor.viewName = LC_PropertyQStringLineEditView::VIEW_NAME;
        viewDescriptor.attributes[LC_PropertyQStringLineEditView::ATTR_MULTILINE_EDIT] = multiLine;
        property->setViewDescriptor(viewDescriptor);
        props->push_back(property);

        auto valueStorage = new LC_EntityPropertyValueDelegate<QString, EntityClass>();
        property->setValueStorage(valueStorage, true);
        valueStorage->setup(entity, this->m_widget, funGet, funSet, [funGet](QString& v, EntityClass* e) -> bool {
            return v == funGet(e);
        });

        bool readonly = false;
        if (funPrepareDescriptor != nullptr) {
            readonly = funPrepareDescriptor(entity, viewDescriptor);
        }
        if (readonly || funSet == nullptr) {
            property->setReadOnly();
        }
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addWCSAngle(const LC_Property::Names& names,
                                                  typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                                                  typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort funSet,
                                                  const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<EntityClass>(names, [this, funGet, funSet](const LC_Property::Names& n, EntityClass* entity, LC_PropertyContainer* container,
                                                   QList<LC_PropertyAtomic*>* props) -> void {
        auto property = createDoubleProperty(n, props, container, LC_ActionContext::InteractiveInputInfo::InputType::ANGLE, m_actionContext,
                                             m_widget);
        auto valueStorage = new LC_EntityPropertyValueDelegate<double, EntityClass>();
        valueStorage->setup(entity, m_widget, [this, funGet](EntityClass* e) -> double {
                                const double wcsAngle = funGet(e);
                                const double ucsAngle = toUCSBasisAngle(wcsAngle); // here we return in UCS for editing*/
                                return ucsAngle;
                            }, (funSet != nullptr)
                                   ? [this, funSet](const double& value,
                                                    EntityClass* e) -> void {
                                       // here we expect value in radians and in ucs
                                       const double ucsBasisAngle = value;
                                       double wcsAngle = toWCSAngle(ucsBasisAngle);
                                       funSet(wcsAngle,  e);
                                   }
                                   : funSet, [funGet](const double& v, EntityClass* e) -> bool {
                                return LC_LineMath::isSameAngle(v, funGet(e));
                            });
        if (funSet == nullptr) {
            property->setReadOnly();
        }
        property->setValueStorage(valueStorage, true);
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addRawAngle(const LC_Property::Names& names,
                                                  typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueGet funGet,
                                                  typename LC_EntityPropertyValueDelegate<double, EntityClass>::FunValueSetShort funSet,
                                                  const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<EntityClass>(names, [this, funGet, funSet](const LC_Property::Names& n, EntityClass* entity, LC_PropertyContainer* container,
                                                   QList<LC_PropertyAtomic*>* props) -> void {
        auto property = createDoubleProperty(n, props, container, LC_ActionContext::InteractiveInputInfo::InputType::ANGLE, m_actionContext,
                                             m_widget);
        auto valueStorage = new LC_EntityPropertyValueDelegate<double, EntityClass>();
        valueStorage->setup(entity, m_widget, funGet, funSet, [funGet](const double& v, EntityClass* e) -> bool {
            return LC_LineMath::isSameAngle(v, funGet(e));
        });
        property->setValueStorage(valueStorage, true);
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addReadOnlyVector(const LC_Property::Names& names,
                                                        typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueGet funGet,
                                                        const QList<RS_Entity*>& list, LC_PropertyContainer* cont) {
    add<EntityClass>(names, [this, funGet](const LC_Property::Names& n, EntityClass* e, LC_PropertyContainer* container,
                                           QList<LC_PropertyAtomic*>* props) -> void {
        auto property = createVectorProperty(n, props, container, m_actionContext, m_widget);
        auto valueStorage = new LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>();
        property->setValueStorage(valueStorage, true);
        typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueSetShort funSet = nullptr;
        valueStorage->setup(e, m_widget, funGet, funSet, nullptr);
        property->setReadOnly();
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::doCreateDelegatedVector(const LC_Property::Names& names,
                                                              typename LC_EntityPropertyValueDelegate<RS_Vector, EntityClass>::FunValueGet
                                                              funGet,
                                                              typename LC_EntityPropertyValueDelegate<
                                                                  RS_Vector, EntityClass>::FunValueSetShort funSet,
                                                              [[maybe_unused]] RS2::EntityType entityType, const QList<RS_Entity*>& list,
                                                              LC_PropertyContainer* cont) {
    add<EntityClass>(names, [this, funGet, funSet](const LC_Property::Names& n, EntityClass* entity, LC_PropertyContainer* container,
                                                   QList<LC_PropertyAtomic*>* props) -> void {
        auto property = createVectorProperty(n, props, container, m_actionContext, m_widget);
        createDelegatedStorage([this, funGet](EntityClass* e) -> RS_Vector {
                                   const RS_Vector wcsVector = funGet(e);
                                   const RS_Vector ucsVector = toUCS(wcsVector); // here we return in UCS for editing
                                   return ucsVector;
                               }, (funSet != nullptr)
                                      ? [this, funSet](const RS_Vector& userUCS, EntityClass* e) -> void {
                                          RS_Vector ucsVector = toWCS(userUCS);
                                          funSet(ucsVector, e);
                                      }
                                      : funSet, [this, funGet](RS_Vector& userUCS, EntityClass* e) -> bool {
                                   auto originalWCS = funGet(e);
                                   auto originalUCS = toUCS(originalWCS);
                                   return userUCS == originalUCS;
                               }, entity, property);
        if (funSet == nullptr) {
            property->setReadOnly();
        }
    }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addEnum(const LC_Property::Names& names, const LC_EnumDescriptor* enumDescriptor,
                                              typename LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>::FunValueGet
                                              funGetValue,
                                              typename LC_EntityPropertyValueDelegate<
                                                  LC_PropertyEnumValueType, EntityClass>::FunValueSetShort funSetValue,
                                              const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                              std::function<bool(EntityClass*, LC_PropertyViewDescriptor& descriptor)>
                                              funPrepareDescriptor) {
    add<EntityClass>(names, [this, funGetValue, funSetValue, funPrepareDescriptor, enumDescriptor](
                     const LC_Property::Names& n, EntityClass* entity, LC_PropertyContainer* container,
                     QList<LC_PropertyAtomic*>* props) -> void {
                         auto property = new LC_PropertyEnum(container, false);
                         property->setNames(n);
                         property->setEnumInfo(enumDescriptor);
                         props->push_back(property);
                         auto valueStorage = new LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>();
                         property->setValueStorage(valueStorage, true);
                         valueStorage->setup(entity, this->m_widget, funGetValue, funSetValue,
                                             [funGetValue](LC_PropertyEnumValueType& v, EntityClass* e) -> bool {
                                                 return v == funGetValue(e);
                                             });

                         bool readonly = false;
                         if (funPrepareDescriptor != nullptr) {
                             LC_PropertyViewDescriptor descriptor;
                             readonly = funPrepareDescriptor(entity, descriptor);
                             property->setViewDescriptor(descriptor);
                         }
                         if (readonly || funSetValue == nullptr) {
                             property->setReadOnly();
                         }
                     }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addVarEnum(const LC_Property::Names& names,
                                                 std::function<LC_EnumDescriptor*(EntityClass*)> funEnumDescriptorProvider,
                                                 typename LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>::FunValueGet
                                                 funGetValue,
                                                 typename LC_EntityPropertyValueDelegate<
                                                     LC_PropertyEnumValueType, EntityClass>::FunValueSetShort funSetValue,
                                                 const QList<RS_Entity*>& list, LC_PropertyContainer* cont,
                                                 std::function<bool(EntityClass*, LC_PropertyViewDescriptor&)> funPrepareDescriptor) {
    add<EntityClass>(names, [this, funGetValue, funSetValue, funEnumDescriptorProvider, funPrepareDescriptor](
                     const LC_Property::Names& n, EntityClass* entity, LC_PropertyContainer* container,
                     QList<LC_PropertyAtomic*>* props) -> void {
                         auto property = new LC_PropertyEnum(container, false);
                         property->setNames(n);
                         auto enumInfo = funEnumDescriptorProvider(entity);
                         property->setEnumInfo(enumInfo);
                         props->push_back(property);

                         auto valueStorage = new LC_EntityPropertyValueDelegate<LC_PropertyEnumValueType, EntityClass>();
                         property->setValueStorage(valueStorage, true);
                         valueStorage->setup(entity, this->m_widget, funGetValue, funSetValue,
                                             [funGetValue](LC_PropertyEnumValueType& v, EntityClass* e) -> bool {
                                                 return v == funGetValue(e);
                                             });

                         bool readonly = false;
                         if (funPrepareDescriptor != nullptr) {
                             LC_PropertyViewDescriptor descriptor;
                             readonly = funPrepareDescriptor(entity, descriptor);
                             property->setViewDescriptor(descriptor);
                         }
                         if (readonly || funSetValue == nullptr) {
                             property->setReadOnly();
                         }
                     }, list, cont);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::addIntSpinbox(const LC_Property::Names& names,
                                                    typename LC_EntityPropertyValueDelegate<int, EntityClass>::FunValueGet funGet,
                                                    typename LC_EntityPropertyValueDelegate<int, EntityClass>::FunValueSetShort funSet,
                                                    const QList<RS_Entity*>& list, LC_PropertyContainer* cont, int minVal, int maxVal) {
    add<EntityClass>(names, [this, funGet, funSet,minVal, maxVal](const LC_Property::Names& n, EntityClass* entity, LC_PropertyContainer* container,
                                                   QList<LC_PropertyAtomic*>* props) -> void {
        auto* property = new LC_PropertyInt(container, false);
        property->setNames(n);
        props->push_back(property);

        LC_PropertyViewDescriptor descriptor(LC_PropertyIntSpinBoxView::VIEW_NAME);
        descriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MIN] = 1;
        descriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_STEP] = minVal;
        if (maxVal > 0) {
            descriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MAX] = maxVal;
        }
        property->setViewDescriptor(descriptor);

        auto valueStorage = new LC_EntityPropertyValueDelegate<int, EntityClass>();
        valueStorage->setup(entity, m_widget, funGet, funSet, [this, funGet](int& v, EntityClass* e) -> bool {
            return v == funGet(e);
        });
        property->setValueStorage(valueStorage, true);
    }, list, cont);
}

template <typename ValueType, typename EntityClass>
void LC_EntityTypePropertiesProvider::createDelegatedStorage(
    typename LC_EntityPropertyValueDelegate<ValueType, EntityClass>::FunValueGet funGet,
    typename LC_EntityPropertyValueDelegate<ValueType, EntityClass>::FunValueSetShort funSet,
    typename LC_EntityPropertyValueDelegate<ValueType, EntityClass>::FunValueEqual funEqual, EntityClass* entity,
    LC_PropertySingle<ValueType>* property) {
    auto valueStorage = new LC_EntityPropertyValueDelegate<ValueType, EntityClass>();
    valueStorage->setup(entity, this->m_widget, funGet, funSet, funEqual);
    property->setValueStorage(valueStorage, true);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::createEntityContextCommand(LC_PropertyContainer* container, const QString& propertyName, RS2::ActionType actionType, const QString& linkTitle,
                                                  const QString& linkTooltip, RS2::ActionType actionTypeRight,  const QString& linkTitleRight,
                                                  const QString& linkTooltipRight,  EntityClass* entity,
                                                  const QString &commonDescription, bool setContextEntity) {
    auto clickHandler = [this, actionType, actionTypeRight, setContextEntity]([[maybe_unused]] EntityClass* ent, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                if (setContextEntity) {
                    m_actionContext->saveContextMenuActionContext(ent, ent->getMiddlePoint(), false);
                }
                m_actionContext->setCurrentAction(actionType, nullptr);
                break;
            }
            case 1: {
                if (setContextEntity) {
                    m_actionContext->saveContextMenuActionContext(ent, ent->getMiddlePoint(), false);
                }
                m_actionContext->setCurrentAction(actionTypeRight, nullptr);
                break;
            }
            default:
                break;
        }
    };
    LC_PropertyProviderUtils::createSingleEntityCommand<EntityClass>(container, propertyName, linkTitle,
                                                                   linkTooltip, linkTitleRight,
                                                                   linkTooltipRight, entity,
                                                                   clickHandler, commonDescription);
}

template <typename EntityClass>
void LC_EntityTypePropertiesProvider::createEntityContextCommands(const std::list<CommandLinkInfo>& links, LC_PropertyContainer* container, EntityClass* entity, const QString &namePrefix,
    bool setContextEntity) {
    int idx = 0;
    for (const auto &i: links) {
        QString propertyName = QString("%1_%2").arg(namePrefix).arg(idx);
        createEntityContextCommand(container, propertyName, i.leftLink.actionType, i.leftLink.title, i.leftLink.tooltip,
                i.rightLink.actionType, i.rightLink.title, i.rightLink.tooltip, entity, i.description, setContextEntity);
        idx++;
    }
}

#endif
