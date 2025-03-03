/****************************************************************************
**
* Options widget for "LinePoints" action.

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
#ifndef LC_LINEPOINTSOPTIONS_H
#define LC_LINEPOINTSOPTIONS_H

#include "lc_actionoptionswidgetbase.h"
#include "lc_actiondrawlinepoints.h"

namespace Ui {
class LC_LinePointsOptions;
}

class LC_LinePointsOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_LinePointsOptions();
    ~LC_LinePointsOptions() override;
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
protected slots:
    void languageChange() override;
    void onPointsCountValueChanged(int value);
    void onEdgePointsModeIndexChanged(int index);
    void onFixedDistanceClicked(bool value);
    void onAngleClicked(bool value);
    void onWithinLineClicked(bool value);
    void onDistanceEditingFinished();
    void onAngleEditingFinished();
private:
    Ui::LC_LinePointsOptions *ui = nullptr;
    LC_ActionDrawLinePoints* action = nullptr;
    bool inUpdateCycle = false;

    void setPointsCountActionAndView(int value);
    void setEdgePointsModeToActionAndView(int index);
    void setFixedDistanceModeToActionAndView(bool value);
    void setWithinLineModeToActionAndView(bool value);
    void setDistanceToActionAndView(QString val);
    void setAngleModeToActionAndView(bool value);
    void setAngleToActionAndView(QString val, bool affectState);
};

#endif // LC_LINEPOINTSOPTIONS_H
