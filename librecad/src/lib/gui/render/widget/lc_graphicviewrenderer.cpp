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

#include "lc_graphicviewrenderer.h"

#include <QApplication>
#include <QScreen>

#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_overlayentitiescontainer.h"
#include "lc_ref_snap_circle.h"
#include "lc_ref_snap_construction_line.h"
#include "lc_ref_snap_entity.h"
#include "lc_ref_snap_line.h"
#include "lc_ref_snap_mark.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_grid.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_settings.h"

namespace {
constexpr char const* g_drawingMdiWindowBackground = "#212830";
}

LC_GraphicViewRenderer::LC_GraphicViewRenderer(LC_GraphicViewport* viewport, QPaintDevice* d)
    : LC_WidgetViewPortRenderer(viewport, d) {
}

void LC_GraphicViewRenderer::loadSettings() {
    LC_WidgetViewPortRenderer::loadSettings();
    //increase grid point size on for DPI>96
    const auto dpiX = static_cast<int>(qApp->screens().front()->logicalDotsPerInch());
    // fixme - sand - potentially that should be something on higher layer
    m_isHiDpi = dpiX > 96;

    m_relZeroOptions.loadSettings();
    m_absZeroOptions.loadSettings();
    m_ucsMarkOptions.loadSettings();
    m_anglesBaseOptions.loadSettings();

    LC_GROUP("Appearance");
    {
        m_entityHandleHalfSize = LC_GET_INT("EntityHandleSize", 4) / 2;
        m_ignoreDraftForHighlight = LC_GET_BOOL("IgnoreDraftForHighlight", false);
        m_scaleLineWidth = !LC_GET_BOOL("DraftLinesMode", false);

        const QString draftMarkerFontName = LC_GET_STR("DraftMarkerFontName", "Verdana");
        const int draftMarkerFontSize = LC_GET_INT("DraftMarkerFontSize", 10);
        m_draftSignFont = QFont(draftMarkerFontName, draftMarkerFontSize);

        m_drawDrawSign = LC_GET_BOOL("ShowDraftModeMarker", true);
    }
    LC_GROUP_END();

    LC_GROUP("Render");
    {
        m_drawTextsAsDraftForPreview = LC_GET_BOOL("DrawTextsAsDraftInPreview", true);
        m_drawTextsAsDraftForPanning = LC_GET_BOOL("DrawTextsAsDraftInPanning", true);
    }
    LC_GROUP_END();

    LC_GROUP_GUARD("Colors");
    {
        const RS_Color background(LC_GET_STR("background", RS_Settings::BACKGROUND));
        if (background.isValid() && background.toIntColor() == RS_Color::Black) {
            setBackground(RS_Color(QColor(g_drawingMdiWindowBackground)), background);
        } else {
            setBackground(background);
        }
        m_colorSelectedEntity = RS_Color(LC_GET_STR("select", RS_Settings::SELECT));
        m_colorHighlightedEntity = RS_Color(LC_GET_STR("highlight", RS_Settings::HIGHLIGHT));
        m_colorStartHandle = RS_Color(LC_GET_STR("start_handle", RS_Settings::START_HANDLE));
        m_colorHangle = RS_Color(LC_GET_STR("handle", RS_Settings::HANDLE));
        m_colorEndHandleColor = RS_Color(LC_GET_STR("end_handle", RS_Settings::END_HANDLE));

        m_colorPreviewReferenceEntities = RS_Color(LC_GET_STR("previewReferencesColor", RS_Settings::PREVIEW_REF_COLOR));
        m_colorPreviewReferenceHighlightedEntities = RS_Color(LC_GET_STR("previewReferencesHighlightColor",
                                                                         RS_Settings::PREVIEW_REF_HIGHLIGHT_COLOR));

        m_colorVisualSnapGuideEntities = RS_Color(LC_GET_STR("VisualSnapGuideEntitiesColor", RS_Settings::VISUAL_SNAP_ENTITIES));
        m_colorVisualSnapVertexes = RS_Color(LC_GET_STR("VisualSnapVertexesColor", RS_Settings::VISUAL_SNAP_VERTEXES));
        m_colorVisualSnapProjectedSnap= RS_Color(LC_GET_STR("VisualSnapProjectedSnapColor", RS_Settings::VISUAL_SNAP_PROJECTED_SNAP));
        m_colorVisualSnapDocumentEntities= RS_Color(LC_GET_STR("VisualSnapDocumentEntitiesColor", RS_Settings::VISUAL_SNAP_DOCUMENT_ENTITIES));

        const QString& name = LC_GET_STR("draft_mode_marker", RS_Settings::SELECT);
        m_draftSignColor = RS_Color(name);
    } // colors group

    m_drawGrid = m_viewport->isGridOn();
}

