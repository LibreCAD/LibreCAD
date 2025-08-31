/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_dimstylepreviewgraphicview.h"

#include <QMouseEvent>

#include "lc_defaultactioncontext.h"
#include "lc_eventhandler.h"
#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_actioninterface.h"
#include "rs_dimension.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_layer.h"

class LC_PreviewGraphic: public RS_Graphic {
public:
    explicit LC_PreviewGraphic() {};

    void setDimStyle(LC_DimStyle* dimStyle) {
        QString styleName = dimStyle->getName();
        RS2::EntityType styleType;
        LC_DimStyle::parseStyleName(styleName, m_currentStyleBaseName, styleType);
        getDimStyleList()->addDimStyle(dimStyle);
    }

    void onLoadingCompleted() override {}
    void clearDimStyles() {getDimStyleList()->clear();}

    LC_DimStyle* getResolvedDimStyle([[maybe_unused]]const QString& dimStyleName, RS2::EntityType dimType) const override {
        return RS_Graphic::getResolvedDimStyle(m_currentStyleBaseName, dimType);
    };
private:
    RS2::EntityType m_dimType {RS2::EntityUnknown};
    QString m_currentStyleBaseName;
};

class LC_PreviewActionContext: public LC_DefaultActionContext {
    public:
    explicit LC_PreviewActionContext(QG_ActionHandler* actionHandler)
        : LC_DefaultActionContext{actionHandler} {
    }

    ~LC_PreviewActionContext() override {deleteActionHandler();}
};


LC_DimStylePreviewGraphicView::LC_DimStylePreviewGraphicView(QWidget* parent, LC_ActionContext* actionContext):
  QG_GraphicView(parent, nullptr, actionContext){
}

LC_DimStylePreviewGraphicView::~LC_DimStylePreviewGraphicView() {
    deleteActionContext();
};

LC_DimStylePreviewGraphicView* LC_DimStylePreviewGraphicView::createAndSetupView(QWidget* parent, LC_PreviewGraphic* graphic, RS_Graphic* originalGraphic, bool showInWCS) {

    graphic->setAnglesBase(originalGraphic->getAnglesBase());
    graphic->setAnglesCounterClockwise((originalGraphic->areAnglesCounterClockWise()));


    auto* actionHandler = new QG_ActionHandler(nullptr);
    auto* actionContext = new LC_DefaultActionContext(actionHandler);
    actionHandler->setActionContext(actionContext);
    auto* result = new LC_DimStylePreviewGraphicView(parent, actionContext);
    actionHandler->setDocumentAndView(graphic->getDocument(), result);
    result->setContainer(graphic);
    auto viewport = result->getViewPort();
    viewport->setBorders(15,15,15,15);

    result->initView();
    result->addScrollbars();
    result->loadSettings();
    result->setDraftMode(false);
    result->setDraftLinesMode(false);

    auto* renderer = dynamic_cast<LC_GraphicViewRenderer*>(result->getRenderer());
    renderer->absZeroOptions()->m_extendAxisLines = false;
    renderer->ucsMarkOptions()->m_showWCSZeroMarker = false;
    renderer->relZeroOptions()->hideRelativeZero = true;

    if (!showInWCS) {
        auto ucs = originalGraphic->getCurrentUCS();
        viewport->applyUCS(ucs);
        delete ucs;
    }

    result->zoomAuto();

    return result;
}

void LC_DimStylePreviewGraphicView::copyBlocks(RS_Graphic* originalGraphic, LC_PreviewGraphic* graphic) {
    // copy blocks to preview graphics for arrows. Can't determine which blocks are for arrows, so force to copy all available ones
    auto srcBlockLock = originalGraphic->getBlockList();
    auto blockList = graphic->getBlockList();
    int blocksCount = srcBlockLock->count();
    if (blocksCount > 0) {
        for (RS_Block* block: *srcBlockLock) {
            auto* blockClone = static_cast<RS_Block*>(block->clone());
            blockList->add(blockClone, false);
        }
    }
}

LC_DimStylePreviewGraphicView* LC_DimStylePreviewGraphicView::init(QWidget* parent,RS_Graphic* originalGraphic, RS2::EntityType dimensionType) {
    auto graphic = new LC_PreviewGraphic();
    graphic->newDoc();
    copyBlocks(originalGraphic, graphic);

    bool loaded = RS_FileIO::instance()->fileImport(*graphic, ":/dxf/dim_sample.dxf", RS2::FormatUnknown);
    if (loaded) {
        LC_DimStylePreviewGraphicView* result = createAndSetupView(parent, graphic, originalGraphic, true);
        result->hideNonRelevantLayers(dimensionType);
        return result;
    }
    else {
        // how it could be???
        return nullptr;
    }
}

LC_DimStylePreviewGraphicView* LC_DimStylePreviewGraphicView::init(QWidget* parent, RS_Graphic* originalGraphic, RS_Dimension* dimension) {
    auto graphic = new LC_PreviewGraphic();
    graphic->newDoc();

    auto clone = dimension->clone();

    graphic->getDocument()->addEntity(clone);

    copyBlocks(originalGraphic, graphic);
    return createAndSetupView(parent, graphic, originalGraphic, false);
}

