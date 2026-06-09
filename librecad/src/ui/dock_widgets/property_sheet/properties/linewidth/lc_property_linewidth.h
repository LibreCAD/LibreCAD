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

#ifndef LC_PROPERTYLINEWIDTH_H
#define LC_PROPERTYLINEWIDTH_H

#include "lc_property_single.h"
#include "rs.h"

class LC_PropertyLineWidth : public LC_PropertySingle<RS2::LineWidth> {
    Q_OBJECT

public:
    using ValueType = typename RS2::LineWidth;

    LC_PropertyLineWidth(const LC_PropertyLineWidth& other) = delete;

    explicit LC_PropertyLineWidth(QObject* parent, const bool holdValue = true)
        : LC_PropertySingle(parent, holdValue) {
    }
};

#endif
