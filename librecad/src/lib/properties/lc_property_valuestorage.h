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

#ifndef LC_PROPERTYVALUESTORAGE_H
#define LC_PROPERTYVALUESTORAGE_H

#include "lc_property.h"
#include "lc_property_atomic.h"

template <typename ValueType>
class LC_PropertyValueStorage {
public:
    virtual ~LC_PropertyValueStorage() = default;
    using ValueTypeStore = std::decay_t<ValueType>;
    using EqPred = std::equal_to<ValueType>;

    virtual ValueType doGetValue() const = 0;
    virtual void doSetValue(ValueType newValue, [[maybe_unused]] LC_PropertyChangeReason reason) = 0;

    virtual bool doAcceptValue([[maybe_unused]] ValueType valueToAccept) {
        return true;
    }

    virtual bool doCheckValueEqualToCurrent(ValueType valueToCompare) {
        return EqPred()(valueToCompare, doGetValue());
    }

    virtual bool isDelegated() const {
        return true;
    }
};

template <typename ValueType>
class LC_PropertyValueHolder : public LC_PropertyValueStorage<ValueType> {
public:
    using ValueTypeStore = typename LC_PropertyValueStorage<ValueType>::ValueTypeStore;

    bool isDelegated() const override {
        return false;
    }

    void doSetValue(ValueType newValue, [[maybe_unused]] LC_PropertyChangeReason reason) override {
        m_value = newValue;
    }

    ValueType doGetValue() const override {
        return m_value;
    }

protected:
    explicit LC_PropertyValueHolder(ValueType value)
        : m_value(value) {
    }

    LC_PropertyValueHolder()
        : m_value(ValueTypeStore()) {
    }

private:
    ValueTypeStore m_value;
};

template <typename ValueType>
class LC_PropertyValueDelegated : public LC_PropertyValueStorage<ValueType> {
public:
    using PropertyType = LC_PropertyAtomic;
    using ValueTypeStore = typename LC_PropertyValueStorage<ValueType>::ValueTypeStore;

    using FunValueAccepted = std::function<bool(ValueType)>;
    using FunValueEqual = std::function<bool(ValueType)>;
    using FunValueSet = std::function<void(ValueType, LC_PropertyChangeReason)>;
    using FunValueSetShort = std::function<void(ValueType)>;
    using FunValueGet = std::function<ValueTypeStore()>;

    explicit LC_PropertyValueDelegated()
            : LC_PropertyValueStorage<ValueType>() {
    }

    bool doAcceptValue(ValueType valueToAccept) override {
        if (m_funIsValueAccepted) {
            return m_funIsValueAccepted(valueToAccept);
        }

        return LC_PropertyValueStorage<ValueType>::doAcceptValue(valueToAccept);
    }

    void doSetValue(ValueType newValue, LC_PropertyChangeReason reason) override {
        Q_ASSERT(m_funSetValue);
        m_funSetValue(newValue, reason);
    }

    ValueType doGetValue() const override {
        Q_ASSERT(m_funGetValue);
        return m_funGetValue();
    }

    bool doCheckValueEqualToCurrent(ValueType valueToCompare) override {
        if (m_funIsValueEqual != nullptr) {
            return m_funIsValueEqual(valueToCompare);
        }

        return LC_PropertyValueStorage<ValueType>::doCheckValueEqualToCurrent(valueToCompare);
    }

    const FunValueAccepted& getFunValueAccepted() const {
        return m_funIsValueAccepted;
    }

    const FunValueSet& getFunValueSet() const {
        return m_funSetValue;
    }

    const FunValueGet& getFunValueGet() const {
        return m_funGetValue;
    }

    const FunValueEqual& getFunValueEqual() const {
        return m_funIsValueEqual;
    }

    void setFunValueGet(const FunValueGet& fun) {
        m_funGetValue = fun;
    }

    void setFunValueSet(const FunValueSet& fun, PropertyType* prop) {
        m_funSetValue = fun;
        prop->setReadOnly(fun == nullptr);
    }

    void setFunValueAccepted(const FunValueAccepted& fun) {
        m_funIsValueAccepted = fun;
    }

    void setFunValueEqual(const FunValueEqual& fun) {
        m_funIsValueEqual = fun;
    }

    void setupFull(const FunValueGet& funGetValue, const FunValueSet& funSetValue,
               const FunValueEqual& funValueEqual = nullptr) {
        m_funGetValue = funGetValue;
        m_funSetValue = funSetValue;
        m_funIsValueEqual = funValueEqual;
    }

    void setup(const FunValueGet& funGetValue, const FunValueSetShort& funSetValue,
             const FunValueEqual& funValueEqual = nullptr) {
        m_funGetValue = funGetValue;
        m_funSetValue = [funSetValue](ValueType val, LC_PropertyChangeReason) {
            funSetValue(val);
        };
        m_funIsValueEqual = funValueEqual;
    }

private:
    FunValueGet m_funGetValue;
    FunValueSet m_funSetValue;
    FunValueAccepted m_funIsValueAccepted;
    FunValueEqual m_funIsValueEqual;
};
#endif
