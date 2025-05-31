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

#include <boost/geometry/strategies/strategy_transform.hpp>

#include "lc_graphicviewport.h"
#include "lc_graphicviewrenderer.h"
#include "rs_dimension.h"
#include "rs_fileio.h"
#include "rs_graphic.h"

LC_DimStylePreviewGraphicView::LC_DimStylePreviewGraphicView(QWidget* parent):
  QG_GraphicView(parent, nullptr, nullptr){
}

LC_DimStylePreviewGraphicView::~LC_DimStylePreviewGraphicView() = default;

class LC_PreviewGraphic: public RS_Graphic {
public:
    explicit LC_PreviewGraphic(LC_DimStyle* dimStyle):RS_Graphic(), m_fixedDimStyle{dimStyle} {
    };

    void setDimStyle(LC_DimStyle* dimStyle) {
        m_fixedDimStyle = dimStyle;
    }

    void onLoadingCompleted() override {};

    LC_DimStyle* getResolvedDimStyle(RS_Dimension* dimension) override{
        return m_fixedDimStyle;
    }
private:
     LC_DimStyle* m_fixedDimStyle {nullptr}; // ignore style in super class, to avoid reset() and deleting passed object
};

LC_DimStylePreviewGraphicView* LC_DimStylePreviewGraphicView::init(QWidget* parent, LC_DimStyle* dimStyle, RS_Graphic* originalGraphic) {
    LC_DimStylePreviewGraphicView* result = nullptr;
    auto graphic = new LC_PreviewGraphic(dimStyle);
    graphic->newDoc();

    // copy blocks to preview graphics for arrows
    auto srcBlockLock = originalGraphic->getBlockList();
    auto blockList = graphic->getBlockList();
    int blocksCount = srcBlockLock->count();
    if (blocksCount > 0) {
        for (RS_Block* block: *srcBlockLock) {
            RS_Block* blockClone = static_cast<RS_Block*>(block->clone());
            blockList->add(blockClone, false);
        }
    }

    bool loaded = RS_FileIO::instance()->fileImport(*graphic, ":/dxf/dim_sample.dxf", RS2::FormatUnknown);
    if (loaded) {
        result = new LC_DimStylePreviewGraphicView(parent);
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

        result->zoomAuto();
    }
    else {
        // how it could be???
    }
    return result;
}

void LC_DimStylePreviewGraphicView::updateDims() {
    auto graphic = getGraphic();
    graphic->updateDimensions(true);
    redraw(RS2::RedrawDrawing);
    repaint();
}

void LC_DimStylePreviewGraphicView::setDimStyle(LC_DimStyle* dimStyle) {
    auto* graphic = dynamic_cast<LC_PreviewGraphic*>(getGraphic());
    graphic->setDimStyle(dimStyle);
    graphic->updateDimensions(true);
    redraw(RS2::RedrawDrawing);
    repaint();
}
