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

#include "lc_graphicviewawarewidget.h"

#include <QLayout>
#include <QToolButton>

#include "rs_settings.h"

LC_GraphicViewAwareWidget::LC_GraphicViewAwareWidget(QWidget* parent, const char* name, const Qt::WindowFlags f):
    QWidget(parent, f){
    setObjectName(name);
}

LC_GraphicViewAwareWidget::~LC_GraphicViewAwareWidget() = default;

void LC_GraphicViewAwareWidget::updateWidgetSettings() const {
    LC_GROUP("Widgets");
    {
        const bool flatIcons = LC_GET_BOOL("DockWidgetsFlatIcons", true);
        const int iconSize = LC_GET_INT("DockWidgetsIconSize", 16);

        const QSize size(iconSize, iconSize);

        QList<QToolButton*> widgets = this->findChildren<QToolButton*>();
        foreach(QToolButton *w, widgets) {
            w->setAutoRaise(flatIcons);
            w->setIconSize(size);
        }
    }
    LC_GROUP_END();
}

void LC_GraphicViewAwareWidget::doAdjustForDockLocation(Qt::DockWidgetArea area) {
    switch (area) {
        case Qt::DockWidgetArea::LeftDockWidgetArea:{
            getTopLevelLayout()->setContentsMargins(2, 1, 0, 2);
            break;
        }
        case Qt::DockWidgetArea::RightDockWidgetArea:{
            getTopLevelLayout()->setContentsMargins(0, 1, 2, 2);
            break;
        }
        case Qt::DockWidgetArea::NoDockWidgetArea:{
            getTopLevelLayout()->setContentsMargins(2, 1, 2, 2);
            break;
        }
        default:
            getTopLevelLayout()->setContentsMargins(0, 1, 2, 0);
            break;
    }
}

void LC_GraphicViewAwareWidget::onDockLocationChanged(Qt::DockWidgetArea area) {
    doAdjustForDockLocation(area);
}
