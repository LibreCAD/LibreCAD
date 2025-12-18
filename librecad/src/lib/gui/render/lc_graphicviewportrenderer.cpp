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

#include "lc_graphicviewportrenderer.h"

#include "lc_defaults.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_painter.h"
#include "rs_units.h"

LC_GraphicViewportRenderer::LC_GraphicViewportRenderer(LC_GraphicViewport* v, QPaintDevice* painterDevice):
    pd{painterDevice}
    , viewport{v}
    , graphic {viewport->getGraphic()}
{
}

void LC_GraphicViewportRenderer::render() {
    renderBoundingClipRect = prepareBoundingClipRect();
    doRender();
}

void LC_GraphicViewportRenderer::renderEntityAsChild(RS_Painter *painter, RS_Entity *e) {
#ifdef DEBUG_RENDERING
    drawEntityCount++;
    drawTimer.start();
#endif
    e->drawAsChild(painter);
#ifdef DEBUG_RENDERING
    qint64 elapsed = drawTimer.nsecsElapsed();
    entityDrawTime+= elapsed;
#endif
}

void LC_GraphicViewportRenderer::loadSettings() {
    auto g = getGraphic();
    if (g != nullptr){
        updateGraphicRelatedSettings(g);
   }
}

LC_Rect LC_GraphicViewportRenderer::prepareBoundingClipRect(){
    int width = viewport->getWidth();
    int height = viewport->getHeight();
    const RS_Vector ucsViewportLeftBottom = viewport->toUCSFromGui(0, 0);
    const RS_Vector ucsViewportRightTop = viewport->toUCSFromGui(width, height);

    if (viewport->hasUCS()){
        // here were extend (enlarge) clipping rect to ensure that if there is shift/rotation in ucs, resulting bounding box cover the entire screen
        // thus we'll paint a bit more entities that is necessary (and more comparing to non-ucs world mode), yet ensure that they are not clipped

        RS_Vector wcsViewportMin;
        RS_Vector wcsViewportMax;

        viewport->worldBoundingBox(ucsViewportLeftBottom, ucsViewportRightTop, wcsViewportMin, wcsViewportMax);

        return LC_Rect(wcsViewportMin, wcsViewportMax);
    }
    else{
        // clipping rect is defined by the screen (0,0 and width/height)
        return LC_Rect(ucsViewportLeftBottom, ucsViewportRightTop);
    }
}

bool LC_GraphicViewportRenderer::isOutsideOfBoundingClipRect(RS_Entity* e, bool constructionEntity){
    // test if the entity is in the viewport
    switch (e->rtti()){
        /* case RS2::EntityGraphic:
             break;*/
        case RS2::EntityLine:{
            if (constructionEntity){
                if (!LC_LineMath::hasIntersectionLineRect(e->getMin(), e->getMax(), renderBoundingClipRect.minP(), renderBoundingClipRect.maxP())){
                    return true;
                }
            }
            else{ // normal line
                if (e->getMax().x < renderBoundingClipRect.minP().x || e->getMin().x > renderBoundingClipRect.maxP().x ||
                    e->getMin().y > renderBoundingClipRect.maxP().y || e->getMax().y < renderBoundingClipRect.minP().y){
                    return true;
                }
            }
            break;
        }
        default:
            if (e->getMax().x < renderBoundingClipRect.minP().x || e->getMin().x > renderBoundingClipRect.maxP().x ||
                e->getMin().y > renderBoundingClipRect.maxP().y || e->getMax().y < renderBoundingClipRect.minP().y){
                return true;
            }
    }
    return false;
}

/**
 * Draws an entity.
 * The painter must be initialized and all the attributes (pen) must be set.
 */
void LC_GraphicViewportRenderer::justDrawEntity(RS_Painter *painter, RS_Entity *e) {
#ifdef DEBUG_RENDERING
    drawEntityCount++;
    drawTimer.start();
#endif
    e->draw(painter);
#ifdef DEBUG_RENDERING
    qint64 elapsed = drawTimer.nsecsElapsed();
    entityDrawTime+= elapsed;
#endif
}

void LC_GraphicViewportRenderer::updateEndCapsStyle(const RS_Graphic *graphic) {//        Lineweight endcaps setting for new objects:
//        0 = none; 1 = round; 2 = angle; 3 = square
    int endCaps = graphic->getGraphicVariableInt("$ENDCAPS", 1);
    switch (endCaps){
        case 0:
            penCapStyle = Qt::FlatCap;
            break;
        case 1:
            penCapStyle = Qt::RoundCap;
            break;
        case 2:
            penCapStyle = Qt::MPenCapStyle;
            break;
        case 3:
            penCapStyle = Qt::SquareCap;
            break;
        default:
            penCapStyle = Qt::FlatCap; // fixme - or round?
    }
}

void LC_GraphicViewportRenderer::updatePointEntitiesStyle(RS_Graphic *graphic) {
    pdmode = graphic->getGraphicVariableInt("$PDMODE", LC_DEFAULTS_PDMode);
    pdsize = graphic->getGraphicVariableDouble("$PDSIZE", LC_DEFAULTS_PDSize);
}

void LC_GraphicViewportRenderer::updateJoinStyle(const RS_Graphic *graphic) {//0=none; 1= round; 2 = angle; 3 = flat
    int joinStyle = graphic->getGraphicVariableInt("$JOINSTYLE", 1);

    switch (joinStyle){
        case 0:
            penJoinStyle = Qt::BevelJoin;
            break;
        case 1:
            penJoinStyle = Qt::RoundJoin;
            break;
        case 2:
            penJoinStyle = Qt::MiterJoin;
            break;
        case 3:
            penJoinStyle = Qt::BevelJoin;
            break;
        default:
            penJoinStyle = Qt::RoundJoin;
    }
}

void LC_GraphicViewportRenderer::setBackground(const RS_Color &bg) {
    m_colorBackground = bg;

    RS_Color black(0, 0, 0);
    if (black.colorDistance(bg) >= RS_Color::MinColorDistance) {
        m_colorForeground = black;
    } else {
        m_colorForeground = RS_Color(255, 255, 255);
    }
}

void LC_GraphicViewportRenderer::updateGraphicRelatedSettings(RS_Graphic *g) {
    updateUnitAndDefaultWidthFactors(g);
    updatePointEntitiesStyle(g);
    updateEndCapsStyle(g);
    updateJoinStyle(g);
    updateAnglesBasis(g);
}

void LC_GraphicViewportRenderer::updateUnitAndDefaultWidthFactors(const RS_Graphic *g) {
    unitFactor = RS_Units::convert(1.0, RS2::Millimeter, g->getUnit());
    unitFactor100 =  unitFactor / 100.0;
    defaultWidthFactor = g->getVariableDouble("$DIMSCALE", 1.0);
}

void LC_GraphicViewportRenderer::setupPainter(RS_Painter *painter) {
    painter->setRenderer(this);
    painter->setViewPort(viewport);
    painter->updatePointsScreenSize(pdsize);
    painter->setPointsMode(pdmode);
    painter->setDefaultWidthFactor(defaultWidthFactor);
    painter->setWorldBoundingRect(renderBoundingClipRect);
}

bool LC_GraphicViewportRenderer::isTextLineNotRenderable([[maybe_unused]]double d) const {
    return false;
}

void LC_GraphicViewportRenderer::updateAnglesBasis(RS_Graphic *g) {
    m_angleBasisBaseAngle = g->getAnglesBase();
    m_angleBasisCounterClockwise = g->areAnglesCounterClockWise();
}
