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

#include "lc_printpreviewviewrenderer.h"

#include "lc_graphicviewport.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_painter.h"

#define DEBUG_PRINT_PREVIEW_POINTS_NO

namespace{
    // fixme - move to nicer settings
    static const RS_Color printPreviewBorderAndShadowColor = RS_Color(64, 64, 64);
    static const RS_Color printPreviewBackgroundColor = RS_Color(200, 200, 200);
    static const RS_Color printPreviewPaperColor = RS_Color(180, 180, 180);
    static const RS_Color printPreviewPrintAreaColor = RS_Color(255, 255, 255);
}

LC_PrintPreviewViewRenderer::LC_PrintPreviewViewRenderer(LC_GraphicViewport *viewport, QPaintDevice* paintDevice)
   :LC_WidgetViewPortRenderer(viewport, paintDevice) {
}

void LC_PrintPreviewViewRenderer::doRender() {
    if (graphic != nullptr){
        m_paperScale = graphic->getPaperScale();
    }
    else{
        m_paperScale = 1.0;
    }
    LC_WidgetViewPortRenderer::doRender();
}

void LC_PrintPreviewViewRenderer::doDrawLayerBackground(RS_Painter *painter) {
    drawPaper(painter);
}

/**
 * Draws the paper border (for print previews).
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void LC_PrintPreviewViewRenderer::drawPaper(RS_Painter *painter) {
   if (m_paperScale < 1.0e-6) {
        return;
    }
// draw paper:
    RS_Vector pinsbase = graphic->getPaperInsertionBase();
    RS_Vector printAreaSize = graphic->getPrintAreaSize();

    double paperFactorX = painter->toGuiDX(1.0) / m_paperScale;
    double paperFactorY = painter->toGuiDX(1.0) / m_paperScale;

    int marginLeft = (int) (graphic->getMarginLeftInUnits() * paperFactorX);
    int marginTop = (int) (graphic->getMarginTopInUnits() * paperFactorY);
    int marginRight = (int) (graphic->getMarginRightInUnits() * paperFactorX);
    int marginBottom = (int) (graphic->getMarginBottomInUnits() * paperFactorY);

    const RS_Vector &wcsLeftBottomCorner = (RS_Vector(0, 0) - pinsbase) / m_paperScale;
    const RS_Vector &wcsTopBottomCorner = (printAreaSize - pinsbase) / m_paperScale;

    double v1x, v1y, v2x, v2y;
    painter->toGui(wcsLeftBottomCorner, v1x, v1y);
    painter->toGui(wcsTopBottomCorner, v2x, v2y);

    int numX = graphic->getPagesNumHoriz();
    int numY = graphic->getPagesNumVert();

    int viewWidth = viewport->getWidth();
    int viewHeight = viewport->getHeight();

// --- below we're graphic-agnostic
    int printAreaW = (int) (v2x - v1x);
    int printAreaH = (int) (v2y - v1y);

    int paperX1 = (int) v1x;
    int paperY1 = (int) v1y;
// Don't show margins between neighbor pages.
    int paperW = printAreaW + marginLeft + marginRight;
    int paperH = printAreaH - marginTop - marginBottom;

    painter->setPen(QColor(Qt::gray));

// gray background:
    painter->fillRect(0, 0, viewWidth, viewHeight,printPreviewBackgroundColor);

// shadow:
    painter->fillRect(paperX1 + 6, paperY1 + 6, paperW, paperH,printPreviewBorderAndShadowColor);

// border:
    painter->fillRect(paperX1, paperY1, paperW, paperH,printPreviewBorderAndShadowColor);

// paper:
    painter->fillRect(paperX1 + 1, paperY1 - 1, paperW - 2, paperH + 2,printPreviewPaperColor);

// print area:
    painter->fillRect(paperX1 + 1 + marginLeft, paperY1 - 1 - marginBottom,
                      printAreaW - 2, printAreaH + 2,
                      printPreviewPrintAreaColor);

// don't paint boundaries if zoom is to small
    if (qMin(std::abs(printAreaW / numX), std::abs(printAreaH / numY)) > 2) {
// boundaries between pages:
        for (int pX = 1; pX < numX; pX++) {
            double offset = ((double) printAreaW * pX) / numX;
            painter->fillRect(paperX1 + marginLeft + offset, paperY1,
                              1, paperH,
                              printPreviewBorderAndShadowColor);
        }
        for (int pY = 1; pY < numY; pY++) {
            double offset = ((double) printAreaH * pY) / numY;
            painter->fillRect(paperX1, paperY1 - marginBottom + offset,
                              paperW, 1,
                              printPreviewBorderAndShadowColor);
        }
    }


#ifdef DEBUG_PRINT_PREVIEW_POINTS
    // drawing zero
    const RS_Vector &zero = RS_Vector(0, 0);
    RS_Vector zeroGui = toGui(RS_Vector(zero) / scale);
    painter->fillRect(zeroGui.x - 5, zeroGui.y - 5, 10, 10,
                      RS_Color(255, 0, 0));

    // paper base point
    RS_Vector pinsBaseGui = toGui(-RS_Vector(pinsbase) / scale);

    painter->fillRect(pinsBaseGui.x - 5, pinsBaseGui.y - 5, 10, 10,
                      RS_Color(0, 255, 0));

    // ui point
    painter->fillRect(0, 0, 10, 10,
                      RS_Color(0, 0, 255));
#endif
}

void LC_PrintPreviewViewRenderer::renderEntity(RS_Painter *painter, RS_Entity *e) {
    // fixme - sand - ucs - is it really necessary for print preview??????
    // check for selected entity drawing
    if (/*!e->isContainer() && */(e->getFlag(RS2::FlagSelected) != painter->shouldDrawSelected())) {
        return;
    }
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

    if (!e->isPrint() || constructionEntity)
        return;

    if (isOutsideOfBoundingClipRect(e, constructionEntity)) {
        return;
    }

    // set pen (color):
    setPenForPrintingEntity(painter, e);
    justDrawEntity(painter, e);
}

