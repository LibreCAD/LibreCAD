/****************************************************************************
**
* Options widget for "SliceDivide" action.

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
#ifndef LC_SLICEDIVIDEOPTIONS_H
#define LC_SLICEDIVIDEOPTIONS_H

#include "lc_action_options_widget.h"

class LC_ActionDrawSliceDivide;
namespace Ui {
    class LC_SliceDivideOptionsWidget;
}

class LC_SliceDivideOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_SliceDivideOptionsWidget();
    ~LC_SliceDivideOptionsWidget() override;
    void updateUI(int mode, const QVariant* value) override;
protected slots:
    void languageChange() override;
    void onCountChanged(int value);
    void onDistanceEditingFinished();
    void onTickLengthEditingFinished();
    void onTickAngleEditingFinished();
    void onTickOffsetEditingFinished();
    void onCircleStartAngleEditingFinished();
    void onDrawTickOnEdgesIndexChanged(int index);
    void onTickSnapIndexChanged(int index);
    void onRelAngleClicked(bool checked);
    void onDivideClicked(bool checked);
    void onModeClicked(bool checked);
protected:
    void doUpdateByAction(RS_ActionInterface *a) override;
    Ui::LC_SliceDivideOptionsWidget *ui;
    LC_ActionDrawSliceDivide* m_action = nullptr;
    bool m_forCircle {false};
};

#endif
