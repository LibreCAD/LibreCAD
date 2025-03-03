/****************************************************************************
**
* Options widget for action that creates a gap in selected line

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
#ifndef LC_MODIFYGAPOPTIONS_H
#define LC_MODIFYGAPOPTIONS_H

#include "lc_actionmodifylinegap.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_ModifyGapOptions;
}

class LC_ModifyGapOptions :public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_ModifyGapOptions();
    ~LC_ModifyGapOptions() override;
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
protected slots:
    void languageChange() override;
    void onFreeGapClicked(bool val);
    void onSizeEditingFinished();
    void onDistanceEditingFinished();
    void onLineSnapModeIndexChanged(int index);
    void onGapSnapModeIndexChanged(int index);
private:
    LC_ActionModifyLineGap* action = nullptr;
    Ui::LC_ModifyGapOptions *ui;
    void setGapSizeToActionAndView(const QString &val);
    void setGapIsFreeToActionAndView(bool val);
    void setLineSnapToActionAndView(int val);
    void setSnapDistanceToActionAndView(const QString &val);
    void setGapSnapToActionAndView(int val);
};

#endif // LC_MODIFYGAPOPTIONS_H
