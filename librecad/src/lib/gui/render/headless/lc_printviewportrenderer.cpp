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

#include "lc_printviewportrenderer.h"

#include "lc_graphicviewport.h"
#include "rs_entitycontainer.h"
#include "rs_math.h"
#include "rs_painter.h"

class RS_EntityContainer;

LC_PrintViewportRenderer::LC_PrintViewportRenderer(LC_GraphicViewport *viewport, RS_Painter* p)
   :LC_GraphicViewportRenderer(viewport, nullptr)
    ,painter{p}
{
   setBackground({255,255,255});
}


void LC_PrintViewportRenderer::doRender() {
    setupPainter(painter);
    RS_EntityContainer *container = viewport->getContainer();
    container->draw(painter);
}


void LC_PrintViewportRenderer::renderEntity(RS_Painter *painter, RS_Entity *e) {
#ifdef DEBUG_RENDERING
    isVisibleTimer.start();
#endif
    // entity is not visible:
    bool visible = e->isVisible();
#ifdef DEBUG_RENDERING
    isVisibleTime += isVisibleTimer.nsecsElapsed();
#endif
    if (!visible) {
        return;
    }

#ifdef DEBUG_RENDERING
    isConstructionTimer.start();
#endif
    bool constructionEntity = e->isConstruction();
#ifdef DEBUG_RENDERING
    isConstructionTime += isConstructionTimer.nsecsElapsed();
#endif
    // do not draw construction layer on print preview or print
    if (!e->isPrint() || constructionEntity)
        return;

    if (isOutsideOfBoundingClipRect(e, constructionEntity)) {
        return;
    }
    setPenForPrintingEntity(painter, e);
    justDrawEntity(painter, e);
}

void LC_PrintViewportRenderer::setPenForPrintingEntity(RS_Painter *painter, RS_Entity *e) {
#ifdef DEBUG_RENDERING
    setPenTimer.start();
#endif
    // Getting pen from entity (or layer)
    RS_Pen pen = e->getPenResolved();
    RS_Pen originalPen = pen;

    double patternOffset = painter->currentDashOffset();
    if (lastPaintEntityPen.isSameAs(pen, patternOffset)) {
        return;
    }
    // Avoid negative widths
    double width = pen.getWidth();
//    int w = std::max(static_cast<int>(pen.getWidth()), 0);

// - Scale pen width.
// - By default pen width is not scaled on print and print preview.
//   This is the standard (AutoCAD like) behaviour.
// bug# 3437941
// ------------------------------------------------------------

    if (pen.getAlpha() == 1.0) {
        if (width >0) {
            double wf = 1.0; // Width factor.

            if (paperScale > RS_TOLERANCE) {
                if (m_scaleLineWidth) {
                    wf = defaultWidthFactor;
                } else {
                    wf = 1.0 / paperScale;
                }
            }
            double screenWidth = painter->toGuiDX(width * unitFactor100 * wf);

            /*// prevent drawing with 1-width which is slow:
            if (RS_Math::round(pen.getScreenWidth()) == 1) {
                pen.setScreenWidth(0.0);
            }*/
            // fixme - not sure about this check. However, without it, lines will stay transparent and then disappear on zooming out.
            //  Probably some other threshold value (instead 1) should be used?
            if (screenWidth < 1){
                screenWidth = 0.0;
            }
            pen.setScreenWidth(screenWidth);
        }
        else{
            pen.setScreenWidth(0.0);
        }
    }
    else{
        // fixme - if we'll support transparency, add necessary processing there
        if (RS_Math::round(pen.getScreenWidth()) == 1) {
            pen.setScreenWidth(0.0);
        }
    }

    RS_Color penColor = pen.getColor();
    if (penColor.isEqualIgnoringFlags(m_colorBackground) ||
        (penColor.toIntColor() == RS_Color::Black && penColor.colorDistance(m_colorBackground) < RS_Color::MinColorDistance)) {
        pen.setColor(m_colorForeground);
    }

    if (pen.getLineType() != RS2::SolidLine){
        pen.setDashOffset(patternOffset * defaultWidthFactor);
    }
    
    if (e->getFlag(RS2::FlagTransparent) ) {
        pen.setColor(m_colorBackground);
    }

    // we store original pen as last painted, not resolved one - since original pen lead to resulting resolved and may be used by the next entity
    lastPaintEntityPen.updateBy(originalPen);
    painter->setPen(pen);
#ifdef DEBUG_RENDERING
    setPenTime += setPenTimer.nsecsElapsed();
#endif
}


void LC_PrintViewportRenderer::loadSettings() {
    LC_GraphicViewportRenderer::loadSettings();
    setBackground({255,255,255});
}


void LC_PrintViewportRenderer::setupPainter(RS_Painter* painter) {
    LC_GraphicViewportRenderer::setupPainter(painter);
    painter->setDrawingMode(drawingMode);
    // fixme - sand - paperspace - disabling UCS for print - for now, restore later
    painter->disableUCS();
}
