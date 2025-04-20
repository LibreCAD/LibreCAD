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
#include "lc_widgetviewportrenderer.h"

#include <QPixmap>

#include "lc_graphicviewport.h"
#include "rs_entitycontainer.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_settings.h"

LC_WidgetViewPortRenderer::LC_WidgetViewPortRenderer(LC_GraphicViewport *viewport, QPaintDevice* paintDevice):
    LC_GraphicViewportRenderer(viewport, paintDevice)
    , pixmapLayerBackground{ std::make_unique<QPixmap>() }
    , pixmapLayerDrawing{ std::make_unique<QPixmap>() }
    , pixmapLayerOverlays{ std::make_unique<QPixmap>() }
    , m_pixmapLayer1{ std::make_unique<QPixmap>(1,1) }
{
}

LC_WidgetViewPortRenderer::~LC_WidgetViewPortRenderer() = default;


void LC_WidgetViewPortRenderer::loadSettings() {
    LC_GraphicViewportRenderer::loadSettings();
    LC_GROUP("Appearance");
    {
        antialiasing  = LC_GET_BOOL("Antialiasing");
        classicRenderer =  LC_GET_BOOL("ClassicRenderer", true);
    }

    LC_GROUP("Render");
    {
        m_render_minRenderableTextHeightInPx = LC_GET_INT("MinRenderableTextHeightPx", 4);
        int minArcRadius100 = LC_GET_INT("MinArcRadius", 80);
        m_render_minArcDrawingRadius = minArcRadius100 / 100.0;

        int minCircleRadius100 = LC_GET_INT("MinCircleRadius", 200);
        m_render_minCircleDrawingRadius = minCircleRadius100 / 100.0;

        int minLineLen100 = LC_GET_INT("MinLineLen", 200);
        m_render_minLineDrawingLen = minLineLen100 / 100.0;

        int minEllipseMajor100 = LC_GET_INT("MinEllipseMajor", 200);
        m_render_minEllipseMajorRadius = minEllipseMajor100 / 100.0;

        int minEllipseMinor100 = LC_GET_INT("MinEllipseMinor", 200);
        m_render_minEllipseMinorRadius = minEllipseMinor100 / 100.0;

        m_render_arcsInterpolate = LC_GET_BOOL("ArcRenderInterpolate", false);

        m_render_arcsInterpolateAngleFixed = LC_GET_BOOL("ArcRenderInterpolateSegmentFixed", true);

        int angle100 = LC_GET_INT("ArcRenderInterpolateSegmentAngle", 500);
        m_render_arcsInterpolateAngleValue = RS_Math::deg2rad(angle100 / 100.0);

        int sagittaMax = LC_GET_INT("ArcRenderInterpolateSegmentSagitta",90);
        m_render_arcsInterpolateMaxSagitta = sagittaMax / 100.0;

        m_render_circlesSameAsArcs = LC_GET_BOOL("CircleRenderAsArcs", false);
    } // Render group
    LC_GROUP_END();
}

void LC_WidgetViewPortRenderer::doRender() {

#ifdef DEBUG_RENDERING
    QElapsedTimer timer;
    timer.start();
    drawEntityCount = 0;
    entityDrawTime = 0;
    isVisibleTime = 0;
    isConstructionTime  = 0;
    setPenTime = 0;
    painterSetPenTime = 0;
    getPenTime = 0;


    drawLayerBackgroundTime = 0;
    drawLayerEntitiesTime = 0;
    drawLayerOverlaysTime = 0;
#endif
    if (antialiasing){
        if (classicRenderer) {
            paintClassicalBuffered(pd);
        }
        else{
            paintSequental(pd);
        }
    }
    else{
        paintClassicalBuffered(pd);
    }

#ifdef DEBUG_RENDERING
    LC_ERR<<"Paint:"  << timer.elapsed() <<
    " Layer 1 - Background: "  << drawLayerBackgroundTime <<" Layer 2 - Entities:"  << drawLayerEntitiesTime  <<" Layer 3 - overlays: "  << drawLayerOverlaysTime
    << " Entity Draw: " << entityDrawTime*1e-6 <<  " isVisible: " << isVisibleTime*1e-6 <<  " isConstruction: " << isConstructionTime*1e-6
    << " setPen: " << setPenTime*1e-6 <<  " getPen: " << getPenTime*1e-6 << " painter setPen: " << painterSetPenTime*1e-6 << " Entities: " << drawEntityCount;
#endif

    redrawMethod=RS2::RedrawNone;
}