void LC_GraphicViewRenderer::renderEntity(RS_Painter* painter, RS_Entity* e) {
    // check for selected entity drawing
    if (/*!e->isContainer() && */e->getFlag(RS2::FlagSelected) != painter->shouldDrawSelected()) {
        return;
    }
#ifdef DEBUG_RENDERING
    isVisibleTimer.start();
#endif
    // entity is not visible:
    const bool visible = e->isVisible();
#ifdef DEBUG_RENDERING
    isVisibleTime += isVisibleTimer.nsecsElapsed();
#endif
    if (!visible) {
        return;
    }

#ifdef DEBUG_RENDERING
    isConstructionTimer.start();
#endif
    const bool constructionEntity = e->isConstruction();
#ifdef DEBUG_RENDERING
    isConstructionTime += isConstructionTimer.nsecsElapsed();
#endif

    if (isOutsideOfBoundingClipRect(e, constructionEntity)) {
        return;
    }

    const RS2::EntityType entityType = e->rtti();
    if (isDraftMode()) {
        switch (entityType) {
            case RS2::EntityMText:
            case RS2::EntityText:
            case RS2::EntityImage:
                // set pen (color):
                setPenForDraftEntity(painter, e, false);
                e->drawDraft(painter);
                break;
            case RS2::EntityHatch:
                //skip hatches
                break;
            default:
                setPenForDraftEntity(painter, e, false);
                justDrawEntity(painter, e);
        }
    }
    else {
        // the code below is ugly as code for normal painting is duplicated.
        // however, it's intentional and made for perfromance reasons - to avoid additional checks or method calls during painting
        if (m_viewport->isPanning()) {
            switch (entityType) {
                case RS2::EntityMText:
                case RS2::EntityText: {
                    if (m_drawTextsAsDraftForPanning) {
                        setPenForDraftEntity(painter, e, false);
                        e->drawDraft(painter);
                    }
                    else {
                        // normal painting
                        if (m_scaleLineWidth) {
                            setPenForEntity(painter, e, false);
                        }
                        else {
                            setPenForDraftEntity(painter, e, false);
                        }
                        justDrawEntity(painter, e);
                    }
                    break;
                }
                default: {
                    // normal painting
                    // set pen (color):
                    if (m_scaleLineWidth) {
                        setPenForEntity(painter, e, false);
                    }
                    else {
                        setPenForDraftEntity(painter, e, false);
                    }
                    justDrawEntity(painter, e);
                }
                break;
            }
        }
        else {
            // normal painting
            // set pen (color):
            if (getLineWidthScaling()) {
                setPenForEntity(painter, e, false);
            }
            else {
                setPenForDraftEntity(painter, e, false);
            }
            justDrawEntity(painter, e);
        }
    }

    // draw reference points:
    if (e->getFlag(RS2::FlagSelected)) {
        if (!e->isParentSelected()) {
            drawEntityReferencePoints(painter, e);
        }
    }
    //RS_DEBUG->print("RS_GraphicView::drawEntity() end");
}

