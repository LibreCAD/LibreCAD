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

#ifndef LC_WIDGETVIEWPORTRENDERER_H
#define LC_WIDGETVIEWPORTRENDERER_H

#include "lc_graphicviewportrenderer.h"

class QPixmap;

class LC_WidgetViewPortRenderer:public LC_GraphicViewportRenderer
{
public:
    explicit LC_WidgetViewPortRenderer(LC_GraphicViewport *viewport, QPaintDevice* paintDevice);
    ~LC_WidgetViewPortRenderer() override;
    void loadSettings() override;
    void setupPainter(RS_Painter* painter) override;
    void setAntialiasing(bool state) {antialiasing = state;}
    void invalidate(RS2::RedrawMethod method) {redrawMethod = static_cast<RS2::RedrawMethod>(redrawMethod | method);}
protected:
    void doRender() override;

    virtual void doSetupBeforeContainerDraw();
    void paintClassicalBuffered(QPaintDevice* pd);
    void paintSequental(QPaintDevice* pd);

    void drawLayerBackground(RS_Painter *painter);
    void drawLayerEntities(RS_Painter* painter);
    void drawLayerOverlays(RS_Painter *painter);

    virtual void drawLayerEntitiesOver([[maybe_unused]]RS_Painter* painter){}
    virtual void doDrawLayerBackground([[maybe_unused]]RS_Painter *painter) {}
    virtual void doDrawLayerOverlays([[maybe_unused]]RS_Painter *painter) {}
    int getMinRenderableTextHeightInPx() const {
        return m_render_minRenderableTextHeightInPx;
    }

#ifdef DEBUG_RENDERING
    QElapsedTimer drawLayerBackgroundTimer;
    QElapsedTimer drawLayerEntitiesTimer;
    QElapsedTimer drawLayerOverlaysTimer;
    long drawLayerBackgroundTime = 0;
    long drawLayerEntitiesTime = 0;
    long drawLayerOverlaysTime = 0;
#endif

private:
    bool antialiasing = false;
    bool classicRenderer = true;

    std::unique_ptr<QPixmap> pixmapLayerBackground;
    std::unique_ptr<QPixmap> pixmapLayerDrawing;
    std::unique_ptr<QPixmap> pixmapLayerOverlays;

    RS2::RedrawMethod redrawMethod = RS2::RedrawAll;

    int m_render_minRenderableTextHeightInPx = 4;
    double m_render_minCircleDrawingRadius = 2.0;
    double m_render_minArcDrawingRadius = 0.5;
    double m_render_minEllipseMajorRadius = 2.;
    double m_render_minEllipseMinorRadius = 1.;
    double m_render_minLineDrawingLen = 2;

    bool m_render_arcsInterpolate = true;
    bool m_render_arcsInterpolateAngleFixed = true;
    double m_render_arcsInterpolateAngleValue = M_PI / 36;
    double m_render_arcsInterpolateMaxSagitta = 0.9;
    bool m_render_circlesSameAsArcs = false;

    // Used for buffering different paint layers
    std::unique_ptr<QPixmap> m_pixmapLayer1;  // Used for grids and absolute 0
    std::unique_ptr<QPixmap> m_pixmapLayer2;  // Used for the actual CAD drawing
    std::unique_ptr<QPixmap> m_pixmapLayer3;  // Used for crosshair and actionitems
};

#endif // LC_WIDGETVIEWPORTRENDERER_H
