/****************************************************************************
**
* Options widget for "CircleByArc" action.

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

#ifndef LC_CIRCLEBYARCOPTIONS_H
#define LC_CIRCLEBYARCOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actiondrawcirclebyarc.h"

namespace Ui {
class LC_CircleByArcOptions;
}
/**
 * Options for CircleByArc action
 */
class LC_CircleByArcOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_CircleByArcOptions(QWidget *parent = nullptr);
    ~LC_CircleByArcOptions() override;

protected:
    void doSaveSettings() override;
    void languageChange() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSetAction(RS_ActionInterface *a, bool update) override;

protected slots:
    void onReplaceClicked(bool value);
    QString getSettingsOptionNamePrefix() override;
    void onPenModeIndexChanged(int mode);
    void onLayerModeIndexChanged(int mode);
    void onRadiusShiftEditingFinished();
private:
    Ui::LC_CircleByArcOptions *ui;
    LC_ActionDrawCircleByArc* action;

    void setReplaceArcToActionAndView(bool value);

    void setPenModeToActionAndView(int mode);
    void setLayerModeToActionAndeView(int mode);
    void setRadiusShiftToModelAndView(QString val);
};

#endif // LC_CIRCLEBYARCOPTIONS_H
