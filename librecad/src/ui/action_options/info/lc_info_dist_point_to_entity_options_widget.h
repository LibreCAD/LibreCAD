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

#include "lc_action_options_widget.h"

namespace Ui {
    class LC_InfoDistPointToEntityOptionsWidget;
}

class LC_ActionInfoDistPointToEntity;

class LC_InfoDistPointToEntityOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_InfoDistPointToEntityOptionsWidget();
    ~LC_InfoDistPointToEntityOptionsWidget() override;
protected slots:
    void languageChange() override;
    void onOnEntityClicked(bool value) const;
protected:
    void doUpdateByAction(RS_ActionInterface * a) override;
private:
    Ui::LC_InfoDistPointToEntityOptionsWidget *ui;
    LC_ActionInfoDistPointToEntity* m_action = nullptr;
};

#endif