void LC_PrintPreviewViewRenderer::setPenForPrintingEntity(RS_Painter *painter, RS_Entity *e) {
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

            if (m_paperScale > RS_TOLERANCE) {
                if (m_scaleLineWidth) {
                    wf = defaultWidthFactor;
                } else {
                    wf = 1.0 / m_paperScale;
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
    if (penColor.isEqualIgnoringFlags(printPreviewPrintAreaColor) ||
        (penColor.toIntColor() == RS_Color::Black && penColor.colorDistance(printPreviewPrintAreaColor) < RS_Color::MinColorDistance)) {
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

void LC_PrintPreviewViewRenderer::setupPainter(RS_Painter *painter)  {
    LC_WidgetViewPortRenderer::setupPainter(painter);
    painter->setDrawingMode(m_drawingMode);

    // disable rendering minimums for printing, let's printer decide
    painter->setMinCircleDrawingRadius(0);
    painter->setMinArcDrawingRadius(0);
    painter->setMinLineDrawingLen(0);
    painter->setMinEllipseMajorRadius(0);
    painter->setMinEllipseMinorRadius(0);
    painter->setPenCapStyle(Qt::RoundCap);
    painter->setPenJoinStyle(Qt::RoundJoin);
    painter->setMinRenderableTextHeightInPx(0);

    painter->setRenderArcsInterpolate(true);
    painter->setRenderArcsInterpolationAngleFixed(true);
    painter->setRenderArcsInterpolationAngleValue(M_PI/36); // 5 degrees
   // fixme - sand - paperspace - disabling UCS for print - for now, restore later
    painter->disableUCS();
}

void LC_PrintPreviewViewRenderer::setDrawingMode(RS2::DrawingMode mode){
    m_drawingMode = mode;
    viewport->notifyChanged();
}
