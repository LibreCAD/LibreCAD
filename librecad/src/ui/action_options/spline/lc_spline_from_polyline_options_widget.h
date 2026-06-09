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

#ifndef LC_SPLINEFROMPOLYLINEOPTIONS_H
#define LC_SPLINEFROMPOLYLINEOPTIONS_H

#include "lc_action_options_widget.h"

class LC_ActionSplineFromPolyline;
namespace Ui {
    class LC_SplineFromPolylineOptionsWidget;
}

class LC_SplineFromPolylineOptionsWidget : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_SplineFromPolylineOptionsWidget();
    ~LC_SplineFromPolylineOptionsWidget() override;
public slots:
    void languageChange() override;
    void cbKeepOriginalsClicked(bool val);
    void cbUseCurrentAttributesClicked(bool val);
    void cbUseCurrentLayerClicked(bool val);
    void cbUseFitPointsClicked(bool val);
    void sbDegreeValueChanged(int value);
    void sbMidPointsValueChanged(int value);
protected:
    Ui::LC_SplineFromPolylineOptionsWidget *ui;
    LC_ActionSplineFromPolyline* m_action = nullptr;
    void doUpdateByAction(RS_ActionInterface *a) override;
};
#endif
