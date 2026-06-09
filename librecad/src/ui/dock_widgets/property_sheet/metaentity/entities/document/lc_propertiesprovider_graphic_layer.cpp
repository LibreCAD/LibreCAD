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

#include "lc_propertiesprovider_graphic_layer.h"

#include "lc_layertreewidget.h"
#include "lc_property_layer.h"
#include "lc_property_linetype.h"
#include "lc_property_linetype_combobox_view.h"
#include "lc_property_linewidth.h"
#include "lc_property_linewidth_combobox_view.h"
#include "lc_property_rscolor.h"
#include "lc_property_rscolor_combobox_view.h"
#include "lc_propertyprovider_utils.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"

const QString LC_PropertiesProviderGraphicLayer::SECTION_LAYER = "_secLayer";

using namespace LC_PropertyProviderUtils;

namespace {
    void updateLayerPen(RS_Layer* layer, const RS_Pen& pen, RS_Graphic* graphic) {
        layer->setPen(pen);
        graphic->activateLayer(layer, true);
        for (const auto ent : *graphic) {
            if (ent->isDeleted()) {
                continue;
            }
            const RS_Layer* l = ent->getLayer();
            if (l == layer) {
                ent->update();
            }
        }
    }
}

void LC_PropertiesProviderGraphicLayer::fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic) {
    if (m_widget->getOptions()->noSelectionActiveLayer) {
        const LC_Property::Names names = {SECTION_GENERAL, tr("Active Layer"), tr("Active layer properties")};
        const auto cont = createSection(container, names);
        createActiveLayer(cont, graphic);
        createLayerType(cont, graphic);
        createColor(cont, graphic);
        createLineWidth(cont, graphic);
        createLineType(cont, graphic);
        createVisible(graphic, cont);
        createLocked(graphic, cont);
        createPrintable(graphic, cont);
        createConstruction(cont, graphic);
        if (isShowLinks()) {
            createLayerCommands(cont, graphic);
        }
    }
}

void LC_PropertiesProviderGraphicLayer::createActiveLayer(LC_PropertyContainer* const cont, RS_Graphic* graphic) {
    const LC_Property::Names names = {"layer", tr("Name"), tr("Currently active layer of the document")};
    RS_LayerList* layerList = graphic->getLayerList();
    auto* activeLayerProperty = new LC_PropertyLayer(cont, false);
    activeLayerProperty->setNames(names);
    activeLayerProperty->setLayerList(layerList);
    activeLayerProperty->setAllowByBlockValues(false);

    auto funGet = [](const RS_Graphic* e) -> RS_Layer* {
        return e->getActiveLayer();
    };
    auto funSet = [](RS_Layer* l, RS_Graphic* e) -> void {
        e->activateLayer(l, true);
    };

    createDirectDelegatedStorage<RS_Layer*, RS_Graphic>(funGet, funSet, graphic, activeLayerProperty);
    cont->addChildProperty(activeLayerProperty);
}

bool LC_PropertiesProviderGraphicLayer::inPrintPreview() const {
    return m_actionContext->getGraphicView()->isPrintPreview();
}

void LC_PropertiesProviderGraphicLayer::createColor(LC_PropertyContainer* const cont, RS_Graphic* graphic) {
    const LC_Property::Names names = {"color", tr("Color"), tr("Color of active layer")};
    auto* layerColorProperty = new LC_PropertyRSColor(cont, false);
    layerColorProperty->setNames(names);
    layerColorProperty->setViewAttribute(LC_PropertyRSColorComboBoxView::ATTR_SHOW_BY_LAYER, false);

    auto funGet = [](const RS_Graphic* e) -> RS_Color {
        const auto layer = e->getActiveLayer();
        return layer->getPen().getColor();
    };
    auto funSet = [](const RS_Color& color, RS_Graphic* e) -> void {
        const auto layer = e->getActiveLayer();
        RS_Pen pen = layer->getPen();
        pen.setColor(color);
        updateLayerPen(layer, pen, e);
    };

    createDirectDelegatedStorage<RS_Color, RS_Graphic>(funGet, funSet, graphic, layerColorProperty);
    cont->addChildProperty(layerColorProperty);
}

