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

#include "lc_propertiesprovider_graphic_grid.h"

#include "lc_property_double_line_edit_view.h"
#include "lc_propertyprovider_utils.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"
#include "rs_grid.h"
#include "rs_settings.h"

const QString LC_PropertiesProviderGraphicGrid::SECTION_GRID = "_secGrid";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderGraphicGrid::fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) {
    if (m_widget->getOptions()->noSelectionGrid) {
        const LC_Property::Names names = {SECTION_GRID, tr("Grid"), tr("Grid settings")};
        const auto cont = createSection(container, names);
        const bool showGrid = createShowGrid(cont, graphic);
        createGridType(cont, graphic, showGrid);
        createGridX(cont, graphic, showGrid);
        createGridY(cont, graphic, showGrid);
        const bool showMetaGrid = createShowMetaGrid(cont, graphic, showGrid);
        createDecimalMetaGrid(cont, graphic, showGrid, showMetaGrid);
    }
}

bool LC_PropertiesProviderGraphicGrid::createShowGrid(LC_PropertyContainer* cont, RS_Graphic* graphic) const {
    const auto gridOn = graphic->isGridOn();
    const auto funGet = [graphic]([[maybe_unused]]const RS_Graphic* e) -> bool {
        return graphic->isGridOn();
    };
    const auto funSet = [this]([[maybe_unused]] const bool& v, [[maybe_unused]]RS_Graphic* e) -> void {
        QC_ApplicationWindow::getAppWindow()->getAction("ViewGrid")->trigger();
        m_widget->refill();
    };

    const LC_Property::Names names = {"gridVisible", tr("Show grid"), tr("Defines whether grid is shown or not")};
    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, graphic);
    return gridOn;
}

void LC_PropertiesProviderGraphicGrid::createGridType(LC_PropertyContainer* cont, RS_Graphic* graphic, bool showGrid) const {
    static LC_EnumDescriptor enumDescriptor = {
        "gridType",
        {
            {RS2::IsoGridViewType::Ortho, tr("Orthogonal")},
            {RS2::IsoGridViewType::IsoLeft, tr("Isometric Left")},
            {RS2::IsoGridViewType::IsoRight, tr("Isometric Right")},
            {RS2::IsoGridViewType::IsoTop, tr("Isometric Top")},
        }
    };

    auto funGet = [](const RS_Graphic* e) -> int {
        if (e->isIsometricGrid()) {
            return e->getIsoView();
        }
        return RS2::IsoGridViewType::Ortho;
    };
    auto funSet = [this](const LC_PropertyEnumValueType& v, [[maybe_unused]]RS_Graphic* e) -> void {
        const LC_PropertyEnumValueType gridType = v;
        QString actionName;
        switch (gridType) {
            case RS2::Ortho: {
                actionName = "ViewGridOrtho";
                break;
            }
            case RS2::IsoLeft: {
                actionName = "ViewGridIsoLeft";
                break;
            }
            case RS2::IsoRight: {
                actionName = "ViewGridIsoRight";
                break;
            }
            case RS2::IsoTop: {
                actionName = "ViewGridIsoTop";
                break;
            }
            default:
                actionName = "ViewGridOrtho";
        }
        const auto gridAction = QC_ApplicationWindow::getAppWindow()->getAction(actionName);
        if (gridAction != nullptr) {
            gridAction->trigger();
        }
        m_widget->refill();
    };

    const LC_Property::Names names = {"gridType", tr("Grid Type"), tr("Defines which time of grid should be drawn")};
    if (showGrid) {
        addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, &enumDescriptor, funGet, funSet, graphic);
    }
    else {
        addDirectEnum<LC_PropertyEnumValueType, RS_Graphic>(cont, names, &enumDescriptor, funGet, nullptr, graphic);
    }
}

void LC_PropertiesProviderGraphicGrid::createGridX(LC_PropertyContainer* const cont, RS_Graphic* graphic, bool showGrid) const {
    const LC_Property::Names names = {"gridX", tr("Spacing X"), tr("Spacing of grid by X axis")};

    const auto funGet = [](const RS_Graphic* e) -> double {
        const auto userGridSpacing = e->getUserGridSpacing();
        return userGridSpacing.x;
    };
    const auto funSet = [this](const double& v, RS_Graphic* e) -> void {
        auto userGridSpacing = e->getUserGridSpacing();
        userGridSpacing.x = v;
        e->setUserGridSpacing(userGridSpacing);
        notifyDrawingOptionsChanged();
        m_actionContext->getGraphicView()->redraw(RS2::RedrawGrid);
    };
    auto* property = new LC_PropertyDouble(cont, false);
    property->setNames(names);
    property->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::InputType::NOTNEEDED);
    LC_PropertyViewDescriptor attrs;
    attrs.viewName = LC_PropertyDoubleLineEditView::VIEW_NAME;
    attrs[LC_PropertyDoubleLineEditView::ATTR_ZERO_PLACEHOLDER] = tr("auto");
    attrs[LC_PropertyDoubleLineEditView::ATTR_MAX_LENGTH] = 5;
    attrs[LC_PropertyDoubleLineEditView::ATTR_POSITIVIE_VALUES_ONLY] = true;
    property->setViewDescriptor(attrs);
    property->setActionContextAndLaterRequestor(m_actionContext, m_widget);
    createDirectDelegatedStorage<double, RS_Graphic>(funGet, funSet, graphic, property);
    property->setReadOnly(!showGrid);
    cont->addChildProperty(property);
}

