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

#include "lc_print_preview_options_filler.h"

#include "lc_enum_descriptor.h"
#include "lc_property_rect.h"
#include "lc_property_rsvector_view.h"
#include "rs_actionprintpreview.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_units.h"

void LC_PrintPreviewOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<RS_ActionPrintPreview*>(m_action);
    addBoolean({"a_scaleFixed", tr("Fixed scale"), tr("Print Scale is locked to the current value")}, [action]()-> bool {
                   return action->isPaperScaleFixed();
               }, [action](bool val)-> void {
                   action->setPaperScaleFixed(val);
               }, container);

    createCommandsLine(container, "a_commands1", tr("Center page"), tr("Page in center of drawing"), tr("Zoom to print"),
                       tr("Zoom to Print Area"), [action](int linkIndex)-> void {
                           if (linkIndex == 0) {
                               action->center();
                               action->updateOptions();
                           }
                           else {
                               action->zoomToPage();
                               action->updateOptions();
                           }
                       }, tr("Positioning commands"));

    createCommandsLine(container, "a_commands2", tr("Fit to 1 page"), tr("Fit the content to be printed on one page"), tr("Calc pages"),
                       tr("Calculate number of pages needed to contain the drawing"), [action](int linkIndex)-> void {
                           if (linkIndex == 0) {
                               action->fit();
                           }
                           else {
                               action->calcPagesNum(true);
                           }
                       }, tr("Content fit command"), !action->isPaperScaleFixed());

    addBoolean({"a_scaleLineWidth", tr("Scaled line width"), tr("Apply Print Scale to line width")}, [action]()-> bool {
                   return action->isLineWidthScaling();
               }, [action](bool val)-> void {
                   action->setLineWidthScaling(val);
               }, container);

    addBoolean({"a_blackWhite", tr("Black&White"), tr("Toggle Black / White mode")}, [action]()-> bool {
                   return action->isBlackWhite();
               }, [action](bool val)-> void {
                   action->setBlackWhite(val);
               }, container);

    addIntSpinbox({"printNumX", tr("Horizontal pages"), tr("Number of pages to print by horizontal")}, [action]() -> int {
                      const RS_Graphic* graphic = action->getActionContext()->getGraphic();
                      return graphic->getPlotSettings()->getPagesNumHoriz();
                  }, [action, this](const int& v) -> void {
                      const RS_Graphic* graphic = action->getActionContext()->getGraphic();
                      graphic->getPlotSettings()->setPagesNum(v, -1);
                      notifyDrawingOptionsChanged();
                  }, container);

    addIntSpinbox({"printNumY", tr("Vertical pages"), tr("Number of pages to print by vertical")}, [action]() -> int {
                      const RS_Graphic* graphic = action->getActionContext()->getGraphic();
                      return graphic->getPlotSettings()->getPagesNumVert();
                  }, [action, this](const int& v) -> void {
                      const RS_Graphic* graphic = action->getActionContext()->getGraphic();
                      graphic->getPlotSettings()->setPagesNum(-1, v);
                      notifyDrawingOptionsChanged();
                  }, container);

    createPaperSection(container->parent(), action);
}

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

void LC_PrintPreviewOptionsFiller::createPaperSection(QObject* parent, RS_ActionPrintPreview* action) {
    const auto parentContainer = dynamic_cast<LC_PropertyContainer*>(parent);
    if (parentContainer != nullptr) {
        const auto paperSection = createSection(parentContainer, {"_secPaper", "Paper", "Print paper settings"});
        createPageFormat(paperSection, action);
        createPageSize(paperSection, action);
        createPrintOrientation(paperSection, action);
        createPrintMargins(paperSection, action);
    }
}

void LC_PrintPreviewOptionsFiller::createPrintMargins(LC_PropertyContainer* cont, RS_ActionPrintPreview* action) {
    auto* propertyMargins = new LC_PropertyRect(cont, false);
    propertyMargins->setNames({"printMargins", tr("Margins"), tr("Margins for print page")});
    propertyMargins->setActionContext(m_actionContext);
    propertyMargins->setLeftDescription(tr("Left page margin"));
    propertyMargins->setRightDescription(tr("Right page margin"));
    propertyMargins->setTopDescription(tr("Top page margin"));
    propertyMargins->setBottomDescription(tr("Bottom page margin"));

    auto funGet = [action]() -> LC_MarginsRect {
        const RS_Graphic* graphic = action->getActionContext()->getGraphic();
        return graphic->getPlotSettings()->getMarginsInUnits();
    };

    auto funSet = [this, action](const LC_MarginsRect& v) -> void {
        const RS_Graphic* graphic = action->getActionContext()->getGraphic();
        graphic->getPlotSettings()->setMarginsInUnits(v.left, v.top, v.right, v.bottom);
        notifyDrawingOptionsChanged();
    };

    createDelegatedStorage<LC_MarginsRect>(propertyMargins, funGet, funSet);
    cont->addChildProperty(propertyMargins);
}

