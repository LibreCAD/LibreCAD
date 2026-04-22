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

#ifndef LC_PROPERTIESPROVIDERACTIVEPEN_H
#define LC_PROPERTIESPROVIDERACTIVEPEN_H

#include "lc_propertiesprovider_graphic_component.h"

class LC_PropertiesProviderActivePen: public LC_PropertiesProviderGenericComponent {
    Q_OBJECT
public:
    LC_PropertiesProviderActivePen(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_PropertiesProviderGenericComponent(actionContext, widget) {
    }
    static const QString SECTION_ACTIVE_PEN;
    void fillDocumentProperties(LC_PropertyContainer* container) override;

protected:
    void createColor(LC_PropertyContainer* container, const RS_Pen& pen) const;
    void createLineType(LC_PropertyContainer* container, const RS_Pen& pen) const;
    void createLineWidth(LC_PropertyContainer* container, const RS_Pen& pen) const;
    void createCommands(LC_PropertyContainer* cont) const;
};

#endif