void LC_PropertiesProviderGraphicGrid::createGridY(LC_PropertyContainer* const cont, RS_Graphic* graphic, bool showGrid) const {
    const LC_Property::Names names = {"gridY", tr("Spacing Y"), tr("Spacing of grid by Y axis")};
    const auto funGet = [](const RS_Graphic* e) -> double {
        const auto userGridSpacing = e->getUserGridSpacing();
        return userGridSpacing.y;
    };
    const auto funSet = [this](const double& v, RS_Graphic* e) -> void {
        auto userGridSpacing = e->getUserGridSpacing();
        userGridSpacing.y = v;
        e->setUserGridSpacing(userGridSpacing);
        notifyDrawingOptionsChanged();
        m_actionContext->getGraphicView()->redraw(RS2::RedrawGrid);
    };
    auto* property = new LC_PropertyDouble(cont, false);
    property->setNames(names);
    property->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::InputType::NOTNEEDED);
    LC_PropertyViewDescriptor attrs;
    attrs.viewName = LC_PropertyDoubleLineEditView::VIEW_NAME;
    attrs[LC_PropertyDoubleLineEditView::ATTR_ZERO_PLACEHOLDER] = tr("auto");
    attrs[LC_PropertyDoubleLineEditView::ATTR_MAX_LENGTH] = 5;
    attrs[LC_PropertyDoubleLineEditView::ATTR_POSITIVIE_VALUES_ONLY] = true;
    property->setViewDescriptor(attrs);
    property->setActionContextAndLaterRequestor(m_actionContext, m_widget);
    createDirectDelegatedStorage<double, RS_Graphic>(funGet, funSet, graphic, property);
    property->setReadOnly(!showGrid);
    cont->addChildProperty(property);
}

bool LC_PropertiesProviderGraphicGrid::createShowMetaGrid(LC_PropertyContainer* cont, RS_Graphic* graphic, bool showGrid) const {
    bool gridOn = false;
    const auto graphicViewport = m_actionContext->getViewport();
    if (graphicViewport != nullptr) {
        const auto grid = graphicViewport->getGrid();
        if (grid != nullptr) {
            gridOn = grid->isDrawMetaGrid();
            auto funGet = [gridOn]([[maybe_unused]]const RS_Graphic* e) -> bool {
                return gridOn;
            };
            const LC_Property::Names names = {"gridMetaVisible", tr("Show meta-grid"), tr("Defines whether meta-grid is shown or not")};
            if (showGrid) {
                auto funSet = [this]([[maybe_unused]] const bool& v, [[maybe_unused]]RS_Graphic* e) -> void {
                    LC_SET_ONE("Appearance", "metaGridDraw", v);
                    const auto viewport = m_actionContext->getViewport();
                    if (viewport != nullptr) {
                        viewport->loadGridSettings();
                    }
                    m_widget->refill();
                };
                createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, graphic);
            }
            else {
                createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, nullptr, graphic);
            }
        }
    }
    return gridOn;
}

void LC_PropertiesProviderGraphicGrid::createDecimalMetaGrid(LC_PropertyContainer* cont, RS_Graphic* graphic, bool showGrid,
                                                             bool showMetaGrid) const {
    const auto graphicViewport = m_actionContext->getViewport();
    if (graphicViewport != nullptr) {
        const auto grid = graphicViewport->getGrid();
        if (grid != nullptr) {
            const bool gridIsMetric = grid->isGridMetric();
            if (gridIsMetric) {
                int metaGridStep = grid->getMetaGridEvery();
                auto* propertyMetaStepEvery = new LC_PropertyInt(cont, false);
                propertyMetaStepEvery->setNames({
                    "gridMetaEvery",
                    tr("Meta grid every"),
                    tr("Frequency of major grid lines compared to minor grid lines (amount of minor cells in major one).")
                });
                LC_PropertyViewDescriptor viewDescriptor;
                viewDescriptor.viewName = LC_PropertyIntSpinBoxView::VIEW_NAME;
                viewDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MIN] = 1;
                viewDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MAX] = 99;
                viewDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_STEP] = 1;
                propertyMetaStepEvery->setViewDescriptor(viewDescriptor);

                const auto funGetPagesHor = [metaGridStep]([[maybe_unused]]const RS_Graphic* e) -> int {
                    return metaGridStep;
                };

                const auto funSetPagesHor = [this](const int& v, [[maybe_unused]]RS_Graphic* e) -> void {
                    LC_SET_ONE("Appearance", "MetaGridEvery", v);
                    const auto viewport = m_actionContext->getViewport();
                    if (viewport != nullptr) {
                        viewport->loadGridSettings();
                    }
                    m_widget->refill();
                };

                createDirectDelegatedStorage<int, RS_Graphic>(funGetPagesHor, funSetPagesHor, graphic, propertyMetaStepEvery);
                propertyMetaStepEvery->setReadOnly(!(showGrid && showMetaGrid));
                cont->addChildProperty(propertyMetaStepEvery);
            }
        }
    }
}
