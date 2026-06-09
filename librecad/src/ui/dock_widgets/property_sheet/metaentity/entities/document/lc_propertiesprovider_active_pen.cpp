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

#include "lc_propertiesprovider_active_pen.h"

#include "lc_property_linetype.h"
#include "lc_property_linetype_combobox_view.h"
#include "lc_property_linewidth.h"
#include "lc_property_linewidth_combobox_view.h"
#include "lc_property_rscolor.h"
#include "lc_property_rscolor_combobox_view.h"
#include "lc_propertyprovider_utils.h"
#include "qc_applicationwindow.h"
#include "qg_pentoolbar.h"
#include "rs_document.h"
#include "rs_pen.h"

const QString LC_PropertiesProviderActivePen::SECTION_ACTIVE_PEN = "_secActivePen";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderActivePen::fillDocumentProperties(LC_PropertyContainer* container) {
    if (m_widget->getOptions()->noSelectionActivePen) {
        const LC_Property::Names names = {SECTION_ACTIVE_PEN, tr("Active Pen"), tr("Pen that will be assigned to newly created entities")};
        const auto cont = createSection(container, names);
        const auto doc = m_actionContext->getDocument();
        if (doc != nullptr) {
            const RS_Pen pen = doc->getActivePen();
            createColor(cont, pen);
            createLineWidth(cont, pen);
            createLineType(cont, pen);
            if (isShowLinks()) {
                createCommands(cont);
            }
        }
    }
}

void LC_PropertiesProviderActivePen::createColor(LC_PropertyContainer* container, const RS_Pen& pen) const {
    const LC_Property::Names names = {"color", tr("Color"), tr("Color of active pen")};
    auto* layerColorProperty = new LC_PropertyRSColor(container, false);
    layerColorProperty->setNames(names);
    layerColorProperty->setViewAttribute(LC_PropertyRSColorComboBoxView::ATTR_SHOW_BY_LAYER, true);

    auto funGet = [pen]([[maybe_unused]] const RS_Graphic* e) -> RS_Color {
        return pen.getColor();
    };
    auto funSet = [](const RS_Color& color, [[maybe_unused]] RS_Graphic* e) -> void {
        const QG_PenToolBar* penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
        if (penToolBar != nullptr) {
            penToolBar->setColor(color);
        }
    };

    createDirectDelegatedStorage<RS_Color, RS_Graphic>(funGet, funSet, nullptr, layerColorProperty);
    container->addChildProperty(layerColorProperty);
}

void LC_PropertiesProviderActivePen::createLineType(LC_PropertyContainer* container, const RS_Pen& pen) const {
    const LC_Property::Names names = {"linewidth", tr("Line Width"), tr("Line width for active pen")};

    auto* layerLineWidthProperty = new LC_PropertyLineWidth(container, false);
    layerLineWidthProperty->setViewAttribute(LC_PropertyLineWidthComboboxView::ATTR_SHOW_BY_LAYER, true);
    layerLineWidthProperty->setNames(names);

    auto funGet = [pen]([[maybe_unused]] const RS_Graphic* e) -> RS2::LineWidth {
        return pen.getWidth();
    };
    auto funSet = [](const RS2::LineWidth& width, [[maybe_unused]]RS_Graphic* e) -> void {
        const QG_PenToolBar* penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
        if (penToolBar != nullptr) {
            penToolBar->setWidth(width);
        }
    };

    createDirectDelegatedStorage<RS2::LineWidth, RS_Graphic>(funGet, funSet, nullptr, layerLineWidthProperty);
    container->addChildProperty(layerLineWidthProperty);
}

void LC_PropertiesProviderActivePen::createLineWidth(LC_PropertyContainer* container, const RS_Pen& pen) const {
    const LC_Property::Names names = {"linetype", tr("Line Type"), tr("Type of line for active pen")};
    auto* layerLineTypeProperty = new LC_PropertyLineType(container, false);
    layerLineTypeProperty->setViewAttribute(LC_PropertyLineTypeComboboxView::ATTR_SHOW_BY_LAYER, true);
    layerLineTypeProperty->setNames(names);
    auto funGet = [pen]([[maybe_unused]]const RS_Graphic* e) -> RS2::LineType {
        return pen.getLineType();
    };
    auto funSet = [](const RS2::LineType& linetype, [[maybe_unused]]RS_Graphic* e) -> void {
        const QG_PenToolBar* penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
        if (penToolBar != nullptr) {
            penToolBar->setLineType(linetype);
        }
    };
    createDirectDelegatedStorage<RS2::LineType, RS_Graphic>(funGet, funSet, nullptr, layerLineTypeProperty);
    container->addChildProperty(layerLineTypeProperty);
}

void LC_PropertiesProviderActivePen::createCommands(LC_PropertyContainer* cont) const {
    auto pickClickHandler = [this]([[maybe_unused]] RS_Document* g, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                m_actionContext->setCurrentAction(RS2::ActionPenPick, nullptr);
                break;
            }
            case 1: {
                m_actionContext->setCurrentAction(RS2::ActionPenPickResolved, nullptr);
                break;
            }
            default:
                break;
        }
    };
    createSingleEntityCommand<RS_Document>(cont, "activePenZoom", tr("Pick"), tr("Pick pen from entity and make it active one"),
                                           tr("Pick resolved"), tr("Pick resolved pen from entity and make it active"), nullptr,
                                           pickClickHandler, tr("Pen pick commands"));

    auto applyClickHandler = [this]([[maybe_unused]] RS_Document* g, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                m_actionContext->setCurrentAction(RS2::ActionPenApply, nullptr);
                break;
            }
            case 1: {
                m_actionContext->setCurrentAction(RS2::ActionPenSyncFromLayer, nullptr);
                break;
            }
            default:
                break;
        }
    };
    createSingleEntityCommand<RS_Document>(cont, "activePenZoom", tr("Apply"), tr("Applies active pen to selected entities"),
                                           tr("From layer"), tr("Pen of active layer becomes active pen"), nullptr, applyClickHandler,
                                           tr("Pen applying commands"));
}
