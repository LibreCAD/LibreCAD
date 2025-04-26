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

#ifndef LC_GRAPHICVIEWPORTRENDERER_H
#define LC_GRAPHICVIEWPORTRENDERER_H

#include "lc_rect.h"
#include "rs_color.h"
#include "rs_pen.h"

#define DEBUG_RENDERING_

#ifdef DEBUG_RENDERING
#include <QElapsedTimer>

#define DEBUG_RENDERING_DETAILS
#endif

class LC_GraphicViewport;
class RS_Entity;
class RS_Painter;
class RS_Graphic;
class QPaintDevice;

class LC_GraphicViewportRenderer{
  public:
    explicit LC_GraphicViewportRenderer(LC_GraphicViewport* viewport, QPaintDevice* painterDevice);
    virtual ~LC_GraphicViewportRenderer() = default;
    virtual void loadSettings();
    void render();
    virtual void renderEntity(RS_Painter* painter, RS_Entity* entity)  = 0;
    void renderEntityAsChild(RS_Painter *painter, RS_Entity *e);
    void justDrawEntity(RS_Painter *painter, RS_Entity *e);
    void setBackground(const RS_Color &bg);
    const LC_Rect &getBoundingClipRect() const {return renderBoundingClipRect;}

    virtual bool isTextLineNotRenderable(double uiLineHeight) const = 0;

    void setLineWidthScaling(bool state){
        m_scaleLineWidth = state;
    }

    bool getLineWidthScaling() const{
        return m_scaleLineWidth;
    }
protected:
    QPaintDevice* pd = nullptr;
    LC_GraphicViewport* viewport = nullptr;
    RS_Graphic* graphic = nullptr;

    LC_Rect renderBoundingClipRect;

    /** background color (any color) */
    RS_Color m_colorBackground;
    /** foreground color (black or white) */
    RS_Color m_colorForeground;

    RS_Pen lastPaintEntityPen = {};

    LC_Rect prepareBoundingClipRect();
    virtual void doRender() = 0;

    // painting cached values
    double unitFactor = 1.0;
    double unitFactor100 = 0.01;
    double defaultWidthFactor = 1.0;

    bool m_scaleLineWidth = true;

    Qt::PenJoinStyle penJoinStyle = Qt::RoundJoin;
    Qt::PenCapStyle penCapStyle = Qt::RoundCap;

    // points rendering settings
    int pdmode = 1;
    double pdsize = 1;

    double m_angleBasisBaseAngle = 0.0;
    bool m_angleBasisCounterClockwise = false;

    virtual void setupPainter(RS_Painter *painter);
    virtual void updateGraphicRelatedSettings(RS_Graphic *g);
    void updateEndCapsStyle(const RS_Graphic *graphic);
    void updateJoinStyle(const RS_Graphic *graphic);
    void updatePointEntitiesStyle(RS_Graphic *graphic);
    void updateUnitAndDefaultWidthFactors(const RS_Graphic *g);
    bool isOutsideOfBoundingClipRect(RS_Entity *e, bool constructionEntity);

    RS_Graphic* getGraphic(){return graphic;}

#ifdef DEBUG_RENDERING
    QElapsedTimer drawTimer;
    QElapsedTimer isVisibleTimer;
    QElapsedTimer setPenTimer;
    QElapsedTimer painterSetPenTimer;
    QElapsedTimer getPenTimer;
    QElapsedTimer isConstructionTimer;

    // painting debug
    int drawEntityCount = 1;
    long long entityDrawTime = 0;
    long long isVisibleTime = 0;
    long long isConstructionTime = 0;
    long long setPenTime = 0;
    long long painterSetPenTime = 0;
    long long getPenTime = 0;
#endif


    void updateAnglesBasis(RS_Graphic *g);
};

#endif // LC_GRAPHICVIEWPORTRENDERER_H