void LC_PropertiesProviderGraphicLayer::createLineWidth(LC_PropertyContainer* const cont, RS_Graphic* graphic) {
    const LC_Property::Names names = {"linewidth", tr("Line Width"), tr("Line width for active layer's pen")};

    auto* layerLineWidthProperty = new LC_PropertyLineWidth(cont, false);
    layerLineWidthProperty->setViewAttribute(LC_PropertyLineWidthComboboxView::ATTR_SHOW_BY_LAYER, false);
    layerLineWidthProperty->setNames(names);

    auto funGet = [](const RS_Graphic* e) -> RS2::LineWidth {
        const auto layer = e->getActiveLayer();
        return layer->getPen().getWidth();
    };
    auto funSet = [](const RS2::LineWidth& width, RS_Graphic* e) -> void {
        const auto layer = e->getActiveLayer();
        RS_Pen pen = layer->getPen();
        pen.setWidth(width);
        updateLayerPen(layer, pen, e);
    };

    createDirectDelegatedStorage<RS2::LineWidth, RS_Graphic>(funGet, funSet, graphic, layerLineWidthProperty);
    cont->addChildProperty(layerLineWidthProperty);
}

void LC_PropertiesProviderGraphicLayer::createLineType(LC_PropertyContainer* const cont, RS_Graphic* graphic) {
    const LC_Property::Names names = {"linetype", tr("Line Type"), tr("Type of line for active layer's pen")};
    auto* layerLineTypeProperty = new LC_PropertyLineType(cont, false);
    layerLineTypeProperty->setViewAttribute(LC_PropertyLineTypeComboboxView::ATTR_SHOW_BY_LAYER, false);
    layerLineTypeProperty->setNames(names);
    auto funGet = [](const RS_Graphic* e) -> RS2::LineType {
        const auto layer = e->getActiveLayer();
        return layer->getPen().getLineType();
    };
    auto funSet = [](const RS2::LineType& linetype, RS_Graphic* e) -> void {
        const auto layer = e->getActiveLayer();
        RS_Pen pen = layer->getPen();
        pen.setLineType(linetype);
        updateLayerPen(layer, pen, e);
    };
    createDirectDelegatedStorage<RS2::LineType, RS_Graphic>(funGet, funSet, graphic, layerLineTypeProperty);
    cont->addChildProperty(layerLineTypeProperty);
}

void LC_PropertiesProviderGraphicLayer::createLayerType(LC_PropertyContainer* const cont, RS_Graphic* graphic) {
    QString layerTypeValue;
    const auto layerType = graphic->getActiveLayer()->getLayerType();
    switch (layerType) {
        case RS_Layer::NORMAL: {
            layerTypeValue = tr("Normal");
            break;
        }
        case RS_Layer::DIMENSIONAL: {
            layerTypeValue = tr("Dimensional");
            break;
        }
        case RS_Layer::ALTERNATE_POSITION: {
            layerTypeValue = tr("Alternative position");
            break;
        }
        case RS_Layer::INFORMATIONAL: {
            layerTypeValue = tr("Informational");
            break;
        }
        default:
            layerTypeValue = tr("Normal");
            break;
    }
    const LC_Property::Names names = {"layerType", tr("Type"), tr("Type of the currently active layer of the document")};
    createDirectDelegatedReadonlyString(cont, names, layerTypeValue);
}

