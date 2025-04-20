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

#ifndef LC_LINEOPTIONS_H
#define LC_LINEOPTIONS_H

#include "lc_actionoptionswidgetbase.h"

class LC_ActionDrawLineSnake;

namespace Ui {
    class Ui_LineOptionsRel;
}

class LC_LineOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    LC_LineOptions();
    ~LC_LineOptions() override;
public slots:
    void closeLine();
    void undo();
    void redo();
    void polyline();
    void start();
protected slots:
    void onAngleClicked(bool value);
    void onXClicked(bool value);
    void onYClicked(bool value);
    void onPointClicked(bool value) const;
    void onSetAngle();
    void onAngleRelativeClicked(bool value);
    void languageChange() override;
protected:
    LC_ActionDrawLineSnake* m_action = nullptr;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
private:
    Ui::Ui_LineOptionsRel* ui;
    bool m_inUpdateCycle = false;
    void setXDirectionToActionAndView(bool value) const;
    void setYDirectionToActionAndView(bool value) const;
    void setAngleDirectionToActionAndView(bool value) const;
    void setPointDirectionToActionAndView(bool value) const;
    void setAngleToActionAndView(const QString& val, bool affectState);
    void setAngleRelativeToActionAndView(bool relative) const;
    void setupAngleRelatedUI(bool value) const;
};

#endif // LC_LINEOPTIONS_H
