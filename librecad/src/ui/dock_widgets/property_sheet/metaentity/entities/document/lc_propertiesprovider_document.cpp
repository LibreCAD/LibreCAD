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

#include "lc_propertiesprovider_document.h"

#include "lc_propertiesprovider_active_pen.h"
#include "lc_propertiesprovider_block.h"
#include "lc_propertiesprovider_graphic_workspace.h"
#include "lc_propertiesprovider_render_options.h"
#include "lc_propertyprovider_utils.h"
#include "qg_graphicview.h"
#include "rs_block.h"
#include "rs_document.h"
#include "rs_graphic.h"

namespace {
}

LC_PropertiesProviderDocument::LC_PropertiesProviderDocument(LC_ActionContext* actionContext, LC_PropertySheetWidget* widget)
    : LC_EntityTypePropertiesProvider(RS2::EntityContainer, actionContext, widget),
      m_providerGraphicLayer{std::make_unique<LC_PropertiesProviderGraphicLayer>(actionContext, widget)},
      m_providerGraphicGrid{std::make_unique<LC_PropertiesProviderGraphicGrid>(actionContext, widget)},
      m_providerGraphicPaper{std::make_unique<LC_PropertiesProviderGraphicPaper>(actionContext, widget)},
      m_providerGraphicUcs{std::make_unique<LC_PropertiesProviderGraphicUCS>(actionContext, widget)},
      m_providerGraphicUnits{std::make_unique<LC_PropertiesProviderGraphicUnits>(actionContext, widget)},
      m_providerGraphicViews{std::make_unique<LC_PropertiesProviderGraphicViews>(actionContext, widget)},
      m_providerGraphicWorkspace{std::make_unique<LC_PropertiesProviderGraphicWorkspace>(actionContext, widget)},
      m_providerBlock{std::make_unique<LC_PropertiesProviderBlock>(actionContext, widget)},
      m_providerRenderOptions{std::make_unique<LC_PropertiesProviderRenderOptions>(actionContext, widget)},
      m_providerActivePen{std::make_unique<LC_PropertiesProviderActivePen>(actionContext, widget)}{
}

void LC_PropertiesProviderDocument::fillDocumentProperties(LC_PropertyContainer* container) {
    if (m_actionContext == nullptr) {
        // check needed for initial load
        return;
    }
    const auto document = getDocument();
    if (document == nullptr) {
        return;
    }
    m_providerActivePen->fillDocumentProperties(container);

    const auto rtti = document->rtti();
    if (rtti == RS2::EntityBlock) {
        const auto block = static_cast<RS_Block*>(document);
        fillBlockProperties(container, block);
    }
    else if (rtti == RS2::EntityGraphic) {
        const auto graphic = static_cast<RS_Graphic*>(document);
        fillGraphicProperties(container, graphic);
    }
    m_providerGraphicWorkspace->fillDocumentProperties(container);
    const bool isPrintPreview = m_actionContext->getGraphicView()->isPrintPreview();
    if (!isPrintPreview) {
        m_providerRenderOptions->fillDocumentProperties(container);
    }
}

LC_PropertyContainer*  LC_PropertiesProviderDocument::createToolOptionsSection(LC_PropertyContainer* container) const{
    const auto result = createSection(container, {SECTION_TOOL_OPTIONS, tr("Action Options"), tr("Options for currently active tool action")});
    return result;
}

void LC_PropertiesProviderDocument::fillDocumentPropertiesForToolOptions(LC_PropertyContainer* container) const {
    if (m_actionContext == nullptr) {
        // check needed for initial load
        return;
    }
    const auto document = getDocument();
    if (document == nullptr) {
        return;
    }
    const bool isPrintPreview = m_actionContext->getGraphicView()->isPrintPreview();
    if (!isPrintPreview) {
        m_providerActivePen->fillDocumentProperties(container);
    }
    const auto rtti = document->rtti();
    if (rtti == RS2::EntityBlock) {
        const auto block = static_cast<RS_Block*>(document);
        // fillBlockProperties(container, block);
    }
    else if (rtti == RS2::EntityGraphic) {
        const auto graphic = static_cast<RS_Graphic*>(document);
        m_providerGraphicLayer->fillDocumentProperties(container, graphic);
        m_providerGraphicViews->fillDocumentProperties(container, graphic);
        if (!isPrintPreview) {
            // m_providerGraphicPaper->fillDocumentProperties(container, graphic);
            m_providerGraphicUcs->fillDocumentProperties(container, graphic);
            m_providerGraphicGrid->fillDocumentProperties(container, graphic);
        }
    }
    m_providerGraphicWorkspace->fillDocumentProperties(container);
    if (!isPrintPreview) {
        m_providerRenderOptions->fillDocumentProperties(container);
    }
}

void LC_PropertiesProviderDocument::fillBlockProperties(LC_PropertyContainer* container, RS_Block* block) const {
    [[maybe_unused]] RS_Graphic* graphic = block->getGraphic();
    m_providerBlock->fillDocumentProperties(container, nullptr, block);
}

void LC_PropertiesProviderDocument::fillGraphicProperties(LC_PropertyContainer* container, RS_Graphic* graphic) const {
    const bool isPrintPreview = m_actionContext->getGraphicView()->isPrintPreview();
    m_providerGraphicLayer->fillDocumentProperties(container, graphic);
    m_providerGraphicViews->fillDocumentProperties(container, graphic);
    if (!isPrintPreview) {
        m_providerGraphicUcs->fillDocumentProperties(container, graphic);
        m_providerGraphicGrid->fillDocumentProperties(container, graphic);
    }
    m_providerGraphicUnits->fillDocumentProperties(container, graphic);

    if (!isPrintPreview) {
        m_providerGraphicPaper->fillDocumentProperties(container, graphic);
    }
}
