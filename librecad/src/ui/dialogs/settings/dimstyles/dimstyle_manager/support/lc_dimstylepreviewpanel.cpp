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

#include "lc_dimstylepreviewpanel.h"

#include "lc_dimstylepreviewgraphicview.h"
#include "lc_graphicviewport.h"
#include "rs_graphicview.h"
#include "rs_settings.h"
#include "ui_lc_dimstylepreviewpanel.h"

void LC_DimStylePreviewPanel::setupButton(bool dockWidgetsFlatIcons, int docWidgetsIconSize, QToolButton* btn) {
    btn->setAutoRaise(dockWidgetsFlatIcons);
    btn->setIconSize({docWidgetsIconSize, docWidgetsIconSize});
}

LC_DimStylePreviewPanel::LC_DimStylePreviewPanel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LC_DimStylePreviewPanel){
    ui->setupUi(this);

    LC_GROUP_GUARD("Widgets");
    {
        bool dockWidgetsFlatIcons = LC_GET_BOOL("DockWidgetsFlatIcons", true);
        int docWidgetsIconSize = LC_GET_INT("DockWidgetsIconSize", 16);

        setupButton(dockWidgetsFlatIcons, docWidgetsIconSize, ui->tbZoomAuto);
        setupButton(dockWidgetsFlatIcons, docWidgetsIconSize, ui->tbZoomIn);
        setupButton(dockWidgetsFlatIcons, docWidgetsIconSize, ui->tbZoomOut);
        setupButton(dockWidgetsFlatIcons, docWidgetsIconSize, ui->tbZoomPan);
    }
}

LC_DimStylePreviewPanel::~LC_DimStylePreviewPanel(){
    delete ui;
}

void LC_DimStylePreviewPanel::setGraphicView(LC_DimStylePreviewGraphicView* gv) {
    m_graphicView = gv;
    connect(ui->tbZoomAuto, &QToolButton::pressed, this, &LC_DimStylePreviewPanel::zoomAuto);
    connect(ui->tbZoomIn, &QToolButton::pressed, this, &LC_DimStylePreviewPanel::zoomIn);
    connect(ui->tbZoomOut, &QToolButton::pressed, this, &LC_DimStylePreviewPanel::zoomOut);
    connect(ui->tbZoomPan, &QToolButton::pressed, this, &LC_DimStylePreviewPanel::zoomPan);
}

void LC_DimStylePreviewPanel::zoomOut() {
    m_graphicView->getViewPort()->zoomOut(1.137, {});
}

void LC_DimStylePreviewPanel::zoomIn() {
    m_graphicView->getViewPort()->zoomIn(1.137, {});
}

void LC_DimStylePreviewPanel::zoomAuto() {
    m_graphicView->zoomAuto();
}

void LC_DimStylePreviewPanel::zoomPan() {
    m_graphicView->zoomPan();
}
