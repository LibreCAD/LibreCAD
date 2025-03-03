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

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "lc_actiondrawellipse1point.h"

namespace Ui {
    class LC_Ellipse1PointOptions;
}

class LC_Ellipse1PointOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    LC_Ellipse1PointOptions();
    ~LC_Ellipse1PointOptions();

protected slots:
    void languageChange() override;
    void onAngleEditingFinished();
    void onMajorRadiusEditingFinished();
    void onMinorRadiusEditingFinished();
    void onUseAngleClicked(bool val);
    void onFreeAngleClicked(bool val);
    void onDirectionChanged(bool val);

protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;

    LC_ActionDrawEllipse1Point* action;
    Ui::LC_Ellipse1PointOptions *ui;

    void setMajorRadiusToActionAndView(QString val);
    void setMinorRadiusToActionAndView(QString val);
    void setAngleToActionAndView(QString val);
    void setAngleIsFreeToActionAndView(bool val);
    void setUseAngleAngleToActionAndView(bool val);

    bool checkActionRttiValid(RS2::ActionType actionType) override;
};

#endif // LC_ELLIPSE1POINTOPTIONS_H
