/****************************************************************************
**
* Options widget for "Rectangle3Point" action.

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

#ifndef LC_RECTANGLE3POINTSOPTIONS_H
#define LC_RECTANGLE3POINTSOPTIONS_H

#include "lc_actiondrawrectangle3points.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_Rectangle3PointsOptions;
}

class LC_Rectangle3PointsOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT
public:
    explicit LC_Rectangle3PointsOptions();
    ~LC_Rectangle3PointsOptions() override;
    void doSaveSettings() override;
protected slots:
    void languageChange() override;
    void onAngleEditingFinished();
    void onCornersIndexChanged(int index);
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);
    void onQuadrangleClicked(bool value);
    void onInnerAngleEditingFinished();
    void onInnerAngleFixedClicked(bool value);
    void onBaseAngleFixedClicked(bool value);
    void onEdgesIndexChanged(int index);
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    LC_ActionDrawRectangle3Points* action;
    Ui::LC_Rectangle3PointsOptions *ui;
    void setSnapToCornerArcCenter(bool value);
    void setUsePolylineToActionAndView(bool value);
    void setRadiusToActionAnView(const QString& value);
    void setLenXToActionAnView(const QString& value);
    void setLenYToActionAnView(const QString& value);
    void setAngleToActionAndView(const QString &val);
    void setCornersModeToActionAndView(int index);
    void setQuadrangleToActionAndView(bool value);
    void setBaseAngleFixedToActionAndView(bool angle);
    void setInnerAngleFixedToActionAndView(bool angle);
    void setInnerAngleToActionAndView(const QString& value);
    void setEdgesModeToActionAndView(int index);
};

#endif // LC_RECTANGLE3POINTSOPTIONS_H
