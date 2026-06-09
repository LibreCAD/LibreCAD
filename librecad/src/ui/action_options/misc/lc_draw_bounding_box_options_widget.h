/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#ifndef LC_DRAWBOUNDINGBOXOPTIONS_H
#define LC_DRAWBOUNDINGBOXOPTIONS_H

#include "lc_action_options_widget.h"

class LC_ActionDrawBoundingBox;

namespace Ui {
    class LC_DrawBoundingBoxOptionsWidget;
}

class LC_DrawBoundingBoxOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_DrawBoundingBoxOptionsWidget();
    ~LC_DrawBoundingBoxOptionsWidget() override;
protected slots:
    void languageChange() override;
    void onAsGroupToggled(bool val);
    void onCornerPointsToggled(bool val);
    void onPolylineToggled(bool val);
    void onOffsetEditingFinished();
protected:
    void doUpdateByAction(RS_ActionInterface *a) override;
private:
    Ui::LC_DrawBoundingBoxOptionsWidget *ui;
    LC_ActionDrawBoundingBox* m_action = nullptr;
};

#endif
