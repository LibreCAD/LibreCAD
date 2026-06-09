/****************************************************************************
**
* Options widget for "Rectangle2Points" action.

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
#ifndef LC_RECTANGLE2POINTSOPTIONS_H
#define LC_RECTANGLE2POINTSOPTIONS_H

#include "lc_action_options_widget.h"

class LC_ActionDrawRectangle2Points;

namespace Ui {
    class LC_Rectangle2PointsOptionsWidget;
}

class LC_Rectangle2PointsOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_Rectangle2PointsOptionsWidget();
    ~LC_Rectangle2PointsOptionsWidget() override;
public slots:
    void onCornersIndexChanged(int index);
    void onInsertionPointSnapIndexChanged(int index);
    void onSecondPointSnapIndexChanged(int index);
    void onEdgesIndexChanged(int index);
    void onAngleEditingFinished();
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);
    void onBaseAngleFixedClicked(bool value);
    void languageChange() override;
protected:
    Ui::LC_Rectangle2PointsOptionsWidget *ui = nullptr;
    LC_ActionDrawRectangle2Points *m_action = nullptr;
    void doUpdateByAction(RS_ActionInterface * a) override;
};

#endif
