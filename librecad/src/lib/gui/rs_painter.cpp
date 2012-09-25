/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

    double aStep=fabs(2.0/radius);         // Angle Step (rad)
    if(aStep>=0.5) aStep=0.5;
    if(reversed) {
        if(a1<=a2+RS_TOLERANCE) a1+=2.*M_PI;
        aStep *= -1;
    }else{
        if(a2<=a1+RS_TOLERANCE) a2+=2.*M_PI;
    }
    double a;             // Current Angle (rad)

//    aStep=aStep/2.0;
    //if (aStep<0.05) {
    //    aStep = 0.05;
    //}

    // less than a pixel long lines:
    //if (radius*aStep<1.0) {
    //	aStep =
    //}

    //QPointArray pa;
    pa.clear();
    //    pa<<QPoint(toScreenX(cp.x+cos(aStart)*radius), toScreenY(cp.y-sin(aStart)*radius));
    double da=fabs(a2-a1);
    for(a=a1; fabs(a-a1)<da; a+=aStep) {
        pa<<QPoint(toScreenX(cp.x+cos(a)*radius), toScreenY(cp.y-sin(a)*radius));
    }

    QPoint pt2(toScreenX(cp.x+cos(a2)*radius), toScreenY(cp.y-sin(a2)*radius));
    if(pa.size()>0 && pa.last() != pt2) pa<<pt2;
}

void RS_Painter::createEllipse(QPolygon& pa,
        const RS_Vector& cp,
                         double radius1, double radius2,
                         double angle,
                         double angle1, double angle2,
                         bool reversed)
{

    const RS_Vector vr(radius1,radius2);
    const RS_Vector rvp(radius2,radius1);
    double ea1=angle1;
    double ea2=angle2;
    if(reversed) std::swap(ea1,ea2);
    if(ea2<ea1+RS_TOLERANCE_ANGLE) {
        ea2 += 2.*M_PI; //potential bug
    }
    const RS_Vector angleVector(-angle);
    // minimum angular step is limited to one pixel
    double rmax=(radius1>radius2)?radius1:radius2;
    double aStep(1./rmax/(radius1*radius2));
    //avoid division by zero;
    if(radius1<1. || radius2<1.) aStep=0.2;
    /*
      draw a new line after tangent changes by 0.01 rad
      ds^2 = (a^2 sin^2 + b^2 cos^2) da^2
      */
    RS_Vector vp(-ea1);
    vp.scale(vr);
    vp.rotate(angleVector);
    vp.move(cp);
    //    vp.set(cp.x+cos(a1)*radius1,
    //           cp.y-sin(a1)*radius2);
    //    vp.rotate(vc, -angle);
    pa.clear();
    pa<<QPoint(toScreenX(vp.x),
               toScreenY(vp.y));
//    moveTo(toScreenX(vp.x),
//           toScreenY(vp.y));
        ea1 += aStep*RS_Vector(-ea1).scale(rvp).squared();
    // Arc Counterclockwise:
    while(ea1<ea2){

        RS_Vector va(-ea1);
        vp=va;
        vp.scale(vr);
        vp.rotate(angleVector);
        vp.move(cp);

        //            vp.set(cp.x+cos(a)*radius1,
        //                   cp.y-sin(a)*radius2);
        //            vp.rotate(vc, -angle);
        pa<<QPoint(toScreenX(vp.x),
               toScreenY(vp.y));
        ea1 += aStep*va.scale(rvp).squared();
    }

    vp.set(cos(ea2)*radius1,
           -sin(ea2)*radius2);
    vp.rotate(angleVector);
    vp.move(cp);
    pa<<QPoint(toScreenX(vp.x),
           toScreenY(vp.y));
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