void LC_GraphicViewRenderer::doDrawLayerBackground(RS_Painter* painter) {
    const RS_Pen penSaved = painter->getPen();

    RS_Grid* grid = m_viewport->getGrid();

    if (grid != nullptr) {
        if (m_drawGrid) {
            grid->calculateGrid();
            grid->drawGrid(painter);
        }
        else {
            grid->calculateSnapSettings();
        }
    }

    if (m_isHiDpi) {
        RS_Pen pen = penSaved;
        pen.setWidth(RS2::Width01);
        painter->setPen(pen);
    }

    if (isDraftMode() && m_drawDrawSign) {
        drawDraftSign(painter);
    }

    if (m_isHiDpi) {
        painter->setPen(penSaved);
    }

    if (m_absZeroOptions.extendAxisLines) {
        const double originPointX = m_viewport->toGuiX(0.0);
        const double originPointY = m_viewport->toGuiY(0.0);
        //ucs absolute zero, x and y axis
        m_overlayAbsZero.updateOrigin(originPointX, originPointY);
        m_overlayAbsZero.draw(painter);
    }
}

void LC_GraphicViewRenderer::drawLayerEntitiesOver(RS_Painter* painter) {
    if (m_graphic != nullptr) {
        // fixme - sand - support of preview in hatch dialog, yet probably it's better to use specialized version of view there...
        drawCoordinateSystems(painter);
    }
}

void LC_GraphicViewRenderer::doDrawLayerOverlays(RS_Painter* painter) {
    if (m_graphic != nullptr) {
        // fixme - sand - again, support for hatch dialog :(
        drawRelativeZero(painter);
    }
    m_lastPaintEntityPen = RS_Pen();
    drawOverlay(painter);
}

void LC_GraphicViewRenderer::drawRelativeZero(RS_Painter* painter) {
    const RS_Vector relativeZero = m_viewport->getRelativeZero();
    if (!relativeZero.valid || m_relZeroOptions.hideRelativeZero) {
        return;
    }
    m_overlayRelZero.setPos(relativeZero);
    m_overlayRelZero.draw(painter);
}

void LC_GraphicViewRenderer::drawOverlay(RS_Painter* painter) {
    // todo - using inOverlayDrawing flag is ugly, yet needed for proper drawing of containers (like dimensions or texts) that are in overlays
    // while draw for container is performed, the pen is resolved as sub-entities of containers as they are in normal drawing...

    const LC_OverlaysManager* overlaysManager = m_viewport->getOverlaysManager();

    m_inOverlayDrawing = true;

    drawEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::OverlayEffects);
    drawOverlayEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::OverlayEffects);
    drawEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::ActionPreviewEntity);
    drawOverlayEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::ActionPreviewEntity);

    drawEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::PermanentHighlights);
    drawOverlayEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::PermanentHighlights);

    drawEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::Snapper);
    drawOverlayEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::Snapper);
    drawEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::InfoCursor);
    drawOverlayEntitiesInOverlay(overlaysManager, painter, RS2::OverlayGraphics::InfoCursor);

    m_inOverlayDrawing = false;
}

void LC_GraphicViewRenderer::drawEntitiesInOverlay(const LC_OverlaysManager* overlaysManager, RS_Painter* painter,
                                                   const RS2::OverlayGraphics overlayType) {
    const RS_EntityContainer* overlayContainer = overlaysManager->entitiesAt(overlayType);
    if (overlayContainer != nullptr) {
        foreach(auto e, overlayContainer->getEntityList()) {
            const bool selected = e->isSelected();
            // within overlays, we use temporary entities (or clones), so it's safe to modify selection flag there
#ifdef DEBUG_INSERT_PAINT
            if (selected) {
                RS2::EntityType entityType = e->rtti();
                if (entityType == RS2::EntityInsert) {
                    LC_ERR << "Insert";
                }
            }
#endif
            setPenForOverlayEntity(painter, e);
            e->clearSelectionFlag();
            e->draw(painter);
            if (selected) {
                drawEntityReferencePoints(painter, e);
            }
        }
    }
}

