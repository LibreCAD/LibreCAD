/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/


#include "rs_painter.h"


void RS_Painter::createArc(QPolygon& pa,
                             const RS_Vector& cp, double radius,
                             double a1, double a2,
                             bool reversed) {

	if (radius<1.0e-6) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
			"RS_Painter::createArc: invalid radius: %f", radius);
		return;
	}

    int   cix;            // Next point on circle
    int   ciy;            //
    double aStep;         // Angle Step (rad)
    double a;             // Current Angle (rad)

    if(fabs(2.0/radius)<=1.0) {
        aStep=asin(2.0/radius);
    } else {
        aStep=1.0;
    }

    aStep=aStep/2.0;
    //if (aStep<0.05) {
    //    aStep = 0.05;
    //}
	
	// less than a pixel long lines:
	//if (radius*aStep<1.0) {
	//	aStep = 
	//}

    //QPointArray pa;
    int i=0;
    pa.resize(i+1);
    pa.setPoint(i++, toScreenX(cp.x+cos(a1)*radius),
                toScreenY(cp.y-sin(a1)*radius));
    //moveTo(toScreenX(cp.x+cos(a1)*radius),
    //       toScreenY(cp.y-sin(a1)*radius));
    if(!reversed) {
        // Arc Counterclockwise:
        if(a1>a2-1.0e-10) {
            a2+=2*M_PI;
        }
        for(a=a1+aStep; a<=a2; a+=aStep) {
            cix = toScreenX(cp.x+cos(a)*radius);
            ciy = toScreenY(cp.y-sin(a)*radius);
            //lineTo(cix, ciy);
            pa.resize(i+1);
            pa.setPoint(i++, cix, ciy);
        }
    } else {
        // Arc Clockwise:
        if(a1<a2+1.0e-10) {
            a2-=2*M_PI;
        }
        for(a=a1-aStep; a>=a2; a-=aStep) {
            cix = toScreenX(cp.x+cos(a)*radius);
            ciy = toScreenY(cp.y-sin(a)*radius);
            //lineTo(cix, ciy);
            pa.resize(i+1);
            pa.setPoint(i++, cix, ciy);
        }
    }
    //lineTo(toScreenX(cp.x+cos(a2)*radius),
    //       toScreenY(cp.y-sin(a2)*radius));
    pa.resize(i+1);
    pa.setPoint(i++,
                toScreenX(cp.x+cos(a2)*radius),
                toScreenY(cp.y-sin(a2)*radius));
    //drawPolyline(pa);
}



void RS_Painter::drawRect(const RS_Vector& p1, const RS_Vector& p2) {
	drawLine(RS_Vector(p1.x, p1.y), RS_Vector(p2.x, p1.y));
    drawLine(RS_Vector(p2.x, p1.y), RS_Vector(p2.x, p2.y));
    drawLine(RS_Vector(p2.x, p2.y), RS_Vector(p1.x, p2.y));
    drawLine(RS_Vector(p1.x, p2.y), RS_Vector(p1.x, p1.y));
}



void RS_Painter::drawHandle(const RS_Vector& p, const RS_Color& c, int size) {
	if (size<0) {
		size = 2;
	}
	fillRect((int)(p.x-size), (int)(p.y-size), 2*size, 2*size, c);
}


