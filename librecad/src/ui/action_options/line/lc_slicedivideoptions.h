/****************************************************************************
**
* Options widget for "SliceDivide" action.

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
#ifndef LC_SLICEDIVIDEOPTIONS_H
#define LC_SLICEDIVIDEOPTIONS_H

#include "lc_actiondrawslicedivide.h"
#include "lc_actionoptionswidget.h"

namespace Ui {
    class LC_SliceDivideOptions;
}

class LC_SliceDivideOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_SliceDivideOptions();
    ~LC_SliceDivideOptions() override;
    void updateUI(int mode) override;

protected slots:
    void languageChange() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    QString getSettingsOptionNamePrefix() override;
    void onCountChanged(int value);
    void onDistanceEditingFinished();
    void onTickLengthEditingFinished();
    void onTickAngleEditingFinished();
    void onTickOffsetEditingFinished();
    void onCircleStartAngleEditingFinished();
    void onDrawTickOnEdgesIndexChanged(int index);
    void onTickSnapIndexChanged(int index);
    void onRelAngleClicked(bool checked);
    void onDivideClicked(bool checked);
    void onModeClicked(bool checked);
private:
    Ui::LC_SliceDivideOptions *ui;

    bool forCircle {false};

    void doSaveSettings() override;
    void setCountToActionAndView(int val);
    void setDistanceToActionAndView(const QString &val);
    void setTickLengthToActionAndView(const QString &qString);
    void setTickAngleToActionAndView(const QString &val);
    void setTickOffsetToActionAndView(const QString &val);
    void setDrawEdgesTicksModeToActionAndView(int index);
    void setCircleStartAngleToActionAndView(const QString &val);

    LC_ActionDrawSliceDivide* action;
    void setTickAngleRelativeToActionAndView(bool relative);    
    void setTicksSnapModeToActionAndView(int index);
    void setDivideFlagToActionAndView(bool value);
    void setFixedDistanceFlagToActionAndView(bool value);

};

#endif // LC_SLICEDIVIDEOPTIONS_H
