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

#ifndef LC_PROPERTIESPROVIDERDOCUMENT_H
#define LC_PROPERTIESPROVIDERDOCUMENT_H

#include "lc_entity_type_propertiesprovider.h"
#include "lc_propertiesprovider_active_pen.h"
#include "lc_propertiesprovider_graphic_grid.h"
#include "lc_propertiesprovider_graphic_layer.h"
#include "lc_propertiesprovider_graphic_paper.h"
#include "lc_propertiesprovider_graphic_ucs.h"
#include "lc_propertiesprovider_graphic_units.h"
#include "lc_propertiesprovider_graphic_views.h"
#include "lc_propertiesprovider_graphic_workspace.h"
#include "lc_propertiesprovider_render_options.h"

class LC_PropertiesProviderActivePen;

class LC_PropertiesProviderDocument : public LC_EntityTypePropertiesProvider {
    Q_OBJECT
public:
    LC_PropertiesProviderDocument(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget);
    virtual void fillDocumentProperties(LC_PropertyContainer* container);
    LC_PropertyContainer* createToolOptionsSection(LC_PropertyContainer* container) const;
    void fillDocumentPropertiesForToolOptions(LC_PropertyContainer* container) const;
protected:
    void doCreateEntitySpecificProperties([[maybe_unused]]LC_PropertyContainer* container,[[maybe_unused]] const QList<RS_Entity*>& list) override {}
    void fillBlockProperties(LC_PropertyContainer* container, RS_Block* block) const;
    void fillGraphicProperties(LC_PropertyContainer* container, RS_Graphic* graphic) const;

    std::unique_ptr<LC_PropertiesProviderGraphicLayer> m_providerGraphicLayer;
    std::unique_ptr<LC_PropertiesProviderGraphicGrid> m_providerGraphicGrid;
    std::unique_ptr<LC_PropertiesProviderGraphicPaper> m_providerGraphicPaper;
    std::unique_ptr<LC_PropertiesProviderGraphicUCS> m_providerGraphicUcs;
    std::unique_ptr<LC_PropertiesProviderGraphicUnits> m_providerGraphicUnits;
    std::unique_ptr<LC_PropertiesProviderGraphicViews> m_providerGraphicViews;
    std::unique_ptr<LC_PropertiesProviderGraphicWorkspace> m_providerGraphicWorkspace;
    std::unique_ptr<LC_PropertiesProviderBlockComponent> m_providerBlock;
    std::unique_ptr<LC_PropertiesProviderRenderOptions> m_providerRenderOptions;
    std::unique_ptr<LC_PropertiesProviderActivePen> m_providerActivePen;
};

#endif
