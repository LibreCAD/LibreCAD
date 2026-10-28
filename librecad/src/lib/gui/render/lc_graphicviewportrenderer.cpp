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

#include <cmath>

#include "lc_defaults.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_linetype.h"
#include "lc_linetypelist.h"
#include "lc_linetypenames.h"
#include "rs_entity.h"
#include "rs_graphic.h"
#include "rs_painter.h"
#include "rs_units.h"

LC_GraphicViewportRenderer::LC_GraphicViewportRenderer(LC_GraphicViewport* v, QPaintDevice* painterDevice):
    m_paintDevice{painterDevice}
    , m_viewport{v}
    , m_graphic {m_viewport->getGraphic()}
{
}

void LC_GraphicViewportRenderer::render() {
    m_renderBoundingClipRect = prepareBoundingClipRect();
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
    const auto g = getGraphic();
    if (g != nullptr){
        updateGraphicRelatedSettings(g);
   }
}

LC_Rect LC_GraphicViewportRenderer::prepareBoundingClipRect() const {
    const int width = m_viewport->getWidth();
    const int height = m_viewport->getHeight();
    const RS_Vector ucsViewportLeftBottom = m_viewport->toUCSFromGui(0, 0);
    const RS_Vector ucsViewportRightTop = m_viewport->toUCSFromGui(width, height);

    if (m_viewport->hasUCS()){
        // here were extend (enlarge) clipping rect to ensure that if there is shift/rotation in ucs, resulting bounding box cover the entire screen
        // thus we'll paint a bit more entities that is necessary (and more comparing to non-ucs world mode), yet ensure that they are not clipped

        RS_Vector wcsViewportMin;
        RS_Vector wcsViewportMax;

        m_viewport->worldBoundingBox(ucsViewportLeftBottom, ucsViewportRightTop, wcsViewportMin, wcsViewportMax);

        return LC_Rect(wcsViewportMin, wcsViewportMax);
    }
    // clipping rect is defined by the screen (0,0 and width/height)
    return LC_Rect(ucsViewportLeftBottom, ucsViewportRightTop);
}

