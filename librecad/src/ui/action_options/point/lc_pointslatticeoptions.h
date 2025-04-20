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

#ifndef LC_POINTSLATTICEOPTIONS_H
#define LC_POINTSLATTICEOPTIONS_H

#include "lc_actionoptionswidget.h"
#include "lc_actionoptionswidgetbase.h"

class LC_ActionDrawPointsLattice;

namespace Ui {
    class LC_PointsLatticeOptions;
}

class LC_PointsLatticeOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT
public:
    explicit LC_PointsLatticeOptions();
    ~LC_PointsLatticeOptions() override;
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
protected slots:
    void languageChange() override;
    void onColumnsChanged(int value);
    void onRowsChanged(int value);
    void onAdjustLastPointToggled(bool value);
private:
    Ui::LC_PointsLatticeOptions *ui;
    LC_ActionDrawPointsLattice* m_action = nullptr;
    void setColumnsToActionAndView(int value);
    void setRowsToActionAndView(int value);
    void setAdjustLastPointToActionAndView(bool value);
};
#endif // LC_POINTSLATTICEOPTIONS_H
