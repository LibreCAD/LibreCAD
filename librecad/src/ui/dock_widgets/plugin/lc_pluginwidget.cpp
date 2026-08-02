/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

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

#include "lc_pluginwidget.h"

#include <QLayout>

LC_PluginWidget::LC_PluginWidget(const QString& title, QWidget *mainWidget, QWidget *parent)
    : LC_GraphicViewAwareWidget(parent) {
    setWindowTitle(title);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->addWidget(mainWidget);
    setLayout(lay);

    updateWidgetSettings();
}

QLayout* LC_PluginWidget::getTopLevelLayout() const {
    return layout();
}

void LC_PluginWidget::setGraphicView(RS_GraphicView* gview) {
    m_graphicView = gview;
}