void LC_WidgetViewPortRenderer::paintSequental(QPaintDevice* pd) {
    int width = viewport->getWidth();
    int height = viewport->getHeight();

    QSize const s0(width, height);
    if (pixmapLayerBackground->size() != s0){
        pixmapLayerBackground = std::make_unique<QPixmap>(width, height);
        redrawMethod=(RS2::RedrawMethod ) (redrawMethod | RS2::RedrawGrid);
    }

    if (redrawMethod & RS2::RedrawGrid) {
        pixmapLayerBackground->fill(m_colorBackground);
        RS_Painter painterBackground(pixmapLayerBackground.get());
        setupPainter(&painterBackground);
        drawLayerBackground(&painterBackground);
        painterBackground.end();
        redrawMethod=(RS2::RedrawMethod ) (redrawMethod | RS2::RedrawDrawing);
    }

    if (redrawMethod & RS2::RedrawDrawing) {
        // DRaw layer 2
        *pixmapLayerDrawing = *pixmapLayerBackground;
        RS_Painter painterLayerDrawing(pixmapLayerDrawing.get());
        setupPainter(&painterLayerDrawing);

        drawLayerEntities(&painterLayerDrawing);
        drawLayerEntitiesOver(&painterLayerDrawing);
        painterLayerDrawing.end();
        redrawMethod=(RS2::RedrawMethod ) (redrawMethod | RS2::RedrawOverlay);
    }

    if (redrawMethod & RS2::RedrawOverlay) {
        *pixmapLayerOverlays = *pixmapLayerDrawing;
        RS_Painter painterLayerOverlays(pixmapLayerOverlays.get());
        setupPainter(&painterLayerOverlays);

        painterLayerOverlays.setRenderHint(QPainter::Antialiasing);
        drawLayerOverlays(&painterLayerOverlays);
    }

    RS_Painter wPainter(pd);
    wPainter.drawPixmap(0, 0, *pixmapLayerOverlays);
}


void LC_WidgetViewPortRenderer::paintClassicalBuffered(QPaintDevice* pd) {
    int width = viewport->getWidth();
    int height = viewport->getHeight();
    QSize const s0(width, height);
    bool sizeDifferent = m_pixmapLayer1->size() != s0;
    if (sizeDifferent){
        m_pixmapLayer1 = std::make_unique<QPixmap>(width, height);
        m_pixmapLayer2 = std::make_unique<QPixmap>(width, height);
        m_pixmapLayer3 = std::make_unique<QPixmap>(width, height);
        redrawMethod = RS2::RedrawAll;
    }

    // Draw Layer 1
    if (redrawMethod & RS2::RedrawGrid) {
        m_pixmapLayer1->fill(m_colorBackground);
        RS_Painter painterBackground(m_pixmapLayer1.get());
        setupPainter(&painterBackground);
        drawLayerBackground(&painterBackground);
    }

    if (redrawMethod & RS2::RedrawDrawing) {
        // DRaw layer 2
        m_pixmapLayer2->fill(Qt::transparent);
        RS_Painter painterLayerDrawing(m_pixmapLayer2.get());
        setupPainter(&painterLayerDrawing);
        drawLayerEntities(&painterLayerDrawing);
        drawLayerEntitiesOver(&painterLayerDrawing);
    }

    if (redrawMethod & RS2::RedrawOverlay) {
        m_pixmapLayer3->fill(Qt::transparent);
        RS_Painter painter3(m_pixmapLayer3.get());
        setupPainter(&painter3);
        drawLayerOverlays( &painter3);
    }

    // Finally paint the layers back on the screen, bitblk to the rescue!
    RS_Painter wPainter(pd);
    wPainter.drawPixmap(0, 0, *m_pixmapLayer1);
    wPainter.drawPixmap(0, 0, *m_pixmapLayer2);
    wPainter.drawPixmap(0, 0, *m_pixmapLayer3);
}

