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

#ifndef LC_ENTITYPROPERTYVALUEDELEGATE_H
#define LC_ENTITYPROPERTYVALUEDELEGATE_H

#include "lc_property_valuestorage.h"

class RS_Entity;

class LC_EntitiesModificationContext {
public:
    virtual ~LC_EntitiesModificationContext() = default;
    virtual void entityModified(RS_Entity* originalEntity, RS_Entity* entityClone) = 0;
};

template <typename ValueType, typename EntityClass>
class LC_EntityPropertyValueDelegate : public LC_PropertyValueStorage<ValueType> {
public:
    using PropertyType = LC_PropertyAtomic;
    using ValueTypeStore = typename LC_PropertyValueStorage<ValueType>::ValueTypeStore;

    using FunValueGet = typename std::function<ValueTypeStore(EntityClass*)>;
    using FunValueSet = typename std::function<void(ValueType &, LC_PropertyChangeReason, EntityClass *)>;
    using FunValueSetShort = typename std::function<void(ValueType &, EntityClass *)>;
    using FunValueAccepted = typename std::function<bool(ValueType)>;
    using FunValueEqual = typename std::function<bool(ValueType&, EntityClass*)>;

    ValueType doGetValue() const override {
        Q_ASSERT(m_funGetValue);
        EntityClass* entity = m_entity;
        return m_funGetValue(entity);
    }

    void doSetValue(ValueType newValue, LC_PropertyChangeReason reason) override {
        Q_ASSERT(m_funSetValue);
        EntityClass* entity = m_entity;
        EntityClass* clone = static_cast<EntityClass*>(entity->clone());
        m_funSetValue(newValue, reason, clone);
        m_modificationContext->entityModified(entity, clone);
        m_entity = clone;
    }

    bool doCheckValueEqualToCurrent(ValueType valueToCompare) override {
        if (m_funIsValueEqual) {
            EntityClass* entity = m_entity;
            return m_funIsValueEqual(valueToCompare, entity);
        }
        return LC_PropertyValueStorage<ValueType>::doCheckValueEqualToCurrent(valueToCompare);
    }

    void setup(EntityClass* entity, LC_EntitiesModificationContext* ctx, const FunValueGet& funGetValue, const FunValueSetShort& funSetValue,
               const FunValueEqual& funValueEqual) {
        setup(entity, ctx, funGetValue, [funSetValue](ValueType &v, [[maybe_unused]] LC_PropertyChangeReason, EntityClass *ent) {
            funSetValue(v,ent);
        }, funValueEqual);
    }

    void setup(EntityClass* entity, LC_EntitiesModificationContext* ctx, const FunValueGet& funGetValue, const FunValueSet& funSetValue,
               const FunValueEqual& funValueEqual) {
        m_entity = entity;
        m_modificationContext = ctx;
        m_funGetValue = funGetValue;
        m_funSetValue = funSetValue;
        m_funIsValueEqual = funValueEqual;
    }

protected:
    EntityClass* m_entity{nullptr};
    LC_EntitiesModificationContext* m_modificationContext{nullptr};

    FunValueGet m_funGetValue;
    FunValueSet m_funSetValue;
    FunValueEqual m_funIsValueEqual;
};

#endif