void LC_GraphicViewRenderer::drawOverlayEntitiesInOverlay(const LC_OverlaysManager* overlaysManager, RS_Painter* painter,
                                                          const RS2::OverlayGraphics overlayType) {
    LC_OverlayDrawablesContainer* overlayContainer = overlaysManager->drawablesAt(overlayType);
    if (overlayContainer != nullptr) {
        overlayContainer->draw(painter);
    }
}

void LC_GraphicViewRenderer::drawEntityReferencePoints(RS_Painter* painter, const RS_Entity* e) const {
    const RS_VectorSolutions& s = e->getRefPoints();
    const int sz = m_entityHandleHalfSize;
    const size_t refsCount = s.getNumber();
    const size_t lastRef = refsCount - 1;
    constexpr qreal maxValue = std::numeric_limits<qreal>::max();
    auto handleUiPos = QPointF(maxValue, maxValue); // artificial, for drawing first handle
    for (size_t i = 0; i < refsCount; ++i) {
        RS_Color col = m_colorHangle;
        if (i == 0) {
            col = m_colorStartHandle;
        }
        else if (i == lastRef) {
            col = m_colorEndHandleColor;
        }
        painter->drawHandleWCS(s.get(i), col, handleUiPos, sz);
    }
}

void LC_GraphicViewRenderer::drawDraftSign(RS_Painter* painter) const {
    painter->setPen(m_draftSignColor);
    painter->setFont(m_draftSignFont);
    const QSize& size = QFontMetrics(painter->font()).size(Qt::TextSingleLine, m_draftMarkText);

    constexpr int TEXT_LABEL_OFFSET = 5;
    constexpr int TEXT_LABEL_COUNT = 4;

    QRect boundingRect{TEXT_LABEL_OFFSET, TEXT_LABEL_OFFSET, size.width(), size.height()};
    for (int i = 1; i <= TEXT_LABEL_COUNT; ++i) {
        painter->drawText(boundingRect, m_draftMarkText, &boundingRect);
        QPoint position{((i & 1) != 0) ? m_viewport->getWidth() - boundingRect.width() - TEXT_LABEL_OFFSET : TEXT_LABEL_OFFSET,
                        ((i & 2) != 0) ? m_viewport->getHeight() - boundingRect.height() - TEXT_LABEL_OFFSET : TEXT_LABEL_OFFSET};
        boundingRect.moveTopLeft(position);
    }
}

void LC_GraphicViewRenderer::setupRefSnapEntityPen(const RS_Painter* painter, RS_Pen &pen, const LC_RefSnapEntity* const ent, bool inVisualSnap) const {
    if (ent->isStrict()) {
        pen.setLineType(RS2::SolidLine);
    }
    else if (ent->isActive()) {
        pen.setLineType(RS2::DotLineTiny);
    }
    else {
        pen.setLineType(RS2::DotLine2);
    }
    pen.setColor(m_colorVisualSnapGuideEntities); // fixme - cache pen for snap marks in painter!
    if (inVisualSnap) {
        pen.setColor(m_colorVisualSnapDocumentEntities);
        pen.setLineType(RS2::DashLineTiny);
    }
    else {
        pen.setWidth(RS2::LineWidth::Width00);
    }

    const double width = pen.getWidth();
    if (pen.isFullyOpaque()) {
        if (width > 0) {
            // todo - sand - ucs - investigate were it's possible to cache calculated screen width at least during the same render pass.
            // The amount of pens widths is limited - so probably accessing precalculated width will be slightly faster
            double screenWidth = painter->toGuiDX(width * m_unitFactor100);
            // prevent drawing with 1-width which is slow:
            /* if (RS_Math::round(screenWidth) == 1) {
                 screenWidth = 0.0;
             }
             else*/
            // fixme - not sure about this check. However, without it, lines will stay transparent and then disappear on zooming out. Probably some other threshold value (instead 1) should be used?
            if (screenWidth < 1) {
                screenWidth = 0.0;
            }
            pen.setScreenWidth(screenWidth);
        }
        else {
            pen.setScreenWidth(0.0);
        }
    }
    else {
        // fixme - if we'll support transparency, add necessary processing there
        if (RS_Math::round(pen.getScreenWidth()) == 1) {
            pen.setScreenWidth(0.0);
        }
    }
}

