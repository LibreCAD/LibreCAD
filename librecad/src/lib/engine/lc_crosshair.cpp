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
#include "rs_painter.h"
#include "rs_graphicview.h"
#include "rs_pen.h"
#include "rs_debug.h"

LC_Crosshair::LC_Crosshair(RS_EntityContainer *parent, const RS_Vector& coord,
                           int shapeType,
                           int linesType,
                           const RS_Pen& linesPen,
                           int pointSize,
                           int pointType)
     :RS_Point(parent, RS_PointData(coord)){
     this->indicatorShape = shapeType;
     this->linesShape = linesType;
     this->linesPen = linesPen;
     this->pointSize = pointSize;
     this->pointType = pointType;
}

double LC_Crosshair::drawIndicator(RS_Painter* painter, RS_GraphicView* view, const RS_Vector &guiPos){
    double offset = 0.0;
      switch (indicatorShape) {
          case Circle: {
              offset = 4.0;
              painter->drawCircle(guiPos, offset);
              break;
          }
          case Point:{
              int screenPDSize = determinePointSreenSize(painter, view, pointSize);
              offset = screenPDSize;
              painter->drawPoint(guiPos, pointType, screenPDSize);
              break;
          }
          case Square: {
              double a = 6.0;
              offset = a;
              RS_Vector p1 = guiPos + RS_Vector(-a, a);
              RS_Vector p2 = guiPos + RS_Vector(a, a);
              RS_Vector p3 = guiPos + RS_Vector(a, -a);
              RS_Vector p4 = guiPos + RS_Vector(-a, -a);

              painter->drawLine(p1,p2);
              painter->drawLine(p2,p3);
              painter->drawLine(p3,p4);
              painter->drawLine(p4,p1);
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

void LC_Crosshair::draw(RS_Painter *painter, RS_GraphicView *view, [[maybe_unused]]double &patternOffset) {
//    LC_ERR << "Draw Crosshair!";
    RS_Vector guiCoord = view->toGui(getPos());

    double offset = drawIndicator(painter, view, guiCoord);

    int width = view->getWidth();
    int height = view->getHeight();

    painter->setPen(linesPen);
    switch (linesShape){
        case Spiderweb:{
            RS_Vector p1(0, 0);
            RS_Vector p2(0, height);
            RS_Vector p3(width, 0);
            RS_Vector p4(width, height);
            drawCrosshairLines(painter, guiCoord, offset, p1, p2, p3, p4);
            break;
        }
        case Adaptive:{
            if (view->isGridIsometric()){
                RS2::IsoGridViewType chType= view->getIsoViewType();
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
                RS_Vector p1 = guiCoord -direction1;
                RS_Vector p2 = guiCoord + direction1;
                RS_Vector p3 = guiCoord - direction2;
                RS_Vector p4 = guiCoord + direction2;
                drawCrosshairLines(painter, guiCoord, offset, p1, p2, p3, p4);
                break;
            }
            // implicit fall-through to draw cartesian orthogonal crosshair
            [[fallthrough]];
        }
        case Crosshair:{
            RS_Vector p1(0, guiCoord.y);
            RS_Vector p2(guiCoord.x, 0);
            RS_Vector p3(guiCoord.x, height);
            RS_Vector p4(width, guiCoord.y);
            drawCrosshairLines(painter, guiCoord, offset, p1, p2, p3, p4);
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

void LC_Crosshair::drawCrosshairLines(
    RS_Painter *painter, const RS_Vector &guiCoord, double offset, const RS_Vector &p1, const RS_Vector &p2, const RS_Vector &p3, const RS_Vector &p4) const {
    RS_Vector offset1 = RS_Vector::polar(offset, guiCoord.angleTo(p1));
    RS_Vector offset2 = RS_Vector::polar(offset, guiCoord.angleTo(p2));
    RS_Vector offset3 = RS_Vector::polar(offset, guiCoord.angleTo(p3));
    RS_Vector offset4 = RS_Vector::polar(offset, guiCoord.angleTo(p4));

    painter->drawLine(guiCoord + offset1, p1);
    painter->drawLine(guiCoord + offset2 , p2);
    painter->drawLine(guiCoord + offset3, p3);
    painter->drawLine(guiCoord + offset4, p4);
}
