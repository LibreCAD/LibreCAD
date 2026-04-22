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

#ifndef LC_ACTIONDRAWARC2POPTIONS_H
#define LC_ACTIONDRAWARC2POPTIONS_H

#include <QLabel>

#include "lc_action_options_widget.h"

namespace Ui {
    class LC_Arc2PointsOptionsWidget;
}

class RS_ActionInterface;
class LC_ActionDrawArc2PointsBase;

class LC_Arc2PointsOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_Arc2PointsOptionsWidget(int actionType);
    ~LC_Arc2PointsOptionsWidget() override;
public slots:
    void onDirectionChanged(bool);
    void languageChange() override;
    void onValueChanged();
protected:
    void doUpdateByAction(RS_ActionInterface *a) override;
private:
    Ui::LC_Arc2PointsOptionsWidget *ui;
    LC_ActionDrawArc2PointsBase* m_action = nullptr;
    void updateTooltip( QLabel *label) const;
};

#endif
