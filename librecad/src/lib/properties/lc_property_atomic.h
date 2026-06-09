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

#ifndef LC_PROPERTYATOMIC_H
#define LC_PROPERTYATOMIC_H

#include "lc_property.h"

class LC_PropertyAtomic : public LC_Property {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertyAtomic)

public:
    ~LC_PropertyAtomic() override;
    LC_PropertyAtomic* asAtomic() override;
    const LC_PropertyAtomic* asAtomic() const override;
    virtual bool isValueEqualTo(LC_PropertyAtomic* prop) = 0;
    virtual void setValueFrom(LC_PropertyAtomic* prop, LC_PropertyChangeReason reason) = 0;
protected:
    explicit LC_PropertyAtomic(QObject* parent);
};

#endif
