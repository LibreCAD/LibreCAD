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

#ifndef LC_PROPERTIESPROVIDERTOLERANCE_H
#define LC_PROPERTIESPROVIDERTOLERANCE_H

#include "lc_entity_type_propertiesprovider.h"

class LC_PropertiesProviderTolerance : public LC_EntityTypePropertiesProvider {
    Q_OBJECT

public:
    LC_PropertiesProviderTolerance(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_EntityTypePropertiesProvider(RS2::EntityTolerance, actionContext, widget) {
    }

protected:
    void doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) override;
    void fillComputedProperites(LC_PropertyContainer* container, const QList<RS_Entity*>& entitiesList) override;
    void fillSingleEntityCommands(LC_PropertyContainer* container, const QList<RS_Entity*>& entitiesList) override;
};

#endif
