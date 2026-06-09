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

#ifndef LC_PROPERTIESPROVIDERDIAMETRIC_H
#define LC_PROPERTIESPROVIDERDIAMETRIC_H

#include "lc_propertiesprovider_dim_base.h"

class LC_PropertiesProviderDimDiametric : public LC_PropertiesProviderDimBase {
    Q_OBJECT

public:
    LC_PropertiesProviderDimDiametric(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_PropertiesProviderDimBase(RS2::EntityDimDiametric, actionContext, widget) {
    }

protected:
    void doCreateDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) override;
    void doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) override;
};

#endif
