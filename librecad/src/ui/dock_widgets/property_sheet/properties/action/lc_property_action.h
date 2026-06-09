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

#ifndef LC_PROPERTYACTION_H
#define LC_PROPERTYACTION_H

#include <functional>

#include "lc_property_atomic.h"
#include "rs_entity.h"

class QStyleOptionButton;

class LC_PropertyAction : public LC_PropertyAtomic {
    Q_OBJECT

public:
    using FunClickHandler = std::function<void(const LC_PropertyAction*, int linkIndex)>;
    explicit LC_PropertyAction(QObject* parent = nullptr, bool holdForTemplateFunction = true);
    void invokeClick(int linkIdx);
    void setClickHandler(const FunClickHandler& clickHandler) const;
    inline LC_PropertyAction& operator=(const LC_PropertyAction&);
    void invokePreDrawButton(QStyleOptionButton* option);

    void setEntity(RS_Entity* entity) {
        m_entity = entity;
    }

    RS_Entity* getEntity() const {
        return m_entity;
    }

    bool isValueEqualTo([[maybe_unused]]LC_PropertyAtomic* prop) override {return false;}
    void setValueFrom([[maybe_unused]]LC_PropertyAtomic* prop, [[maybe_unused]]LC_PropertyChangeReason reason) override {}

signals:
    void click(const LC_PropertyAction* property, int linkIdx);
    void preDrawButton(const LC_PropertyAction* property, QStyleOptionButton* option);

protected:
    RS_Entity* m_entity{nullptr};
};

LC_PropertyAction& LC_PropertyAction::operator=(const LC_PropertyAction&) {
    // do nothing
    return *this;
}

#endif
