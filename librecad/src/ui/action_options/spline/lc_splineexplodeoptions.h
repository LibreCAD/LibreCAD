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

#ifndef LC_SPLINEEXPLODEOPTIONS_H
#define LC_SPLINEEXPLODEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "lc_actionsplineexplode.h"

namespace Ui {
class LC_SplineExplodeOptions;
}

class LC_SplineExplodeOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT


public:
    explicit LC_SplineExplodeOptions();
    ~LC_SplineExplodeOptions();

public slots:
    void languageChange() override;
    void cbKeepOriginalsClicked(bool val);
    void cbUseCurrentAttributesClicked(bool val);
    void cbUseCurrentLayerClicked(bool val);
    void cbCustomSegmentCountClicked(bool val);
    void cbPolylineClicked(bool val);
    void sbSegmentsCountValueChanged(int value);
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void doSaveSettings() override;
private:
    Ui::LC_SplineExplodeOptions *ui;
    LC_ActionSplineExplode* action;

    int segmentsCountFromDrawing;

    void setKeepOriginalsToActionAndView(bool val);
    void setUseCurrentAttributesToActionAndView(bool val);
    void setUseCurrentLayerToActionAndView(bool val);
    void setUseCustomSegmentCount(bool val);
    void setPolylineToActionAndView(bool val);

    void setSegmentsCountValueToActionAndView(int value);
};

#endif // LC_SPLINEEXPLODEOPTIONS_H
