/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_LINE_TO_PERSPECTIVE_POINT_OPTIONS_H
#define LC_LINE_TO_PERSPECTIVE_POINT_OPTIONS_H


#include "lc_action_draw_line_radiant.h"
#include "lc_action_options_widget.h"

class LC_ActionDrawLineRadiant;
class RS_ActionInterface;

namespace Ui {
    class LC_LineRadiantOptionsWidget;
}

class LC_LineRadiantOptionsWidget : public LC_ActionOptionsWidget {
    Q_OBJECT public:
    explicit LC_LineRadiantOptionsWidget();
    ~LC_LineRadiantOptionsWidget() override;
protected slots:
    void languageChange() override;
    void onLengthTypeIndexChanged(int index);
    void onActivePointIndexChanged(int index);
    void onXEditingFinished();
    void onYEditingFinished();
    void onLengthEditingFinished();
protected:
    void doUpdateByAction(RS_ActionInterface* a) override;
private:
    Ui::LC_LineRadiantOptionsWidget* ui;
    LC_ActionDrawLineRadiant* m_action = nullptr;
};
#endif
