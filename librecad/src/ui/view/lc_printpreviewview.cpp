/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_printpreviewview.h"
#include "lc_printpreviewviewrenderer.h"

LC_PrintPreviewView::LC_PrintPreviewView(QWidget* parent, RS_Document* doc):QG_GraphicView(parent, doc){};
LC_PrintPreviewView::~LC_PrintPreviewView() {};

void LC_PrintPreviewView::createViewRenderer() {
    renderer =  new LC_PrintPreviewViewRenderer(viewport, this);
}

void LC_PrintPreviewView::setDrawingMode(RS2::DrawingMode m) {
    auto previewRenderer = dynamic_cast<LC_PrintPreviewViewRenderer *>(renderer);
    if (previewRenderer != nullptr) {
        previewRenderer->setDrawingMode(m);
    }
}

RS2::DrawingMode LC_PrintPreviewView::getDrawingMode() const {
    auto previewRenderer = dynamic_cast<LC_PrintPreviewViewRenderer *>(renderer);
    if (previewRenderer != nullptr) {
        return previewRenderer->getDrawingMode();
    }
    return RS2::DrawingMode::ModeFull;
}