void LC_GraphicViewRenderer::setPenForOverlayEntity(RS_Painter* painter, const RS_Entity* e) {
    // todo - potentially, for overlays (preview etc) we may have simpler processing for pens rather than for normal drawing,
    // todo - therefore, review this later
    const int rtti = e->rtti();
    switch (rtti) {
        case RS2::EntityRefEllipse:
        case RS2::EntityRefPoint:
        case RS2::EntityRefLine:
        case RS2::EntityRefConstructionLine:
        case RS2::EntityRefCircle:
        case RS2::EntityRefArc: {
            // todo - if not ref point are enabled, draw as transparent? Actually, if actions are correct, we should not be there..
            // fixme - cache pen for ref marks in painter!
            RS_Pen pen = e->getPen(true);
            if (e->isHighlighted()) {
                pen.setColor(m_colorPreviewReferenceHighlightedEntities);
            }
            else {
                pen.setColor(m_colorPreviewReferenceEntities);
            }
            pen.setLineType(RS2::SolidLine);
            pen.setWidth(RS2::LineWidth::Width00);

            // todo - sand - ucs -  USE THE SAME CACHING OF THE PEN!!!! The amount of overlay entities should be small, yet still...
            e->setPen(pen);
            painter->setPen(pen);
            break;
        }
        case RS2::EntitySnapMark: {
            RS_Pen pen;
            // fixme - cache pen for snap marks in painter!
            auto snapMark = static_cast<const LC_RefSnapMark*>(e);
            pen.setColor(snapMark->getMarkType() == LC_RefSnapMark::PROJECTED ? m_colorVisualSnapProjectedSnap : m_colorVisualSnapVertexes);
            pen.setLineType(RS2::SolidLine);
            pen.setWidth(RS2::LineWidth::Width00);
            painter->setPen(pen);
            break;
        }
        case RS2::EntitySnapConstructionLine: {
            RS_Pen pen;
            pen.setColor(m_colorVisualSnapGuideEntities); // fixme - cache pen for snap marks in painter!
            const auto ent = static_cast<const LC_RefSnapConstructionLine*>(e);
            setupRefSnapEntityPen(painter, pen, ent, e->getFlag(RS2::FlagInVisualSnap));
            painter->setPen(pen);
            break;
        }
        case RS2::EntitySnapCircle: {
            RS_Pen pen;
            pen.setColor(m_colorVisualSnapGuideEntities); // fixme - cache pen for snap marks in painter!
            const auto ent = static_cast<const LC_RefSnapCircle*>(e);
            setupRefSnapEntityPen(painter, pen, ent, e->getFlag(RS2::FlagInVisualSnap));
            painter->setPen(pen);
            break;
        }
        case RS2::EntitySnapArc: {
            RS_Pen pen;
            pen.setColor(m_colorVisualSnapGuideEntities); // fixme - cache pen for snap marks in painter!
            const auto ent = static_cast<const LC_RefSnapCircle*>(e);
            setupRefSnapEntityPen(painter, pen, ent, e->getFlag(RS2::FlagInVisualSnap));
            painter->setPen(pen);
            break;
        }
        case RS2::EntitySnapLine:{

            RS_Pen pen = e->getPen(true);
            const auto ent = static_cast<const LC_RefSnapLine*>(e);
            setupRefSnapEntityPen(painter, pen, ent, e->getFlag(RS2::FlagInVisualSnap));
            painter->setPen(pen);
            break;
        }
        default: {
            if (m_draftMode) {
                if (m_ignoreDraftForHighlight) {
                    setPenForEntity(painter, e, true);
                }
                else {
                    setPenForDraftEntity(painter, e, true);
                }
            }
            else {
                if (m_scaleLineWidth) {
                    setPenForEntity(painter, e, true);
                }
                else {
                    setPenForDraftEntity(painter, e, true);
                }
            }
        }
    }
}

