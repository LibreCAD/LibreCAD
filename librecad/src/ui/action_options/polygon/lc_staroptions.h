/****************************************************************************
**
* Options widget for "DrawStar" action.

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
#ifndef LC_STAROPTIONS_H
#define LC_STAROPTIONS_H

#include "lc_actiondrawstar.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_StarOptions;
}

class LC_StarOptions  :public LC_ActionOptionsWidgetBase {
    Q_OBJECT

public:
    explicit LC_StarOptions();
    ~LC_StarOptions() override;

protected slots:
    void onRadiusInnerEditingFinished();
    void onRadiusOuterEditingFinished();
    void onSymmetricClicked(bool value);
    void onRadiusInnerClicked(bool value);
    void onRadiusOuterClicked(bool value);
    void onPolylineClicked(bool value);
    void onNumberChanged(int value);
    void doSaveSettings() override;
    void languageChange() override;
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_StarOptions *ui;
    LC_ActionDrawStar* action;
    void setRadiusOuterToModelAndView(const QString& val);
    void setRadiusInnerToModelAndView(const QString& val);
    void setRadiusInnerEnabledToModelAndView(bool value);
    void setSymmetricToModelAndView(bool value);
    void setRadiusOuterEnabledToModelAndView(bool value);
    void setNumberToModelAndView(int value);
    void setUsePolylineToActionAndView(bool value);
};

#endif // LC_STAROPTIONS_H
