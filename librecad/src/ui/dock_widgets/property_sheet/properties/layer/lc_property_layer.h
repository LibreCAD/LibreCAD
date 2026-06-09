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

#ifndef LC_PROPERTYLAYER_H
#define LC_PROPERTYLAYER_H

#include "lc_property_single.h"
#include "rs_layer.h"

class RS_LayerList;

class LC_PropertyLayer : public LC_PropertySingle<RS_Layer*> {
    Q_OBJECT

public:
    using ValueType = RS_Layer*;
    LC_PropertyLayer(const LC_PropertyLayer& other) = delete;
    explicit LC_PropertyLayer(QObject* parent = nullptr, bool holdValue = true): LC_PropertySingle(parent, holdValue) {
    }

    void setLayerList(RS_LayerList* layerList) {
        m_layerList = layerList;
    }

    RS_LayerList* layerList() const {
        return m_layerList;
    }

    bool isAllowByBlockValues() const {
        return m_allowByBlockValues;
    }

    void setAllowByBlockValues(const bool allowByBlockValues) {
        m_allowByBlockValues = allowByBlockValues;
    }

protected:
    RS_LayerList* m_layerList = nullptr;
    bool m_allowByBlockValues = false;
};

#endif
