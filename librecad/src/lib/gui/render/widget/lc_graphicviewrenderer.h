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

#ifndef LC_GRAPHICVIEWRENDERER_H
#define LC_GRAPHICVIEWRENDERER_H
#include "lc_overlayanglesbasemark.h"
#include "lc_overlayrelativezero.h"
#include "lc_overlayucszero.h"
#include "lc_ucs_mark.h"
#include "lc_widgetviewportrenderer.h"

class RS_EntityContainer;
class LC_OverlaysManager;

class LC_GraphicViewRenderer:public LC_WidgetViewPortRenderer
{
public:
    LC_GraphicViewRenderer(LC_GraphicViewport *viewport, QPaintDevice* d);

    void loadSettings() override;
    /**
  * @retval true Draft mode is on for this view (all lines with 1 pixel / no style scaling).
  * @retval false Otherwise.
  */
    bool isDraftMode() const {return m_draftMode;}
    void setDraftMode(bool dm) { m_draftMode = dm;}
    bool isTextLineNotRenderable(double uiLineHeight) const override
    {
        return uiLineHeight <getMinRenderableTextHeightInPx();
    }
protected:
    bool m_inOverlayDrawing = false;
    bool m_isHiDpi = false;

    bool m_drawGrid = true;
    // fixme - sand - files - use uniq_ptr?
    LC_OverlayRelZeroOptions m_relZeroOptions;
    LC_OverlayUCSZeroOptions m_absZeroOptions;
    LC_UCSMarkOptions m_ucsMarkOptions;
    LC_AnglesBaseMarkOptions m_anglesBaseOptions;

    int m_entityHandleHalfSize = 2;

    bool m_drawTextsAsDraftForPanning = true;
    bool m_drawTextsAsDraftForPreview = true;

    bool m_ignoreDraftForHighlight = false;

    /** grid color */
    RS_Color m_gridColor = Qt::gray;
    /** meta grid color */
    RS_Color m_metaGridColor;
    /** selected color */
    RS_Color m_colorSelectedEntity;
    /** highlighted color */
    RS_Color m_colorHighlightedEntity;
    /** Start handle color */
    RS_Color m_colorStartHandle;
    /** Intermediate (not start/end vertex) handle color */
    RS_Color m_colorHangle;
    /** End handle color */
    RS_Color m_colorEndHandleColor;
    /** reference entities on preview color */
    RS_Color m_colorPreviewReferenceEntities;

    /** reference entities on preview color */
    RS_Color m_colorPreviewReferenceHighlightedEntities;

    bool m_lastPaintedHighlighted = false;
    bool m_lastPaintedSelected = false;
    bool m_lastPaintOverlay = false;
    bool m_draftMode = false;

    QString m_draftMarkText = QObject::tr("Draft");
    RS_Color m_draftSignColor = Qt::white;
    bool m_drawDrawSign = true;
    QFont m_draftSignFont;

    LC_OverlayRelativeZero m_overlayRelZero = LC_OverlayRelativeZero(&m_relZeroOptions);
    LC_OverlayUCSZero m_overlayAbsZero = LC_OverlayUCSZero(&m_absZeroOptions);
    LC_OverlayUCSMark m_overlayUCSMark = LC_OverlayUCSMark(&m_ucsMarkOptions);
    LC_OverlayAnglesBaseMark m_overlayAnglesBaseMark = LC_OverlayAnglesBaseMark(&m_anglesBaseOptions);

    void doDrawLayerBackground(RS_Painter *painter) override;
    void doDrawLayerOverlays(RS_Painter *painter) override;
    void drawLayerEntitiesOver(RS_Painter *painter) override;
    void drawRelativeZero(RS_Painter *painter);
    void drawOverlay(RS_Painter *painter);
    void drawDraftSign(RS_Painter *painter);
    void drawCoordinateSystems(RS_Painter *painter);
    void drawEntitiesInOverlay(LC_OverlaysManager *overlaysManager, RS_Painter *painter, RS2::OverlayGraphics overlayType);
    void drawOverlayEntitiesInOverlay(LC_OverlaysManager *overlaysManager, RS_Painter *painter, RS2::OverlayGraphics overlayType);
    void drawEntityReferencePoints(RS_Painter *painter, const RS_Entity *e) const;
    void setPenForEntity(RS_Painter *painter, RS_Entity *e, bool inOverlay);
    void setPenForDraftEntity(RS_Painter *painter, RS_Entity *e, bool inOverlay);
    void setPenForOverlayEntity(RS_Painter *painter, RS_Entity *e);
    void renderEntity(RS_Painter *painter, RS_Entity *e) override;
    void doSetupBeforeContainerDraw() override;
};

#endif // LC_GRAPHICVIEWRENDERER_H
