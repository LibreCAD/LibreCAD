/****************************************************************************
**
* Options widget for "Rectangle1Point" action.

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
**********************************************************************/

#ifndef LC_RECTANGLE1POINTOPTIONS_H
#define LC_RECTANGLE1POINTOPTIONS_H

#include "lc_actiondrawrectangle1point.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_Rectangle1PointOptions;
}

class LC_Rectangle1PointOptions :public LC_ActionOptionsWidgetBase {
    Q_OBJECT

public:
    explicit LC_Rectangle1PointOptions();
    ~LC_Rectangle1PointOptions() override;

    enum {
        UPDATE_ANGLE
    };

public slots:
    void onCornersIndexChanged(int index);
    void onSnapPointIndexChanged(int index);
    void onAngleEditingFinished();
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onWidthEditingFinished();
    void onHeightEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);
    void onInnerSizeClicked(bool value);
    void onFreeAngleClicked(bool value);
    void onEdgesIndexChanged(int index);
    void onBaseAngleFixedClicked(bool value);
    void languageChange() override;

    void updateUI(int mode) override;

protected:
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void doSaveSettings() override;
private:
    Ui::LC_Rectangle1PointOptions *ui;
    LC_ActionDrawRectangle1Point *action;
    void setAngleToActionAndView(const QString &val);
    void setLenYToActionAnView(const QString& value);
    void setLenXToActionAnView(const QString& value);
    void setRadiusToActionAnView(const QString& value);
    void setHeightToActionAnView(const QString& height);
    void setWidthToActionAnView(const QString& width);
    void setCornersModeToActionAndView(int index);
    void setSnapPointModeToActionAndView(int index);
    void setUsePolylineToActionAndView(bool value);
    void setSnapToCornerArcCenterToActionAndView(bool value);
    void setSizeInnerToActionAndView(bool value);
    void setFreeAngleToActionAndView(bool value);
    void setEdgesModeToActionAndView(int index);
    void setBaseAngleFixedToActionAndView(bool angle);
};


#endif // LC_RECTANGLE1POINTOPTIONS_H
