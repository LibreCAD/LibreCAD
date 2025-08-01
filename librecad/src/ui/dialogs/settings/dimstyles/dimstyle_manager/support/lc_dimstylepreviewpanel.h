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

#ifndef LC_DIMSTYLEPREVIEWPANEL_H
#define LC_DIMSTYLEPREVIEWPANEL_H

#include <QWidget>

class LC_DimStylePreviewGraphicView;
class RS_GraphicView;
class QToolButton;

namespace Ui{
    class LC_DimStylePreviewPanel;
}

class LC_DimStylePreviewPanel : public QWidget {
    Q_OBJECT
public:
    explicit LC_DimStylePreviewPanel(QWidget* parent = nullptr);
    ~LC_DimStylePreviewPanel() override;
    void setGraphicView(LC_DimStylePreviewGraphicView *gv);
protected slots:
    void zoomOut();
    void zoomIn();
    void zoomAuto();
    void zoomPan();
private:
    void setupButton(bool dockWidgetsFlatIcons, int docWidgetsIconSize, QToolButton* btn);
    Ui::LC_DimStylePreviewPanel* ui;
    LC_DimStylePreviewGraphicView* m_graphicView{nullptr};
};

#endif // LC_DIMSTYLEPREVIEWPANEL_H
