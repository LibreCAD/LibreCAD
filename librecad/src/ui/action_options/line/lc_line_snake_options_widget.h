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

#include "lc_action_options_widget.h"

class LC_ActionDrawLineSnake;

namespace Ui {
    class LC_LineSnakeOptionsWidget;
}

class LC_LineSnakeOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    LC_LineSnakeOptionsWidget();
    ~LC_LineSnakeOptionsWidget() override;
public slots:
    void closeLine() const;
    void undo() const;
    void redo() const;
    void polyline() const;
    void start() const;
protected slots:
    void onAngleClicked(bool value) const;
    void onXClicked(bool value) const;
    void onYClicked(bool value) const;
    void onPointClicked(bool value) const;
    void onSetAngle();
    void onAngleRelativeClicked(bool value) const;
    void languageChange() override;
protected:
    LC_ActionDrawLineSnake* m_action = nullptr;
    void doUpdateByAction(RS_ActionInterface *a) override;
private:
    Ui::LC_LineSnakeOptionsWidget* ui;
    void setupAngleRelatedUI(bool value) const;
};

#endif
