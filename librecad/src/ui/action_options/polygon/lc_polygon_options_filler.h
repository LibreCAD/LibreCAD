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

#ifndef LC_POLYGONOPTIONSFILLER_H
#define LC_POLYGONOPTIONSFILLER_H

#include "lc_action_options_properties_filler_base.h"

class LC_PolygonOptionsFiller : public LC_ActionOptionsPropertiesFillerBase {
    Q_OBJECT public:
    LC_PolygonOptionsFiller() = default;
    ~LC_PolygonOptionsFiller() override = default;
    void fillToolOptionsContainer(LC_PropertyContainer* container) override;
};

#endif
