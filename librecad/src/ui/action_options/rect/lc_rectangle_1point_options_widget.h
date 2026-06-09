/****************************************************************************
**
* Options widget for "Rectangle1Point" action.

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

#ifndef LC_RECTANGLE1POINTOPTIONS_H
#define LC_RECTANGLE1POINTOPTIONS_H

#include "lc_action_options_widget.h"

class LC_ActionDrawRectangle1Point;
namespace Ui {
    class LC_Rectangle1PointOptionsWidget;
}

class LC_Rectangle1PointOptionsWidget :public LC_ActionOptionsWidget {
    Q_OBJECT
public:
    explicit LC_Rectangle1PointOptionsWidget();
    ~LC_Rectangle1PointOptionsWidget() override;
    enum {
        UPDATE_ANGLE
    };
public slots:
    void onCornersIndexChanged(int index);
    void onSnapPointIndexChanged(int index);
    void onAngleEditingFinished();
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onWidthEditingFinished();
    void onHeightEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);
    void onInnerSizeClicked(bool value);
    void onFreeAngleClicked(bool value);
    void onEdgesIndexChanged(int index);
    void onBaseAngleFixedClicked(bool value);
    void languageChange() override;
    void updateUI(int mode, const QVariant* value) override;
protected:
    void doUpdateByAction(RS_ActionInterface * a) override;
private:
    Ui::LC_Rectangle1PointOptionsWidget *ui;
    LC_ActionDrawRectangle1Point *m_action = nullptr;

};


#endif
