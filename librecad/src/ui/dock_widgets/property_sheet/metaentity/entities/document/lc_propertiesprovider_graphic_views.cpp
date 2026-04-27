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

#include "lc_propertiesprovider_graphic_views.h"

#include "lc_namedviewslistwidget.h"
#include "lc_propertyprovider_utils.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"

const QString LC_PropertiesProviderGraphicViews::SECTION_VIEWS = "_secViews";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderGraphicViews::fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) {
    if (m_widget->getOptions()->noSelectionNamedView) {
        const auto viewsList = graphic->getViewList();
        if (viewsList->isEmpty() && !isShowLinks()) {
            return;
        }
        const auto cont = createSection(container, {SECTION_VIEWS, tr("Named View"), tr("Saved named views")});
        const auto viewList = graphic->getViewList();
        if (!viewList->isEmpty()) {
            createViewSelector(cont, graphic, viewList);
        }
        if (m_widget->getOptions()->showLinks) {
            createViewCommands(cont, graphic, viewsList);
        }
    }
}

void LC_PropertiesProviderGraphicViews::createViewSelector(LC_PropertyContainer* container, RS_Graphic* graphic, LC_ViewList* viewList) const {
    int currentUCSIndex = -1;
    QVector<LC_EnumValueDescriptor> values;
    const auto count = viewList->count();
    auto desc = LC_EnumValueDescriptor(-1, tr("<Select View>"));
    values.append(desc);

    for (size_t i = 0; i < count; i++) {
        const auto view = viewList->at(i);
        auto name = view->getName();
        if (name.isEmpty()) {
            name = tr("<No name>");
        }
        desc = LC_EnumValueDescriptor(i, name);
        values.append(desc);
    }
    const auto enumDescriptor = new LC_EnumDescriptor("ucsListEnum", values);

    auto funGet = [currentUCSIndex]([[maybe_unused]] const RS_Graphic* e)-> LC_PropertyEnumValueType {
        return currentUCSIndex;
    };

    auto funSet = [this,viewList](const LC_PropertyEnumValueType& v, [[maybe_unused]] const RS_Graphic* e) -> void {
        const LC_View* view = viewList->at(v);
        if (view != nullptr) {
            const auto graphicViewport = m_actionContext->getViewport();
            if (graphicViewport != nullptr) {
                graphicViewport->restoreView(view);
            }
        }
        m_widget->refill();
    };

    const LC_Property::Names names = {"viewSelection", tr("View to restore"), tr("Restoring one of previously saved named views")};
    addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(container, names, enumDescriptor, funGet, funSet, graphic, true);
}

void LC_PropertiesProviderGraphicViews::createViewCommands(LC_PropertyContainer* const cont, RS_Graphic* graphic,
                                                           [[maybe_unused]] LC_ViewList* const viewsList) const {
    auto saveClickHandler = []([[maybe_unused]] RS_Graphic* g, [[maybe_unused]]const int linkIndex) {
        QC_ApplicationWindow::getAppWindow()->getNamedViewsListWidget()->addNewView();
    };

    createSingleEntityCommand<RS_Graphic>(cont, "viewSave", tr("Save current view..."),
                                          tr("Create view for current zoom and offset of drawing"),
                                          "", "", graphic, saveClickHandler,
                                          tr("Create view for current zoom and offset of drawing"));

    auto zoomClickHandler = [this]([[maybe_unused]] RS_Graphic* g, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                //adding ucs
                m_actionContext->setCurrentAction(RS2::ActionZoomAuto, nullptr);
                break;
            }
            case 1: {
                m_actionContext->setCurrentAction(RS2::ActionZoomPrevious, nullptr);
                break;
            }
            default:
                break;
        }
    };
    createSingleEntityCommand<RS_Graphic>(cont, "viewsZoom", tr("Auto zoom"),
                                          tr("Adjusts drawing zoom and view to ensure that all content of drawing is visible"),
                                          tr("Previous View..."), tr("Positions drawing to previous view"), graphic, zoomClickHandler,
                                          tr("Zoom related commands"));
}