void LC_PropertiesProviderGraphicLayer::createVisible(RS_Graphic* graphic, LC_PropertyContainer* const cont) {
    auto funGet = [](const RS_Graphic* e) -> bool {
        const auto layer = e->getActiveLayer();
        return layer->isFrozen();
    };
    auto funSet = []([[maybe_unused]] const bool& v, RS_Graphic* e) -> void {
        const auto layer = e->getActiveLayer();
        e->toggleLayer(layer);
    };

    const LC_Property::Names names = {"layerVisible", tr("Visible layer"), tr("Defines whether active layer is visible or not")};
    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicLayer::createLocked(RS_Graphic* graphic, LC_PropertyContainer* const cont) {
    auto funGet = [](const RS_Graphic* e) -> bool {
        const auto layer = e->getActiveLayer();
        return layer->isLocked();
    };
    auto funSet = []([[maybe_unused]] const bool& v, RS_Graphic* e) -> void {
        const auto layer = e->getActiveLayer();
        e->toggleLayerLock(layer);
    };

    const LC_Property::Names names = {"layerLocked", tr("Locked"), tr("Defines whether active layer is locked or not")};
    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicLayer::createPrintable(RS_Graphic* graphic, LC_PropertyContainer* const cont) {
    auto funGet = [](const RS_Graphic* e) -> bool {
        const auto layer = e->getActiveLayer();
        return layer->isPrint();
    };
    auto funSet = []([[maybe_unused]] const bool& v, RS_Graphic* e) -> void {
        const auto layer = e->getActiveLayer();
        e->toggleLayerPrint(layer);
    };

    const LC_Property::Names names = {"layerPrintable", tr("Printable layer"), tr("Defines whether active layer is printable or not")};
    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicLayer::createConstruction(LC_PropertyContainer* const cont, RS_Graphic* graphic) {
    auto funGet = [](const RS_Graphic* e) -> bool {
        const auto layer = e->getActiveLayer();
        return layer->isConstruction();
    };

    auto funSet = []([[maybe_unused]] const bool& v, RS_Graphic* e) -> void {
        const auto layer = e->getActiveLayer();
        e->toggleLayerConstruction(layer);
    };

    const LC_Property::Names names = {"layerConstruction", tr("Construction layer"), tr("Defines whether active layer is construction or not")};
    createDirectDelegatedBool<RS_Graphic>(cont, names, funGet, funSet, graphic);
}

void LC_PropertiesProviderGraphicLayer::createAddRemoveCommands(LC_PropertyContainer* const cont, RS_Graphic* graphic, const bool nonZeroLayer) const {
    auto clickHandler = []([[maybe_unused]] RS_Graphic* g, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                QC_ApplicationWindow::getAppWindow()->getLayerTreeWidget()->addLayer();
                break;
            }
            case 1: {
                // removing layer
                // todo - add possibility to define whether child layers shoild be deleted too...
                QC_ApplicationWindow::getAppWindow()->getLayerTreeWidget()->removeActiveLayer(
                    false);
                break;
            }
            default:
                break;
        }
    };
    createSingleEntityCommand<RS_Graphic>(cont, "layerStructure", tr("Add layer..."), tr("Invokes creation of the layer"),
                                          nonZeroLayer ? tr("Remove layer...") : "",
                                          nonZeroLayer ? tr("Invokes removal of the layer from the drawing") : "", graphic,
                                          clickHandler, tr("Layers list commands"));
}

void LC_PropertiesProviderGraphicLayer::createLockingCommand(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    auto clickHandler = [](RS_Graphic* g, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                // unlock all
                QList<RS_Layer*> layersToUnlock;
                const QList<RS_Layer*> layersToLock;
                // auto activeLayer = graphic->getActiveLayer();
                const auto layerList = g->getLayerList();
                for (const auto l : *layerList) {
                    layersToUnlock.append(l);
                }
                g->setLockLayers(layersToUnlock, layersToLock);
                break;
            }
            case 1: {
                const QList<RS_Layer*> layersToUnlock;
                QList<RS_Layer*> layersToLock;
                const auto activeLayer = g->getActiveLayer();
                const auto layerList = g->getLayerList();
                for (const auto l : *layerList) {
                    if (l != activeLayer) {
                        layersToLock.append(l);
                    }
                }
                g->setLockLayers(layersToUnlock, layersToLock);
                break;
            }
            default:
                break;
        }
    };
    createSingleEntityCommand<RS_Graphic>(cont, "layerLocking", tr("Unlock all layers"), tr("All layers will be unlocked"),
                                          tr("Lock other layers"), tr("All layers except active one will be locked"), graphic,
                                          clickHandler, tr("Layers locking commands"));
}

void LC_PropertiesProviderGraphicLayer::createVisibleCommand(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    auto clickHandler = [](RS_Graphic* g, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                // show all
                QList<RS_Layer*> layersToShow;
                const QList<RS_Layer*> layersToHide;
                // auto activeLayer = graphic->getActiveLayer();
                const auto layerList = g->getLayerList();
                for (const auto l : *layerList) {
                    layersToShow.append(l);
                }
                g->setFreezeLayers(layersToShow, layersToHide);
                break;
            }
            case 1: {
                // hide other
                const QList<RS_Layer*> layersToShow;
                QList<RS_Layer*> layersToHide;
                const auto activeLayer = g->getActiveLayer();
                const auto layerList = g->getLayerList();
                for (const auto l : *layerList) {
                    if (l != activeLayer) {
                        layersToHide.append(l);
                    }
                }
                g->setFreezeLayers(layersToShow, layersToHide);
                break;
            }
            default:
                break;
        }
    };
    createSingleEntityCommand<RS_Graphic>(cont, "layerHiding", tr("Show all layers"), tr("All layers become visible"),
                                          tr("Hide other layers"), tr("All layers except active one will be hidden"), graphic,
                                          clickHandler, tr("Layers visibility commands"));
}

void LC_PropertiesProviderGraphicLayer::createLayerCommands(LC_PropertyContainer* const cont, RS_Graphic* graphic) const {
    const auto activeLayer = graphic->getActiveLayer();
    if (activeLayer != nullptr) {
        const auto layerName = activeLayer->getName();
        const bool nonZeroLayer = "0" != layerName;
        const bool isPrintPreview = inPrintPreview();
        if (!isPrintPreview) {
            createAddRemoveCommands(cont, graphic, nonZeroLayer);
        }

        createLockingCommand(cont, graphic);
        createVisibleCommand(cont, graphic);
    }
}
