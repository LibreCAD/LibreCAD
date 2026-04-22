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

#include "lc_overlayucszero.h"

#include "lc_graphicviewport.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_settings.h"

class LC_GraphicViewport;

void LC_OverlayUCSZeroOptions::loadSettings(){
    LC_GROUP("Appearance");
    {
        extendAxisLines = LC_GET_BOOL("ExtendAxisLines", false);
        extendAxisModeX = LC_GET_INT("ExtendModeXAxis", 0);
        extendAxisModeY = LC_GET_INT("ExtendModeYAxis", 0);
        zeroShortAxisMarkSize = LC_GET_INT("ZeroShortAxisMarkSize", 20);
    }
    LC_GROUP_GUARD("Colors");
    {
        colorXAxisExtension = RS_Color(LC_GET_STR("grid_x_axisColor", "red"));
        colorYAxisExtension = RS_Color(LC_GET_STR("grid_y_axisColor", "green"));
    }
}

LC_OverlayUCSZero::LC_OverlayUCSZero(const double uiOriginPointX, const double uiOriginPointY, LC_OverlayUCSZeroOptions *options):m_uiOriginPointX(
    uiOriginPointX), m_uiOriginPointY(uiOriginPointY), m_options{options} {}

LC_OverlayUCSZero::LC_OverlayUCSZero(LC_OverlayUCSZeroOptions *options): m_options{options} {}

void LC_OverlayUCSZero::draw(RS_Painter *painter) {
    // draw absolute zero point and axies
    const int zr = m_options->zeroShortAxisMarkSize;

    RS_Pen penXAxis (m_options->colorXAxisExtension, RS2::Width00, RS2::SolidLine);
    penXAxis.setScreenWidth(0);

    RS_Pen penYAxis (m_options->colorYAxisExtension, RS2::Width00, RS2::SolidLine);
    penYAxis.setScreenWidth(0);

    auto* viewport = painter->getViewPort();

    int width = viewport->getWidth();
    int height = viewport->getHeight();

    if (m_options->extendAxisLines){ // axises are extended
        double xAxisStartPoint = 0.0;
        double xAxisEndPoint = 0.0;

        switch (m_options->extendAxisModeX){
            case LC_OverlayUCSZeroOptions::Both:
                xAxisStartPoint = 0;
                xAxisEndPoint = width;
                break;
            case LC_OverlayUCSZeroOptions::Positive:
                xAxisStartPoint = m_uiOriginPointX;
                if (m_uiOriginPointX < width){
                    xAxisEndPoint = width;
                }
                else{
                    xAxisEndPoint = 0;
                }
                break;
            case LC_OverlayUCSZeroOptions::Negative:
                xAxisStartPoint  = m_uiOriginPointX;
                if (m_uiOriginPointX < width){
                    xAxisEndPoint = 0;
                }
                else{
                    xAxisEndPoint = width;
                }
                break;
            case LC_OverlayUCSZeroOptions::None:{ // draw short
                xAxisStartPoint  = m_uiOriginPointX - zr;
                xAxisEndPoint = m_uiOriginPointX + zr;
                break;
            }
            default:
                xAxisStartPoint = 0;
                xAxisEndPoint = 0;
                break;
        }

        painter->setPen(penXAxis);
        painter->drawLineUISimple(xAxisStartPoint, m_uiOriginPointY, xAxisEndPoint, m_uiOriginPointY);

        int yAxisStartPoint;
        int yAxisEndPoint;
        switch (m_options->extendAxisModeY){
            case LC_OverlayUCSZeroOptions::Both:
                yAxisStartPoint  = 0;
                yAxisEndPoint  = height;
                break;
            case LC_OverlayUCSZeroOptions::Positive:
                yAxisStartPoint = m_uiOriginPointY;
                if (m_uiOriginPointY < height){
                    yAxisEndPoint = 0;
                }
                else{
                    yAxisEndPoint = height;
                }
                break;
            case LC_OverlayUCSZeroOptions::Negative:
                yAxisStartPoint  = m_uiOriginPointY;
                if (m_uiOriginPointY < height){
                    yAxisEndPoint = height;
                }
                else{
                    yAxisEndPoint = 0;
                }
                break;
            case LC_OverlayUCSZeroOptions::None:
                yAxisStartPoint  = m_uiOriginPointY - zr;
                yAxisEndPoint = m_uiOriginPointY + zr;
                break;
            default:
                yAxisStartPoint = 0;
                yAxisEndPoint = 0;
                break;
        }

        painter->setPen(penYAxis);
        painter->drawLineUISimple(m_uiOriginPointX, yAxisStartPoint, m_uiOriginPointX, yAxisEndPoint);
    }
    else { // axises are short
        double xAxisPoints [2];
        double yAxisPoints [2];

        if (((m_uiOriginPointX + zr) < 0) || ((m_uiOriginPointX - zr) > width)) {
            return;
        }
        if (((m_uiOriginPointY + zr) < 0) || ((m_uiOriginPointY - zr) > height)) {
            return;
        }
        xAxisPoints [0] = m_uiOriginPointX - zr;
        xAxisPoints [1] = m_uiOriginPointX + zr;

        yAxisPoints [0] = m_uiOriginPointY - zr;
        yAxisPoints [1] = m_uiOriginPointY + zr;

        painter->setPen(penXAxis);
        painter->drawLineUISimple(xAxisPoints[0], m_uiOriginPointY,xAxisPoints[1], m_uiOriginPointY);

        painter->setPen(penYAxis);
        painter->drawLine(m_uiOriginPointX, yAxisPoints[0], m_uiOriginPointX, yAxisPoints[1]);
    }
}
