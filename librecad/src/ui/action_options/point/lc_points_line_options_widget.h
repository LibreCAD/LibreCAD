/****************************************************************************
**
* Options widget for "LinePoints" action.

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
#ifndef LC_LINEPOINTSOPTIONS_H
#define LC_LINEPOINTSOPTIONS_H

#include "lc_action_options_widget.h"

class LC_ActionDrawPointsLine;
namespace Ui {
    class LC_LinePointsOptionsWidget;
}

class LC_LinePointsOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_LinePointsOptionsWidget();
    ~LC_LinePointsOptionsWidget() override;
protected:
    void doUpdateByAction(RS_ActionInterface *a) override;
protected slots:
    void languageChange() override;
    void onPointsCountValueChanged(int value) const;
    void onEdgePointsModeIndexChanged(int index) const;
    void onFixedDistanceClicked(bool value) const;
    void onAngleClicked(bool value) const;
    void onWithinLineClicked(bool value) const;
    void onDistanceEditingFinished();
    void onAngleEditingFinished();
private:
    Ui::LC_LinePointsOptionsWidget *ui = nullptr;
    LC_ActionDrawPointsLine* m_action = nullptr;
};
#endif
