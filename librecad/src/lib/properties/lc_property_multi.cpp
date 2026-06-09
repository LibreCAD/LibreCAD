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

#include "lc_property_multi.h"


LC_PropertyMulti::LC_PropertyMulti(const QMetaObject* propertyMetaObject, QObject* parent)
    : LC_PropertyAtomic(parent), m_propertyMetaObject(propertyMetaObject), m_subPropertyUpdates(0), m_calculateMultipleValues(true),
      m_multipleValues(false) {
    setViewDescriptor(LC_PropertyViewDescriptor());
}

LC_PropertyMulti::~LC_PropertyMulti() {
    for (const auto property : m_properties) {
        disconnect(property, &LC_Property::beforePropertyChange, this, &LC_PropertyMulti::onPropertyWillChange);
        disconnect(property, &LC_Property::afterPropertyChange, this, &LC_PropertyMulti::onPropertyDidChange);
    }

    const auto childProperties = std::move(m_properties);
    for (const auto child : childProperties) {
        if (child->parent() == this) {
            delete child;
        }
    }
}

const QMetaObject* LC_PropertyMulti::propertyMetaObject() const {
    return m_propertyMetaObject;
}

void LC_PropertyMulti::addProperty(LC_PropertyAtomic* property, const bool own) {
    Q_ASSERT(property != nullptr);
    Q_ASSERT(m_propertyMetaObject->cast(property) != nullptr);

    if (own) {
        property->setParent(this);
    }
    if (m_properties.end() != std::find(m_properties.begin(), m_properties.end(), property)) {
        return;
    }
    m_properties.push_back(property);

    if (property->isCollapsed()) {
        collapse();
    }
    updateStateFrom(property);

    connect(property, &LC_Property::beforePropertyChange, this, &LC_PropertyMulti::onPropertyWillChange);
    connect(property, &LC_Property::afterPropertyChange, this, &LC_PropertyMulti::onPropertyDidChange);
}

bool LC_PropertyMulti::hasMultipleValues(){
    const qsizetype size = m_properties.size();
    if (size < 2) {
        m_multipleValues = false;
    }
    else {
        m_multipleValues = false;
        LC_PropertyAtomic* first = getFirstProperty();
        for (qsizetype i = 1; i < size; i++) {
            const auto prop = m_properties.at(i);
            if (!prop->isValueEqualTo(first)) {
                m_multipleValues = true;
                break;
            }
        }
    }
    return m_multipleValues;
}

void LC_PropertyMulti::onPropertyWillChange(const LC_PropertyChangeReason reason, const PropertyValuePtr newValue, const int typeId) {
    if (m_subPropertyUpdates != 0u) {
        return;
    }
    emit beforePropertyChange(reason, newValue, typeId);
}

void LC_PropertyMulti::onPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (m_subPropertyUpdates != 0u) {
        return;
    }

    Q_ASSERT(nullptr != qobject_cast<LC_PropertyAtomic*>(sender()));
    const auto changedProperty = static_cast<LC_PropertyAtomic*>(sender());
    if (m_edited && ((reason & PropertyChangeReasonEdit) != 0u) && ((reason & PropertyChangeReasonValue) != 0u)) {
        // const auto value = changedProperty->valueAsVariant();
        const auto singleReason = reason & ~PropertyChangeReasonMultiEdit;
        m_subPropertyUpdates++;
        for (const auto property : m_properties) {
            if (property != changedProperty && property->isEditableByUser()) {
                // property->fromVariant(value, singleReason);
                property->setValueFrom(changedProperty, singleReason);
            }
        }
        m_subPropertyUpdates--;
    }

    if ((reason & (PropertyChangeReasonState | PropertyChangeReasonValue)) != 0u) {
        updateStateFrom(changedProperty);
        updateMultipleState(true);
    }

    emit afterPropertyChange(reason);
}

void LC_PropertyMulti::updatePropertyState() {
    LC_PropertyAtomic::updatePropertyState();
    if (m_subPropertyUpdates != 0u) {
        return;
    }

    const bool immutable = stateLocal().testFlag(PropertyStateReadonly);
    m_subPropertyUpdates++;
    for (const auto prop : m_properties) {
        prop->setReadOnly(immutable);
    }
    m_subPropertyUpdates--;
}

void LC_PropertyMulti::doOnBeforeMasterPropertyChange(const LC_PropertyChangeReason reason) {
    if (m_subPropertyUpdates != 0u) {
        return;
    }
    LC_PropertyAtomic::doOnBeforeMasterPropertyChange(reason);
}

void LC_PropertyMulti::doOnAfterMasterPropertyChange(const LC_PropertyChangeReason reason) {
    if (m_subPropertyUpdates != 0u) {
        return;
    }
    if ((reason & (PropertyChangeReasonState | PropertyChangeReasonValue)) != 0u) {
        updateMultipleState(true);
    }
    LC_PropertyAtomic::doOnAfterMasterPropertyChange(reason);
}

void LC_PropertyMulti::updateStateFrom(const LC_PropertyAtomic* source) {
    static constexpr LC_PropertyState UNCHANGED_STATE(PropertyStateValueMulti | PropertyStateCollapsed | PropertyStateValueModified);
    auto state = stateLocal() & UNCHANGED_STATE;
    state |= source->stateLocal() & ~UNCHANGED_STATE;

    state &= ~(PropertyStateReadonly |/* PropertyStateResettable |*/ PropertyStateInvisible /*| PropertyStateUnlockable*/);
    for (const auto property : m_properties) {
        auto childState = property->stateLocal();
        if (childState.testFlag(PropertyStateInvisible)) {
            state |= PropertyStateInvisible;
        }

        if (childState.testFlag(PropertyStateReadonly)) {
            state |= PropertyStateReadonly;
        }
    }
    m_subPropertyUpdates++;
    setState(state);
    m_subPropertyUpdates--;
}

void LC_PropertyMulti::updateMultipleState(const bool force) {
    if (force) {
        m_calculateMultipleValues = true;
    }

    const bool multipleValues = hasMultipleValues();
    auto state = stateLocal() & ~PropertyStateValueModified;

    state.setFlag(PropertyStateValueMulti, multipleValues);

    for (const auto prop : m_properties) {
        if (prop->isValueModified()) {
            state |= PropertyStateValueModified;
            break;
        }
    }

    m_subPropertyUpdates++;
    setState(state);
    m_subPropertyUpdates--;
}