void LC_GraphicViewRenderer::setPenForEntity(RS_Painter* painter, const RS_Entity* e, const bool inOverlay) {
#ifdef DEBUG_RENDERING
    getPenTimer.start();
#endif
    // Getting pen from entity (or layer)
    RS_Pen pen = e->getPenResolved();
#ifdef DEBUG_RENDERING
    getPenTime += getPenTimer.nsecsElapsed();
#endif
    const RS_Pen originalPen = pen;
    const bool highlighted = e->getFlag(RS2::FlagHighlighted);
    const bool selected = e->getFlag(RS2::FlagSelected);
    const bool overlayPaint = inOverlay || m_inOverlayDrawing;
    const bool inVisualSnap = e->getFlag(RS2::FlagInVisualSnap);
    // try to avoid pen setup if the pen and entity flags are the same as for previous entity. This is important for performance reasons, so we'll reuse
    // painter pen set previously. This check assumed that that all previous entity drawing were performed via this function and no
    // arbitrary QPainter::setPen was called between drawing entities.
    const double patternOffset = painter->currentDashOffset();
    // fixme - replace several booleans by Flags value
    if (m_lastPaintedHighlighted == highlighted && m_lastPaintedSelected == selected && m_lastPaintOverlay == overlayPaint && m_lastPenInVisualSnap == inVisualSnap) {
        if (m_lastPaintEntityPen.isSameAs(pen, patternOffset)) {
            return;
        }
    }
    else {
        m_lastPaintedHighlighted = highlighted;
        m_lastPaintedSelected = selected;
        m_lastPaintOverlay = overlayPaint;
        m_lastPenInVisualSnap = inVisualSnap;
    }

#ifdef DEBUG_RENDERING
    setPenTimer.start();
#endif
    // Avoid negative widths
    //    int w = std::max(static_cast<int>(pen.getWidth()), 0);
    const double width = pen.getWidth();
    if (pen.isFullyOpaque()) {
        if (width > 0) {
            // todo - sand - ucs - investigate were it's possible to cache calculated screen width at least during the same render pass.
            // The amount of pens widths is limited - so probably accessing precalculated width will be slightly faster
            double screenWidth = painter->toGuiDX(width * m_unitFactor100);
            // prevent drawing with 1-width which is slow:
            /* if (RS_Math::round(screenWidth) == 1) {
                 screenWidth = 0.0;
             }
             else*/
            // fixme - not sure about this check. However, without it, lines will stay transparent and then disappear on zooming out. Probably some other threshold value (instead 1) should be used?
            if (screenWidth < 1) {
                screenWidth = 0.0;
            }
            pen.setScreenWidth(screenWidth);
        }
        else {
            pen.setScreenWidth(0.0);
        }
    }
    else {
        // fixme - if we'll support transparency, add necessary processing there
        if (RS_Math::round(pen.getScreenWidth()) == 1) {
            pen.setScreenWidth(0.0);
        }
    }

    if (overlayPaint) {
        // fixme - sand - ucs - rethink style for overlay entity!!
        if (highlighted) {
            // Glowing effects on mouse hovering: use the "selected" color
            // for glowing effects on mouse hovering, draw solid lines
            pen.setColor(m_colorSelectedEntity);
            pen.setLineType(RS2::SolidLine);
        }
        else if (inVisualSnap) {
            pen.setColor(m_colorVisualSnapDocumentEntities);
            pen.setLineType(RS2::SolidLine);
        }
        else {
            if (pen.getColor().isEqualIgnoringFlags(m_colorBackgroundForContrast) || (pen.getColor().toIntColor() == RS_Color::Black)
                // fixme - sand - think about Black... is it really necessary there?
                || (pen.getColor().colorDistance(m_colorBackgroundForContrast) < RS_Color::MinColorDistance)) {
                pen.setColor(m_colorForeground);
            }
        }
    }
    else {
        // this entity is selected:
        if (selected) {
            pen.setLineType(RS2::DashLineTiny);
            pen.setWidth(RS2::Width00); // fixme - move to settings?
            pen.setColor(m_colorSelectedEntity);
        }
        // this entity is highlighted:
        else if (highlighted) {
            pen.setColor(m_colorVisualSnapDocumentEntities);
            pen.setLineType(RS2::SolidLine);
        }
        else if (inVisualSnap) {
            pen.setColor(m_colorHighlightedEntity);
        }
        else if (e->getFlag(RS2::FlagTransparent)) {
            pen.setColor(m_colorBackground);
        }
        else if (pen.getColor().isEqualIgnoringFlags(m_colorBackgroundForContrast) || (pen.getColor().toIntColor() == RS_Color::Black
            // fixme - sand - think about Black... is it really necessary there?
            && pen.getColor().colorDistance(m_colorBackgroundForContrast) < RS_Color::MinColorDistance)) {
            pen.setColor(m_colorForeground);
        }
    }

    if (pen.getLineType() != RS2::SolidLine) {
        pen.setDashOffset(patternOffset * m_defaultWidthFactor);
    }

    // deleting not drawing:

    // LC_ERR << "PEN " << pen.getColor().name() << "Width: " << pen.getWidth() <<  " | " << pen.getScreenWidth() << " LT " << pen.getLineType();
#ifdef DEBUG_RENDERING
    setPenTime += setPenTimer.nsecsElapsed(); painterSetPenTimer.start();
#endif
    m_lastPaintEntityPen.updateBy(originalPen);
    painter->setPen(pen);
#ifdef DEBUG_RENDERING
    painterSetPenTime += painterSetPenTimer.nsecsElapsed();

#endif
}

