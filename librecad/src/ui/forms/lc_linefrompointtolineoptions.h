/****************************************************************************
**
* Options widget for "LineFromPointToLine" action.

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
#ifndef LC_LINEFROMPOINTTOLINEOPTIONS_H
#define LC_LINEFROMPOINTTOLINEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actiondrawlinefrompointtoline.h"

namespace Ui {
class LC_LineFromPointToLineOptions;
}
/**
 * UI options for LC_ActionDrawLineFromPointToLine
 */
class LC_LineFromPointToLineOptions :public LC_ActionOptionsWidget
{
    Q_OBJECT
public:
    explicit LC_LineFromPointToLineOptions(QWidget *parent = nullptr);
    ~LC_LineFromPointToLineOptions() override;

protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSaveSettings() override;
    void languageChange() override;
protected slots:
    void onAngleEditingFinished();
    void onLengthEditingFinished();
    void onEndOffsetEditingFinished();
    void onSnapModeIndexChanged(int index);
    void onSizeModeIndexChanged(int index);
    void onOrthogonalClicked(bool value);
    QString getSettingsOptionNamePrefix() override;

private:
    LC_ActionDrawLineFromPointToLine* action;
    Ui::LC_LineFromPointToLineOptions *ui;
    void setOrthogonalToActionAndView(bool value);
    void setSizeModelIndexToActionAndView(int index);
    void setSnapModeToActionAndView(int index);
    void setAngleToActionAndView(const QString& value);
    void setLengthToActionAndView(const QString& value);
    void setEndOffsetToActionAndView(const QString& value);
};

#endif // LC_LINEFROMPOINTTOLINEOPTIONS_H