void LC_DimStylePreviewGraphicView::hideNonRelevantLayers(RS2::EntityType dimType) {
    if (dimType == RS2::EntityUnknown) {
        auto layersList = getGraphic(false)->getLayerList();
        for (auto layer: *layersList) {
            layer->freeze(false);
        }
    }
    else {
        QString layerToShow;
        switch (dimType) {
            case RS2::EntityDimLinear:
            case RS2::EntityDimAligned:
                layerToShow = "DIM$LINEAR";
                break;
            case RS2::EntityDimAngular:
                layerToShow = "DIM$ANGULAR";
                break;
            case RS2::EntityDimRadial:
                layerToShow = "DIM$RADIAL";
                break;
            case RS2::EntityDimDiametric:
                layerToShow = "DIM$DIAMETRIC";
                break;
            case RS2::EntityDimOrdinate:
                layerToShow = "DIM$ORDINATE";
                break;
            case RS2::EntityDimLeader:
            case RS2::EntityTolerance:
                layerToShow = "DIM$LEADER";
                break;
            default:
                break;
        }
        auto layersList = getGraphic(false)->getLayerList();
        for (auto layer: *layersList) {
            QString layerName = layer->getName();
            if (layerName != "0") {
                layer->freeze(layerName != layerToShow);
            }
        }
    }
}

bool LC_DimStylePreviewGraphicView::proceedEvent(QEvent* event) {
    return QWidget::event(event);
}

void LC_DimStylePreviewGraphicView::updateDims() {
    auto graphic = getGraphic();
    graphic->updateVisibleDimensions(true);
    redraw(RS2::RedrawDrawing);
    repaint();
}

void LC_DimStylePreviewGraphicView::refresh() {
    redraw(RS2::RedrawDrawing);
    repaint();
}

void LC_DimStylePreviewGraphicView::addDimStyle(LC_DimStyle* dimStyle) {
    auto* graphic = dynamic_cast<LC_PreviewGraphic*>(getGraphic());
    graphic->addDimStyle(dimStyle);
}

void LC_DimStylePreviewGraphicView::setDimStyle(LC_DimStyle* dimStyle) {
    auto* graphic = dynamic_cast<LC_PreviewGraphic*>(getGraphic());
    graphic->clearDimStyles();
    graphic->setDimStyle(dimStyle);
    RS2::EntityType dimType = dimStyle->getDimensionType();
    hideNonRelevantLayers(dimType);
}

void LC_DimStylePreviewGraphicView::setEntityDimStyle(LC_DimStyle* dimStyle, bool override, const QString& baseName) {
    auto* graphic = dynamic_cast<LC_PreviewGraphic*>(getGraphic());
    auto doc = graphic->getDocument();

    LC_DimStyle* styleOverride = nullptr;
    QString styleName = baseName;

    if (override) {
        styleOverride = dimStyle;
    }

    for (auto en : *doc) {
        if (RS2::isDimensionalEntity(en->rtti())) {
            auto dim = dynamic_cast<RS_Dimension*>(en);
            if (dim) {
                dim->setStyle(styleName);
                dim->setDimStyleOverride(styleOverride);
            }
        }
    }
}

void LC_DimStylePreviewGraphicView::setEntityPen(const RS_Pen &pen) {
    auto* graphic = dynamic_cast<LC_PreviewGraphic*>(getGraphic());
    auto doc = graphic->getDocument();

    for (auto en : *doc) {
        if (RS2::isDimensionalEntity(en->rtti())) {
            auto dim = dynamic_cast<RS_Dimension*>(en);
            if (dim) {
                dim->setPen(pen);
            }
        }
    }
}

void LC_DimStylePreviewGraphicView::setEntityArrowsFlipMode(bool flipArrow1, bool flipArrow2) {
    auto* graphic = dynamic_cast<LC_PreviewGraphic*>(getGraphic());
    auto doc = graphic->getDocument();

    for (auto en : *doc) {
        if (RS2::isDimensionalEntity(en->rtti())) {
            auto dim = dynamic_cast<RS_Dimension*>(en);
            if (dim) {
                dim->setFlipArrow1(flipArrow1);
                dim->setFlipArrow2(flipArrow2);
            }
        }
    }
}

void LC_DimStylePreviewGraphicView::zoomPan() {
    switchToAction(RS2::ActionZoomPan);
}

void LC_DimStylePreviewGraphicView::mousePressEvent(QMouseEvent* event){
    // pan zoom with middle mouse button
    if (event->button()==Qt::MiddleButton){
        switchToAction(RS2::ActionZoomPan);
        getCurrentAction()->mousePressEvent(event);
    }
    else {
        getEventHandler()->mousePressEvent(event);
    }
}

void LC_DimStylePreviewGraphicView::mouseDoubleClickEvent(QMouseEvent* e){
    switch(e->button()){
        case Qt::MiddleButton:
            switchToAction(RS2::ActionZoomAuto);
            break;
        default:
            break;
    }
    e->accept();
}

void LC_DimStylePreviewGraphicView::mouseReleaseEvent(QMouseEvent* event){
    event->accept();

    switch (event->button()) {
    case Qt::RightButton: {
        back();
        break;
    }
    case Qt::XButton1:
        processEnterKey();
        emit xbutton1_released();
        break;
    default:
        getEventHandler()->mouseReleaseEvent(event);
        break;
    }
}