void LC_GraphicViewRenderer::setPenForDraftEntity(RS_Painter* painter, const RS_Entity* e, const bool inOverlay) {
#ifdef DEBUG_RENDERING
    setPenTimer.start();
#endif
    RS_Pen pen = e->getPenResolved();
    const RS_Pen originalPen = pen;
    const bool highlighted = e->getFlag(RS2::FlagHighlighted);
    const bool selected = e->getFlag(RS2::FlagSelected);
    const bool overlayPaint = inOverlay || m_inOverlayDrawing;
    const bool inVisualSnap = e->getFlag(RS2::FlagInVisualSnap);
    // try to avoid pen setup if the pen and entity flags are the same as for previous entity. This is important for performance reasons, so we'll reuse
    // painter pen set previously. This check assumed that that all previous entity drawing were performed via this function and no
    // arbitrary QPainter::setPen was called between drawing entities.
    const double patternOffset = painter->currentDashOffset();
    if (m_lastPaintedHighlighted == highlighted && m_lastPaintedSelected == selected && m_lastPaintOverlay == overlayPaint && m_lastPenInVisualSnap == inVisualSnap) {
        if (m_lastPaintEntityPen.isSameAs(pen, patternOffset)) {
            return;
        }
    }
    else {
        m_lastPaintedHighlighted = highlighted;
        m_lastPaintedSelected = selected;
        m_lastPaintOverlay = overlayPaint;
        m_lastPenInVisualSnap = inVisualSnap;
    }
    pen.setScreenWidth(0.0);

    if (overlayPaint) {
        if (highlighted) {
            // Glowing effects on mouse hovering: use the "selected" color
            // for glowing effects on mouse hovering, draw solid lines
            pen.setColor(m_colorSelectedEntity);
            pen.setLineType(RS2::SolidLine);
        }
        else if (inVisualSnap) {
            pen.setColor(m_colorVisualSnapDocumentEntities);
        }
        else {
            if (pen.getColor().isEqualIgnoringFlags(m_colorBackgroundForContrast)
                || (pen.getColor().toIntColor() == RS_Color::Black && pen.getColor().
                colorDistance(m_colorBackgroundForContrast) < RS_Color::MinColorDistance)) {
                pen.setColor(m_colorForeground);
            }
        }
    }
    else {
        // this entity is selected:
        if (selected) {
            pen.setLineType(RS2::DashLineTiny);
            pen.setWidth(RS2::Width00);
            pen.setColor(m_colorSelectedEntity);
        }
        else if (highlighted) {
            pen.setColor(m_colorHighlightedEntity);
        }
        else if (inVisualSnap) {
            pen.setColor(m_colorVisualSnapDocumentEntities);
        }
        else if (e->getFlag(RS2::FlagTransparent)) {
            pen.setColor(m_colorBackground);
        }
        else if (pen.getColor().isEqualIgnoringFlags(m_colorBackgroundForContrast)
                 || (pen.getColor().toIntColor() == RS_Color::Black && pen.getColor()
           .colorDistance(m_colorBackgroundForContrast) < RS_Color::MinColorDistance)) {
            pen.setColor(m_colorForeground);
        }
    }

    if (pen.getLineType() != RS2::SolidLine) {
        pen.setDashOffset(patternOffset * m_defaultWidthFactor);
    }

    // LC_ERR << "PEN " << pen.getColor().name() << "Width: " << pen.getWidth() <<  " | " << pen.getScreenWidth() << " LT " << pen.getLineType();
    m_lastPaintEntityPen.updateBy(originalPen);
    painter->setPen(pen);
#ifdef DEBUG_RENDERING
    setPenTime += setPenTimer.nsecsElapsed();
#endif
}

