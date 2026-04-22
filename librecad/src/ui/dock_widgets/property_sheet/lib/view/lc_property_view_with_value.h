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

#ifndef LC_PROPERTYVIEWWITHVALUE_H
#define LC_PROPERTYVIEWWITHVALUE_H

#include "lc_property_view_with_values.h"

class LC_PropertyViewWithValue : public LC_PropertyViewWithValues {
    Q_DISABLE_COPY(LC_PropertyViewWithValue)

protected:
    explicit LC_PropertyViewWithValue(LC_Property& property);
    virtual bool doBuildPartValue(LC_PropertyPaintContext& ctx, LC_PropertyViewPart& parts) = 0;
    void doBuildPartValues(LC_PropertyPaintContext& ctx, const QRect& valueRect, QList<LC_PropertyViewPart>& parts) override;
};

#endif
