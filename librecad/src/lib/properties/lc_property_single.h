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

#ifndef LC_PROPERTYSINGLE_H
#define LC_PROPERTYSINGLE_H

#include "lc_property_atomic.h"
#include "lc_property_valuestorage.h"

template <typename T>
class LC_PropertySingle : public LC_PropertyAtomic {
public:
    using ValueType = T;
    using ValueTypeStore = std::decay_t<ValueType>;
    using ValueTag = PropertyValueTag<LC_PropertySingle>;

    class ValueHolder : public LC_PropertyValueHolder<ValueType> {
    public:
        explicit ValueHolder(ValueType value)
            : LC_PropertyValueHolder<ValueType>{value} {
        }

        ValueHolder()
            : LC_PropertyValueHolder<ValueType>{} {
        }

        bool isDelegated() const override {
            return false;
        }
    };

    class ValueDelegate : public LC_PropertyValueDelegated<ValueType> {
    public:
        explicit ValueDelegate()
            : LC_PropertyValueDelegated<ValueType>() {
        }
    };

    explicit LC_PropertySingle(const LC_Property&) = delete;

    ~LC_PropertySingle() override {
        cleanup();
    }

    ValueType value() const {
        auto result = doGetValue(ValueTag());
        return result;
    }

    bool isValueEqualTo(LC_PropertyAtomic* prop) override {
        LC_PropertySingle<T>* otherProp = dynamic_cast<LC_PropertySingle<T>*>(prop);
        if (otherProp != nullptr) {
            return otherProp->value() == value();
        }
        return false;
    }
    void setValueFrom(LC_PropertyAtomic* prop, LC_PropertyChangeReason reason) override {
        LC_PropertySingle<T>* otherProp = dynamic_cast<LC_PropertySingle<T>*>(prop);
        if (otherProp != nullptr) {
            setValue(otherProp->value(), reason);
        }
    }

    bool setValue(ValueType newValue, LC_PropertyChangeReason reason = LC_PropertyChangeReason()) {
        if ((reason & PropertyChangeReasonEdit) && !isEditableByUser()) {
            return false;
        }
        /// fixme - delegate to value storage?
        if (!isWritable() || !doAcceptValue(newValue)) {
            return false;
        }
        /// fixme - delegate to value storage?
        if (!(reason & PropertyChangeReasonMultiEdit) && doCheckValueEqualToCurrent(newValue)) {
            return true;
        }

        if (!(reason & PropertyChangeReasonValue)) {
            reason |= PropertyChangeReasonValueNew;
        }

        emit beforePropertyChange(reason, static_cast<PropertyValuePtr>(&newValue), qMetaTypeId<ValueTypeStore>());
        doSetValue(newValue, reason);
        emit afterPropertyChange(reason);

        return true;
    }

    LC_PropertySingle<ValueType>& operator=(const LC_PropertySingle<ValueType>& newValue) {
        setValue(newValue.value());
        return *this;
    }

    LC_PropertyValueStorage<ValueType>* getValueStorage() {
        return doGetValueStorage();
    }

    void setValueStorage(LC_PropertyValueStorage<ValueType>* storage, const bool own) {
        doSetValueStorage(storage, own);
    }

    ValueDelegate* useOwnedDelegatedValue() {
        auto* store = new ValueDelegate();
        setValueStorage(store, true);
        return store;
    }

    ValueHolder* useOwnedHoldValue() {
        auto* store = new ValueHolder();
        setValueStorage(store, true);
        return store;
    }

    ValueDelegate* getAsDelegateValue() {
        if (m_valueStorage != nullptr) {
            if (m_valueStorage->isDelegated()) {
                return static_cast<ValueDelegate*>(m_valueStorage);
            }
        }
        return nullptr;
    }

protected:
    explicit LC_PropertySingle(QObject* parent, const bool holdValue = true)
        : LC_PropertyAtomic(parent) {
        if (holdValue) {
            useOwnedHoldValue();
        }
        else {
            useOwnedDelegatedValue();
        }
    }

    virtual ValueType doGetValue(ValueTag) const {
        return m_valueStorage->doGetValue();
    }

    virtual void doSetValue(ValueType newValue, LC_PropertyChangeReason reason) {
        m_valueStorage->doSetValue(newValue, reason);
    }

    virtual bool doAcceptValue(ValueType) {
        return true;
    }

    virtual bool doCheckValueEqualToCurrent(ValueType valueToCompare) {
        return m_valueStorage->doCheckValueEqualToCurrent(valueToCompare);
    }

    LC_PropertyValueStorage<ValueType>* doGetValueStorage() {
        return m_valueStorage;
    }

    void cleanup() {
        if (m_storageOwner && m_valueStorage != nullptr) {
            delete m_valueStorage;
        }
    }

    void doSetValueStorage(LC_PropertyValueStorage<ValueType>* storage, const bool own) {
        cleanup();
        m_storageOwner = own;
        m_valueStorage = storage;
    }

    LC_PropertyValueStorage<ValueType>* m_valueStorage{nullptr};

private:
    bool m_storageOwner;
};

template class LC_PropertySingle<bool>;
template class LC_PropertySingle<qint32>;
template class LC_PropertySingle<quint32>;
template class LC_PropertySingle<qint64>;
template class LC_PropertySingle<quint64>;
template class LC_PropertySingle<float>;
template class LC_PropertySingle<double>;
template class LC_PropertySingle<QString>;
template class LC_PropertySingle<QVariant>;

#endif
