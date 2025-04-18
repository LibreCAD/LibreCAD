/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_ucs_mark.h"

#include "rs_math.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_settings.h"

void LC_UCSMarkOptions::loadSettings() {
    LC_GROUP("Appearance");
    {
        m_showUCSZeroMarker = LC_GET_BOOL("ShowUCSZeroMarker", false);
        m_showWCSZeroMarker = LC_GET_BOOL("ShowWCSZeroMarker", true);
        m_csZeroMarkerSize = LC_GET_INT("ZeroMarkerSize", 30);
        m_csZeroMarkerFontSize = LC_GET_INT("ZeroMarkerFontSize", 10);
        m_csZeroMarkerfontName = LC_GET_STR("ZeroMarkerFontName", "Verdana");
        m_csZeroMarkerFont = QFont(m_csZeroMarkerfontName, m_csZeroMarkerFontSize);
    }
    LC_GROUP_GUARD("Colors");
    {
        m_colorXAxisExtension = QColor(LC_GET_STR("grid_x_axisColor", "red"));
        m_colorYAxisExtension = QColor(LC_GET_STR("grid_y_axisColor", "green"));
        m_colorAngleMark = QColor(LC_GET_STR("previewReferencesColor", RS_Settings::previewRefColor));
    }
}

LC_OverlayUCSMark::LC_OverlayUCSMark(RS_Vector uiOrigin, double xAxisAngle, bool forWcs, LC_UCSMarkOptions *options):uiOrigin(uiOrigin), xAxisAngle(xAxisAngle), forWCS(
    forWcs), options(options) {}

LC_OverlayUCSMark::LC_OverlayUCSMark(LC_UCSMarkOptions *options):uiOrigin({0,0,0}), xAxisAngle(0), forWCS(false), options(options) {}

void LC_OverlayUCSMark::update(RS_Vector uiPos, double xAngle, bool wcs){
    uiOrigin = uiPos;
    xAxisAngle = xAngle;
    forWCS = wcs;
}

void LC_OverlayUCSMark::draw(RS_Painter *painter) {

    int const zr = options->m_csZeroMarkerSize;
    RS_Vector uiXAxisEnd = uiOrigin.relative(zr, xAxisAngle);

    double yAxisAngle = xAxisAngle-M_PI_2; // as we're in UI coordinate space
    RS_Vector uiYAxisEnd = uiOrigin.relative(zr,yAxisAngle);

    RS_Pen pen_xAxis (options->m_colorXAxisExtension, RS2::Width00, RS2::SolidLine);
    pen_xAxis.setScreenWidth(2);

    RS_Pen pen_yAxis (options->m_colorYAxisExtension, RS2::Width00, RS2::SolidLine);
    pen_yAxis.setScreenWidth(2);

    double anchorFactor = 0.2;
    double anchorAngle = RS_Math::deg2rad(70);
    double anchorSize = zr * anchorFactor;

    double resultingAchorAngle = M_PI_2 + anchorAngle;

    painter->setFont(options->m_csZeroMarkerFont);

    painter->setPen(pen_xAxis);
    // axis
    painter->drawLineUISimple(uiOrigin, uiXAxisEnd);
    // anchor
    painter->drawLineUISimple(uiXAxisEnd, uiXAxisEnd.relative(anchorSize, xAxisAngle+resultingAchorAngle));
    painter->drawLineUISimple(uiXAxisEnd, uiXAxisEnd.relative(anchorSize, xAxisAngle-resultingAchorAngle));

    const char* xString = forWCS ? "wX" : "X";

    const QSize &xSize = QFontMetrics(options->m_csZeroMarkerFont).size(Qt::TextSingleLine, xString);

    RS_Vector offset = RS_Vector(20,10);

    RS_Vector xTextPosition = uiXAxisEnd.relative(offset.x, yAxisAngle);

    QRect xRect = QRect(QPoint(xTextPosition.x, xTextPosition.y), xSize);
    QRect xBoundingRect;
    painter->drawText(xRect,  Qt::AlignTop | Qt::AlignRight | Qt::TextDontClip, xString, &xBoundingRect);

    painter->setPen(pen_yAxis);
    // axis
    painter->drawLineUISimple(uiOrigin, uiYAxisEnd);
    // anchor
    painter->drawLineUISimple(uiYAxisEnd, uiYAxisEnd.relative(anchorSize, yAxisAngle+resultingAchorAngle));
    painter->drawLineUISimple(uiYAxisEnd, uiYAxisEnd.relative(anchorSize, yAxisAngle-resultingAchorAngle));

    const char* yString = forWCS ? "wY" : "Y";
    const QSize &ySize = QFontMetrics(options->m_csZeroMarkerFont).size(Qt::TextSingleLine, yString);

    RS_Vector yTextPosition = uiYAxisEnd.relative(offset.x, xAxisAngle);

    QRect yRect = QRect(QPoint(yTextPosition.x, yTextPosition.y), ySize);
    QRect yBoundingRect;
    painter->drawText(yRect, Qt::AlignTop | Qt::AlignRight | Qt::TextDontClip, yString, &yBoundingRect);

    // square
    double angleFactor = 0.05;
    double angleLen = zr*angleFactor;

    RS_Vector angleX0 = uiOrigin.relative(angleLen, xAxisAngle);
    RS_Vector angleX1 = angleX0.relative(angleLen, yAxisAngle);
    RS_Vector anchorY0 = uiOrigin.relative(angleLen, yAxisAngle);

    RS_Pen anglePen (options->m_colorAngleMark, RS2::Width00, RS2::SolidLine);
    anglePen.setScreenWidth(0);

    painter->setPen(anglePen);

    painter->drawLineUISimple(anchorY0, angleX1);
    painter->drawLineUISimple(angleX0, angleX1);

    painter->drawGridPoint(uiOrigin);
}
