/****************************************************************************
**
* Options widget for ModifyBreakDivide action that breaks line, arc or circle
* to segments by points of intersection with other entities.

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

#ifndef LC_MODIFYBREAKOUTOPTIONS_H
#define LC_MODIFYBREAKOUTOPTIONS_H

#include "lc_actionmodifybreakdivide.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_ModifyBreakDivideOptions;
}

class LC_ModifyBreakDivideOptions :public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_ModifyBreakDivideOptions();
    ~LC_ModifyBreakDivideOptions() override;
protected slots:
    void onRemoveSegmentsClicked(bool value);
    void onRemoveSelectedClicked(bool value);
    void languageChange() override;
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
private:
    Ui::LC_ModifyBreakDivideOptions *ui;
    LC_ActionModifyBreakDivide* action;
    void setRemoveSegmentsToActionAndView(bool val);
    void setRemoveSelectedToActionAndView(bool val);
};

#endif // LC_MODIFYBREAKOUTOPTIONS_H
