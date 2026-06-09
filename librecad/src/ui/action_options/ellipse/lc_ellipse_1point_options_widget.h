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

#ifndef LC_ELLIPSE1POINTOPTIONS_H
#define LC_ELLIPSE1POINTOPTIONS_H

#include "lc_action_options_widget.h"

namespace Ui {
    class LC_Ellipse1PointOptionsWidget;
}

class LC_ActionDrawEllipse1Point;

class LC_Ellipse1PointOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    LC_Ellipse1PointOptionsWidget();
    ~LC_Ellipse1PointOptionsWidget() override;
protected slots:
    void languageChange() override;
    void onAngleEditingFinished();
    void onMajorRadiusEditingFinished();
    void onMinorRadiusEditingFinished();
    void onUseAngleClicked(bool val) const;
    void onFreeAngleClicked(bool val) const;
    void onDirectionChanged(bool val) const;
protected:
    LC_ActionDrawEllipse1Point* m_action = nullptr;
    Ui::LC_Ellipse1PointOptionsWidget *ui;
    void doUpdateByAction(RS_ActionInterface *a) override;
};

#endif
