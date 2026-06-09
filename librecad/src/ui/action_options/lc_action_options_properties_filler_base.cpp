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

#include "lc_action_options_properties_filler_base.h"

#include "lc_enum_descriptor.h"
#include "lc_linemath.h"
#include "lc_property_action.h"
#include "lc_property_action_link_view.h"
#include "lc_property_bool_checkbox_view.h"
#include "lc_property_double_interactivepick_view.h"
#include "lc_property_enum.h"
#include "lc_property_enum_combobox_view.h"
#include "lc_property_int_spinbox_view.h"
#include "lc_property_qstring_lineedit_view.h"
#include "lc_property_rsvector_view.h"
#include "rs_settings.h"

template <typename T, typename TProperty>
void LC_ActionOptionsPropertiesFillerBase::setupViewDescriptor(const typename LC_PropertyValueDelegated<T>::FunValueSetShort& funSet,
                                                               const FunPepareDescriptor& funFillViewAttrs, TProperty* property,
                                                               const QByteArray& viewName) {
    bool readonly = false;
    if (funFillViewAttrs != nullptr) {
        LC_PropertyViewDescriptor descriptor;
        descriptor.viewName = viewName;
        readonly = funFillViewAttrs(descriptor);
        property->setViewDescriptor(descriptor);
    }
    if (readonly || funSet == nullptr) {
        property->setReadOnly();
    }
}