bool LC_GraphicViewportRenderer::isOutsideOfBoundingClipRect(const RS_Entity* e, const bool constructionEntity) const {
    // test if the entity is in the viewport
    switch (e->rtti()){
        /* case RS2::EntityGraphic:
             break;*/
        case RS2::EntityLine:{
            if (constructionEntity){
                if (!LC_LineMath::hasIntersectionLineRect(e->getMin(), e->getMax(), m_renderBoundingClipRect.minP(), m_renderBoundingClipRect.maxP())){
                    return true;
                }
            }
            else{ // normal line
                if (e->getMax().x < m_renderBoundingClipRect.minP().x || e->getMin().x > m_renderBoundingClipRect.maxP().x ||
                    e->getMin().y > m_renderBoundingClipRect.maxP().y || e->getMax().y < m_renderBoundingClipRect.minP().y){
                    return true;
                }
            }
            break;
        }
        default:
            if (e->getMax().x < m_renderBoundingClipRect.minP().x || e->getMin().x > m_renderBoundingClipRect.maxP().x ||
                e->getMin().y > m_renderBoundingClipRect.maxP().y || e->getMax().y < m_renderBoundingClipRect.minP().y){
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
    const int endCaps = graphic->getGraphicVariableInt("$ENDCAPS", 1);
    switch (endCaps){
        case 0:
            m_penCapStyle = Qt::FlatCap;
            break;
        case 1:
            m_penCapStyle = Qt::RoundCap;
            break;
        case 2:
            m_penCapStyle = Qt::MPenCapStyle;
            break;
        case 3:
            m_penCapStyle = Qt::SquareCap;
            break;
        default:
            m_penCapStyle = Qt::FlatCap; // fixme - or round?
    }
}

void LC_GraphicViewportRenderer::updatePointEntitiesStyle(const RS_Graphic *graphic) {
    m_pdmode = graphic->getGraphicVariableInt("$PDMODE", LC_DEFAULTS_PDMode);
    m_pdsize = graphic->getGraphicVariableDouble("$PDSIZE", LC_DEFAULTS_PDSize);
}

void LC_GraphicViewportRenderer::updateJoinStyle(const RS_Graphic *graphic) {//0=none; 1= round; 2 = angle; 3 = flat
    const int joinStyle = graphic->getGraphicVariableInt("$JOINSTYLE", 1);

    switch (joinStyle){
        case 0:
            m_penJoinStyle = Qt::BevelJoin;
            break;
        case 1:
            m_penJoinStyle = Qt::RoundJoin;
            break;
        case 2:
            m_penJoinStyle = Qt::MiterJoin;
            break;
        case 3:
            m_penJoinStyle = Qt::BevelJoin;
            break;
        default:
            m_penJoinStyle = Qt::RoundJoin;
    }
}

void LC_GraphicViewportRenderer::setBackground(const RS_Color &bg) {
    setBackground(bg, bg);
}

void LC_GraphicViewportRenderer::setBackground(const RS_Color &bg, const RS_Color &backgroundForContrast) {
    m_colorBackground = bg;
    m_colorBackgroundForContrast = backgroundForContrast;

    const RS_Color black(0, 0, 0);
    if (black.colorDistance(m_colorBackgroundForContrast) >= RS_Color::MinColorDistance) {
        m_colorForeground = black;
    } else {
        m_colorForeground = RS_Color(255, 255, 255);
    }
}

void LC_GraphicViewportRenderer::updateGraphicRelatedSettings(RS_Graphic *g) {
    updateUnitFactors(g);
    updatePointEntitiesStyle(g);
    updateEndCapsStyle(g);
    updateJoinStyle(g);
    updateAnglesBasis(g);
}

void LC_GraphicViewportRenderer::updateUnitFactors(const RS_Graphic *g) {
    m_unitFactor = RS_Units::convert(1.0, RS2::Millimeter, g->getUnit());
    m_unitFactor100 =  m_unitFactor / 100.0;
}

void LC_GraphicViewportRenderer::setupPainter(RS_Painter *painter) {
    // The line type table is read once per pass; every paint path starts here.
    m_dashByFold.clear();
    m_dashByEnum.clear();
    m_dashCacheBuilt = false;
    painter->setRenderer(this);
    painter->setViewPort(m_viewport);
    painter->updatePointsScreenSize(m_pdsize);
    painter->setPointsMode(m_pdmode);
    // A new view rectangle invalidates the dash memo.
    painter->setWorldBoundingRect(m_renderBoundingClipRect);
}

const QVector<qreal>* LC_GraphicViewportRenderer::resolveDashPattern(const RS_Pen& pen, qreal* phaseOffset) {
    // An unnamed pen paints as its enum says, whatever the table holds.
    if (!pen.hasLineTypeName())
        return nullptr;
    if (!m_dashCacheBuilt)
        buildDashPatternCache();
    // No entry is ever added after the build, so the addresses stay valid.
    const DashPattern* found = nullptr;
    const std::uint16_t foldId = pen.getLineTypeFoldId();
    if (foldId != 0) {
        const auto it = m_dashByFold.constFind(foldId);
        if (it != m_dashByFold.cend())
            found = &it.value();
    }
    else {
        const auto it = m_dashByEnum.constFind(static_cast<int>(pen.getLineType()));
        if (it != m_dashByEnum.cend())
            found = &it.value();
    }
    if (found == nullptr)
        return nullptr;
    if (phaseOffset != nullptr)
        *phaseOffset = found->phaseOffset;
    return &found->dashes;
}

void LC_GraphicViewportRenderer::buildDashPatternCache() {
    m_dashCacheBuilt = true;
    const RS_Graphic* graphic = m_viewport != nullptr ? m_viewport->getGraphic() : nullptr;
    if (graphic == nullptr)
        return;
    // The stored list is group 49 before $LTSCALE, and the table is drawn once.
    double lineTypeScale = graphic->getVariableDouble("$LTSCALE", 1.0);
    if (!std::isfinite(lineTypeScale) || lineTypeScale <= 0.0)
        lineTypeScale = 1.0;
    const unsigned count = graphic->countLineTypes();
    for (unsigned i = 0; i < count; ++i) {
        const LC_LineType* entry = graphic->lineTypeAt(i);
        if (entry == nullptr || entry->pattern.empty())
            continue;
        const bool known = LC_LineTypeNames::isKnownName(entry->key());
        if (known) {
            const LC_LineTypeNames::BuiltinMetric* seed =
                LC_LineTypeNames::builtinMetric(LC_LineTypeNames::nameToLineType(entry->key()));
            // A record with dashes for a line type that has none is nonsense; the enum stays solid.
            if (seed == nullptr || seed->pattern.empty())
                continue;
            // A built-in the file agrees with keeps the shipped screen pattern.
            if (LC_LineTypeList::patternMatchesSeed(*entry))
                continue;
        }
        RS_Pen probe;
        probe.setLineTypeName(entry->name);
        const std::uint16_t foldId = probe.getLineTypeFoldId();
        qreal phaseOffset = 0.0;
        QVector<qreal> pattern = RS_Painter::canonicalDashPattern(entry->pattern, &phaseOffset);
        if (pattern.isEmpty())
            continue;
        bool valid = std::isfinite(phaseOffset);
        for (qreal& dash : pattern) {
            dash *= lineTypeScale;
            valid = valid && std::isfinite(dash) && dash >= 0.0;
        }
        phaseOffset *= lineTypeScale;
        if (!valid || !std::isfinite(phaseOffset))
            continue;
        if (known && foldId == 0) {
            // The canonical spelling of a built-in: the enum map, which the
            // painter reads only for a named pen.
            const int enumKey = static_cast<int>(probe.getLineType());
            if (!m_dashByEnum.contains(enumKey))
                m_dashByEnum.insert(enumKey, DashPattern{pattern, phaseOffset});
        }
        else if (foldId != 0 && !m_dashByFold.contains(foldId)) {
            // The first entry of a fold wins.
            m_dashByFold.insert(foldId, DashPattern{pattern, phaseOffset});
        }
    }
}

bool LC_GraphicViewportRenderer::isTextLineNotRenderable([[maybe_unused]]double uiLineHeight) const {
    return false;
}

void LC_GraphicViewportRenderer::updateAnglesBasis(const RS_Graphic *g) {
    m_angleBasisBaseAngle = g->getAnglesBase();
    m_angleBasisCounterClockwise = g->areAnglesCounterClockWise();
}
