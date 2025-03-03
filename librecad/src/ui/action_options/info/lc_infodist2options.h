/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#ifndef LC_INFODIST2OPTIONS_H
#define LC_INFODIST2OPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "rs_actioninfodist2.h"

namespace Ui {
class LC_InfoDist2Options;
}

class LC_InfoDist2Options : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_InfoDist2Options();
    ~LC_InfoDist2Options() override;
protected slots:
    void languageChange() override;
    void onOnEntityClicked(bool value);
protected:
    void doSaveSettings() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    QString getSettingsOptionNamePrefix() override;
private:
    Ui::LC_InfoDist2Options *ui;
    RS_ActionInfoDist2* action = nullptr;
    void setOnEntitySnapToActionAndView(bool value);
};

#endif // LC_INFODIST2OPTIONS_H
