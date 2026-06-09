/****************************************************************************
**
* Options widget for Angle Line from line action.

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
#ifndef LC_LINEANGLERELOPTIONS_H
#define LC_LINEANGLERELOPTIONS_H

#include <memory>

#include "lc_action_options_widget.h"

class LC_ActionDrawLineAngleRel;

namespace Ui {
    class LC_LineAngleRelOptionsWidget;
}

class LC_LineAngleRelOptionsWidget : public LC_ActionOptionsWidget {
    Q_OBJECT public:
    explicit LC_LineAngleRelOptionsWidget();
    ~LC_LineAngleRelOptionsWidget() override;

protected:
    void doUpdateByAction(RS_ActionInterface* a) override;
protected slots:
    void onLengthEditingFinished();
    void onDistanceEditingFinished();
    void onOffsetEditingFinished();
    void onAngleEditingFinished();
    void onLineSnapModeIndexChanged(int index);
    void onTickSnapModeIndexChanged(int index);
    void onAngleRelatedClicked(bool clicked);
    void onDivideClicked(bool clicked);
    void onFreeLengthClicked(bool clicked);
    void languageChange() override;
private:
    std::unique_ptr<Ui::LC_LineAngleRelOptionsWidget> ui;
    LC_ActionDrawLineAngleRel* m_action{nullptr};
};

#endif
