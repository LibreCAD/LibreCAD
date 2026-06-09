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

#ifndef LC_CIRCLEBYARCOPTIONS_H
#define LC_CIRCLEBYARCOPTIONS_H

#include "lc_action_options_widget.h"

namespace Ui {
    class LC_CircleByArcOptionsWidget;
}

class LC_ActionDrawCircleByArc;
/**
 * Options for CircleByArc action
 */
class LC_CircleByArcOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_CircleByArcOptionsWidget();
    ~LC_CircleByArcOptionsWidget() override;
protected:
    void languageChange() override;
    void doUpdateByAction(RS_ActionInterface *a) override;
protected slots:
    void onReplaceClicked(bool value) const;
    void onPenModeIndexChanged(int mode) const;
    void onLayerModeIndexChanged(int mode) const;
    void onRadiusShiftEditingFinished();
private:
    Ui::LC_CircleByArcOptionsWidget *ui = nullptr;
    LC_ActionDrawCircleByArc* m_action;
};

#endif
