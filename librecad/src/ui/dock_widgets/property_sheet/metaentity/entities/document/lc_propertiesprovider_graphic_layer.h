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

#ifndef LC_PROPERTYPROVIDER_GRAPHIC_LAYER_H
#define LC_PROPERTYPROVIDER_GRAPHIC_LAYER_H


#include "lc_propertiesprovider_graphic_component.h"

class LC_PropertiesProviderGraphicLayer : public LC_PropertiesProviderGraphicComponent {
    Q_OBJECT
public:
    LC_PropertiesProviderGraphicLayer(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
        : LC_PropertiesProviderGraphicComponent(actionContext, widget) {
    }

    static const QString SECTION_LAYER;
    void fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) override;
protected:
    void createActiveLayer(LC_PropertyContainer* cont, RS_Graphic* graphic);
    bool inPrintPreview() const;
    void createLayerType(LC_PropertyContainer* cont, RS_Graphic* graphic);
    void createVisible(RS_Graphic* graphic, LC_PropertyContainer* cont);
    void createLocked(RS_Graphic* graphic, LC_PropertyContainer* cont);
    void createPrintable(RS_Graphic* graphic, LC_PropertyContainer* cont);
    void createColor(LC_PropertyContainer* cont, RS_Graphic* graphic);
    void createLineWidth(LC_PropertyContainer* cont, RS_Graphic* graphic);
    void createLineType(LC_PropertyContainer* cont, RS_Graphic* graphic);
    void createLayerCommands(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
    void createConstruction(LC_PropertyContainer* cont, RS_Graphic* graphic);
    void createAddRemoveCommands(LC_PropertyContainer* cont, RS_Graphic* graphic, bool nonZeroLayer) const;
    void createLockingCommand(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
    void createVisibleCommand(LC_PropertyContainer* cont, RS_Graphic* graphic) const;
};

#endif
