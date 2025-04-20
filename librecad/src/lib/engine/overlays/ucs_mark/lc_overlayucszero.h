/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#ifndef LC_OVERLAYUCSZERO_H
#define LC_OVERLAYUCSZERO_H

#include "lc_overlayentity.h"
#include "rs_color.h"

struct LC_OverlayUCSZeroOptions{

    enum ExtendAxisArea{
        Both,
        Positive,
        Negative,
        None
    };

    bool m_extendAxisLines = false;
    int m_extendAxisModeX = 0;
    int m_extendAxisModeY = 0;
    int m_zeroShortAxisMarkSize = 20;
    RS_Color m_colorXAxisExtension = Qt::red;
    RS_Color m_colorYAxisExtension = Qt::green;

    void loadSettings();
};

class RS_Painter;

class LC_OverlayUCSZero:public LC_OverlayDrawable{
public:
    explicit LC_OverlayUCSZero(LC_OverlayUCSZeroOptions *options);
    LC_OverlayUCSZero(double uiOriginPointX, double uiOriginPointY, LC_OverlayUCSZeroOptions *options);
    void draw(RS_Painter *painter) override;
    void updateOrigin(double uiOriginX, double uiOriginY) {uiOriginPointX = uiOriginX; uiOriginPointY = uiOriginY;}
protected:
    double uiOriginPointX = 0.0;
    double uiOriginPointY = 0.0;
    LC_OverlayUCSZeroOptions *options = nullptr;
};

#endif // LC_OVERLAYUCSZERO_H
