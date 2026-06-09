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

#ifndef LC_UCS_MARK_H
#define LC_UCS_MARK_H

#include <QFont>

#include "lc_overlayentity.h"
#include "rs_color.h"
#include "rs_vector.h"

struct LC_UCSMarkOptions{
    /** coordinate origin marker */
    bool showUcsZeroMarker = false;
    bool showWcsZeroMarker = true;
    int csZeroMarkerSize = 30;
    int csZeroMarkerFontSize = 10;
    QString csZeroMarkerfontName = "Verdana";
    QFont csZeroMarkerFont = QFont("Arial", 10);
    RS_Color colorXAxisExtension = RS_Color(Qt::red);
    RS_Color colorYAxisExtension = RS_Color(Qt::green);
    RS_Color colorAngleMark = RS_Color(Qt::yellow);
    void loadSettings();
};


class LC_OverlayUCSMark:public LC_OverlayDrawable{
public:
    LC_OverlayUCSMark(const RS_Vector& uiOrigin, double xAxisAngle, bool forWcs, LC_UCSMarkOptions *options);
    explicit LC_OverlayUCSMark(LC_UCSMarkOptions *options) ;
    void draw(RS_Painter *painter) override;
    void update(const RS_Vector& uiPos, double xAngle, bool wcs);
protected:
    RS_Vector m_uiOrigin;
    double m_xAxisAngle;
    bool m_forWcs = false;
    LC_UCSMarkOptions* m_options;
};

#endif
