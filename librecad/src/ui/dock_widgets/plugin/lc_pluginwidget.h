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

#ifndef LC_PLUGINWIDGET_H
#define LC_PLUGINWIDGET_H

#include "lc_graphicviewawarewidget.h"

class LC_PluginWidget : public LC_GraphicViewAwareWidget
{
    Q_OBJECT
public:
    explicit LC_PluginWidget(const QString& title, QWidget *mainWidget, QWidget *parent);
    ~LC_PluginWidget() override = default;
    void setGraphicView(RS_GraphicView *gv) override;

protected:
    QLayout* getTopLevelLayout() const override;

private:
    RS_GraphicView* m_graphicView = nullptr;
};

#endif // LC_PLUGINWIDGET_H