void LC_WidgetViewPortRenderer::setupPainter(RS_Painter *painter) {
    LC_GraphicViewportRenderer::setupPainter(painter);
    painter->setMinCircleDrawingRadius(m_render_minCircleDrawingRadius);
    painter->setMinArcDrawingRadius(m_render_minArcDrawingRadius);
    painter->setMinLineDrawingLen(m_render_minLineDrawingLen);
    painter->setMinEllipseMajorRadius(m_render_minEllipseMajorRadius);
    painter->setMinEllipseMinorRadius(m_render_minEllipseMinorRadius);
    painter->setPenCapStyle(penCapStyle);
    painter->setPenJoinStyle(penJoinStyle);
    painter->setMinRenderableTextHeightInPx(m_render_minRenderableTextHeightInPx);

    painter->setRenderArcsInterpolate(m_render_arcsInterpolate);
    painter->setRenderArcsInterpolationAngleFixed(m_render_arcsInterpolateAngleFixed);
    painter->setRenderArcsInterpolationAngleValue(m_render_arcsInterpolateAngleValue);
    painter->setRenderArcsInterpolationMaxSagitta(m_render_arcsInterpolateMaxSagitta);
    painter->setRenderCirclesSameAsArcs(m_render_circlesSameAsArcs);

    if (antialiasing) {
        painter->setRenderHint(QPainter::Antialiasing);
    }
}


void LC_WidgetViewPortRenderer::drawLayerBackground(RS_Painter *painter) {
#ifdef DEBUG_RENDERING_DETAILS
    drawLayerBackgroundTimer.start();
#endif
    doDrawLayerBackground(painter);
#ifdef DEBUG_RENDERING_DETAILS
    drawLayerBackgroundTime += drawLayerBackgroundTimer.elapsed();
#endif
}


// fixme - sand - ADD additional pass with ordering of entities - in order to draw construction entities under normal ones!!!

void LC_WidgetViewPortRenderer::drawLayerEntities(RS_Painter* painter) {
#ifdef DEBUG_RENDERING_DETAILS
    drawLayerEntitiesTimer.start();
#endif

    RS_EntityContainer *container = viewport->getContainer();
    painter->setDrawSelectedOnly(false);
    doSetupBeforeContainerDraw();
    justDrawEntity(painter, container);

    painter->setDrawSelectedOnly(true);
    doSetupBeforeContainerDraw();
    justDrawEntity(painter, container);

#ifdef DEBUG_RENDERING_DETAILS
    drawLayerEntitiesTime += drawLayerEntitiesTimer.elapsed();
#endif
}

void LC_WidgetViewPortRenderer::doSetupBeforeContainerDraw() {
    lastPaintEntityPen = RS_Pen{};
    lastPaintEntityPen.setFlags(RS2::FlagInvalid);
}


void LC_WidgetViewPortRenderer::drawLayerOverlays(RS_Painter *painter) {
#ifdef DEBUG_RENDERING_DETAILS
    drawLayerOverlaysTimer.start();
#endif
    doDrawLayerOverlays(painter);
#ifdef DEBUG_RENDERING_DETAILS
    drawLayerOverlaysTime +=  drawLayerOverlaysTimer.elapsed();
#endif
}