/**
 * This virtual method can be overwritten to draw the absolute
 * zero. It's called from within drawIt(). The default implementation
 * draws a simple red cross on the zero of the sheet
 * This function can ONLY be called from within a paintEvent because it will
 * use the painter
 *
 * @see drawIt()
 */
void LC_GraphicViewRenderer::drawCoordinateSystems(RS_Painter* painter) {
    double originPointX = m_viewport->toGuiX(0.0);
    double originPointY = m_viewport->toGuiY(0.0);
    if (!m_absZeroOptions.extendAxisLines) {
        //ucs absolute zero, x and y axis
        m_overlayAbsZero.updateOrigin(originPointX, originPointY);
        m_overlayAbsZero.draw(painter);
    }

    // origin of UCS coordinates marker (0.0 in UCS)
    if (m_ucsMarkOptions.showUcsZeroMarker) {
        m_overlayUCSMark.update({originPointX, originPointY}, 0, false);
        m_overlayUCSMark.draw(painter);
    }

    // origin of WCS
    if (m_viewport->hasUCS() && m_ucsMarkOptions.showWcsZeroMarker) {
        double uiWCSOriginX, uiWCSOriginY;
        painter->toGui(RS_Vector(0, 0), uiWCSOriginX, uiWCSOriginY);
        const double wcsXAxisAngleInUCS = -m_viewport->getXAxisAngle();
        m_overlayUCSMark.update({uiWCSOriginX, uiWCSOriginY}, wcsXAxisAngleInUCS, true);
        m_overlayUCSMark.draw(painter);
    }

    if (m_anglesBaseOptions.showAnglesBaseMark) {
        bool showByPolicy = true;
        const double baseAngle = m_angleBasisBaseAngle;
        const bool counterClockWise = m_angleBasisCounterClockwise;
        if (m_anglesBaseOptions.displayPolicy != LC_AnglesBaseMarkOptions::SHOW_ALWAYS) {
            showByPolicy = LC_LineMath::isMeaningfulAngle(baseAngle) || !counterClockWise;
        }
        if (showByPolicy) {
            m_overlayAnglesBaseMark.update({originPointX, originPointY}, baseAngle, counterClockWise);
            m_overlayAnglesBaseMark.draw(painter);
        }
    }
}

void LC_GraphicViewRenderer::doSetupBeforeContainerDraw() {
    LC_WidgetViewPortRenderer::doSetupBeforeContainerDraw();
    m_lastPaintedHighlighted = false;
    m_lastPaintedSelected = false;
    m_lastPaintOverlay = false;
}
