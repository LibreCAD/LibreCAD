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

#include "lc_graphicviewport.h"
#include "rs_painter.h"
#include "rs_settings.h"

void LC_OverlayRelZeroOptions::loadSettings() {
    LC_GROUP("Appearance");
    {
        hideRelativeZero = LC_GET_BOOL("hideRelativeZero");
        relativeZeroRadius = LC_GET_INT("RelZeroMarkerRadius", 5);
    }
    LC_GROUP_GUARD("Colors");
    {
        colorRelativeZero = RS_Color(LC_GET_STR("relativeZeroColor", RS_Settings::RELATIVE_ZERO_COLOR));
    }
}

LC_OverlayRelativeZero::LC_OverlayRelativeZero(const RS_Vector &wcsPos, LC_OverlayRelZeroOptions *options)
    :m_wcsPosition(wcsPos)
    , m_options(options)
{}

  LC_OverlayRelativeZero::LC_OverlayRelativeZero(LC_OverlayRelZeroOptions *options)
    :m_wcsPosition(RS_Vector(false))
    , m_options(options)
{}

void LC_OverlayRelativeZero::draw(RS_Painter *painter) {
    const RS_Vector uiPos = painter->toGui(m_wcsPosition);

    constexpr RS2::LineType relativeZeroPenType = RS2::SolidLine;

    RS_Pen p(m_options->colorRelativeZero, RS2::Width00, relativeZeroPenType);
    p.setScreenWidth(0);
    painter->setPen(p);

    const double zr = m_options->relativeZeroRadius;

    const RS_Vector vpMin = uiPos - RS_Vector{zr, zr};
    const RS_Vector vpMax = uiPos + RS_Vector{zr, zr};

    const LC_GraphicViewport *viewport = painter->getViewPort();

    if (vpMax.x < 0 || vpMin.x > viewport->getWidth()) {
        return;
    }
    if (vpMax.y < 0 || vpMin.y > viewport->getHeight()) {
        return;
    }

    painter->drawLineUISimple(vpMin, vpMax);

    painter->drawCircleUIDirect(uiPos, m_options->relativeZeroRadius);
}

void LC_OverlayRelativeZero::setPos(const RS_Vector &wcsPos) {
    m_wcsPosition = wcsPos;
}