void LC_PrintPreviewOptionsFiller::createPageSize(LC_PropertyContainer* const cont, RS_ActionPrintPreview* action) const {
    bool landscape = false;
    const RS_Graphic* graphic = action->getActionContext()->getGraphic();
    if (graphic == nullptr) {
        return;
    }
    const RS2::PaperFormat paperFormat = graphic->getPlotSettings()->getPaperFormat(&landscape);
    const bool readOnlyPageSize = paperFormat != RS2::PaperFormat::Custom;
    auto* propertyPageSize = new LC_PropertyRSVector(cont, false);
    propertyPageSize->setNames({"printPageSize", tr("Page size"), tr("Size of page used for printing")});
    propertyPageSize->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::InputType::POINT);
    propertyPageSize->setViewDescriptorProvider([]() -> LC_PropertyViewDescriptor {
        return {
            {{LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME, tr("Width")}, {LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME, tr("Height")}}
        };
    });

    propertyPageSize->setActionContextAndLaterRequestor(m_actionContext, m_widget);

    auto funGet = [action]() -> RS_Vector {
        const RS_Graphic* g = action->getActionContext()->getGraphic();
        const auto paperSize = g->getPlotSettings()->getPaperSize();
        return paperSize;
    };

    auto funSet = [this, action](const RS_Vector& v) -> void {
        const RS_Graphic* graphic = action->getActionContext()->getGraphic();
        graphic->getPlotSettings()->setPaperSize(v);
        notifyDrawingOptionsChanged();
    };

    createDelegatedStorage<RS_Vector>(propertyPageSize, funGet, funSet);
    propertyPageSize->setReadOnly(readOnlyPageSize);
    cont->addChildProperty(propertyPageSize);
}

void LC_PrintPreviewOptionsFiller::notifyDrawingOptionsChanged() const {
    m_widget->stopInplaceEdit();
    QTimer::singleShot(30, []()-> void {
        QC_ApplicationWindow::getAppWindow()->notifyCurrentDrawingOptionsChanged();
    });
}

void LC_PrintPreviewOptionsFiller::createPageFormat(LC_PropertyContainer* const cont, RS_ActionPrintPreview* action) {
    auto funGetValue = [action]() -> RS2::PaperFormat {
        [[maybe_unused]] bool landscape = false;
        const RS_Graphic* g = action->getActionContext()->getGraphic();
        return g->getPlotSettings()->getPaperFormat(&landscape);
    };

    auto funSetValue = [action, this](LC_PropertyEnumValueType v) -> void {
        const RS_Graphic* g = action->getActionContext()->getGraphic();
        bool landscape = false;
        const auto ps = g->getPlotSettings();
        [[maybe_unused]] auto oldFormat = ps->getPaperFormat(&landscape);
        if (v == RS2::PaperFormat::Custom) {
            auto paperSize = ps->getPaperSize();
            ps->setPaperFormat(static_cast<RS2::PaperFormat>(v), landscape);
            //  this is very artificial trick - just to set some non-zero values for the paper size.
            // otherwise - page size for custom page format will be always 0,0. The user should changed it for custom anyway
            if (LC_LineMath::isMeaningful(paperSize.x) || LC_LineMath::isNotMeaningful(paperSize.y)) {
                paperSize.x = paperSize.x - 1;
                paperSize.y = paperSize.y - 1;
                ps->setPaperSize(paperSize);
            }
        }
        else {
            ps->setPaperFormat(static_cast<RS2::PaperFormat>(v), landscape);
        }
        notifyDrawingOptionsChanged();
    };

    static LC_EnumDescriptor paperFormatDescriptor = createPaperFormatDescriptor();
    const LC_Property::Names names = {"printPage", tr("Page"), tr("Page format used for printing")};
    addEnum(names, &paperFormatDescriptor, funGetValue, funSetValue, cont);
}

void LC_PrintPreviewOptionsFiller::createPrintOrientation(LC_PropertyContainer* const cont, RS_ActionPrintPreview* action) {
    static const LC_EnumDescriptor orientationDescriptor = {"printOrientation", {{0, tr("Landscape")}, {1, tr("Portrait")}}};

    auto funGetValue = [action]() -> LC_PropertyEnumValueType {
        const bool portrait = action->isPortrait();
        const int result = portrait ? 1 : 0;
        return result;
    };

    auto funSetValue = [action](LC_PropertyEnumValueType v) -> void {
        const bool portrait = v == 1;
        action->setPaperOrientation(portrait);
    };

    const LC_Property::Names names = {"printOrientation", tr("Orientation"), tr("Orientation of page for printing")};
    addEnum(names, &orientationDescriptor, funGetValue, funSetValue, cont);
}
