/****************************************************************************
**
* Options widget for "Rectangle2Points" action.

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
#ifndef LC_RECTANGLE2POINTSOPTIONS_H
#define LC_RECTANGLE2POINTSOPTIONS_H

#include "lc_actiondrawrectangle2points.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_Rectangle2PointsOptions;
}

class LC_Rectangle2PointsOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_Rectangle2PointsOptions();
    ~LC_Rectangle2PointsOptions() override;

public slots:
    void onCornersIndexChanged(int index);
    void onInsertionPointSnapIndexChanged(int index);
    void onSecondPointSnapIndexChanged(int index);
    void onEdgesIndexChanged(int index);
    void onAngleEditingFinished();
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);
    void onBaseAngleFixedClicked(bool value);
    void languageChange() override;
protected:
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void doSaveSettings() override;
private:
    void setAngleToActionAndView(const QString &val);
    void setLenYToActionAnView(const QString& value);
    void setLenXToActionAnView(const QString& value);
    void setRadiusToActionAnView(const QString& value);
    void setCornersModeToActionAndView(int index);
    void setInsertSnapPointModeToActionAndView(int index);
    void setSecondPointSnapPointModeToActionAndView(int index);
    void setUsePolylineToActionAndView(bool value);
    void setSnapToCornerArcCenter(bool value);
    void setBaseAngleFixedToActionAndView(bool angle);
    Ui::LC_Rectangle2PointsOptions *ui = nullptr;
    LC_ActionDrawRectangle2Points *action = nullptr;
    void setEdgesModeToActionAndView(int index);
};

#endif // LC_RECTANGLE2POINTSOPTIONS_H
