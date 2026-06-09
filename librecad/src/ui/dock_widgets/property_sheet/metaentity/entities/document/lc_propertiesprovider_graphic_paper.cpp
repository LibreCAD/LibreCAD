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

#include "lc_propertiesprovider_graphic_paper.h"

#include "lc_property_rect.h"
#include "lc_property_rsvector_view.h"
#include "lc_propertyprovider_utils.h"
#include "rs_graphic.h"
#include "rs_units.h"

const QString LC_PropertiesProviderGraphicPaper::SECTION_PAPER = "_secPrintPaper";

using namespace LC_PropertyProviderUtils;

namespace {
    LC_EnumDescriptor createPaperFormatDescriptor() {
        QVector<LC_EnumValueDescriptor> pageFormatValues;
        for (RS2::PaperFormat i = RS2::FirstPaperFormat; RS2::NPageFormat > i; i = static_cast<RS2::PaperFormat>(i + 1)) {
            auto desc = LC_EnumValueDescriptor(i, RS_Units::paperFormatToString(i));
            pageFormatValues.append(desc);
        }
        auto result = LC_EnumDescriptor("pageFormats", pageFormatValues);
        return result;
    }
}

void LC_PropertiesProviderGraphicPaper::fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) {
    if (m_widget->getOptions()->noSelectionPrintPaper) {
        const LC_Property::Names names = {SECTION_PAPER, tr("Print Paper"), tr("Print paper format and margins settings")};
        const auto cont = createSection(container, names);
        createPageFormat(cont, graphic);
        createPageSize(cont, graphic);
        createPrintOrientation(cont, graphic);
        createPrintMargins(cont, graphic);
        createTiledPrintingPages(cont, graphic);
    }
}

void LC_PropertiesProviderGraphicPaper::createTiledPrintingPages(LC_PropertyContainer* const cont, RS_Graphic* graphic) {
    LC_PropertyViewDescriptor pagesDescriptor;
    pagesDescriptor.viewName = LC_PropertyIntSpinBoxView::VIEW_NAME;
    pagesDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MIN] = 1;
    pagesDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_MAX] = 99;
    pagesDescriptor.attributes[LC_PropertyIntSpinBoxView::ATTR_STEP] = 1;

    auto* propertyPagesHor = new LC_PropertyInt(cont, false);
    propertyPagesHor->setNames({"printNumX", tr("Horizontal pages"), tr("Number of pages to print by horizontal")});
    propertyPagesHor->setViewDescriptor(pagesDescriptor);

    LC_PlotSettings* ps = graphic->getPlotSettings();

    auto funGetPagesHor = [](const LC_PlotSettings* e) -> int {
        return e->getPagesNumHoriz();
    };

    auto funSetPagesHor = [](const int& v, LC_PlotSettings* e) -> void {
        e->setPagesNum(v, -1);
    };

    createDirectDelegatedStorage<int, LC_PlotSettings>(funGetPagesHor, funSetPagesHor, ps, propertyPagesHor);
    cont->addChildProperty(propertyPagesHor);

    auto* propertyPagesVert = new LC_PropertyInt(cont, false);
    propertyPagesVert->setNames({"printNumY", tr("Vertical pages"), tr("Number of pages to print by vertical")});
    propertyPagesVert->setViewDescriptor(pagesDescriptor);

    auto funGetPagesVert = [](const LC_PlotSettings* e) -> int {
        return e->getPagesNumVert();
    };

    auto funSetPagesVert = [](const int& v, LC_PlotSettings* e) -> void {
        e->setPagesNum(-1, v);
    };

    createDirectDelegatedStorage<int, LC_PlotSettings>(funGetPagesVert, funSetPagesVert, ps, propertyPagesVert);
    cont->addChildProperty(propertyPagesVert);
}

void LC_PropertiesProviderGraphicPaper::createPrintMargins(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    auto* propertyMargins = new LC_PropertyRect(cont, false);
    propertyMargins->setNames({"printMargins", tr("Margins"), tr("Margins for print page")});
    propertyMargins->setActionContext(m_actionContext);
    propertyMargins->setLeftDescription(tr("Left page margin"));
    propertyMargins->setRightDescription(tr("Right page margin"));
    propertyMargins->setTopDescription(tr("Top page margin"));
    propertyMargins->setBottomDescription(tr("Bottom page margin"));

    LC_PlotSettings* ps = graphic->getPlotSettings();

    auto funGet = [](const LC_PlotSettings* e) -> LC_MarginsRect {
        return e->getMarginsInUnits();
    };

    auto funSet = [this](const LC_MarginsRect& v, LC_PlotSettings* e) -> void {
        e->setMarginsInUnits(v.left, v.top,  v.right,  v.bottom);
        notifyDrawingOptionsChanged();
    };

    createDirectDelegatedStorage<LC_MarginsRect, LC_PlotSettings>(funGet, funSet, ps, propertyMargins);
    cont->addChildProperty(propertyMargins);
}

