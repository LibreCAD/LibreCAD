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

#ifndef LC_PROPERTYMULTI_H
#define LC_PROPERTYMULTI_H

#include "lc_property_atomic.h"

struct LC_VariantMultiValues {
    QVariantList values;
};

Q_DECLARE_METATYPE(LC_VariantMultiValues)

class LC_PropertyMulti : public LC_PropertyAtomic {
    Q_OBJECT

public:
    using ValueType = LC_PropertyMulti;
    explicit LC_PropertyMulti(const QMetaObject* propertyMetaObject, QObject* parent = nullptr);
    ~LC_PropertyMulti() override;
    const QMetaObject* propertyMetaObject() const override;
    void addProperty(LC_PropertyAtomic* property, bool own = true);

    const std::vector<LC_PropertyAtomic*>& getProperties() const {
        return m_properties;
    }

    LC_PropertyAtomic* getFirstProperty() const {
        return m_properties.at(0);
    }

    void markEdited(const bool val) {
        m_edited = val;
    }

    void updateMultipleState(bool force);

    bool isValueEqualTo([[maybe_unused]]LC_PropertyAtomic* prop) override {return false;}
    void setValueFrom([[maybe_unused]]LC_PropertyAtomic* prop, [[maybe_unused]]LC_PropertyChangeReason reason) override {}

protected:
    void updatePropertyState() override;
    void doOnBeforeMasterPropertyChange(LC_PropertyChangeReason reason) override;
    void doOnAfterMasterPropertyChange(LC_PropertyChangeReason reason) override;
    bool hasMultipleValues();
private:
    std::vector<LC_PropertyAtomic*> m_properties;
    const QMetaObject* m_propertyMetaObject;
    unsigned m_subPropertyUpdates;
    bool m_edited{false};
    bool m_calculateMultipleValues;
    bool m_multipleValues;

    void onPropertyWillChange(LC_PropertyChangeReason reason, PropertyValuePtr newValue, int typeId);
    void onPropertyDidChange(LC_PropertyChangeReason reason);
    void updateStateFrom(const LC_PropertyAtomic* source);
};

#endif
