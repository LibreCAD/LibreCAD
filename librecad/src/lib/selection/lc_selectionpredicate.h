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

#ifndef LC_SELECTIONPREDICATE_H
#define LC_SELECTIONPREDICATE_H

#include <functional>

#include "rs.h"

class RS_Entity;

class LC_SelectionPredicate {
public:
    using FunAcceptEntity = std::function<bool(RS_Entity*)>;
    explicit LC_SelectionPredicate(RS2::EntityType entityType);
    virtual ~LC_SelectionPredicate() = default;
    virtual bool accept(RS_Entity* entity) const;
    bool acceptRtti(RS_Entity* entity) const;
private:
    RS2::EntityType m_entityType = RS2::EntityType::EntityUnknown;
    FunAcceptEntity m_acceptRTTIFunction;
    FunAcceptEntity m_acceptEntityFunction;
};

#endif
