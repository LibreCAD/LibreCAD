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

#include "lc_overlayrelativezero.h"

#include "rs_painter.h"
#include "lc_graphicviewport.h"
#include "rs_settings.h"

void LC_OverlayRelZeroOptions::loadSettings() {
    LC_GROUP("Appearance");
    {
        hideRelativeZero = LC_GET_BOOL("hideRelativeZero");
        m_relativeZeroRadius = LC_GET_INT("RelZeroMarkerRadius", 5);
    }
    LC_GROUP_GUARD("Colors");
    {
        m_colorRelativeZero = QColor(LC_GET_STR("relativeZeroColor", RS_Settings::relativeZeroColor));
    }
}

LC_OverlayRelativeZero::LC_OverlayRelativeZero(const RS_Vector &wcsPos, LC_OverlayRelZeroOptions *options)
    :wcsPosition(wcsPos)
    , options(options)
{}

  LC_OverlayRelativeZero::LC_OverlayRelativeZero(LC_OverlayRelZeroOptions *options)
  :wcsPosition(RS_Vector(false))
    , options(options)
{}

void LC_OverlayRelativeZero::draw(RS_Painter *painter) {
    RS_Vector vp = painter->toGui(wcsPosition);

    RS2::LineType relativeZeroPenType = RS2::SolidLine;

    RS_Pen p(options->m_colorRelativeZero, RS2::Width00, relativeZeroPenType);
    p.setScreenWidth(0);
    painter->setPen(p);

    const double zr = options->m_relativeZeroRadius * 2;// todo - why diameter is used there?? Is is correct?
    const RS_Vector zrSize{zr, zr};
    const RS_Vector xyMin = vp - zrSize;
    const RS_Vector xyMax = vp + zrSize;

    LC_GraphicViewport *viewport = painter->getViewPort();

    if (xyMax.x < 0 || xyMin.x > viewport->getWidth()) return;
    if (xyMax.y < 0 || xyMin.x > viewport->getHeight()) return;

    painter->drawLineUISimple(xyMin.x, vp.y, xyMax.x, vp.y);
    painter->drawLineUISimple(vp.x, xyMin.y, vp.x, xyMax.y);

    painter->drawCircleUI(vp, options->m_relativeZeroRadius);
}

void LC_OverlayRelativeZero::setPos(const RS_Vector &wcsPos) {
    wcsPosition = wcsPos;
}
