/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_DIMSTYLEPREVIEWGRAPHICVIEW_H
#define LC_DIMSTYLEPREVIEWGRAPHICVIEW_H

#include "lc_dimstyleitem.h"
#include "qg_graphicview.h"

class LC_PreviewGraphic;
class LC_DimStyle;

class LC_DimStylePreviewGraphicView: public QG_GraphicView{
public:
    void updateDims();
    void refresh();
    void setDimStyle(LC_DimStyle *dimStyle);
    void setEntityDimStyle(LC_DimStyle* dimStyle, bool override, const QString& baseName);
    void setEntityPen(const RS_Pen& pen);
    void zoomPan();
    void addDimStyle(LC_DimStyle* dim_style);
    void setEntityArrowsFlipMode(bool flip_arrow1, bool flip_arrow2);
    static LC_DimStylePreviewGraphicView* init(QWidget* parent,RS_Graphic* originalGraphic, RS2::EntityType dimensionType);
    static LC_DimStylePreviewGraphicView* init(QWidget* parent,RS_Graphic* originalGraphic, RS_Dimension* dimension);
protected:
    LC_DimStylePreviewGraphicView(QWidget* parent,LC_ActionContext* actionContext);
    ~LC_DimStylePreviewGraphicView() override;
    bool proceedEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void hideNonRelevantLayers(RS2::EntityType dimType);
    static LC_DimStylePreviewGraphicView* createAndSetupView(QWidget* parent,
            LC_PreviewGraphic* graphic, RS_Graphic* originalGraphic, bool showInWCS);
    static void copyBlocks(RS_Graphic* originalGraphic, LC_PreviewGraphic* graphic);
};
#endif // LC_DIMSTYLEPREVIEWGRAPHICVIEW_H