void LC_ActionOptionsPropertiesFillerBase::addBoolean(const LC_Property::Names& names,
                                                      const typename LC_PropertyValueDelegated<bool>::FunValueGet& funGet,
                                                      const typename LC_PropertyValueDelegated<bool>::FunValueSetShort& funSet,
                                                      LC_PropertyContainer* cont, const FunPepareDescriptor& funFillViewAttrs) {
    const auto property = new LC_PropertyBool(cont, false);
    property->setNames(names);
    createDelegatedStorage<bool>(property, funGet, funSet);
    setupViewDescriptor<double>(funSet, funFillViewAttrs, property, LC_PropertyBoolCheckBoxView::VIEW_NAME);
    cont->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addDouble(const LC_Property::Names& names,
                                                     const typename LC_PropertyValueDelegated<double>::FunValueGet& funGet,
                                                     const typename LC_PropertyValueDelegated<double>::FunValueSetShort& funSet,
                                                     LC_PropertyContainer* cont, const FunPepareDescriptor& funFillViewAttrs) {
    const auto property = createDoubleProperty(names, cont, LC_ActionContext::InteractiveInputInfo::InputType::NOTNEEDED, m_actionContext,
                                               m_widget);
    createDelegatedStorage<double>(property, funGet, funSet);
    setupViewDescriptor<double>(funSet, funFillViewAttrs, property, LC_PropertyDoubleInteractivePickView::VIEW_NAME);
    cont->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addLinearDistance(const LC_Property::Names& names,
                                                             const typename LC_PropertyValueDelegated<double>::FunValueGet& funGet,
                                                             const typename LC_PropertyValueDelegated<double>::FunValueSetShort& funSet,
                                                             LC_PropertyContainer* cont, const FunPepareDescriptor& funFillViewAttrs) {
    const auto property = createDoubleProperty(names, cont, LC_ActionContext::InteractiveInputInfo::InputType::DISTANCE, m_actionContext,
                                               m_widget);

    createDelegatedStorage<double>(property, funGet, funSet);
    setupViewDescriptor<double>(funSet, funFillViewAttrs, property, LC_PropertyDoubleInteractivePickView::VIEW_NAME);
    cont->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addRawAngle(const LC_Property::Names& names,
                                                       typename LC_PropertyValueDelegated<double>::FunValueGet funGet,
                                                       const typename LC_PropertyValueDelegated<double>::FunValueSetShort& funSet,
                                                       LC_PropertyContainer* cont, const FunPepareDescriptor& funFillViewAttrs) {
    const auto property = createDoubleProperty(names, cont, LC_ActionContext::InteractiveInputInfo::InputType::ANGLE, m_actionContext,
                                               m_widget);

    createDelegatedStorage<double>(property, funGet, funSet, [funGet](const double& v) -> bool {
        return LC_LineMath::isSameAngle(v, funGet());
    });

    setupViewDescriptor<double>(funSet, funFillViewAttrs, property, LC_PropertyDoubleInteractivePickView::VIEW_NAME);
    cont->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addRawAngleDegrees(const LC_Property::Names& names,
                                                              typename LC_PropertyValueDelegated<double>::FunValueGet funGet,
                                                              typename LC_PropertyValueDelegated<double>::FunValueSetShort funSet,
                                                              LC_PropertyContainer* cont, const FunPepareDescriptor& funFillViewAttrs) {
    const auto property = createDoubleProperty(names, cont, LC_ActionContext::InteractiveInputInfo::InputType::ANGLE, m_actionContext,
                                               m_widget);

    auto funGetValue = [funGet]()-> double {
        return RS_Math::deg2rad(funGet());
    };

    const auto funSetValue = funSet == nullptr
                                 ? funSet
                                 : [funSet](double v) -> void {
                                     funSet(RS_Math::rad2deg(v));
                                 };

    createDelegatedStorage<double>(property, funGetValue, funSetValue, [funGet](const double& v) -> bool {
        return LC_LineMath::isSameAngle(v, funGet());
    });
    setupViewDescriptor<double>(funSet, funFillViewAttrs, property, LC_PropertyDoubleInteractivePickView::VIEW_NAME);
    cont->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addWCSAngle(const LC_Property::Names& names,
                                                       typename LC_PropertyValueDelegated<double>::FunValueGet funGet,
                                                       typename LC_PropertyValueDelegated<double>::FunValueSetShort funSet,
                                                       LC_PropertyContainer* cont, const FunPepareDescriptor& funFillViewAttrs) {
    const auto property = createDoubleProperty(names, cont, LC_ActionContext::InteractiveInputInfo::InputType::ANGLE, m_actionContext,
                                               m_widget);

    auto funGetValue = [this, funGet]() -> double {
        const double wcsAngle = funGet();
        const double ucsAngle = toUCSBasisAngle(wcsAngle); // here we return in UCS for editing*/
        return ucsAngle;
    };

    const auto funSetValue = (funSet != nullptr)
                                 ? [this, funSet](const double& value) -> void {
                                     // here we expect value in radians and in ucs
                                     const double ucsBasisAngle = value;
                                     const double wcsAngle = toWCSAngle(ucsBasisAngle);
                                     funSet(wcsAngle);
                                 }
                                 : funSet;

    auto funValueEqual = [funGet](const double& v) -> bool {
        return LC_LineMath::isSameAngle(v, funGet());
    };
    createDelegatedStorage<double>(property, funGetValue, funSetValue, funValueEqual);
    setupViewDescriptor<double>(funSet, funFillViewAttrs, property, LC_PropertyDoubleInteractivePickView::VIEW_NAME);
    cont->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addEnum(const LC_Property::Names& names, const LC_EnumDescriptor* enumDescriptor,
                                                   const typename LC_PropertyValueDelegated<LC_PropertyEnumValueType>::FunValueGet&
                                                   funGetValue,
                                                   const typename LC_PropertyValueDelegated<LC_PropertyEnumValueType>::FunValueSetShort&
                                                   funSetValue, LC_PropertyContainer* container,
                                                   const FunPepareDescriptor& funPrepareDescriptor) {
    const auto property = new LC_PropertyEnum(container, false);
    property->setNames(names);
    property->setEnumInfo(enumDescriptor);

    createDelegatedStorage<LC_PropertyEnumValueType>(property, funGetValue, funSetValue);

    setupViewDescriptor<int>(funSetValue, funPrepareDescriptor, property, LC_PropertyEnumComboBoxView::VIEW_NAME);
    container->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addVector(const LC_Property::Names& names,
                                                     typename LC_PropertyValueDelegated<RS_Vector>::FunValueGet funGet,
                                                     typename LC_PropertyValueDelegated<RS_Vector>::FunValueSetShort funSet,
                                                     LC_PropertyContainer* cont, const FunPepareDescriptor& funFillViewAttrs) {
    const auto property = createVectorProperty(names, cont, m_actionContext, m_widget);

    auto funGetValue = [this, funGet]() -> RS_Vector {
        const RS_Vector wcsVector = funGet();
        const RS_Vector ucsVector = toUCS(wcsVector); // here we return in UCS for editing
        return ucsVector;
    };

    const auto funSetValue = (funSet != nullptr)
                                 ? [this, funSet](const RS_Vector& userUCS) -> void {
                                     const RS_Vector ucsVector = toWCS(userUCS);
                                     funSet(ucsVector);
                                 }
                                 : funSet;

    auto funValueEqual = [this, funGet](const RS_Vector& userUCS) -> bool {
        const auto originalWCS = funGet();
        const auto originalUCS = toUCS(originalWCS);
        return userUCS == originalUCS;
    };

    createDelegatedStorage<RS_Vector>(property, funGetValue, funSetValue, funValueEqual);
    setupViewDescriptor<RS_Vector>(funSetValue, funFillViewAttrs, property, LC_PropertyRSVectorView::VIEW_NAME);
    cont->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addIntSpinbox(const LC_Property::Names& names,
                                                         const typename LC_PropertyValueDelegated<int>::FunValueGet& funGet,
                                                         const typename LC_PropertyValueDelegated<int>::FunValueSetShort& funSet,
                                                         LC_PropertyContainer* container, int minVal, int maxVal,
                                                         const FunPepareDescriptor& funFillViewAttrs) {
    auto* property = new LC_PropertyInt(container, false);
    property->setNames(names);

    LC_PropertyViewDescriptor descriptor(LC_PropertyIntSpinBoxView::VIEW_NAME);
    descriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MIN] = 1;
    descriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_STEP] = minVal;
    if (maxVal > 0) {
        descriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MAX] = maxVal;
    }
    property->setViewDescriptor(descriptor);

    createDelegatedStorage<LC_PropertyEnumValueType>(property, funGet, funSet);

    setupViewDescriptor<int>(funSet, funFillViewAttrs, property, LC_PropertyIntSpinBoxView::VIEW_NAME);
    container->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::addString(const LC_Property::Names& names,
                                                     const typename LC_PropertyValueDelegated<QString>::FunValueGet& funGet,
                                                     const typename LC_PropertyValueDelegated<QString>::FunValueSetShort& funSet,
                                                     LC_PropertyContainer* container, bool multiLine,
                                                     const FunPepareDescriptor& funPrepareDescriptor) const {
    const auto property = new LC_PropertyQString(container, false);
    property->setNames(names);
    LC_PropertyViewDescriptor viewDescriptor;
    viewDescriptor.viewName = LC_PropertyQStringLineEditView::VIEW_NAME;
    viewDescriptor.attributes[LC_PropertyQStringLineEditView::ATTR_MULTILINE_EDIT] = multiLine;
    property->setViewDescriptor(viewDescriptor);

    createDelegatedStorage<QString>(property, funGet, funSet);

    bool readonly = false;
    if (funPrepareDescriptor != nullptr) {
        readonly = funPrepareDescriptor(viewDescriptor);
    }
    if (readonly || funSet == nullptr) {
        property->setReadOnly();
    }
    container->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::createCommandsLine(LC_PropertyContainer* container, const QString& propertyName,
                                                              const QString& linkTitle, const QString& linkTooltip,
                                                              const QString& linkTitleRight, const QString& linkTooltipRight,
                                                              const std::function<void(int linkIndex)>& clickHandler,
                                                              const QString& commonDescription, bool leftEnabled, bool rightEnabled) {
    auto* property = new LC_PropertyAction(container, true);
    property->setName(propertyName);
    property->setDisplayName("");
    LC_PropertyViewDescriptor viewDescriptor("Link");
    viewDescriptor[LC_PropertyActionLinkView::ATTR_TITLE] = linkTitle;
    viewDescriptor[LC_PropertyActionLinkView::ATTR_TOOLTIP_LEFT] = linkTooltip;
    viewDescriptor[LC_PropertyActionLinkView::ATTR_ENABLED_LEFT] = leftEnabled;
    if (!linkTitleRight.isEmpty()) {
        viewDescriptor[LC_PropertyActionLinkView::ATTR_TITLE_RIGHT] = linkTitleRight;
        viewDescriptor[LC_PropertyActionLinkView::ATTR_TOOLTIP_RIGHT] = linkTooltipRight;
        viewDescriptor[LC_PropertyActionLinkView::ATTR_ENABLED_RIGHT] = rightEnabled;
    }

    property->setClickHandler([clickHandler](const LC_PropertyAction*, int linkIndex) {
        QTimer::singleShot(10, [linkIndex, clickHandler] {
            clickHandler(linkIndex);
        });
    });
    property->setDescription(commonDescription);
    property->setViewDescriptor(viewDescriptor);
    container->addChildProperty(property);
}

void LC_ActionOptionsPropertiesFillerBase::doUpdateByAction(RS_ActionInterface* a) {
    m_action = a;
}

void LC_ActionOptionsPropertiesFillerBase::preSetupByAction(RS_ActionInterface* a) {
    m_actionContext = a->getActionContext();
}

bool LC_ActionOptionsPropertiesFillerBase::hasSnapOptions() {
    const auto snapMode = m_action->getSnapMode();
    return snapMode->snapDistance || snapMode->snapMiddle;
}

void LC_ActionOptionsPropertiesFillerBase::fillSnapToolOptionsContainer(LC_PropertyContainer* propertyContainer) {
    const auto snapMode = m_action->getSnapMode();
    if (snapMode->snapDistance) {
        addLinearDistance({"a_snapDistance", tr("Snap Distance"), tr("Distance of snap point from initially resolved snap point")},
                          []()-> double {
                              const QString distance = LC_GET_ONE_STR("Snap", "Distance", "1.0");
                              double dist = RS_Math::eval(distance, 1.0);
                              return dist;
                          }, [this](double val)-> void {
                              QString value = QString::number(val, 'g', 6);
                              LC_SET_ONE("Snap", "Distance", value);
                              double dist = NAN;
                              m_actionContext->requestSnapDistOptions(&dist, true);
                          }, propertyContainer);
    }

    if (snapMode->snapMiddle) {
        addIntSpinbox({"a_snapMiddleNum", tr("Snap middle"), tr("Number of equidistant division points")}, []()-> int {
                          int points = LC_GET_ONE_INT("Snap", "MiddlePoints", 1);
                          if (!(points >= 1 && points <= 99)) {
                              points = 1;
                          }
                          return points;
                      }, [this](int val)-> void {
                          LC_SET_ONE("Snap", "MiddlePoints", val);
                          int mpoints = 0;
                          m_actionContext->requestSnapMiddleOptions(&mpoints, true);
                      }, propertyContainer);
    }
}
