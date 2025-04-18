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

#ifndef LC_PRINTPREVIEWVIEWRENDERER_H
#define LC_PRINTPREVIEWVIEWRENDERER_H

#include "lc_widgetviewportrenderer.h"

class RS_Painter;

class LC_PrintPreviewViewRenderer:public LC_WidgetViewPortRenderer{
public:
    LC_PrintPreviewViewRenderer(LC_GraphicViewport *viewport, QPaintDevice* paintDevice);
    void renderEntity(RS_Painter *painter, RS_Entity *e) override;

    RS2::DrawingMode getDrawingMode() const {
        return m_drawingMode;
    }

    void setDrawingMode(RS2::DrawingMode mode);
    bool isTextLineNotRenderable([[maybe_unused]]double uiLineHeight) const override { return false;}
protected:
    void drawPaper(RS_Painter *painter);
    void setPenForPrintingEntity(RS_Painter *painter, RS_Entity *e);
    void doDrawLayerBackground(RS_Painter *painter) override;
    void setupPainter(RS_Painter *painter) override;
    void doRender() override;
private:
    RS2::DrawingMode m_drawingMode = RS2::DrawingMode::ModeAuto;
    double m_paperScale = 1.0;
};

#endif // LC_PRINTPREVIEWVIEWRENDERER_H
