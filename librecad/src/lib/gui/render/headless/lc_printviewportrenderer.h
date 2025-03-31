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

#ifndef LC_PRINTVIEWPORTRENDERER_H
#define LC_PRINTVIEWPORTRENDERER_H

#include "lc_graphicviewportrenderer.h"

class QPaintDevice;

class LC_PrintViewportRenderer :public LC_GraphicViewportRenderer{
public:
    explicit LC_PrintViewportRenderer(LC_GraphicViewport *viewport, RS_Painter* painter);
    void renderEntity(RS_Painter *painter, RS_Entity *entity) override;
    RS2::DrawingMode getDrawingMode() {
        return drawingMode;
    }
    void setDrawingMode(RS2::DrawingMode mode){
        drawingMode = mode;
    }
    void setupPainter(RS_Painter *painter) override;
    bool isTextLineNotRenderable([[maybe_unused]]double uiLineHeight) const override
    {
        return false;
    }
    void setPaperScale(double p){paperScale = p;}
    void loadSettings() override;
protected:
    RS_Painter* painter {nullptr};
    double paperScale = 1.0;
    RS2::DrawingMode drawingMode = RS2::DrawingMode::ModeAuto;
    void setPenForPrintingEntity(RS_Painter *painter, RS_Entity *e);
    void doRender() override;
};

#endif // LC_PRINTVIEWPORTRENDERER_H
