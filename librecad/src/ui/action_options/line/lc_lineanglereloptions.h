/****************************************************************************
**
* Options widget for Angle Line from line action.

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
#ifndef LC_LINEANGLERELOPTIONS_H
#define LC_LINEANGLERELOPTIONS_H

#include <memory>

#include "lc_actiondrawlineanglerel.h"
#include "lc_actionoptionswidget.h"

namespace Ui {
class LC_LineAngleRelOptions;
}

class LC_LineAngleRelOptions : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_LineAngleRelOptions();
    ~LC_LineAngleRelOptions() override;
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
protected slots:
    void onLengthEditingFinished();
    void onDistanceEditingFinished();
    void onOffsetEditingFinished();
    void onAngleEditingFinished();
    void onLineSnapModeIndexChanged(int index);
    void onTickSnapModeIndexChanged(int index);
    void onAngleRelatedClicked(bool clicked);
    void onDivideClicked(bool clicked);
    void onFreeLengthClicked(bool clicked);
    void languageChange() override;
protected:
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSaveSettings() override;
    QString getSettingsOptionNamePrefix() override;
private:
    std::unique_ptr<Ui::LC_LineAngleRelOptions> ui;
    LC_ActionDrawLineAngleRel* action {nullptr};
    bool fixedAngle {false};

    void setAngleToActionAndView(const QString &expr);
    void setLengthToActionAndView(const QString& val);
    void setDistanceToActionAndView(const QString& val);
    void setOffsetToActionAndView(const QString& val);
    void setLineSnapModeToActionAndView(int mode);
    void setTickSnapModeToActionAndView(int mode);
    void setAngleIsRelativeToActionAndView(bool relative);
    void setLengthIsFreeToActionAndView(bool free);
    void setDivideToActionAndView(bool divide);
};

#endif // LC_LINEANGLERELOPTIONS_H
