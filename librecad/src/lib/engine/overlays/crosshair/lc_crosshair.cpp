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

#include "lc_crosshair.h"

#include "lc_graphicviewport.h"
#include "rs_painter.h"

LC_Crosshair::LC_Crosshair(const RS_Vector& coord,
                           int shapeType,
                           int linesType,
                           const RS_Pen& linesPen,
                           int pointSize,
                           int pointType)
     :wcsPos(coord){
     this->indicatorShape = shapeType;
     this->linesShape = linesType;
     this->linesPen = linesPen;
     this->pointSize = pointSize;
     this->pointType = pointType;
}

double LC_Crosshair::drawIndicator(RS_Painter* painter, const RS_Vector& uiPos)
{
    double offset = 0.0;
      switch (indicatorShape) {
          case Circle: {
              offset = 4.0;
              painter->drawCircleUIDirect(uiPos, offset);
              break;
          }
          case Point:{
              int screenPDSize = painter->determinePointScreenSize(pointSize);
              offset = screenPDSize;
              painter->drawPointEntityUI(uiPos, pointType, screenPDSize);
              break;
          }
          case Square: {
              double a = 6.0;
              offset = a;
              RS_Vector p1 = uiPos + RS_Vector(-a, a);
              RS_Vector p2 = uiPos + RS_Vector(a, a);
              RS_Vector p3 = uiPos + RS_Vector(a, -a);
              RS_Vector p4 = uiPos + RS_Vector(-a, -a);

              painter->drawLineUISimple(p1,p2);
              painter->drawLineUISimple(p2,p3);
              painter->drawLineUISimple(p3,p4);
              painter->drawLineUISimple(p4,p1);
              break;
          }
          case Gap:{
              offset = 5.0;
              break;
          }
          default:
              break;
    }
    return offset;
}

void LC_Crosshair::draw(RS_Painter *painter) {
    RS_Vector uiCoord = painter->toGui(wcsPos);

    double offset = drawIndicator(painter, uiCoord);

    LC_GraphicViewport* viewport = painter->getViewPort();

    int width = viewport->getWidth();
    int height = viewport->getHeight();

    painter->setPen(linesPen);

    switch (linesShape){
        case Spiderweb:{
            RS_Vector p1(0, 0);
            RS_Vector p2(0, height);
            RS_Vector p3(width, 0);
            RS_Vector p4(width, height);
            drawCrosshairLines(painter, uiCoord, offset, p1, p2, p3, p4);
            break;
        }
        case Adaptive:{
            if (viewport->isGridIsometric()){
                RS2::IsoGridViewType chType= viewport->getIsoViewType();
                RS_Vector direction1;
                RS_Vector direction2(0.,1.);
                double l=width+height;
                switch(chType){
                case RS2::IsoRight:
                    direction1=RS_Vector(M_PI*5./6.)*l;
                    direction2*=l;
                    break;
                case RS2::IsoLeft:
                    direction1=RS_Vector(M_PI*1./6.)*l;
                    direction2*=l;
                    break;
                default:
                    direction1=RS_Vector(M_PI*1./6.)*l;
                    direction2=RS_Vector(M_PI*5./6.)*l;
                }
                RS_Vector p1 = uiCoord - direction1;
                RS_Vector p2 = uiCoord + direction1;
                RS_Vector p3 = uiCoord - direction2;
                RS_Vector p4 = uiCoord + direction2;
                drawCrosshairLines(painter, uiCoord, offset, p1, p2, p3, p4);
                break;
            }
            // implicit fall-through to draw cartesian orthogonal crosshair
            [[fallthrough]];
        }
        case Crosshair:{
            RS_Vector p1(0, uiCoord.y);
            RS_Vector p2(uiCoord.x, 0);
            RS_Vector p3(uiCoord.x, height);
            RS_Vector p4(width, uiCoord.y);
            drawCrosshairLines(painter, uiCoord, offset, p1, p2, p3, p4);
            break;
        }
    }
}

void LC_Crosshair::setLinesPen(const RS_Pen &pen) {
    linesPen = pen;
}

void LC_Crosshair::setPointType(int type) {
    pointType = type;
}

void LC_Crosshair::setPointSize(int size) {
    pointSize = size;
}

void LC_Crosshair::drawCrosshairLines(RS_Painter *painter, const RS_Vector &guiCoord, double offset,
                                      const RS_Vector &p1, const RS_Vector &p2, const RS_Vector &p3, const RS_Vector &p4) const {
    RS_Vector offset1 = RS_Vector::polar(offset, guiCoord.angleTo(p1));
    RS_Vector offset2 = RS_Vector::polar(offset, guiCoord.angleTo(p2));
    RS_Vector offset3 = RS_Vector::polar(offset, guiCoord.angleTo(p3));
    RS_Vector offset4 = RS_Vector::polar(offset, guiCoord.angleTo(p4));

    painter->drawLineUISimple(guiCoord + offset1, p1);
    painter->drawLineUISimple(guiCoord + offset2 , p2);
    painter->drawLineUISimple(guiCoord + offset3, p3);
    painter->drawLineUISimple(guiCoord + offset4, p4);
}
