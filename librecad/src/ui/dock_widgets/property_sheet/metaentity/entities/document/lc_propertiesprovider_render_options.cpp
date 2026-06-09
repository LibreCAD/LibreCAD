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

#include "lc_propertiesprovider_render_options.h"

#include "lc_propertyprovider_utils.h"
#include "qc_applicationwindow.h"
#include "qg_graphicview.h"

const QString LC_PropertiesProviderRenderOptions::SECTION_VIEW = "_secView";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderRenderOptions::fillDocumentProperties(LC_PropertyContainer* container) {
    if (m_widget->getOptions()->noSelectionGraphicView) {
        const LC_Property::Names names = {SECTION_VIEW, tr("Graphic View"), tr("Setting that affects drawing appearance")};
        const auto cont = createSection(container, names);
        const bool draftMode = createDraft(cont);
        createDraftLines(cont, draftMode);
        createAntialiasing(cont);
    }
}

bool LC_PropertiesProviderRenderOptions::createDraft(LC_PropertyContainer* cont) const {
    const auto graphicView = static_cast<QG_GraphicView*>(m_actionContext->getGraphicView());
    if (graphicView == nullptr) {
        return false;
    }
    const auto draftMode = graphicView->isDraftMode();
    const auto funGet = [draftMode]([[maybe_unused]] const RS_Graphic* e) -> bool {
        return draftMode;
    };
    const auto funSet = [this]([[maybe_unused]] const bool& v, [[maybe_unused]] RS_Graphic* e) -> void {
        QC_ApplicationWindow::getAppWindow()->getAction("ViewDraft")->trigger();
        m_widget->refill();
    };

    const LC_Property::Names names = {"graphicViewShowDraft", tr("Draft mode"), tr("Defines whether draft mode is enabled for drawing")};
    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, nullptr);
    return draftMode;
}

void LC_PropertiesProviderRenderOptions::createDraftLines(LC_PropertyContainer* cont, bool draftMode) const {
    const auto graphicView = static_cast<QG_GraphicView*>(m_actionContext->getGraphicView());
    if (graphicView == nullptr) {
        return;
    }
    const auto draftLines = graphicView->isDraftLinesMode();
    const auto funGet = [draftLines]([[maybe_unused]] const RS_Graphic* e) -> bool {
        return draftLines;
    };

    const LC_Property::Names names = {
        "graphicViewDraftLines",
        tr("Draft lines"),
        tr("Defines whether draft or actual line width is rendered in drawing")
    };
    if (draftMode) {
        createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, nullptr, nullptr);
    }
    else {
        const auto funSet = [this]([[maybe_unused]] const bool& v, [[maybe_unused]] RS_Graphic* e) -> void {
            QC_ApplicationWindow::getAppWindow()->getAction("ViewLinesDraft")->trigger();
            m_widget->refill();
        };
        createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, nullptr);
    }
}

void LC_PropertiesProviderRenderOptions::createAntialiasing(LC_PropertyContainer* cont) const {
    const auto graphicView = static_cast<QG_GraphicView*>(m_actionContext->getGraphicView());
    if (graphicView == nullptr) {
        return;
    }
    const auto antialiasing = graphicView->isAntialiasing();
    const auto funGet = [antialiasing]([[maybe_unused]] const RS_Graphic* e) -> bool {
        return antialiasing;
    };
    const auto funSet = [this]([[maybe_unused]] const bool& v, [[maybe_unused]] RS_Graphic* e) -> void {
        QC_ApplicationWindow::getAppWindow()->getAction("ViewAntialiasing")->trigger();
        m_widget->refill();
    };

    const LC_Property::Names names = {
        "graphicAntialiasing",
        tr("Antialiasing"),
        tr("Defines whether antialiased rendering is used in drawing")
    };
    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, nullptr);
}
