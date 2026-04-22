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

#ifndef LC_PROPERTIESPROVIDERGRAPHICUNITS_H
#define LC_PROPERTIESPROVIDERGRAPHICUNITS_H

#include "lc_propertiesprovider_graphic_component.h"

class LC_PropertiesProviderGraphicUnits : public LC_PropertiesProviderGraphicComponent {
    Q_OBJECT
public:
    LC_PropertiesProviderGraphicUnits(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_PropertiesProviderGraphicComponent(actionContext, widget) {
    }

    static const QString SECTION_UNITS;
    void fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) override;
protected:
    void createAnglesBasisZeroDirection(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
    void createAngularPrecision(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
    void createLinearFormat(RS_Graphic* graphic, LC_PropertyContainer* cont) const;
    void createLinearPrecision(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
    void createDrawingUnit(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
    void createAnglesBasisIncreaseDirection(RS_Graphic* graphic, LC_PropertyContainer* cont) const;
    void createAngleFormat(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
};

#endif
