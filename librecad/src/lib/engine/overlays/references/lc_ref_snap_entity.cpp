/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_ref_snap_entity.h"

#include "lc_graphicviewport.h"
#include "rs_painter.h"





void LC_RefSnapEntity::drawMarker(RS_Painter* painter, const QFont& font, const RS_Vector &basePoint, const int uiOffset, const double offsetLevel, const QString& markerLetter) {
    const QSize &size = QFontMetrics(font).size(Qt::TextSingleLine, markerLetter);

    double nearestX;
    double nearestY;
    painter->getViewPort()->toUI(m_snapInfo.nearestPoint, nearestX, nearestY);

    double baseX;
    double baseY;
    painter->getViewPort()->toUI(basePoint, baseX, baseY);

    RS_Vector uiNearest(nearestX, nearestY);
    RS_Vector uiBase(baseX, baseY);

    RS_Vector uiMark = uiNearest.relative( uiOffset * offsetLevel, uiBase.angleTo(uiNearest));

    auto yRect = QRect(QPoint(uiMark.x - size.width()/2, uiMark.y-size.height()/2), size);
    QRect yBoundingRect;

    // we'll intenctionally will not restore painter - so far, the font is used only in overlays and draft mark... so it' should be ok not to
    // restore painter state (which is costly from performance point of view)
    // painter->save();
    painter->setFont(font);
    painter->drawText(yRect, Qt::AlignHCenter | Qt::AlignCenter | Qt::TextDontClip, markerLetter, &yBoundingRect);
    // painter->restore();
}
