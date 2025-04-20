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
        m_extendAxisLines = LC_GET_BOOL("ExtendAxisLines", false);
        m_extendAxisModeX = LC_GET_INT("ExtendModeXAxis", 0);
        m_extendAxisModeY = LC_GET_INT("ExtendModeYAxis", 0);
        m_zeroShortAxisMarkSize = LC_GET_INT("ZeroShortAxisMarkSize", 20);
    }
    LC_GROUP_GUARD("Colors");
    {
        m_colorXAxisExtension = QColor(LC_GET_STR("grid_x_axisColor", "red"));
        m_colorYAxisExtension = QColor(LC_GET_STR("grid_y_axisColor", "green"));
    }
}

LC_OverlayUCSZero::LC_OverlayUCSZero(double uiOriginPointX, double uiOriginPointY, LC_OverlayUCSZeroOptions *options):uiOriginPointX(
    uiOriginPointX), uiOriginPointY(uiOriginPointY), options{options} {}

LC_OverlayUCSZero::LC_OverlayUCSZero(LC_OverlayUCSZeroOptions *options):uiOriginPointX(0), uiOriginPointY(0), options{options} {}

void LC_OverlayUCSZero::draw(RS_Painter *painter) {
    // draw absolute zero point and axies
    int const zr = options->m_zeroShortAxisMarkSize;

    RS_Pen pen_xAxis (options->m_colorXAxisExtension, RS2::Width00, RS2::SolidLine);
    pen_xAxis.setScreenWidth(0);

    RS_Pen pen_yAxis (options->m_colorYAxisExtension, RS2::Width00, RS2::SolidLine);
    pen_yAxis.setScreenWidth(0);

    auto viewport = painter->getViewPort();

    int width = viewport->getWidth();
    int height = viewport->getHeight();

    if (options->m_extendAxisLines){ // axises are extended
        int xAxisStartPoint;
        int xAxisEndPoint;

        switch (options->m_extendAxisModeX){
            case LC_OverlayUCSZeroOptions::Both:
                xAxisStartPoint = 0;
                xAxisEndPoint = width;
                break;
            case LC_OverlayUCSZeroOptions::Positive:
                xAxisStartPoint = uiOriginPointX;
                if (uiOriginPointX < width){
                    xAxisEndPoint = width;
                }
                else{
                    xAxisEndPoint = 0;
                }
                break;
            case LC_OverlayUCSZeroOptions::Negative:
                xAxisStartPoint  = uiOriginPointX;
                if (uiOriginPointX < width){
                    xAxisEndPoint = 0;
                }
                else{
                    xAxisEndPoint = width;
                }
                break;
            case LC_OverlayUCSZeroOptions::None:{ // draw short
                xAxisStartPoint  = uiOriginPointX - zr;
                xAxisEndPoint = uiOriginPointX + zr;
                break;
            }
            default:
                xAxisStartPoint = 0;
                xAxisEndPoint = 0;
                break;
        }

        painter->setPen(pen_xAxis);
        painter->drawLineUISimple(xAxisStartPoint, uiOriginPointY, xAxisEndPoint, uiOriginPointY);

        int yAxisStartPoint;
        int yAxisEndPoint;
        switch (options->m_extendAxisModeY){
            case LC_OverlayUCSZeroOptions::Both:
                yAxisStartPoint  = 0;
                yAxisEndPoint  = height;
                break;
            case LC_OverlayUCSZeroOptions::Positive:
                yAxisStartPoint = uiOriginPointY;
                if (uiOriginPointY < height){
                    yAxisEndPoint = 0;
                }
                else{

                    yAxisEndPoint = height;
                }
                break;
            case LC_OverlayUCSZeroOptions::Negative:
                yAxisStartPoint  = uiOriginPointY;
                if (uiOriginPointY < height){
                    yAxisEndPoint = height;
                }
                else{
                    yAxisEndPoint = 0;
                }
                break;
            case LC_OverlayUCSZeroOptions::None:
                yAxisStartPoint  = uiOriginPointY - zr;
                yAxisEndPoint = uiOriginPointY + zr;
                break;
            default:
                yAxisStartPoint = 0;
                yAxisEndPoint = 0;
                break;
        }

        painter->setPen(pen_yAxis);
        painter->drawLineUISimple(uiOriginPointX, yAxisStartPoint, uiOriginPointX, yAxisEndPoint);
    }
    else { // axises are short
        double xAxisPoints [2];
        double yAxisPoints [2];

        if (((uiOriginPointX + zr) < 0) || ((uiOriginPointX - zr) > width)) return;
        if (((uiOriginPointY + zr) < 0) || ((uiOriginPointY - zr) > height)) return;
        xAxisPoints [0] = uiOriginPointX - zr;
        xAxisPoints [1] = uiOriginPointX + zr;

        yAxisPoints [0] = uiOriginPointY - zr;
        yAxisPoints [1] = uiOriginPointY + zr;

        painter->setPen(pen_xAxis);
        painter->drawLineUISimple(xAxisPoints[0], uiOriginPointY,xAxisPoints[1], uiOriginPointY);

        painter->setPen(pen_yAxis);
        painter->drawLine(uiOriginPointX, yAxisPoints[0], uiOriginPointX, yAxisPoints[1]);
    }

}