void LC_PropertiesProviderGraphicPaper::createPrintOrientation(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    static const LC_EnumDescriptor orientationDescriptor = {"printOrientation", {
        {0, tr("Landscape")},
        {1, tr("Portrait")}}
    };

    LC_PlotSettings* ps = graphic->getPlotSettings();

    auto funGetValue = [](const LC_PlotSettings* e) -> LC_PropertyEnumValueType {
        bool landscape = false;
        [[maybe_unused]] RS2::PaperFormat format = e->getPaperFormat(&landscape);
        const int result = landscape ? 0 : 1;
        return result;
    };

    auto funSetValue = [this]([[maybe_unused]] const LC_PropertyEnumValueType& v,  LC_PlotSettings* e) -> void {
        bool landscape = false;
        [[maybe_unused]] const RS2::PaperFormat format = e->getPaperFormat(&landscape);
        const bool newLandscape = v == 0;
        e->setPaperFormat(format, newLandscape);
        notifyDrawingOptionsChanged();
    };

    const LC_Property::Names names = {"printOrientation", tr("Orientation"), tr("Orientation of page for printing")};
    addDirectEnum<LC_PropertyEnumValueType, LC_PlotSettings>(cont, names, &orientationDescriptor, funGetValue, funSetValue, ps);
}

void LC_PropertiesProviderGraphicPaper::createPageSize(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    bool landscape = false;
    const auto ps = graphic->getPlotSettings();
    const RS2::PaperFormat paperFormat = ps->getPaperFormat(&landscape);

    const bool readOnlyPageSize = paperFormat != RS2::PaperFormat::Custom;
    auto* propertyPageSize = new LC_PropertyRSVector(cont, false);
    propertyPageSize->setNames({"printPageSize", tr("Page size"), tr("Size of page used for printing")});
    propertyPageSize->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::InputType::POINT);
    propertyPageSize->setViewDescriptorProvider([]() -> LC_PropertyViewDescriptor {
        return {
            {{LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME, tr("Width")},
                        {LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME, tr("Height")}
            }
        };
    });

    propertyPageSize->setActionContextAndLaterRequestor(m_actionContext, m_widget);

    auto funGet = [](const LC_PlotSettings* e) -> RS_Vector {
        const auto paperSize = e->getPaperSize();
        return paperSize;
    };

    auto funSet = [this](const RS_Vector& v, LC_PlotSettings* e) -> void {
        e->setPaperSize(v);
        notifyDrawingOptionsChanged();
    };

    createDirectDelegatedStorage<RS_Vector, LC_PlotSettings>(funGet, funSet, ps, propertyPageSize);
    propertyPageSize->setReadOnly(readOnlyPageSize);
    cont->addChildProperty(propertyPageSize);
}

void LC_PropertiesProviderGraphicPaper::createPageFormat(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    LC_PlotSettings* ps = graphic->getPlotSettings();
    auto funGetValue = [](const LC_PlotSettings* e) -> RS2::PaperFormat {
        [[maybe_unused]] bool landscape = false;
        return e->getPaperFormat(&landscape);
    };

    auto funSetValue = [this]([[maybe_unused]] const LC_PropertyEnumValueType& v,  LC_PlotSettings* e) -> void {
        bool landscape = false;
        [[maybe_unused]] auto oldFormat = e->getPaperFormat(&landscape);
        e->setPaperFormat(static_cast<RS2::PaperFormat>(v), landscape);
        notifyDrawingOptionsChanged();
    };

    static LC_EnumDescriptor paperFormatDescriptor = createPaperFormatDescriptor();
    const LC_Property::Names names = {"printPage", tr("Page"), tr("Page format used for printing")};
    addDirectEnum<LC_PropertyEnumValueType, LC_PlotSettings>(cont, names, &paperFormatDescriptor, funGetValue, funSetValue, ps);
}
